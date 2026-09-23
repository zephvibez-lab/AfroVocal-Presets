# JUCE 8 real-time-safety and plug-in architecture review

**Project reviewed:** AfroVocalPresets, pinned to JUCE `8.0.10` in `CMakeLists.txt` (line 19).  
**Review target:** `Source/PluginProcessor.cpp`, `Source/PluginProcessor.h`, `Source/PluginEditor.cpp`, and `Source/PluginEditor.h`.  
**Purpose:** identify defects and risk areas visible in the current source, then translate current JUCE guidance into concrete implementation and validation work.

> **Executive conclusion.** The project has a sound high-level split: DSP runs in `processBlock()`, buffers are created in `prepareToPlay()`, the optional network request is moved to a worker, APVTS owns host-facing parameters, and `ScopedNoDenormals` is present. However, it is not yet safe to describe the processor as fully real-time safe. The highest-priority problems are allocation-capable filter-coefficient construction in the audio callback, hot-path buffer-copy assumptions for variable block sizes, unsynchronised editor callback state, worker-thread mutation of APVTS parameters and `lastPreset`, incomplete host bypass integration, and missing latency/reset contracts. These issues can produce glitches, races, stale UI, or host incompatibility even though the code compiles and the ordinary stereo path appears to work.

## 1. Review basis and source-specific findings

JUCE defines `processBlock()` as the callback that renders the next block. `prepareToPlay()` is where resources should be prepared, and the documented block size is only the host's typical block size: actual calls may be smaller or larger and may vary from call to call [1]. The JUCE DSP lifecycle follows the same model: prepare processors before processing, process without changing the processing topology, and reset DSP state when required [6].

The current code does the following in every block:

* `ChannelDSP::process()` calls `updateFilters()`, and `updateFilters()` constructs four new `IIR::Coefficients<float>` objects through `makeHighPass`, `makePeakFilter`, and `makeHighShelf` before assigning them to the filters (`PluginProcessor.cpp:68-74, 84`). Coefficients are reference-counted JUCE objects [9]. The factory calls and reference-count changes are allocation/deallocation-capable work in the real-time callback and must not be treated as harmless arithmetic.
* The same method calls `parallelBuffer.makeCopyOf(buffer, true)` and `reverbBuffer.makeCopyOf(buffer, true)` on the hot path (`PluginProcessor.cpp:99, 130`). The buffers are prepared for `samplesPerBlock` and two channels (`PluginProcessor.cpp:48-59`), but JUCE explicitly warns that the actual callback size is not guaranteed to equal that value [1]. The current code has no explicit guard for a larger block and no preallocated fallback. The hard-coded two-channel preparation also does not match the supported mono layout (`PluginProcessor.cpp:263-266`).
* Most parameter values are looked up by string on every callback through `getRawParameterValue()` (`PluginProcessor.cpp:17-20, 269-272`). The returned atomic value is appropriate for real-time reads [2], but the pointer should be cached once in the constructor. The current bypass read also uses the default atomic memory order while the other reads use relaxed order; a consistent relaxed load is sufficient for scalar parameter snapshots because no dependent data structure is published with those values.
* `outputGain` and `saturation` are smoothed, but filter cutoff/gain, compressor settings, de-esser amount, parallel mix, reverb parameters, delay mix, and delay time are changed directly at block boundaries. JUCE's `SmoothedValue` is specifically intended to avoid audio glitches and supports sample-based ramps [4]. The gain-only smoothing pattern shown by JUCE's own tutorial is a useful baseline [3].
* The custom `bypass` parameter causes `processBlock()` to return immediately (`PluginProcessor.cpp:267-270`), but the processor does not override `getBypassParameter()`. JUCE exposes `getBypassParameter()` so wrappers and hosts can understand plug-in bypass [1]. A private parameter that only the custom editor uses is not equivalent to host bypass integration.
* The processor does not override `reset()`. `releaseResources()` resets `ChannelDSP`, but a host can reset a processor while keeping it prepared. Delay, reverb, compressor, filter, de-esser, and smoother state should be reset in the processor's `reset()` override, together with a defined output-gain state.
* The code does not call `setLatencySamples()`. The current delay is a wet parallel effect, so it does not automatically create reported plug-in latency; the placeholder tuning stage also adds no delay. The correct current report is therefore probably zero samples, but that should be stated explicitly in initialization. If a real pitch algorithm, lookahead compressor, or fixed alignment delay is added, the actual delay must be reported as soon as it is known and whenever it changes [1].
* `getTailLengthSeconds()` returns a fixed `4.0` (`PluginProcessor.h:22`) without a measured relationship to the reverb and feedback delay. The value should be a defensible upper bound for the enabled effect, and its assumptions should be documented. A tail is not the same thing as latency.
* `getStateInformation()` correctly starts from `parameters.copyState()` and serializes XML. `setStateInformation()` checks the APVTS root type before calling `replaceState()` (`PluginProcessor.cpp:291-292`), matching the official APVTS tutorial [3]. The custom `lastPreset` value should be protected against concurrent access, and state loading should include a version/schema property and a clear policy for old or malformed states.
* `applyAiJson()` is called by the AI worker (`PluginProcessor.cpp:184-220, 296-302`). It calls `setValueNotifyingHost()` from that worker. This is a cross-thread state/host-notification operation that should be marshalled to the message thread or represented as a lock-free command/result hand-off. JSON values also need finite-value and range validation before becoming parameters.
* `lastPreset` is written by factory-preset UI calls and by `applyAiJson()` on the worker, while it can be read during state saving. That is a data race. The same ownership problem exists in `onAiStatusChanged`: `setAiStatus()` reads and calls the `std::function` from the worker while the editor writes it and clears it in its destructor (`PluginProcessor.cpp:293`, `PluginEditor.cpp:28-30`). The queued `callAsync` lambda captures the raw `this` processor pointer and can run after the editor or processor lifetime has changed.
* The editor's timer only changes the label while AI is busy (`PluginEditor.cpp:32`). It does not poll and display the final status. The callback is therefore both unsafe and unnecessary. A message-thread timer can read a processor-owned status snapshot and update the label without storing a callback into the processor.
* The `key`, `scale`, and `retuneSpeed` parameters are exposed but not used by the DSP (`PluginProcessor.cpp:231-255, 267-272`). This is not directly a real-time violation, but it creates misleading automation/state semantics. Either implement their documented behavior, or remove/deprecate them while preserving stable IDs for released plug-ins.

## 2. Recommended processing architecture

### 2.1 Make the audio callback bounded and allocation-free

Move all topology changes, buffer allocation, coefficient-object creation, and expensive state construction out of `processBlock()`. At minimum:

1. Cache `std::atomic<float>*` parameter pointers once in the processor constructor. Read them with `load(std::memory_order_relaxed)` into local scalar snapshots at the start of each block. Do not call `getParameter()`, perform string searches, or touch `ValueTree` state in the callback.
2. Store the prepared channel count and maximum block size. In `prepareToPlay()`, size scratch buffers for the actual current layout, not an unconditional two channels. Add a debug assertion and a production fallback for `buffer.getNumSamples() > preparedMaximum` or an unsupported channel count. Do not resize from `processBlock()`; split into bounded sub-blocks only if every DSP object and scratch buffer supports that safely, or clear/fail gracefully.
3. Replace `makeCopyOf()` with a design whose capacity is established before playback. A scratch `AudioBuffer` should be copied using known-valid channel pointers and the current sample count after a preflight check. If mono and stereo use different processing paths, prepare both paths or use a channel-independent temporary layout.
4. Stop calling coefficient factory functions on every callback. Viable designs include a preallocated coefficient representation updated by an allocation-free formula, a filter type with an allocation-free parameter update, or a non-real-time parameter-change path that constructs a complete immutable coefficient set and hands it to the audio thread using a safe publication scheme. Do not place a `ReferenceCountedObjectPtr` replacement whose last reference can be destroyed on the audio thread unless its lifetime strategy has been explicitly designed for that thread.
5. Keep the processing graph fixed between `prepareToPlay()` and `releaseResources()`. Parameter changes should change scalar targets or already-existing state, not create/destroy DSP objects.

