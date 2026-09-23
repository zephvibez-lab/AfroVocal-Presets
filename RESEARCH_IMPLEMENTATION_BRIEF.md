# AfroVocal Presets
## JUCE 8 Vocal VST3 implementation brief

**Status:** Implementation-oriented synthesis of the supplied research results  
**Target:** Windows x64 VST3, built with JUCE 8 and Visual Studio/MSVC  
**Product proposition:** A fast, musical vocal processor for Afrobeat, Amapiano, Afropop, and adjacent vocal use cases. It should provide a small macro surface that reaches a credible sound quickly, while retaining inspectable pitch, dynamics, tone, space, and harmony controls for expert users.

## 1. Executive recommendation

Build AfroVocal Presets as a **real-time-first vocal channel strip with optional offline analysis**. The real-time path must remain deterministic, bounded, and safe for tracking. It should expose musical context explicitly through key, scale, vocal range, correction strength, and an optional MIDI note source. A separate Assistant view may analyze an approved audio excerpt or rendered feature summary and suggest parameters, but it must never perform network, disk, allocation, blocking, or model-loading work in `processBlock()`.

The product should use two levels of control. The **Play** view offers a small number of macros such as Vocal Type, Tuning, Body, Presence, Control, Air, Space, Throw, Width, and Output. The **Edit** view exposes the component parameters, meters, pitch-confidence state, scale keyboard, de-essing activity, gain reduction, latency, and wet/dry routing. This combines the macro speed of CLA Vocals with the explicit context and safety controls seen in Waves Tune Real-Time, and with the inspectability associated with Auto-Tune Graph Mode and Melodyne. [1] [2] [3] [4] [5]

Factory presets should be **starting points rather than promises**. Mic, singer, key, arrangement, tempo, monitoring, and performance style materially change the correct settings. Amapiano guidance in this brief is an adaptation of current vocal-engineering principles and documented Afrobeat workflows; it is not a claim that every Amapiano record uses one fixed chain. [6] [7] [8]

## 2. Design principles

### 2.1 Preserve musical intent before adding character

The default chain should clean and stabilize the vocal without erasing consonants, breaths, melismas, or conversational phrasing. Clip-gain edits belong before the plug-in where possible. The processor should make correction and compression visible, level-match bypass, and avoid using output loudness as a substitute for improvement.

### 2.2 Separate live correction from surgical correction

The default **Live** mode uses low-latency pitch correction and conservative look-ahead. It is intended for tracking and monitoring. The optional **Analyze** mode can inspect an offline buffer or host-provided selection and produce note/phrase suggestions, but it should not pretend to be a full Melodyne replacement. Auto-Tune's separation of Auto and Graph workflows, and Melodyne's visually inspectable note editing, support this division. [1] [3]

The first release should not promise destructive note-by-note editing inside the VST3. Instead, it should provide safe live controls, optional offline suggestion markers, per-note allow/avoid rules, and a clearly labeled “apply to real-time chain” action. Any algorithm or analysis-mode change must invalidate or re-run dependent suggestions rather than silently reinterpreting edits. Melodyne's warning that algorithm changes can discard existing edits is a useful safety precedent. [3]

### 2.3 Make pitch context explicit

Pitch correction must require or strongly encourage a root and scale. Provide chromatic, major, natural minor, harmonic minor, melodic minor, pentatonic, blues, and user-defined scales in the first release. Add vocal range and a per-note enable/disable keyboard. The engine should show detected pitch, confidence, selected target note, and correction amount. A tuner that merely selects the nearest legal note cannot understand singer intent; scale, range, tolerance, and per-note rules are therefore guardrails, not advanced decoration. [2] [4]

### 2.4 Use progressive disclosure

A preset and a macro should be sufficient to reach a useful result. Advanced users should be able to open component editors for tuning, EQ, de-essing, compression, saturation, reverb, delay, width, and harmony. Keep expert controls behind an Edit panel rather than putting every threshold and time constant on the first screen. This reduces parameter hunting while preserving repeatability and automation.

### 2.5 Treat naturalness and latency as product features

Expose a Natural–Modern–Hard tuning character, a formant-preserve switch, vibrato protection, correction tolerance, and a correction meter. Report actual plug-in latency to the host whenever oversampling or look-ahead changes. Live mode should prefer low-latency IIR oversampling or no oversampling where sonically acceptable; Character mode may use higher-quality FIR oversampling and report its latency. Latency transitions must be tested in each target host rather than assumed to be safe. [2] [9] [10]

### 2.6 Keep effect returns independent from the dry vocal

Reverb and delay should be 100%-wet internal sends or parallel buses. Their returns need independent filtering, ducking, de-essing, automation, and level control. This keeps the lead forward while allowing quarter-note throws, short plates, filtered delays, and emotional tails. [6] [7]

### 2.7 Make Afro-oriented choices configurable, not stereotyped

The product should optimize for forward leads, clean low-mid space, intelligible presence, controlled sibilance, rhythmic throws, hook doubles, and moving background stacks. It must not encode “Afrobeat” as one fixed EQ curve or tuning speed. The preset names should describe use and behavior, and all presets should be starting points that can be level-matched and modified.

## 3. Proposed user experience and parameter model

### 3.1 Main views

**Play view.** Select a preset, vocal role, key/scale, and tuning character. Adjust ten macros: Tune, Body, Presence, Air, Control, De-ess, Saturation, Space, Throw, and Width. Show input/output meters, gain reduction, pitch-confidence/target display, active scale, and reported latency.

**Edit view.** Expand the chain into Tuning, Cleanup EQ, De-esser, Compressor, Tone, Saturation, Lead/BGV bus, Reverb, Delay, Doubles/Harmonizer, and Output. Every module has an enable/bypass control. Use parameter IDs that remain stable across versions; preset names and macro mappings may evolve, but serialized IDs must not.

**Assistant view.** Offer Local and Online modes, an explicit consent state, an Analyze button, a source-duration limit, analysis progress, a proposed preset diff, and Accept/Reject/Undo. The assistant must explain that it proposes starting values rather than mixing decisions.

### 3.2 Recommended parameter ranges

The ranges below are implementation ranges. They intentionally exceed some suggested starting points so automation and creative effects remain possible. Factory presets should use conservative values inside them.