A safe structural outline is:

```cpp
void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) override
{
    juce::ignoreUnused (midi);
    juce::ScopedNoDenormals noDenormals;

    const auto channels = buffer.getNumChannels();
    const auto samples  = buffer.getNumSamples();
    jassert (channels <= preparedChannels && samples <= preparedMaximum);

    if (channels > preparedChannels || samples > preparedMaximum)
    {
        buffer.clear();                 // deterministic fail-safe; never resize here
        return;
    }

    const auto bypass = bypassParameter->load (std::memory_order_relaxed) > 0.5f;
    if (bypass)
    {
        processBlockBypassed (buffer, midi);
        return;
    }

    const auto hp = highPassParameter->load (std::memory_order_relaxed);
    // Read the remaining cached atomics once, then process only prepared state.
    dsp.process (buffer, hp /* ... scalar snapshot ... */);
}
```

The exact fallback policy can instead pass safe sub-blocks to the DSP, but silently resizing scratch buffers in the callback is not an acceptable default.

### 2.2 Use smoothing according to the parameter's signal role

A parameter needs smoothing when a discontinuity changes the waveform or a stateful filter. Use a sample ramp for output gain, dry/wet mixes, saturation drive, de-esser gain, compressor controls that are audible when automated, and any delay-time transition that would otherwise create a discontinuity. Use a multiplicative smoother for frequency-like values where the perceptual trajectory should be logarithmic; JUCE documents that multiplicative smoothing cannot reach zero [4].

For the four IIR filters, do not replace coefficients abruptly at high-rate automation. Choose one of these explicit policies:

* smooth the user frequency/gain parameter and update coefficients at a controlled rate with a coefficient implementation that cannot allocate;
* crossfade two pre-existing filter instances when a topology or coefficient discontinuity cannot be avoided; or
* use a filter implementation whose state-variable parameters can be updated safely per block or per sample.

The compressor and reverb settings should be set only when the target changes, not unconditionally on every callback. This reduces work and makes it easier to test that the setters are real-time-safe. `reverb.setParameters()` is currently called for every block (`PluginProcessor.cpp:122-129`); cache the last parameter set and update it only when needed, provided the setter's JUCE implementation is verified allocation-free.

The current `saturation` smoother is a good direction, but initialize it to the actual prepared target rather than zero if the first block must be click-free and numerically robust. Keep a non-zero denominator guard around `tanh(drive)` if the drive range is later changed.

### 2.3 Treat bypass as a host-facing audio behavior

Create or retain one canonical bypass parameter and return it from `getBypassParameter()`. Do not maintain a separate host bypass and editor bypass unless their relationship is deliberate. Implement a click-safe bypass transition: either use JUCE's `processBlockBypassed()` contract, or ramp the processed/dry mix over a short period while allowing effect tails to be handled intentionally. If bypass should flush delay/reverb state, do so in a controlled reset path rather than by returning early and leaving stale tails in the buffers.

The current early return also skips output-gain smoothing and every state update. That may be acceptable for a hard bypass, but it should be a documented choice and tested under host automation. The host's bypass button must be tested separately from the editor's button.

## 3. APVTS parameters and state

### 3.1 Parameter access and IDs

APVTS is the correct owner for host-facing values. Its `getRawParameterValue()` API returns an atomic floating-point representation specifically so a real-time process can read a parameter [2]. Cache those pointers. Keep IDs stable once a plug-in is released; names and display labels can change, but IDs are serialized and automated by hosts. JUCE's parameter tutorial also notes that normalisable values are stored in the host-facing `0..1` domain and that `NormalisableRange` controls mapping and text conversion [11].

Review each range and label:

* `air` is a `0..1` parameter but is multiplied by `8.0f` before filter construction. Give the user-facing parameter a clear macro meaning and keep the DSP conversion in one named function.
* `delayMs` has a linear 20–800 ms range. A skewed/logarithmic `NormalisableRange` is usually easier to automate musically across a wide time range.
* `compRatio` uses a linear 1–12 range. Consider whether a perceptual mapping is more useful, but preserve the existing ID if changing it.
* `key`, `scale`, and `retuneSpeed` must either affect the implemented pitch stage or be removed from the visible contract. If they are intentionally reserved, document them as not currently functional rather than implying that automation changes audio.

When setting a parameter from a factory preset or AI result, use the parameter's range, reject non-finite values, clamp to the range, and call `setValueNotifyingHost()` only from the designated non-audio command path. Factory preset loading is initiated by the UI, but AI loading currently is not.

### 3.2 State serialization

The current `copyState()`/XML/`replaceState()` pattern is aligned with JUCE's official APVTS tutorial [3]. Strengthen it as follows:

* check the binary/XML size before parsing and reject malformed data without changing the current state;
* check the root type and a schema/version property;
* read custom metadata from the restored `ValueTree`, not a separately maintained unsynchronised string;
* use a safe default for missing properties and let APVTS defaults fill missing parameters;
* make `lastPreset` either a property owned by the APVTS state or an atomic/message-thread-owned value with one clear writer;
* never store the API key in plug-in state, which the current project README correctly avoids.

State callbacks normally perform serialization and can allocate; they must not be called by the audio callback. Conversely, do not assume that state replacement and custom metadata access are safe while a worker thread is writing the same members. Define the host/thread contract and protect or serialize the hand-off.

## 4. Thread separation and editor/processor communication

Timur Doumler summarizes the real-time rule well: an audio callback has a hard deadline, and allocation, system calls, I/O, or waiting on a mutex can cause an audible glitch [7]. A scalar can be shared with an atomic; streams of commands can use a bounded single-producer/single-consumer FIFO; larger changing structures should use immutable snapshots or a safe ownership-transfer scheme rather than locking the audio thread [7].

The AI network request correctly avoids the audio thread, but the hand-off after the request does not. Use this ownership model:

1. The editor sends a bounded request to the worker. Copy the prompt and API key before starting the request; reject a second request or replace it deterministically.
2. The worker performs network and JSON work only on the worker thread. It validates a complete preset into a plain, finite, range-clamped value object.
3. The worker publishes that value object to a processor-owned pending-result slot under a short non-audio lock, or through a bounded FIFO. It then triggers a message-thread `AsyncUpdater` owned by the processor/editor lifetime model.
4. The message thread consumes the result and performs `setValueNotifyingHost()` and the `lastPreset` update. If a host requires parameter changes to be serialized differently, use a message-thread timer/async callback and document it.
5. The editor reads status through a message-thread-safe snapshot. A timer that always polls `getAiStatus()` is simpler than a raw `std::function` callback stored in the processor.
6. Destruction first prevents new requests, signals cancellation, waits for the worker, then invalidates any queued UI update. No queued lambda should capture a raw pointer whose lifetime is not guaranteed.

The current `setAiStatus()` has an unsynchronised read of `onAiStatusChanged`, and the editor destructor races with that read. Clearing the callback does not cancel already queued callbacks. Replace it with one of these patterns:

* processor-owned `juce::ChangeBroadcaster` plus an editor listener that is removed before destruction;
* editor-owned `AsyncUpdater`/timer with a processor status snapshot; or
* a lifetime-safe message mechanism that does not capture a raw processor/editor pointer.

Do not call UI methods from the worker. Do not make `AudioProcessorValueTreeState::Listener::parameterChanged()` perform UI work or allocate: JUCE warns that the callback is synchronous and that reading the parameter again inside it is not guaranteed to return the new value; use the callback's `newValue` argument [8]. If a future implementation adds APVTS listeners, they should only publish atomics or enqueue a bounded event.

## 5. Latency, tail, denormals, and lifecycle contracts

### Latency