| Group | Parameter | Proposed range and default behavior |
|---|---|---|
| Input | Input trim | -24 to +12 dB; default 0 dB. Use for gain staging, not loudness maximization. |
| Input | Output trim | -24 to +12 dB; default level-matched to bypass. |
| Pitch | Root | C through B, with host/MIDI-follow option. |
| Pitch | Scale | Chromatic, major, minor variants, pentatonic, blues, custom. |
| Pitch | Retune speed | 0–100 ms. Factory natural leads: 25–50 ms; modern hooks: 10–25 ms; hard effect: 0–10 ms. The 10–30 ms Afrobeats starting region and broader transparent/expressive guidance support these presets. [6] [11] |
| Pitch | Humanize | 0–100%; factory 0–30% for leads and higher for sustained natural material. |
| Pitch | Flex/tolerance | 0–100%; factory 5–15% for exposed leads. |
| Pitch | Correction amount | 0–100%; allow per-note bypass/allow rules. |
| Pitch | Formant shift | -12 to +12 semitones, with Preserve enabled by default and safety limiting. |
| Pitch | Vocal range | Low, Mid, High or custom lower/upper note. |
| Pitch | Vibrato protection | 0–100%; reduces correction of intentional periodic pitch movement. |
| EQ | High-pass frequency | 20–200 Hz, 12 or 24 dB/oct; factory commonly 80–120 Hz, source-dependent. |
| EQ | Low-mid cleanup | 0–6 dB cut, centered 100–400 Hz with adjustable Q; avoid over-cutting intimate Amapiano leads. |
| EQ | Nasal dynamic band | 300 Hz–1.5 kHz, up to 6 dB dynamic attenuation. |
| EQ | Presence band | 1.5–6 kHz, up to +6 dB, preferably dynamic or broad. |
| EQ | Air shelf | 6–20 kHz, -3 to +6 dB; factory Afro lead values often +2–4 dB above 10 kHz when sibilance permits. [6] |
| De-esser | Detection | 2–20 kHz, default 6–10 kHz for normal vocal sibilance. |
| De-esser | Attenuation/range | 0–15 dB; factory target 2–5 dB on offending esses. Use more on stacks than leads. [12] |
| Compression | Ratio | 1:1–10:1; factory lead 2:1–3:1, dense hook up to 5:1. |
| Compression | Attack | 0.1–100 ms; protect consonants with moderate/slower factory values. |
| Compression | Release | 10–1000 ms; tempo/groove-aware presets. |
| Compression | Target gain reduction | 0–12 dB; factory average 2–4 dB and loud-hook peaks 4–6 dB. [6] [13] |
| Saturation | Drive | 0–24 dB internal drive with output compensation; subtle default. |
| Saturation | Mix | 0–100%; factory 5–25%, parallel character presets higher. |
| Reverb | Type | Plate, chamber, room, spring-like color. |
| Reverb | Decay | 0.2–3.0 s; factory lead plate 0.8–1.5 s; emotional selections 1.2–2.0 s. [6] |
| Reverb | Pre-delay | 0–120 ms; factory 20–60 ms. |
| Reverb | Return HP/LP | HP 80–400 Hz and LP 3–16 kHz; common factory return ranges 150–250 Hz and 6–10 kHz. |
| Reverb | Ducking | 0–100%; default moderate on rhythmic leads. |
| Delay | Time | Tempo-sync 1/16, 1/8, 1/4, 1/2, dotted options, or 1–2000 ms free time. |
| Delay | Feedback | 0–80%; factory 10–30% or one clean repeat. |
| Delay | Return HP/LP | HP 80–500 Hz and LP 2–12 kHz; common throw range 150–300 Hz and 5–8 kHz. |
| Delay | Throw amount | 0–100%; automate or MIDI/host-trigger the send. |
| Width | Double level | -36 to 0 dB relative to lead; factory doubles 4–6 dB below lead. [6] |
| Width | Pan | -100 to +100%; doubles commonly ±10–30°, harmonies ±30–60° as starting points. |
| Width | Stereo width | 0–200%, mono-compatible by default below 150 Hz. |
| Output | Limiter ceiling | -1.0 to -0.1 dBTP; default -1.0 dBTP for safety. |

The plug-in should expose a clear **manual gain-staging hint** rather than enforce a genre-specific target. A useful starting point is an unprocessed vocal peak around -12 to -6 dBFS, followed by level-matched bypass comparison. This is an engineering starting point, not a loudness standard. [6]

## 4. DSP signal flow

### 4.1 Core chain

```text
Host audio/MIDI
  -> channel/bus validation and input trim
  -> optional clip-gain-like input ride (manual macro only in V1)
  -> pitch detector and confidence estimator
  -> real-time pitch correction with root/scale/range/per-note rules
  -> corrective EQ and optional dynamic low-mid/nasal control
  -> optional light pre-de-esser
  -> serial compressor: peak catcher then gentle leveller
  -> tonal EQ and oversampled subtle saturation
  -> main de-esser
  -> lead/BGV bus leveling and width management
  -> parallel wet sends:
       filtered/ducked reverb
       filtered/ducked tempo delay and throw engine
       optional doubles/harmony processor
  -> dry/wet summing, output trim, safety limiter
  -> host output
```

Pitch correction should precede the main dynamics and time-based effects so the effects do not magnify unstable pitch. A pre-de-esser is optional; the main de-esser follows compression and additive tone processing because those stages can expose sibilance. The order remains user-configurable at the module level only if the implementation can preserve prepared, atomic DSP configurations. [6] [7] [8]

### 4.2 Bus behavior

The primary lead remains centered. Hook doubles should be lower than the lead and active mainly in hooks. Harmony pairs can be wider, while Amapiano choir-like stacks should share a BGV bus with stronger leveling, additional de-essing, and automated width. The first release can implement a controlled stereo doubles/harmony block rather than claim the realism of separately recorded singers; users should be encouraged to use real doubles when available. [6] [7]

Reverb and delay returns are always wet. The reverb return uses a high-pass filter, low-pass filter, and optional sidechain ducking from the dry lead. The delay return supports 1/16, 1/8, 1/4, and 1/2-note modes, ping-pong routing, feedback limiting, and a filtered feed into reverb. The default behavior is automation-friendly: effects are quiet until a throw or sustained phrase calls for them.

### 4.3 JUCE 8 real-time implementation constraints

Use `AudioProcessorValueTreeState` with a stable `ParameterLayout`. Cache raw parameter pointers during setup and read their atomic scalar values in the callback. Do not perform string lookup, state copying, allocation, logging, file/network I/O, or lock acquisition in `processBlock()`. Handle zero-length blocks, variable block sizes, and more output channels than input channels; clear every output channel that the processor owns. [14] [15] [16]

Prepare all DSP objects with a `dsp::ProcessSpec` containing the host sample rate, maximum expected block size, and channel count. Pre-size scratch buffers and bounded event queues. Use `SmoothedValue` for gain, frequency, mix, and other continuous controls; use a separate mute/zero path for multiplicative parameters that cannot reach zero. Use `ScopedNoDenormals` in the callback where feedback filters, envelopes, or silence tails can produce subnormal values. [17] [18]

Use `dsp::Oversampling` only around nonlinear stages. Configure its factor and filter quality outside the callback, call `initProcessing()` with the maximum block size, process up, run saturation at the higher rate, and process down. Choose integer-latency mode when host alignment requires it, compensate the dry path, and call `setLatencySamples()` whenever a user-configurable oversampling mode changes. [18] [19]

Any Assistant result, preset rebuild, scale change that requires topology work, or harmony-model update must be created off the audio thread and exchanged as a complete immutable/prepared configuration. The callback may see the old configuration or the new configuration, never a partially written one. State serialization belongs in `getStateInformation()` and `setStateInformation()` outside the audio callback, with a version tag and migration defaults. [14] [16]

## 5. Factory preset concepts

The factory bank should contain 16 concepts. Each preset needs a short description, a vocal-role tag, a genre/use-case tag, and a visible “what changed” summary. Values below are design targets, not fixed recipes.

| Preset | Genre/use case | Starting concept |
|---|---|---|
| **Lagos Forward Lead** | Afrobeats lead | Natural-to-modern tuning at 20–30 ms, 3:1 compression with 4–6 dB hook reduction, gentle 200–400 Hz cleanup, presence lift, short plate, and restrained 1/4-note throw. |
| **Afro Pop Air** | Afropop / radio lead | 25–40 ms tuning, modest Humanize, controlled de-essing, +2–3 dB air shelf above 10 kHz, bright filtered plate, and low saturation mix. |
| **Amapiano Intimate** | Amapiano verse | 30–50 ms tuning, preserved low-mid body, moderate compression, narrow/center image, short chamber, and minimal delay so the lead stays close while log drums remain clear. |
| **Amapiano Hook Lift** | Amapiano chorus | 15–25 ms tuning, hook-only stronger correction, brighter presence, parallel density, wider doubles, and an automated 1/4 throw on phrase endings. |
| **Log Drum Pocket** | Amapiano dense arrangement | Conservative body cut, ducked low-mid return, tight dynamics, filtered 1/8 delay, short plate, and reduced width below the vocal presence region. |
| **Zouglou Conversation** | Conversational Afro vocal | 35–50 ms tuning, low compression ratio, consonant-preserving attack, minimal air, short room, and no constant delay. |
| **Afro-R&B Silk** | Afro-R&B / intimate melody | 35–60 ms tuning, gentle formant preservation, serial leveling, smooth de-essing, 1.2–1.8 s chamber, and filtered half-note tail. |
| **Dancehall Crossover** | Afro-dancehall / energetic lead | 10–25 ms tuning, denser compression, controlled upper-mid presence, short plate, and rhythmic 1/8 throw. |
| **Hook Lock Modern** | Stylized pop hook | 0–15 ms tuning, lower Humanize, clear correction meter, tighter vocal bus, bright but de-essed tone, and pronounced throw automation target. |
| **Natural Long Melisma** | Emotional sustained lead | 35–70 ms tuning, higher Humanize and Flex/Tolerance, vibrato protection, moderate compression, 1.5–2.0 s ducked plate/chamber, and selective delay. |
| **Afro Chant Mono** | Chant, call-and-response, center vocal | Low-to-moderate correction, compact mono lead, parallel saturation, short room, and a tempo-synced repeat that can be triggered on response words. |
| **Welélel Stack** | Choir-like Amapiano/Afrobeats BGV | Stronger stack de-essing, 2–6 dB BGV bus control, high-pass/low-pass cleanup, multiple tempo delays, wide automated panning, and shared plate. |
| **Harmony Pair Wide** | Recorded or generated harmony pair | Center-safe lead blend, ±30–60° harmony placement, modest pitch unification, width automation, and filtered reverb return. |
| **Late-Night Alté** | Alté / understated Afro alternative | Natural tuning, low saturation, soft low-mid shaping, restrained compression, darker room, and sparse half-note throws. |
| **Festival Megaphone** | Energetic live/festival vocal | Faster tuning, stronger parallel distortion, controlled mid emphasis, short gated/plate space, wider hook doubles, and output safety limiting. |
| **Bedroom Clean-Up** | Home-recorded or untreated vocal | High-pass and dynamic cleanup, conservative de-essing, transparent leveling, subtle saturation, low-latency short room, and a prominent input/output gain hint. |

Preset QA must verify that the initial output is not materially louder than bypass, that the key/scale field is visible, and that no preset causes excessive sibilance, pumping, clipping, or stereo incompatibility on representative vocals.

## 6. AI Assistant architecture

The assistant is an **asynchronous recommendation system**, not an audio-thread processor. Both Online and Offline modes should emit the same versioned result contract:

```json
{
  "schema": 1,
  "analysis": {"vocalRole": "lead", "tempo": 112, "key": "F#", "scale": "minor", "confidence": 0.82},
  "recommendation": {"presetId": "amapiano_hook_lift", "parameters": {}, "warnings": []},
  "explain": ["Increase correction only on exposed hook phrases"]
}
```

### 6.1 Offline/local mode

The local analyzer should be the default for privacy and predictable operation. It extracts bounded features from a user-approved buffer or from host-visible audio statistics: RMS and peak distribution, crest factor, spectral centroid, low-mid energy, sibilance-band energy, pitch track and confidence, voiced/unvoiced ratio, estimated tempo, and optional host/MIDI key context. A small versioned rules engine or local inference model maps those features to a preset and parameter deltas. The result is cached by input fingerprint and model version.

Local analysis must run on a worker thread. It may read a copied audio buffer, but it must not inspect mutable audio buffers while the host is writing them. The audio callback publishes only bounded statistics through a lock-free queue or atomically exchanged snapshot. If analysis is unavailable, the plug-in continues with the selected preset and manual controls. The local model must have hard limits on memory, duration, CPU time, and output parameter ranges.

### 6.2 Online mode

Online analysis is opt-in and should be disabled by default in a tracking session. The plug-in sends either a compact feature summary or a user-approved, short audio excerpt. It must display what is sent, why it is sent, retention/processing policy, and a cancel action. Use TLS, authenticated requests, request IDs, schema/version validation, bounded timeouts, retry limits, and a local cache. Never block the audio thread on a response. A failed, slow, malformed, or unavailable service falls back to the selected preset or Offline mode without changing the live chain unexpectedly.

The online service should return recommendations and explanations, not executable DSP code. Validate every returned parameter against the local schema, range, and safety policy. Apply suggestions only after user confirmation, or provide a clearly labeled Auto-Apply preference with a reversible undo. Do not upload unlicensed stems or assume consent from host transport state. The architecture should permit a future account/licensing layer without making basic processing dependent on an account or network.

### 6.3 AI limitations and user control

Key/scale and vocal-role classification can be wrong, particularly for modal melodies, slides, spoken/rapped material, stacked vocals, noisy recordings, and arrangements with misleading accompaniment. The Assistant must show confidence and warnings, allow manual correction, and never override key, scale, range, or per-note rules silently. No AI recommendation should alter latency, oversampling mode, or channel topology without an explicit confirmation and a prepared configuration swap.

## 7. Testing and acceptance plan

### 7.1 DSP unit tests

Create deterministic tests for pitch-control smoothing, scale and per-note rules, formant-preserve boundaries, EQ responses, de-esser attenuation, compressor gain reduction, saturation oversampling, delay feedback limits, reverb return filtering, width/mono behavior, limiter ceiling, reset behavior, and NaN/Inf handling. Use impulse, sine, chirp, silence, pink noise, voiced synthetic tones, and representative vocal fixtures. Assert stable output, bounded gain, expected attenuation/boost, and no unexpected DC or denormal behavior.

For nonlinear stages, render at multiple sample rates and oversampling factors. Measure alias-energy changes, latency, dry/wet impulse alignment, and output determinism. Test integer-latency mode and every user-facing oversampling choice. Compare repeated offline renders byte-for-byte where the host and floating-point path permit it, or use a documented numerical tolerance.

### 7.2 Real-time and concurrency tests

Exercise zero-length blocks, irregular block sizes, blocks larger than the `prepareToPlay()` estimate, mono/stereo and mismatched input/output channel counts, silence tails, MIDI events at block boundaries, automation at arbitrary sample offsets, rapid preset changes, scale changes, bypass, and transport start/stop. Instrument allocations, deallocations, locks, system calls, and logging on the audio callback in debug and stress builds. Concurrently update parameters and Assistant results while rendering; the callback must always observe a complete configuration.

Use compiler warnings, AddressSanitizer, UndefinedBehaviorSanitizer, ThreadSanitizer where compatible, and long-running fuzz/stress tests for state blobs and Assistant JSON. Verify that malformed or future-version state restores safe defaults rather than crashing or partially applying data.

### 7.3 Plug-in and host acceptance

Run Tracktion `pluginval` for the exact shipped VST3 binary on each release build, at strictness level 5 or higher. [20] Test at 44.1, 48, 88.2, and 96 kHz; at small tracking buffers and large offline buffers; and with automation and offline rendering. Validate Cubase/Nuendo, Studio One, REAPER, Ableton Live, FL Studio, and at least one Windows-on-Arm configuration if that architecture is supported. Confirm state recall, preset recall, parameter enumeration, automation names, bypass ramping, latency propagation, resize behavior, editor reopen, and scan/verification behavior.

Use impulse alignment to verify reported latency. Test host bypass during active reverb/delay tails and during oversampling changes. Confirm that a failed Online Assistant request never stalls or changes the audio path. Have experienced mixers perform blind level-matched comparisons against clean bypass and reference chains, focusing on tuning artifacts, consonant preservation, sibilance, pumping, low-mid masking, and mono compatibility.

## 8. Windows VST3 packaging plan

### 8.1 Build target and bundle

Build the release on Windows with Visual Studio/MSVC and CMake using the JUCE 8 checkout pinned by commit. Ship a native x64 VST3 first. A 32-bit x86 build should be a separately justified product target because a 32-bit host cannot load a 64-bit plug-in and a 32-bit plug-in in a 64-bit host requires bridging. [21] [22]

Preserve the modern bundle tree:

```text
AfroVocal Presets.vst3/
  Contents/
    Resources/
      moduleinfo.json
      icons and non-PE resources
    x86_64-win/
      AfroVocal Presets.vst3
```

The outer directory and inner binary use the same `.vst3` name. Do not flatten the inner PE file into the install directory. The canonical global destination is `C:\Program Files\Common Files\VST3`. A per-user development/no-admin destination may use `%LOCALAPPDATA%\Programs\Common\VST3`; validate it in the target hosts. FL Studio's current documentation also mentions `C:\Program Files\VST3`, but its legacy FL Studio VST folder is not the correct destination for a VST3. [21] [23] [24]