JUCE requires the processor to report the number of samples of delay it imposes. It should call `setLatencySamples()` during initialization and may call it later if the value changes [1]. For the current algorithm, the wet delay is parallel with a dry path and should not be counted as plug-in latency. Set and document zero latency in `prepareToPlay()` unless measurement proves otherwise. If the future tuner needs lookahead or a phase-vocoder window, report that fixed or changing latency and test host compensation after every parameter transition.

### Tail

`getTailLengthSeconds()` should describe the maximum audible decay after input stops. It should account for the reverb configuration and feedback delay, not the user interface's nominal effect name. Measure the actual decay at the maximum allowed settings or document a conservative bound. Test hosts that use the tail for offline rendering and silence detection.

### Denormals

`juce::ScopedNoDenormals` is an RAII helper that temporarily disables CPU denormals [5]. It is correctly placed at the top of `processBlock()` (`PluginProcessor.cpp:269`). Keep it around the complete audio callback, including helper DSP calls. It does not make allocations, locks, callbacks, or other unrelated work real-time safe. The de-esser envelope and feedback/reverb state should still be tested for finite output and stable decay; use explicit tiny-value clamping only if measurement shows a need.

### Reset and sample-rate changes

Override `AudioProcessor::reset()` and reset every stateful DSP object and smoother. In `prepareToPlay()`, handle sample-rate changes by rebuilding sample-rate-dependent coefficients and delay limits before the next callback. The current `delaySamples` calculation is bounded to 190,000 samples while the delay line is constructed with 192,000 samples; make the relationship explicit and assert that the requested delay cannot exceed capacity at the current sample rate.

## 6. Validation plan

The repository's build is useful evidence that the code compiles, but compilation does not test real-time behavior. Run the following after each safety refactor.

### Static and build checks

Build Debug and Release with warnings enabled. Add AddressSanitizer and UndefinedBehaviorSanitizer configurations for non-host unit tests. Run a thread-sanitizer configuration against processor/worker/editor hand-off tests where the host wrapper does not create false positives. Search the audio callback and all functions reachable from it for `new`, `delete`, `std::function`, `String` construction, JSON/XML, URL/I/O, locks, `ValueTree`, and container growth.

### Host-style plug-in validation

Use Tracktion's `pluginval`. Its project describes it as a cross-platform validator, runs validation in a separate process, supports headless CI, and recommends `--strictness-level 5 <path_to_plugin>` as a baseline; higher levels add parameter fuzzing and repeated state restoration [10]. Run at least levels 5 and 10 on the VST3 bundle. Treat any realtime-safety, crash, state, or parameter-fuzz failure as a release blocker.

Also run the JUCE AudioPluginHost and at least one production DAW. Exercise:

* mono and stereo layouts, including transitions through `prepareToPlay()`;
* 44.1, 48, 88.2, 96, and 192 kHz sample rates;
* block sizes 1, 16, 31, 32, 127, 128, 257, 512, 1024, and a block larger than the nominal prepare size;
* automation of every parameter at audio rate and at block boundaries;
* bypass from the editor, host bypass, and automated bypass;
* state save/load before playback, during playback, after malformed state input, and across a second processor instance;
* repeated editor open/close while AI status updates are pending;
* worker completion, timeout, malformed JSON, cancellation, and processor destruction during a network request;
* silence and near-silence for denormal/NaN/Inf detection;
* delay/reverb tail behavior and reported latency against an impulse-measurement test.

A useful audio assertion is that every output sample is finite and that the maximum absolute sample remains within a documented bound for bounded input. A useful allocation test is to run the processor under a real-time allocation checker while automating filters and changing block sizes. A useful race test repeatedly opens/closes the editor while the worker publishes status and AI results.

## 7. Prioritized implementation checklist