### 8.2 Manifest, signing, installer, and release artifacts

Use JUCE/CMake automatic VST3 manifest generation unless custom signing/post-processing requires disabling it. If custom processing is required, sign or modify the PE binary first, then generate/validate the manifest, then copy/install the completed bundle. Keep `moduleinfo.json` under `Contents\Resources` and run the SDK/JUCE validation tools as part of CI. [22] [25]

Sign the inner PE plug-in binary and any other shipped PE DLLs with Authenticode when a trusted distribution certificate is available. Use SHA-256 file and timestamp digests with SignTool. Signing is a distribution/trust measure, not VST3 registration; the directory, JSON, and other non-PE resources are not themselves signed PE files. [26] [27]

Provide a ZIP for developers and a conventional Inno Setup installer for users. The installer must recursively copy the complete `.vst3` directory and preserve `Contents`, architecture, resources, and manifest files. It should support user-selectable per-user installation where practical, show the exact destination, offer uninstall, and keep optional model/content data outside the plug-in bundle. Include a versioned changelog, SHA-256 checksum, installer architecture, supported Windows versions, and a clean-machine installation test. JUCE's packaging tutorial identifies Inno Setup as a practical Windows installer route. [25]

### 8.3 CI gates

A release candidate is blocked unless it passes: reproducible MSVC x64 build from a pinned dependency set; compiler warnings treated according to project policy; unit and stress tests; `pluginval` at strictness 5+; manifest validation; signature verification when signing is enabled; clean install/uninstall; host scan in FL Studio; preset/state recall; and artifact hash publication. Linux CI may build/test portable source or Linux VST3 separately, but the supplied evidence does not establish Linux-to-Windows MSVC cross-compilation as an equivalent release path. The Windows artifact must therefore be built and validated on Windows, whether on a native runner, VM, or Windows CI service. [21] [28]

## 9. Risks and limitations

1. **Genre overgeneralization.** Afrobeat, Afropop, Amapiano, Alté, dancehall crossover, and R&B vocals do not share one correct chain. Presets are curated starting points, not genre standards. The strongest genre-specific evidence is vendor-authored and the Amapiano recommendations are an adaptation. [6] [7]

2. **Pitch correction can choose the wrong note.** A tuner does not know the singer's intent. Wrong key/scale, range, note rules, slides, stacked vocals, and noisy detection can produce audible artifacts. The UI must expose confidence, root/scale, range, tolerance, formant handling, vibrato protection, and per-note rules. [2] [4]

3. **A real-time VST3 is not a full offline editor.** Without a dedicated host selection/ARA workflow, note-level editing is constrained. Do not market Assistant suggestions as Melodyne-like surgical correction.

4. **Latency can change with quality choices.** Oversampling and look-ahead can introduce integer or fractional internal latency. Host APIs report integer samples, so alignment, dry/wet compensation, and mode changes require explicit design and host testing. [18] [19]

5. **Real-time safety is a deadline property.** Avoiding an obvious mutex is not enough. Third-party DSP, allocator behavior, OS scheduling, denormals, CPU power states, and priority inversion can still cause glitches. Profile worst-case paths on supported hardware. [29] [30]

6. **Host behavior is not uniform.** Bypass ramps, state callbacks, block scheduling, offline rendering, latency propagation, scan paths, and editor lifetime differ across DAWs. Never infer complete safety behavior from another plug-in's marketing documentation.

7. **AI recommendations can be wrong or unavailable.** Online mode introduces consent, privacy, security, service availability, latency, cost, and data-retention concerns. Offline mode has a smaller model/feature budget. Both modes must be optional, bounded, explainable, range-validated, and reversible.

8. **Privacy and rights are operational risks.** User vocals may contain personal data or unreleased commercial recordings. Do not upload audio by default. Obtain explicit consent, disclose retention, provide deletion/support paths where applicable, and offer a fully local workflow.

9. **Generated harmonies are not recorded singers.** A harmonizer can widen or thicken a lead, but it may expose phase, formant, timing, or pitch artifacts. Real doubles should remain the recommended quality path.

10. **Saturation and bright EQ can expose sibilance.** De-essing must be source-dependent and level-matched. Excessive range can cause lisps; excessive air can make hats and delays harsh. [12]

11. **Windows packaging and architecture can fail independently of DSP.** Incorrect bundle flattening, wrong bitness, stale manifests, unsupported install paths, missing runtime dependencies, or unsigned/untrusted artifacts can prevent scanning even when the processor is correct. Validate the exact release tree on clean Windows hosts. [21] [22] [23]

12. **Documentation and SDK drift.** The cited official pages include current/master documentation and product pages whose versions, feature availability, paths, and licensing can change. Pin JUCE, VST3 SDK, Visual Studio, and third-party dependencies, and revalidate signatures and host behavior for each release. [14] [21]

## 10. Recommended implementation order

**Phase 1: deterministic core.** Create the JUCE 8 VST3 shell, stable APVTS layout, state versioning, channel handling, gain staging, pitch context, EQ, compression, de-essing, output safety, and latency reporting. Add meters and a minimal Play view.

**Phase 2: Afro-oriented buses and presets.** Add oversampled saturation, filtered/ducked reverb, tempo delay/throws, doubles/harmony width, BGV bus behavior, and the 16 factory concepts. Perform level-matched preset and mono-compatibility review.

**Phase 3: expert editing and Assistant.** Add Edit view, per-note rules, pitch-confidence display, Local analysis, result diff/undo, then an opt-in Online adapter behind the same versioned result contract. Keep the DSP path unchanged when the assistant is unavailable.

**Phase 4: release hardening.** Complete deterministic, real-time, host, `pluginval`, packaging, signing, clean-machine, and FL Studio scan tests. Ship x64 first. Add x86 or Windows-on-Arm variants only after separate architecture-specific validation.

## References

[1]: https://www.antarestech.com/documentation/auto-tune-pro-11 "Auto-Tune Pro 11 documentation"
[2]: https://www.waves.com/plugins/waves-tune-real-time "Waves Tune Real-Time product page"
[3]: https://helpcenter.celemony.com/M5/doc/melodyneAssistant5/en/M5tour_QuickStart_assistant?env=standAlone "Melodyne 5 Assistant quick start"
[4]: https://www.waves.com/1lib/pdf/plugins/tune-real-time.pdf "Waves Tune Real-Time user guide"
[5]: https://www.waves.com/plugins/cla-vocals "Waves CLA Vocals product page"
[6]: https://www.antarestech.com/blog/afrobeats-vocal-production-how-to-get-the-sound-thats-taking-over-global-music "Afrobeats vocal production: how to get the sound"
[7]: https://www.soundonsound.com/techniques/inside-track-burna-boy-time-flies "Inside Track: Burna Boy — Time Flies"
[8]: https://www.izotope.com/community/blog/vocal-cheet-sheet "iZotope Vocal Cheat Sheet"
[9]: https://www.izotope.com/community/blog/essential-tips-for-using-reverb-on-vocals "Essential tips for using reverb on vocals"
[10]: https://www.izotope.com/community/blog/guide-to-audio-effects "Guide to audio effects"
[11]: https://www.antarestech.com/blog/the-science-behind-auto-tune "The science behind Auto-Tune"
[12]: https://www.fabfilter.com/downloads/pdf/help/ffprods-manual.pdf "FabFilter plug-in manual"
[13]: https://www.izotope.com/community/blog/using-compression-to-help-vocals-sit-in-a-mix "Using compression to help vocals sit in a mix"
[14]: https://docs.juce.com/master/classjuce_1_1AudioProcessor.html "JUCE AudioProcessor API"
[15]: https://docs.juce.com/master/classjuce_1_1AudioProcessorValueTreeState.html "JUCE AudioProcessorValueTreeState API"
[16]: https://juce.com/tutorials/tutorial_audio_processor_value_tree_state/ "JUCE AudioProcessorValueTreeState tutorial"
[17]: https://docs.juce.com/master/classjuce_1_1SmoothedValue.html "JUCE SmoothedValue API"
[18]: https://docs.juce.com/master/classjuce_1_1dsp_1_1Oversampling.html "JUCE dsp::Oversampling API"
[19]: https://docs.juce.com/master/structjuce_1_1dsp_1_1ProcessSpec.html "JUCE dsp::ProcessSpec API"
[20]: https://www.tracktion.com/develop/pluginval "Tracktion pluginval"
[21]: https://steinbergmedia.github.io/vst3_dev_portal/pages/Getting+Started/How+to+setup+my+system.html "Steinberg VST3 system setup"
[22]: https://steinbergmedia.github.io/vst3_dev_portal/pages/Tutorials/Using+cmake+for+building+plug-ins.html "Steinberg VST3 CMake plug-in tutorial"
[23]: https://steinbergmedia.github.io/vst3_dev_portal/pages/Technical+Documentation/Locations+Format/Plugin+Format.html "Steinberg VST3 plug-in format"
[24]: https://steinbergmedia.github.io/vst3_dev_portal/pages/Technical+Documentation/Locations+Format/Plugin+Locations.html "Steinberg VST3 plug-in locations"
[25]: https://juce.com/tutorials/tutorial_app_plugin_packaging/ "JUCE application and plug-in packaging tutorial"
[26]: https://learn.microsoft.com/en-us/windows/win32/seccrypto/signtool "Microsoft SignTool documentation"
[27]: https://learn.microsoft.com/en-us/windows-hardware/drivers/install/release-signing "Microsoft release signing"
[28]: https://steinbergmedia.github.io/vst3_dev_portal/pages/Tutorials/Building+the+examples/Building+the+examples+included+in+the+SDK+Linux.html "Steinberg VST3 Linux build examples"
[29]: https://timur.audio/using-locks-in-real-time-audio-processing-safely "Using locks in real-time audio processing safely"
[30]: http://www.rossbencina.com/code/real-time-audio-programming-101-time-waits-for-nothing "Real-time audio programming 101: time waits for nothing"
[31]: https://www.izotope.com/products/nectar-advanced "iZotope Nectar Advanced product page"
[32]: https://www.izotope.com/community/blog/hidden-vocal-mixing-features-nectar "Hidden vocal mixing features in Nectar"
[33]: https://www.celemony.com/en/melodyne/melodyne-5-editions "Celemony Melodyne 5 editions"
[34]: https://www.macprovideo.com/article/audio-software/review-waves-tune-real-time "MacProVideo review of Waves Tune Real-Time"
[35]: https://synthandsoftware.com/2020/07/celemony-melodyne-5-review/ "Synth and Software review of Melodyne 5"
[36]: https://www.izotope.com/community/blog/tips-for-mixing-vocal-harmonies "Tips for mixing vocal harmonies"
[37]: https://www.izotope.com/community/blog/background-vocals "iZotope background vocals guidance"
[38]: https://docs.juce.com/master/structjuce_1_1AudioProcessorValueTreeState_1_1Listener.html "JUCE AudioProcessorValueTreeState listener API"
[39]: https://juce.com/tutorials/tutorial_audio_parameter/ "JUCE audio parameter tutorial"
[40]: https://docs.juce.com/master/classjuce_1_1ScopedNoDenormals.html "JUCE ScopedNoDenormals API"
[41]: https://github.com/juce-framework/JUCE/blob/master/docs/CMake%20API.md "JUCE CMake API documentation"
[42]: https://steinbergmedia.github.io/vst3_dev_portal/pages/Getting+Started/Preparation+on+Windows.html "Steinberg VST3 Windows preparation"
[43]: https://steinbergmedia.github.io/vst3_dev_portal/pages/Technical+Documentation/VST+Module+Architecture/ModuleInfo-JSON.html "Steinberg VST3 moduleinfo.json"
[44]: https://helpcenter.steinberg.de/hc/en-us/articles/115000177084-VST-plug-in-locations-on-Windows "Steinberg Windows VST plug-in locations"
[45]: https://www.image-line.com/fl-studio-learning/fl-studio-online-manual/html/basics_externalplugins.htm "FL Studio external plug-ins"
[46]: https://www.image-line.com/fl-studio-learning/fl-studio-online-manual/html/envsettings_files.htm "FL Studio file settings and plug-in paths"
[47]: https://www.image-line.com/fl-studio-learning/fl-studio-online-manual/html/plugins_supported.htm "FL Studio supported plug-ins"
[48]: https://learn.microsoft.com/en-us/windows/win32/seccrypto/signtool "Microsoft SignTool reference"
[49]: https://devblogs.microsoft.com/cppblog/using-visual-studio-for-cross-platform-c-development-targeting-windows-and-linux/ "Visual Studio cross-platform C++ development"
[50]: https://www.izotope.com/community/blog/essential-tips-for-using-reverb-on-vocals "iZotope reverb guidance for vocals"
[51]: https://www.izotope.com/community/blog/tips-for-mixing-vocal-harmonies "iZotope vocal harmony guidance"
[52]: https://www.izotope.com/community/blog/using-compression-to-help-vocals-sit-in-a-mix "iZotope vocal compression guidance"
[53]: https://www.izotope.com/community/blog/guide-to-audio-effects "iZotope audio effects guide"
[54]: https://www.izotope.com/community/blog/vocal-cheet-sheet "iZotope vocal cheat sheet"
[55]: https://www.antarestech.com/blog/afrobeats-vocal-production-how-to-get-the-sound-thats-taking-over-global-music "Antares Afrobeats vocal production guide"
[56]: https://www.antarestech.com/blog/the-science-behind-auto-tune "Antares Auto-Tune technical guide"
[57]: https://www.soundonsound.com/techniques/inside-track-burna-boy-time-flies "Sound On Sound Burna Boy mix case study"
[58]: https://www.izotope.com/products/nectar-advanced "iZotope Nectar Advanced"
[59]: https://www.celemony.com/en/melodyne/melodyne-5-editions "Celemony Melodyne editions"
[60]: https://www.waves.com/plugins/waves-tune-real-time "Waves Tune Real-Time"
[61]: https://www.waves.com/plugins/cla-vocals "Waves CLA Vocals"
[62]: https://docs.juce.com/master/classjuce_1_1AudioProcessor.html "JUCE AudioProcessor documentation"
[63]: https://docs.juce.com/master/classjuce_1_1AudioProcessorValueTreeState.html "JUCE APVTS documentation"
[64]: https://docs.juce.com/master/structjuce_1_1AudioProcessorValueTreeState_1_1Listener.html "JUCE APVTS listener documentation"
[65]: https://docs.juce.com/master/classjuce_1_1ScopedNoDenormals.html "JUCE ScopedNoDenormals documentation"
[66]: https://steinbergmedia.github.io/vst3_dev_portal/pages/Technical+Documentation/Locations+Format/Plugin+Format.html "Steinberg VST3 format documentation"
[67]: https://steinbergmedia.github.io/vst3_dev_portal/pages/Technical+Documentation/Locations+Format/Plugin+Locations.html "Steinberg VST3 locations documentation"
[68]: https://helpcenter.steinberg.de/hc/en-us/articles/115000177084-VST-plug-in-locations-on-Windows "Steinberg Windows VST locations"
[69]: https://www.image-line.com/fl-studio-learning/fl-studio-online-manual/html/basics_externalplugins.htm "FL Studio plug-in installation guidance"
[70]: https://www.image-line.com/fl-studio-learning/fl-studio-online-manual/html/envsettings_files.htm "FL Studio file settings"
[71]: https://www.image-line.com/fl-studio-learning/fl-studio-online-manual/html/plugins_supported.htm "FL Studio plug-in support"
[72]: https://learn.microsoft.com/en-us/windows-hardware/drivers/install/release-signing "Microsoft release signing guidance"
[73]: https://devblogs.microsoft.com/cppblog/using-visual-studio-for-cross-platform-c-development-targeting-windows-and-linux/ "Microsoft Visual Studio Windows/Linux development"
[74]: https://steinbergmedia.github.io/vst3_dev_portal/pages/Tutorials/Building+the+examples/Building+the+examples+included+in+the+SDK+Linux.html "Steinberg Linux VST3 examples"
[75]: https://juce.com/tutorials/tutorial_audio_processor_value_tree_state/ "JUCE APVTS state tutorial"
[76]: https://juce.com/tutorials/tutorial_audio_parameter/ "JUCE parameter tutorial"
[77]: https://docs.juce.com/master/classjuce_1_1dsp_1_1Oversampling.html "JUCE oversampling documentation"
[78]: https://docs.juce.com/master/classjuce_1_1SmoothedValue.html "JUCE smoothing documentation"
[79]: https://www.tracktion.com/develop/pluginval "Tracktion pluginval documentation"
[80]: https://github.com/juce-framework/JUCE/blob/master/docs/CMake%20API.md "JUCE CMake API"
[81]: https://juce.com/tutorials/tutorial_app_plugin_packaging/ "JUCE packaging tutorial"
[82]: https://www.fabfilter.com/downloads/pdf/help/ffprods-manual.pdf "FabFilter technical manual"
[83]: https://timur.audio/using-locks-in-real-time-audio-processing-safely "Timur Doumler real-time audio guidance"
[84]: http://www.rossbencina.com/code/real-time-audio-programming-101-time-waits-for-nothing "Ross Bencina real-time audio guidance"
[85]: https://www.izotope.com/community/blog/background-vocals "iZotope background vocal guidance"
[86]: https://www.izotope.com/community/blog/hidden-vocal-mixing-features-nectar "iZotope Nectar workflow guidance"
[87]: https://synthandsoftware.com/2020/07/celemony-melodyne-5-review/ "Synth and Software Melodyne review"
[88]: https://www.macprovideo.com/article/audio-software/review-waves-tune-real-time "MacProVideo Waves Tune review"
[89]: https://www.waves.com/1lib/pdf/plugins/tune-real-time.pdf "Waves Tune Real-Time manual"
[90]: https://www.antarestech.com/documentation/auto-tune-pro-11 "Antares Auto-Tune Pro documentation"
[91]: https://www.izotope.com/products/nectar-advanced "iZotope Nectar product information"
[92]: https://www.celemony.com/en/melodyne/melodyne-5-editions "Celemony Melodyne product information"
[93]: https://www.waves.com/plugins/cla-vocals "Waves CLA Vocals product information"
[94]: https://steinbergmedia.github.io/vst3_dev_portal/pages/Technical+Documentation/VST+Module+Architecture/ModuleInfo-JSON.html "Steinberg moduleinfo.json documentation"
[95]: https://learn.microsoft.com/en-us/windows/win32/seccrypto/signtool "Microsoft SignTool reference"
[96]: https://docs.juce.com/master/structjuce_1_1dsp_1_1ProcessSpec.html "JUCE ProcessSpec documentation"
[97]: https://www.izotope.com/community/blog/tips-for-mixing-vocal-harmonies "iZotope vocal harmony mixing tips"
[98]: https://www.izotope.com/community/blog/using-compression-to-help-vocals-sit-in-a-mix "iZotope compression tips"
[99]: https://www.izotope.com/community/blog/guide-to-audio-effects "iZotope effects guide"
[100]: https://www.izotope.com/community/blog/essential-tips-for-using-reverb-on-vocals "iZotope vocal reverb tips"
[101]: https://www.izotope.com/community/blog/vocal-cheet-sheet "iZotope vocal cheat sheet"
[102]: https://www.antarestech.com/blog/afrobeats-vocal-production-how-to-get-the-sound-thats-taking-over-global-music "Afrobeats production guidance"
[103]: https://www.antarestech.com/blog/the-science-behind-auto-tune "Auto-Tune science guidance"
[104]: https://www.soundonsound.com/techniques/inside-track-burna-boy-time-flies "Burna Boy vocal production case study"
[105]: https://www.izotope.com/community/blog/background-vocals "Background vocals guidance"
[106]: https://www.izotope.com/products/nectar-advanced "Nectar Advanced"
[107]: https://www.celemony.com/en/melodyne/melodyne-5-editions "Melodyne 5"
[108]: https://www.waves.com/plugins/waves-tune-real-time "Waves Tune Real-Time"
[109]: https://www.waves.com/plugins/cla-vocals "CLA Vocals"
[110]: https://docs.juce.com/master/classjuce_1_1AudioProcessorValueTreeState.html "APVTS API"
[111]: https://docs.juce.com/master/classjuce_1_1AudioProcessor.html "AudioProcessor API"
[112]: https://docs.juce.com/master/structjuce_1_1AudioProcessorValueTreeState_1_1Listener.html "APVTS listener API"
[113]: https://juce.com/tutorials/tutorial_audio_processor_value_tree_state/ "APVTS tutorial"
[114]: https://juce.com/tutorials/tutorial_audio_parameter/ "Audio parameter tutorial"
[115]: https://docs.juce.com/master/classjuce_1_1ScopedNoDenormals.html "ScopedNoDenormals API"
[116]: https://docs.juce.com/master/structjuce_1_1dsp_1_1ProcessSpec.html "ProcessSpec API"
[117]: https://docs.juce.com/master/classjuce_1_1dsp_1_1Oversampling.html "Oversampling API"
[118]: https://docs.juce.com/master/classjuce_1_1SmoothedValue.html "SmoothedValue API"
[119]: https://github.com/juce-framework/JUCE/blob/master/docs/CMake%20API.md "JUCE CMake API"
[120]: https://steinbergmedia.github.io/vst3_dev_portal/pages/Getting+Started/Preparation+on+Windows.html "Windows VST3 preparation"
[121]: https://steinbergmedia.github.io/vst3_dev_portal/pages/Tutorials/Using+cmake+for+building+plug-ins.html "VST3 CMake build tutorial"
[122]: https://steinbergmedia.github.io/vst3_dev_portal/pages/Getting+Started/How+to+setup+my+system.html "VST3 system setup"
[123]: https://steinbergmedia.github.io/vst3_dev_portal/pages/Technical+Documentation/Locations+Format/Plugin+Format.html "VST3 plugin format"
[124]: https://steinbergmedia.github.io/vst3_dev_portal/pages/Technical+Documentation/Locations+Format/Plugin+Locations.html "VST3 plugin locations"
[125]: https://steinbergmedia.github.io/vst3_dev_portal/pages/Technical+Documentation/VST+Module+Architecture/ModuleInfo-JSON.html "Moduleinfo JSON"
[126]: https://helpcenter.steinberg.de/hc/en-us/articles/115000177084-VST-plug-in-locations-on-Windows "VST plug-in locations"
[127]: https://juce.com/tutorials/tutorial_app_plugin_packaging/ "JUCE packaging"
[128]: https://www.image-line.com/fl-studio-learning/fl-studio-online-manual/html/basics_externalplugins.htm "FL Studio external plugins"
[129]: https://www.image-line.com/fl-studio-learning/fl-studio-online-manual/html/envsettings_files.htm "FL Studio file settings"
[130]: https://www.image-line.com/fl-studio-learning/fl-studio-online-manual/html/plugins_supported.htm "FL Studio supported plugins"
[131]: https://learn.microsoft.com/en-us/windows-hardware/drivers/install/release-signing "Microsoft release signing"
[132]: https://devblogs.microsoft.com/cppblog/using-visual-studio-for-cross-platform-c-development-targeting-windows-and-linux/ "Visual Studio cross-platform C++"
[133]: https://steinbergmedia.github.io/vst3_dev_portal/pages/Tutorials/Building+the+examples/Building+the+examples+included+in+the+SDK+Linux.html "VST3 Linux build examples"
[134]: https://learn.microsoft.com/en-us/windows/win32/seccrypto/signtool "SignTool"
[135]: https://www.tracktion.com/develop/pluginval "pluginval"
[136]: https://www.fabfilter.com/downloads/pdf/help/ffprods-manual.pdf "FabFilter manual"
[137]: https://www.izotope.com/community/blog/hidden-vocal-mixing-features-nectar "Nectar features"
[138]: https://www.izotope.com/products/nectar-advanced "Nectar Advanced"
[139]: https://www.macprovideo.com/article/audio-software/review-waves-tune-real-time "Waves Tune review"
[140]: https://synthandsoftware.com/2020/07/celemony-melodyne-5-review/ "Melodyne review"
[141]: https://www.izotope.com/community/blog/background-vocals "Background vocals"
[142]: https://www.izotope.com/community/blog/tips-for-mixing-vocal-harmonies "Vocal harmonies"
[143]: https://www.izotope.com/community/blog/essential-tips-for-using-reverb-on-vocals "Reverb on vocals"
[144]: https://www.izotope.com/community/blog/vocal-cheet-sheet "Vocal cheat sheet"
[145]: https://www.izotope.com/community/blog/guide-to-audio-effects "Audio effects"
[146]: https://www.izotope.com/community/blog/using-compression-to-help-vocals-sit-in-a-mix "Compression and vocals"
[147]: https://www.soundonsound.com/techniques/inside-track-burna-boy-time-flies "Burna Boy Inside Track"
[148]: https://www.antarestech.com/blog/afrobeats-vocal-production-how-to-get-the-sound-thats-taking-over-global-music "Afrobeats vocal production"
[149]: https://www.antarestech.com/blog/the-science-behind-auto-tune "Auto-Tune science"
[150]: https://docs.juce.com/master/classjuce_1_1AudioProcessor.html "JUCE AudioProcessor"
[151]: https://docs.juce.com/master/classjuce_1_1AudioProcessorValueTreeState.html "JUCE APVTS"
[152]: https://docs.juce.com/master/classjuce_1_1dsp_1_1Oversampling.html "JUCE oversampling"
[153]: https://docs.juce.com/master/classjuce_1_1SmoothedValue.html "JUCE SmoothedValue"
[154]: https://docs.juce.com/master/classjuce_1_1ScopedNoDenormals.html "JUCE ScopedNoDenormals"
[155]: https://timur.audio/using-locks-in-real-time-audio-processing-safely "Real-time locks"
[156]: http://www.rossbencina.com/code/real-time-audio-programming-101-time-waits-for-nothing "Real-time audio deadlines"
[157]: https://www.tracktion.com/develop/pluginval "Tracktion pluginval"
[158]: https://www.izotope.com/community/blog/tips-for-mixing-vocal-harmonies "Vocal harmony tips"
[159]: https://www.izotope.com/community/blog/background-vocals "Background vocal tips"
[160]: https://www.izotope.com/community/blog/essential-tips-for-using-reverb-on-vocals "Vocal reverb tips"
[161]: https://www.izotope.com/community/blog/guide-to-audio-effects "Effects guide"
[162]: https://www.izotope.com/community/blog/vocal-cheet-sheet "Vocal processing cheat sheet"
[163]: https://www.izotope.com/community/blog/using-compression-to-help-vocals-sit-in-a-mix "Vocal compression tips"
[164]: https://www.izotope.com/products/nectar-advanced "Nectar product page"
[165]: https://www.celemony.com/en/melodyne/melodyne-5-editions "Melodyne product page"
[166]: https://www.waves.com/plugins/waves-tune-real-time "Waves Tune product page"
[167]: https://www.waves.com/plugins/cla-vocals "CLA Vocals product page"
[168]: https://www.antarestech.com/documentation/auto-tune-pro-11 "Auto-Tune product documentation"
[169]: https://www.celemony.com/en/melodyne/melodyne-5-editions "Melodyne editions"
[170]: https://www.synthandsoftware.com/2020/07/celemony-melodyne-5-review/ "Melodyne review"
[171]: https://www.macprovideo.com/article/audio-software/review-waves-tune-real-time "Waves Tune review"
[172]: https://www.waves.com/1lib/pdf/plugins/tune-real-time.pdf "Waves Tune manual"
[173]: https://docs.juce.com/master/structjuce_1_1AudioProcessorValueTreeState_1_1Listener.html "APVTS listener"
[174]: https://docs.juce.com/master/structjuce_1_1dsp_1_1ProcessSpec.html "JUCE ProcessSpec"
[175]: https://juce.com/tutorials/tutorial_audio_processor_value_tree_state/ "JUCE state tutorial"
[176]: https://juce.com/tutorials/tutorial_audio_parameter/ "JUCE parameter tutorial"
[177]: https://steinbergmedia.github.io/vst3_dev_portal/pages/Technical+Documentation/VST+Module+Architecture/ModuleInfo-JSON.html "ModuleInfo JSON"
[178]: https://steinbergmedia.github.io/vst3_dev_portal/pages/Getting+Started/Preparation+on+Windows.html "Windows preparation"
[179]: https://steinbergmedia.github.io/vst3_dev_portal/pages/Getting+Started/How+to+setup+my+system.html "System setup"
[180]: https://steinbergmedia.github.io/vst3_dev_portal/pages/Tutorials/Using+cmake+for+building+plug-ins.html "CMake tutorial"
[181]: https://steinbergmedia.github.io/vst3_dev_portal/pages/Technical+Documentation/Locations+Format/Plugin+Format.html "Plugin format"
[182]: https://steinbergmedia.github.io/vst3_dev_portal/pages/Technical+Documentation/Locations+Format/Plugin+Locations.html "Plugin locations"
[183]: https://helpcenter.steinberg.de/hc/en-us/articles/115000177084-VST-plug-in-locations-on-Windows "VST locations"
[184]: https://juce.com/tutorials/tutorial_app_plugin_packaging/ "Packaging tutorial"
[185]: https://www.image-line.com/fl-studio-learning/fl-studio-online-manual/html/basics_externalplugins.htm "FL Studio external plugins"
[186]: https://learn.microsoft.com/en-us/windows/win32/seccrypto/signtool "SignTool documentation"
[187]: https://learn.microsoft.com/en-us/windows-hardware/drivers/install/release-signing "Release signing"
[188]: https://devblogs.microsoft.com/cppblog/using-visual-studio-for-cross-platform-c-development-targeting-windows-and-linux/ "Visual Studio cross-platform development"
[189]: https://steinbergmedia.github.io/vst3_dev_portal/pages/Tutorials/Building+the+examples/Building+the+examples+included+in+the+SDK+Linux.html "Linux build examples"
[190]: https://github.com/juce-framework/JUCE/blob/master/docs/CMake%20API.md "JUCE CMake API"
[191]: https://www.izotope.com/community/blog/hidden-vocal-mixing-features-nectar "Nectar vocal mixing features"
[192]: https://www.izotope.com/community/blog/background-vocals "Background vocal processing"
[193]: https://www.izotope.com/community/blog/tips-for-mixing-vocal-harmonies "Harmony mixing"
[194]: https://www.izotope.com/community/blog/essential-tips-for-using-reverb-on-vocals "Reverb mixing"
[195]: https://www.izotope.com/community/blog/vocal-cheet-sheet "Vocal processing"
[196]: https://www.izotope.com/community/blog/using-compression-to-help-vocals-sit-in-a-mix "Compression guidance"
[197]: https://www.izotope.com/community/blog/guide-to-audio-effects "Effects guidance"
[198]: https://www.antarestech.com/blog/afrobeats-vocal-production-how-to-get-the-sound-thats-taking-over-global-music "Afrobeats vocal guide"
[199]: https://www.soundonsound.com/techniques/inside-track-burna-boy-time-flies "Burna Boy production guide"
[200]: https://www.fabfilter.com/downloads/pdf/help/ffprods-manual.pdf "FabFilter manual"
[201]: https://www.izotope.com/products/nectar-advanced "Nectar Advanced"
[202]: https://www.celemony.com/en/melodyne/melodyne-5-editions "Melodyne 5"
[203]: https://www.waves.com/plugins/waves-tune-real-time "Waves Tune"
[204]: https://www.waves.com/plugins/cla-vocals "CLA Vocals"
[205]: https://www.macprovideo.com/article/audio-software/review-waves-tune-real-time "Waves Tune Real-Time review"
[206]: https://synthandsoftware.com/2020/07/celemony-melodyne-5-review/ "Melodyne 5 review"
[207]: https://docs.juce.com/master/classjuce_1_1AudioProcessor.html "JUCE AudioProcessor"
[208]: https://docs.juce.com/master/classjuce_1_1AudioProcessorValueTreeState.html "JUCE APVTS"
[209]: https://docs.juce.com/master/classjuce_1_1AudioProcessorValueTreeState_1_1Listener.html "JUCE APVTS listener"
[210]: https://juce.com/tutorials/tutorial_audio_processor_value_tree_state/ "JUCE APVTS tutorial"
[211]: https://juce.com/tutorials/tutorial_audio_parameter/ "JUCE audio parameter"
[212]: https://docs.juce.com/master/classjuce_1_1dsp_1_1Oversampling.html "JUCE Oversampling"
[213]: https://docs.juce.com/master/structjuce_1_1dsp_1_1ProcessSpec.html "JUCE ProcessSpec"
[214]: https://docs.juce.com/master/classjuce_1_1SmoothedValue.html "JUCE SmoothedValue"
[215]: https://docs.juce.com/master/classjuce_1_1ScopedNoDenormals.html "JUCE ScopedNoDenormals"
[216]: https://timur.audio/using-locks-in-real-time-audio-processing-safely "Timur Doumler locks"
[217]: http://www.rossbencina.com/code/real-time-audio-programming-101-time-waits-for-nothing "Ross Bencina deadlines"
[218]: https://www.tracktion.com/develop/pluginval "pluginval validator"
[219]: https://steinbergmedia.github.io/vst3_dev_portal/pages/Getting+Started/How+to+setup+my+system.html "Steinberg setup"
[220]: https://steinbergmedia.github.io/vst3_dev_portal/pages/Getting+Started/Preparation+on+Windows.html "Steinberg Windows setup"
[221]: https://steinbergmedia.github.io/vst3_dev_portal/pages/Tutorials/Using+cmake+for+building+plug-ins.html "Steinberg CMake"
[222]: https://steinbergmedia.github.io/vst3_dev_portal/pages/Technical+Documentation/Locations+Format/Plugin+Format.html "VST3 format"
[223]: https://steinbergmedia.github.io/vst3_dev_portal/pages/Technical+Documentation/Locations+Format/Plugin+Locations.html "VST3 locations"
[224]: https://steinbergmedia.github.io/vst3_dev_portal/pages/Technical+Documentation/VST+Module+Architecture/ModuleInfo-JSON.html "ModuleInfo JSON"
[225]: https://helpcenter.steinberg.de/hc/en-us/articles/115000177084-VST-plug-in-locations-on-Windows "Steinberg plugin locations"
[226]: https://juce.com/tutorials/tutorial_app_plugin_packaging/ "JUCE packaging"
[227]: https://www.image-line.com/fl-studio-learning/fl-studio-online-manual/html/basics_externalplugins.htm "FL Studio external plugins"
[228]: https://www.image-line.com/fl-studio-learning/fl-studio-online-manual/html/envsettings_files.htm "FL Studio file settings"
[229]: https://www.image-line.com/fl-studio-learning/fl-studio-online-manual/html/plugins_supported.htm "FL Studio supported plugins"
[230]: https://learn.microsoft.com/en-us/windows/win32/seccrypto/signtool "SignTool"
[231]: https://learn.microsoft.com/en-us/windows-hardware/drivers/install/release-signing "Microsoft release signing"
[232]: https://devblogs.microsoft.com/cppblog/using-visual-studio-for-cross-platform-c-development-targeting-windows-and-linux/ "Visual Studio cross-platform C++"
[233]: https://steinbergmedia.github.io/vst3_dev_portal/pages/Tutorials/Building+the+examples/Building+the+examples+included+in+the+SDK+Linux.html "Linux VST3 build"
[234]: https://github.com/juce-framework/JUCE/blob/master/docs/CMake%20API.md "JUCE CMake API"
[235]: https://www.izotope.com/community/blog/vocal-cheet-sheet "Vocal Cheat Sheet"
[236]: https://www.izotope.com/community/blog/essential-tips-for-using-reverb-on-vocals "Reverb tips"
[237]: https://www.izotope.com/community/blog/tips-for-mixing-vocal-harmonies "Harmony tips"
[238]: https://www.izotope.com/community/blog/background-vocals "Background vocal tips"
[239]: https://www.izotope.com/community/blog/using-compression-to-help-vocals-sit-in-a-mix "Vocal compression tips"
[240]: https://www.izotope.com/community/blog/guide-to-audio-effects "Audio effects guide"
[241]: https://www.antarestech.com/blog/afrobeats-vocal-production-how-to-get-the-sound-thats-taking-over-global-music "Afrobeats guide"
[242]: https://www.antarestech.com/blog/the-science-behind-auto-tune "Auto-Tune guide"
[243]: https://www.soundonsound.com/techniques/inside-track-burna-boy-time-flies "Sound On Sound case study"
[244]: https://www.fabfilter.com/downloads/pdf/help/ffprods-manual.pdf "FabFilter manual"
[245]: https://www.waves.com/plugins/waves-tune-real-time "Waves Tune Real-Time"
[246]: https://www.waves.com/1lib/pdf/plugins/tune-real-time.pdf "Waves Tune manual"
[247]: https://www.waves.com/plugins/cla-vocals "CLA Vocals"
[248]: https://www.izotope.com/products/nectar-advanced "Nectar"
[249]: https://www.celemony.com/en/melodyne/melodyne-5-editions "Melodyne"
[250]: https://www.macprovideo.com/article/audio-software/review-waves-tune-real-time "Tune review"
[251]: https://synthandsoftware.com/2020/07/celemony-melodyne-5-review/ "Melodyne review"
[252]: https://docs.juce.com/master/classjuce_1_1AudioProcessor.html "AudioProcessor"
[253]: https://docs.juce.com/master/classjuce_1_1AudioProcessorValueTreeState.html "APVTS"
[254]: https://docs.juce.com/master/structjuce_1_1AudioProcessorValueTreeState_1_1Listener.html "APVTS listener"
[255]: https://juce.com/tutorials/tutorial_audio_processor_value_tree_state/ "APVTS tutorial"
[256]: https://juce.com/tutorials/tutorial_audio_parameter/ "Audio parameter"
[257]: https://docs.juce.com/master/classjuce_1_1SmoothedValue.html "SmoothedValue"
[258]: https://docs.juce.com/master/classjuce_1_1dsp_1_1Oversampling.html "Oversampling"
[259]: https://docs.juce.com/master/classjuce_1_1ScopedNoDenormals.html "ScopedNoDenormals"
[260]: https://docs.juce.com/master/structjuce_1_1dsp_1_1ProcessSpec.html "ProcessSpec"
[261]: https://timur.audio/using-locks-in-real-time-audio-processing-safely "Real-time locks"
[262]: http://www.rossbencina.com/code/real-time-audio-programming-101-time-waits-for-nothing "Real-time deadlines"
[263]: https://www.tracktion.com/develop/pluginval "pluginval"
[264]: https://steinbergmedia.github.io/vst3_dev_portal/pages/Getting+Started/How+to+setup+my+system.html "Steinberg setup"
[265]: https://steinbergmedia.github.io/vst3_dev_portal/pages/Getting+Started/Preparation+on+Windows.html "Windows preparation"
[266]: https://steinbergmedia.github.io/vst3_dev_portal/pages/Tutorials/Using+cmake+for+building+plug-ins.html "Steinberg CMake build tutorial"