| Priority | Location | Finding | Recommended change | Release criterion |
| --- | --- | --- | --- | --- |
| P0 | `ChannelDSP::updateFilters()` | Coefficient factories run in `processBlock()` and can allocate/reference-count on the audio thread. | Replace with allocation-free coefficient updates or a prebuilt immutable hand-off. | Realtime checker and parameter fuzzing show no audio-thread allocation or glitch. |
| P0 | `ChannelDSP::process()` and `prepare()` | Scratch buffers are copied in the callback; prepared size/channel count is not enforced. | Prepare for the active layout, guard variable blocks, and never resize in the callback. | Mono/stereo and oversized-block tests pass without allocation or memory errors. |
| P0 | `setAiStatus()`, editor destructor | `std::function` callback is raced and queued raw-pointer callbacks can outlive the editor/processor. | Replace with timer/snapshot or lifetime-safe async broadcaster; cancel pending updates. | Repeated editor destruction during worker activity is race- and crash-free. |
| P0 | `applyAiJson()` | Worker directly calls `setValueNotifyingHost()` and writes `lastPreset`. | Validate on worker, publish a result, and apply parameters/metadata on the designated message-thread path. | Thread sanitizer and host automation/state tests pass. |
| P1 | `processBlock()` / bypass | Custom bypass is not returned by `getBypassParameter()` and early return leaves state/tail policy implicit. | Implement canonical host bypass and click-safe behavior; add `reset()`. | Host bypass and editor bypass behave consistently without clicks. |
| P1 | latency/tail methods | No latency is reported; tail is an unexplained fixed four seconds. | Call `setLatencySamples(0)` for the measured current design; measure/document tail; update when DSP changes. | Impulse and tail tests agree with host-reported values. |
| P1 | parameter reads | Repeated string lookup and mixed atomic load style in the hot path. | Cache raw parameter pointers; snapshot with relaxed loads. | Code review confirms no APVTS lookup/value-tree access from audio callback. |
| P1 | smoothing | Many audible controls jump at block boundaries. | Add role-appropriate `SmoothedValue` ramps or allocation-free coefficient interpolation. | Automated sweeps produce no zipper/click artifacts. |
| P2 | parameter contract | `key`, `scale`, and `retuneSpeed` are exposed but unused. | Implement, hide, or deprecate without breaking IDs; document behavior. | Every automatable parameter changes a defined behavior or is explicitly reserved. |
| P2 | validation | Existing verification is primarily a compile/build claim. | Add pluginval, host, variable-block, state, race, sanitizer, and audio-finiteness tests to CI/release checklist. | Repeatable validation logs are archived for each release. |

## References

[1]: https://docs.juce.com/master/classjuce_1_1AudioProcessor.html "JUCE AudioProcessor class reference"

[2]: https://docs.juce.com/master/classjuce_1_1AudioProcessorValueTreeState.html "JUCE AudioProcessorValueTreeState class reference"

[3]: https://juce.com/tutorials/tutorial_audio_processor_value_tree_state/ "JUCE tutorial: Saving and loading your plug-in state"

[4]: https://docs.juce.com/master/classjuce_1_1SmoothedValue.html "JUCE SmoothedValue class reference"

[5]: https://docs.juce.com/master/classjuce_1_1ScopedNoDenormals.html "JUCE ScopedNoDenormals class reference"

[6]: https://juce.com/tutorials/tutorial_dsp_introduction/ "JUCE tutorial: Introduction to DSP"

[7]: https://timur.audio/using-locks-in-real-time-audio-processing-safely "Timur Doumler: Using locks in real-time audio processing, safely"

[8]: https://docs.juce.com/master/structjuce_1_1AudioProcessorValueTreeState_1_1Listener.html "JUCE APVTS Listener class reference"

[9]: https://docs.juce.com/master/structjuce_1_1dsp_1_1IIR_1_1Coefficients.html "JUCE DSP IIR Coefficients class reference"

[10]: https://github.com/Tracktion/pluginval "Tracktion pluginval: Cross-platform plugin testing and validation tool"

[11]: https://juce.com/tutorials/tutorial_audio_parameter/ "JUCE tutorial: Adding plug-in parameters"

> **Documentation note.** The linked JUCE API pages are the current official documentation pages and may be labelled `master`; the project itself pins JUCE `8.0.10`. Before adopting an API-specific change, verify the exact declaration and behavior in the pinned JUCE checkout and rerun the validation matrix.
