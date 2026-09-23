# Real-Time Monophonic Vocal Pitch Correction in JUCE/VST3

## Executive recommendation

For a usable first engine, separate the problem into two coupled but independently testable paths:

1. **A causal pitch/voicing tracker** based on YIN-style difference-function analysis, with several candidate periods retained rather than only the first minimum.
2. **A voice-aware retuner/pitch shifter** that changes only the periodic component and crossfades or bypasses unvoiced material.

Use a short-hop ring-buffer architecture. A practical starting point at 48 kHz is a 2,048-sample analysis frame (42.7 ms), a 256-sample hop (5.33 ms), and an explicitly measured algorithmic delay. Reduce the frame to 1,024 samples (21.3 ms) only after confirming that the lowest intended singer pitch remains reliably voiced. Search approximately 65–1,200 Hz for general singing, or expose a lower/upper range per voice category. Keep at least two or three YIN candidate minima so that octave errors can be rejected using harmonic evidence, confidence, and continuity rather than a fragile single threshold.

For the correction path, **TD-PSOLA or a carefully engineered pitch-synchronous granular/OLA shifter** is the most direct low-latency choice for a monophonic vocal. A phase-vocoder path is more flexible and can be a useful second mode, but it requires transient handling, phase coherence, and more latency. Preserve consonants by detecting voiced/unvoiced and transient regions, leaving fricative/noise energy mostly dry, and blending the corrected periodic signal with the original at boundaries. Preserve formants by retaining a fixed-duration vocal grain or by explicitly separating the spectral envelope from the excitation before shifting it.

The numerical values in this report are **engineering starting points, not universal constants**. Singer, microphone, room noise, vocal register, sample rate, host block size, and the chosen shifter all change the correct values.

## 1. Signal model and processing topology

A singing voice is usefully treated as a time-varying source-filter signal. In voiced regions, quasi-periodic glottal excitation has a fundamental frequency \(F_0\), while the vocal-tract spectral envelope contains formants. In unvoiced regions, turbulent excitation produces consonants and fricatives with weak or absent periodicity. A pitch corrector should therefore avoid treating every sample as a stationary periodic oscillator.

A robust plug-in topology is:

```text
input -> mono analysis copy -> light band-limit / pre-emphasis -> ring buffer
                                              |
                              YIN candidates + confidence + voicing
                                              |
                         continuity / octave rejection / note hysteresis
                                              |
                           smoothed pitch-ratio and correction authority
                                              |
input ring buffer -> voiced pitch shifter (TD-PSOLA or granular/OLA)
        \-> unvoiced/transient protection and dry path -> equal-power mix -> output
```

The analysis copy should not unnecessarily alter the audio that is rendered. For stereo input, either downmix with a documented policy or analyse the channel with the best signal-to-noise ratio; do not let a phase-cancelled stereo sum silently destroy the tracker. A modest analysis high-pass around 50–80 Hz and a low-pass around 3–5 kHz can improve periodicity estimates, but the audio path should retain the full vocal bandwidth.

A JUCE processor receives audio in `processBlock()` and must be prepared for actual blocks that differ from the host's typical block size. JUCE documents `prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock)` as the place to allocate and configure processing resources, and notes that `getBlockSize()` is only the typical block size, not a guarantee for every callback [1]. Allocate ring buffers, FFT objects, windows, scratch buffers, and shifter state before the callback begins. Treat the audio callback as real-time code: no heap allocation, locks, file I/O, logging, GUI calls, or unbounded work in `processBlock()`.

Use `AudioProcessorValueTreeState` for host parameters. Its `getRawParameterValue()` returns a pointer to an atomic floating-point representation intended for a real-time process to read [2]. Read parameters once per block or at a controlled rate, then smooth them in the audio path. Do not rebuild a pitch-shifter or resize a buffer in response to a parameter from inside the callback.

## 2. YIN and autocorrelation pitch detection

### 2.1 YIN core

YIN refines autocorrelation-style periodicity analysis to reduce common pitch errors. For a frame \(x[j]\), calculate the squared difference function:

\[
d(\tau)=\sum_{j=0}^{W-\tau-1}(x[j]-x[j+\tau])^2.
\]

YIN then uses a cumulative-mean normalized difference function, commonly written for \(\tau>0\) as:

\[
d'(\tau)=\frac{d(\tau)}{\frac{1}{\tau}\sum_{k=1}^{\tau}d(k)}.
\]

The first lag whose normalized difference falls below a threshold, followed by a local-minimum search, is selected as a period candidate. Parabolic interpolation around the minimum improves sub-sample period resolution. Estimate frequency as \(F_0=f_s/\tau\). The original YIN paper reports substantially lower error rates than competing methods in its speech evaluation and emphasizes that the method can be implemented with relatively low latency and few tunable parameters [3].

Use the same underlying computation to retain **multiple candidates**. The first acceptable trough is not always the correct trough: a vocal harmonic, strong formant, breathiness, clipping, or noise can make \(F_0/2\) or \(2F_0\) appear more convincing locally. Search lags only in the configured range:

\[
\tau_{min}=\lfloor f_s/F_{max}\rfloor,\qquad
\tau_{max}=\lceil f_s/F_{min}\rceil.
\]

A normalized autocorrelation can be kept as an additional feature. A useful implementation records, for every candidate, the lag, interpolated frequency, normalized difference value, normalized autocorrelation, local-minimum depth, and distance to the next-best candidate.

### 2.2 Practical frame and hop values

The analysis window must contain enough cycles of the lowest expected note. At 48 kHz, a 65 Hz note has a period of about 738 samples. A 1,024-sample frame contains only about 1.4 periods and is vulnerable to onset and noise; a 2,048-sample frame contains about 2.8 periods and is a safer general starting point. The singing-voice comparison by Babacan et al. explicitly identifies the trade-off: low voices need longer windows containing multiple glottal cycles, while short windows follow pitch changes more precisely [4]. That study used a 60–1,500 Hz comparison range and a 10-ms frame shift, but the best window varied by algorithm and singing condition; do not copy a single speech default blindly.

Recommended initial configurations at 48 kHz:

| Mode | Frame | Hop | Intended behavior | Main risk |
|---|---:|---:|---|---|
| Low-latency | 1,024 samples (21.3 ms) | 128–256 (2.7–5.3 ms) | Fast response for mid/high voices | Octaves and unstable low notes |
| General vocal | 2,048 (42.7 ms) | 256 (5.3 ms) | Reliable sustained singing | Noticeable analysis delay |
| Low register / noisy | 3,072–4,096 (64–85 ms) | 256–512 | More periodicity evidence | Slow onsets, more consonant contamination |

Use a window such as Hann or a smooth tapered window. A causal implementation should define the frame timestamp precisely. A frame ending at the current input sample avoids future look-ahead but still needs enough accumulated samples; a centered frame adds approximately half a window of future context and therefore latency. The reported plug-in latency must reflect the actual audio alignment, not merely the hop size.

### 2.3 Confidence and voicing

Do not use the pitch candidate's existence as a voicing decision. A usable detector needs at least two related outputs:

- **Voicing confidence:** probability or score that the region is periodic and should be corrected.
- **Pitch confidence:** probability or score that the selected period is the true period rather than an octave, harmonic, or noise artifact.

The classic autocorrelation work by Krubsack and Niederjohn describes an integrated detector with a voicing decision, a confidence measure for the voicing decision, a confidence measure for expected pitch deviation, and smoothing of the pitch, voicing, and confidence measures. Their features are derived from the autocorrelation function and are evaluated under high white-noise levels [5]. This supports keeping confidence as a first-class signal rather than a single hard threshold.

A practical score can combine normalized features:

```text
periodicity = clamp01(1 - yinCMNDAtCandidate)
strength     = clamp01(normalizedAutocorrelationAtCandidate)
margin       = clamp01((secondBestScore - bestScore) / scale)
energy       = soft gate from RMS / noise-floor estimate
continuity   = soft gate from cents distance to previous accepted F0
voicing      = weightedSum(periodicity, strength, energy)
pitchCertainty = weightedSum(periodicity, strength, margin, continuity)
```

Keep the components available for diagnostics. A high periodicity score with low candidate margin means “probably voiced, pitch uncertain,” which should usually preserve the dry sound rather than force an aggressive retune. pYIN makes the same conceptual separation: it computes candidate probabilities with YIN and then uses Viterbi decoding to estimate the most likely F0 sequence, voiced flags, and voiced probability [6].

Do not copy pYIN's offline defaults directly into a causal plug-in. The librosa documentation describes a 2,048-sample default frame, a frame hop of one quarter of the frame, a 35.92-octaves-per-second maximum transition rate, and a Viterbi stage with switch and transition probabilities [6]. Those values are useful references, but a real-time engine must account for look-ahead, fixed memory, and a bounded per-hop computation.

Recommended starting gates, expressed in normalized confidence where 1 is strongest:

| Decision | Starting value | Implementation note |
|---|---:|---|
| Enter voiced/correctable | 0.70–0.80 for 2 consecutive hops | Require minimum RMS above adaptive noise floor |
| Continue voiced | 0.45–0.55 | Hold through brief breath/noise gaps |
| Force unvoiced | below 0.30–0.40 for 3–5 hops | Fade correction authority, do not hard mute |
| Accept pitch candidate | pitch certainty 0.60–0.75 | Also require continuity or strong candidate margin |
| Dry fallback | pitch certainty below 0.45–0.55 | Preserve original phase and consonants |

These values should be exposed to a debug view before being exposed as user controls. Hysteresis is essential: separate enter and leave thresholds prevent chatter at the voiced/unvoiced boundary.

## 3. Octave-error rejection and note tracking

### 3.1 Candidate-based octave rejection

The most damaging error is often not a small cents error but a stable octave error. Use at least three defenses together:

1. **Candidate enumeration:** retain minima at the selected lag and approximately double/halve lags instead of discarding them.
2. **Harmonic evidence:** compare candidate support at integer multiples of the candidate frequency. A candidate whose harmonic pattern is coherent across several partials is more credible than a candidate supported by one strong formant.
3. **Continuity and bounded movement:** penalize a candidate that jumps by roughly 1,200 cents unless the recent confidence is low or an explicit glide/onset permits it.

A simple candidate cost is:

\[
C_i = a\,d'_i - b\,H_i - c\,M_i + d\,\rho(\Delta_i),
\]

where \(d'_i\) is normalized YIN difference, \(H_i\) is harmonic support, \(M_i\) is the local-minimum margin, and \(\rho(\Delta_i)\) is a robust penalty for the cents distance from the previous accepted track. Evaluate candidates at \(f\), \(2f\), and \(f/2\) when they lie in range. The octave alternatives should be accepted if they have materially better evidence, not rejected categorically; singers can execute legitimate octave leaps.

An autocorrelation detector may also be combined with a spectral harmonic-sum score. For example, for each candidate \(f\), sum energy near \(nf\) for \(n=1\ldots N\), weighting lower harmonics but excluding a band known to be dominated by noise. This is a disambiguation cue, not an independent truth source. Formants and microphone coloration can make spectral evidence misleading.

### 3.2 Note tracking hysteresis

The raw F0 should remain continuous even when the correction target is a discrete musical note. Maintain separate state for:

- current voiced/unvoiced state;
- current accepted continuous F0 in log2-Hz or cents;
- current target note or scale degree;
- pending note candidate and its dwell time;
- correction authority / wetness.

A useful state machine is `Unvoiced -> Candidate -> Voiced -> NoteChangePending -> Voiced`, with a separate `Release` fade. Quantize to the nearest permitted note only after the candidate is stable. Switch the target when the new note is supported for 2–4 hops, or when the continuous estimate moves more than a larger boundary margin. Hold the old target through a weak one-hop frame.

Starting values at a 5.3-ms hop:

- candidate note dwell: 10–25 ms;
- loss-of-confidence hold: 15–35 ms;
- correction release at unvoiced boundary: 10–30 ms;
- note-change hysteresis: approximately 25–60 cents beyond the ordinary nearest-note boundary, depending on desired “hardness”;
- allowed continuous transition rate: about 4–12 octaves/second for normal singing, with a higher temporary limit on detected portamento or onset.

The transition-rate value must not be interpreted as “the singer cannot move faster.” It is a prior that suppresses impossible tracker jumps. pYIN exposes a maximum pitch transition rate and voiced-state switch probability for this reason [6].

## 4. Retune smoothing and musical response

Compute the desired correction in log frequency, not in Hertz:

\[
\text{ratio}=2^{(F_{target,cents}-F_{estimated,cents})/1200}.
\]

Smooth the ratio or the cents error with a one-pole or critically damped trajectory. Smoothing in Hertz produces a perceptually uneven response across registers. Update the shifter's read/write phase increment at sample or small sub-block resolution so that a block-size change does not produce a zipper.

Useful starting presets are:

| Character | Retune attack | Retune release | Notes |
|---|---:|---:|---|
| Hard / effect | 0–8 ms | 20–60 ms | Quantized target; can expose tracking errors |
| Controlled pop vocal | 10–30 ms | 50–120 ms | Good default for real-time use |
| Natural correction | 40–100 ms | 100–250 ms | Lets vibrato and portamento remain audible |

These are smoothing times, not the full detector-to-output delay. A fast retune can still feel late if the analysis frame is long. Conversely, a very fast corrector fed by an unstable note state creates audible warbling. Add an optional “preserve vibrato” mode: estimate a slowly varying note center and retain a bounded fraction of fast F0 deviation, rather than correcting every cents fluctuation to zero.

Limit the correction ratio. A starting clamp of ±400 cents avoids turning a tracker failure into a large discontinuity; an optional artistic mode can permit ±1,200 cents but should reduce wetness as confidence falls. When the target or pitch is invalid, ramp correction authority to zero instead of freezing a stale ratio indefinitely.

## 5. Pitch shifting choices

### 5.1 TD-PSOLA: best fit for a monophonic vocal when pitch epochs are reliable

Pitch-Synchronous Overlap-Add (PSOLA) extracts short, pitch-synchronous waveform segments around glottal epochs, changes the spacing of synthesis epochs, and overlap-adds the segments. The primary PSOLA literature describes both time-domain and frequency-domain variants. The time-domain method is efficient for real-time implementation, while the frequency-domain method gives more flexibility for spectral modification [7].

For a vocal corrector, TD-PSOLA can preserve the vocal-tract envelope well because the grain waveform and duration are retained while the epoch spacing is changed. It is low-cost and can produce natural voiced results at moderate correction ratios. Its risks are correspondingly specific: incorrect epoch locations, doubled or missing periods, large F0 jumps, unvoiced speech, and onset transients. A YIN period estimate alone is not a perfect glottal epoch detector; refine candidate epochs using local waveform peaks or polarity-aware energy peaks, and fall back to overlap-add grains when epoch confidence is low.

Implement a bounded circular input history and output accumulator. Read enough past samples to form the current grain, place grains at the smoothed synthesis spacing, and use a smooth equal-power or raised-cosine window. Ensure the overlap-add normalization never approaches zero. Crossfade between old and new period models when the period changes quickly.

### 5.2 Granular / OLA / WSOLA: simpler, but inspect the artifacts

A granular shifter can use a fixed-duration window, read positions that advance at one rate, and write positions that advance at another. OLA is simple and predictable. WSOLA adds waveform-similarity search to align overlapping grains, which can reduce phase discontinuities for quasi-periodic sources. These methods are attractive when the tracker is not reliable enough to promise pitch-synchronous epochs.

The trade-off is that arbitrary grains can produce phasing, chorusing, transient smearing, or a “robotic” amplitude modulation. Keep grains long enough to contain several periods of the low register, but shorter grains reduce latency. Use a fixed grain duration for vocal formant stability; do not let the grain duration change abruptly with the target pitch. The shifter should expose a quality/latency mode rather than silently changing its buffer geometry during playback.

### 5.3 Phase vocoder: flexible and general, but more difficult to make vocal-natural

A phase vocoder uses STFT analysis, phase propagation, resampling or bin remapping, and overlap-add synthesis. Laroche and Dolson describe direct frequency-domain peak shifting, regions of influence around spectral peaks, phase adjustment for inter-frame continuity, and the need for greater overlap for more flexible fractional shifts [8].

For vocals, basic phase-vocoder processing can produce phasiness and smeared consonants. Use peak phase locking or coherent phase propagation, reset or reinitialize phases at transients, and separate steady harmonic regions from attacks. The Rubber Band technical notes are a practical engineering reference: its R2 engine uses transient phase resets, adaptive stretch behavior, phase “lamination” for vertical coherence, and resampling combined with time-stretching for stable pitch ratios [9]. The notes also explicitly warn that time-stretching is not magic; corner cases and material-dependent tuning dominate quality.

A phase-vocoder implementation is a reasonable higher-quality mode when a 1,024–4,096-sample FFT and its associated buffering fit the product's latency budget. It is not the easiest first implementation for a low-latency single-vocal plug-in.

## 6. Formant preservation

Naive resampling shifts the spectral envelope together with the fundamental. Large upward shifts then sound “chipmunk-like,” while downward shifts can produce an unnaturally dark voice. Formant preservation means keeping the apparent vocal-tract resonances closer to their original frequencies while changing the excitation pitch.

Three practical levels are available:

1. **Approximate preservation with TD-PSOLA or fixed-duration grains.** The vocal waveform segment retains much of its local spectral envelope. This is inexpensive and often sufficient for small correction intervals.
2. **Residual/envelope processing.** Estimate the spectral envelope, inverse-filter or whiten the input to obtain a residual, pitch-shift the residual, and restore the envelope after synthesis. Lenarczyk demonstrates a real-time phase-vocoder architecture using spectral whitening before transformation and envelope reconstruction after it [10]. The paper discusses linear-predictive all-pole envelope modeling, spectral smoothing to avoid fitting individual harmonics as formants, and a more expensive true-envelope alternative. It also reports that insufficient high-frequency bandwidth harms fricatives and motivates bandwidth preservation [10].
3. **Dedicated formant tracking/correction.** Estimate formant peaks and warp the transformed envelope back toward the source. This offers more control but is sensitive to vowel transitions, register, breathiness, and noisy consonants.

Start with level 1 for a correction engine. Add level 2 only after the basic tracker and consonant protection are stable. LPC becomes unreliable when the frame is unvoiced or contains a strong transient; gate envelope estimation by voicing and interpolate envelope parameters between reliable frames. Add a small regularization/noise floor to prevent unstable inverse filters. Do not apply a full spectral envelope correction to a fricative without a separate noise-band policy.

## 7. Consonant and transient protection

Consonants carry intelligibility and timing. A pitch corrector that sounds good on an isolated sustained vowel can still fail in lyrics because it stretches or phase-rotates /s/, /t/, /k/, /f/, and plosive attacks.

Use a conservative voiced/unvoiced and transient decision:

- voiced confidence from periodicity and autocorrelation;
- high-frequency energy ratio or spectral flatness for fricatives;
- short-term energy derivative and spectral flux for attacks;
- optional zero-crossing rate as a weak unvoiced cue;
- pitch-shifter wetness that follows confidence rather than a hard binary switch.

Recommended policy:

- Keep the dry signal dominant during the first 10–30 ms of a detected transient.
- Do not create pitch-synchronous grains from low-confidence unvoiced material.
- For a voiced-to-unvoiced boundary, ramp the corrected path down over 10–30 ms while retaining the original unvoiced tail.
- For an unvoiced-to-voiced boundary, wait for 2–3 reliable periods before increasing correction authority.
- Crossfade at zero-energy-risk points or use equal-power fades; do not hard-switch buffers.
- In a phase-vocoder mode, reset/lock phases around attacks and preserve a dry transient layer.

The Lenarczyk real-time formant-preserving work is particularly relevant to the bandwidth problem: after downward pitch scaling, the transformed spectrum can lose high-frequency content, which damages fricatives; its bandwidth-extension treatment improves the result in listening tests [10]. This is evidence for preserving or separately regenerating the noise/consonant band, not for indiscriminately pitch-shifting it.

## 8. JUCE/VST3 latency and real-time engineering

Latency consists of all delays from input to time-aligned output: analysis look-ahead, pitch-shifter history, FFT overlap, resampler delay, crossfade buffering, and any safety buffer. Measure it with an impulse and with a known periodic signal. Do not report only the nominal frame length.

JUCE's `AudioProcessor::getLatencySamples()` returns the delay imposed by the processor, and `setLatencySamples()` is the mechanism for reporting it to the host. JUCE states that the processor should set the value as early as possible during initialization and may update it later if the value changes [1]. Therefore:

- compute the chosen quality mode's delay in `prepareToPlay()`;
- call `setLatencySamples()` before processing starts;
- if a user mode changes the actual delay, update it and notify the host through the normal JUCE/plug-in path;
- keep latency fixed during ordinary correction parameter automation where possible;
- test offline rendering, transport start, bypass, and mode changes because host delay compensation behavior differs.

At 48 kHz, useful latency budgets are approximately:

| Product mode | Total algorithmic target | Comments |
|---|---:|---|
| Live monitoring | 5–15 ms | Requires short analysis and a low-latency shifter; quality compromises likely |
| General real-time insert | 15–45 ms | Practical for most tracking/correction work |
| High quality | 45–100+ ms | More stable low notes and formant processing; poor for live monitoring |

The analysis frame is not the only determinant. A 2,048-sample frame at 48 kHz is 42.7 ms, but a causal frame with a ring-buffered shifter may have a different input-output alignment. Verify with the actual implementation.

Use lock-free or single-producer/single-consumer communication for telemetry. Never make the audio thread wait for the GUI. Store diagnostic counters atomically or in a preallocated ring and drain them outside the callback. Avoid denormals in silence and very low-level residuals. Keep all FFT plans, windows, interpolation tables, candidate arrays, and state machines bounded and preallocated.

## 9. Suggested implementation parameters

The following is a defensible first configuration for a 44.1/48 kHz plug-in intended for sung vocals around C2–C6:

| Parameter | Starting point | Tuning range / rationale |
|---|---:|---|
| F0 minimum | 60–65 Hz | Lower only for bass voices; raises required frame length |
| F0 maximum | 1,200–1,500 Hz | Higher values increase false harmonic candidates |
| Analysis frame | 2,048 samples at 48 kHz | 1,024 for low-latency high-register mode |
| Hop | 128–256 samples | 2.7–5.3 ms at 48 kHz |
| YIN threshold | 0.10–0.20 initial trough threshold | Validate against confidence; do not use alone |
| Candidate count | 3–5 minima | Include octave alternatives explicitly |
| Voicing enter | 0.70–0.80 for 2 hops | Adaptive RMS gate also required |
| Voicing hold | 0.45–0.55 | Prevent boundary chatter |
| Pitch-certainty gate | 0.60–0.75 | Below it, reduce wetness or hold target |
| Note-change dwell | 2–4 hops | Longer for hard correction in noisy input |
| Continuous correction | ±100–250 cents default | Clamp ±400 cents on tracker failure |
| Retune attack | 10–30 ms | 0–8 ms for effect mode |
| Retune release | 50–120 ms | Longer for natural correction |
| Unvoiced fade | 10–30 ms | Retain original consonant energy |
| Periodic grain length | 2–4 estimated periods or fixed 20–45 ms | Avoid abrupt duration changes |
| Phase-vocoder FFT, if used | 1,024–2,048 | Use transient/phase-coherence handling |
| Audio-thread update cadence | per sample or 8–32 samples | Smooth pitch ratio and wetness |

Treat the YIN threshold, confidence thresholds, and dwell times as coupled parameters. A longer frame generally improves periodicity but increases onset delay and consonant contamination. A faster note switch can compensate for delay only by increasing the risk of octave and boundary errors.

## 10. Test strategy

### 10.1 Detector unit tests

Generate deterministic signals with known ground truth:

- sine waves from the minimum to maximum F0;
- saw and pulse waves with strong second or third harmonics;
- additive harmonic voices with moving formants;
- vibrato at several rates and depths;
- portamento and octave leaps;
- amplitude ramps and glottal-like pulses;
- white, pink, and band-limited noise at controlled signal-to-noise ratios;
- voiced/unvoiced and consonant-like transitions;
- clipping, DC offset, and phase-inverted stereo inputs.

For each frame, log raw candidates, selected candidate, YIN value, autocorrelation, confidence, voiced state, target note, and correction ratio. Do not test only the final audio; tracker failures must be diagnosable.

Measure:

- **gross pitch error:** percentage of voiced frames beyond 20 or 50 cents;
- **octave error:** percentage beyond approximately 600 cents or specifically near ±1,200 cents;
- **fine error:** median and 95th-percentile cents error after excluding gross errors;
- **voicing precision, recall, and F1** against a known or manually annotated voiced mask;
- **onset and release latency:** time from ground-truth periodicity to accepted voiced state and back;
- **note-switch latency:** time from a stable new note to target change;
- **jitter:** frame-to-frame cents deviation on a constant input.

For singing, evaluate varied singer categories, laryngeal mechanisms, registers, vibrato, and reverberation. Babacan et al. provide a useful precedent: their benchmark uses annotated singing sounds with aligned electroglottograph recordings, assesses voicing boundaries and pitch contour, and specifically studies singer category, laryngeal mechanism, and reverberation [4].

### 10.2 Shifter and output tests

Use known input/output ratios and check that the measured output F0 follows the target. Test ±50, ±100, ±200, and ±400 cents, as well as ratio modulation from vibrato and portamento. For sustained vowels, measure cents error and listen for beating or periodic amplitude modulation. For consonants, compare onset timing and high-frequency energy to the dry reference. For formants, compare spectral-envelope peaks before and after shifting; a pitch shift that moves F0 correctly but moves vowel formants excessively is not a successful vocal result.

Measure impulse latency and verify that the declared JUCE latency equals the measured sample alignment. Repeat at 44.1, 48, 88.2, and 96 kHz, with block sizes of 1, 7, 32, 64, 128, 257, 512, and the host's maximum expected size. The output must be stable when blocks split in arbitrary places. Verify bypass alignment, state restore, automation, sample-rate changes, and mode changes.

### 10.3 Plug-in and real-time tests

Run Tracktion's open-source `pluginval` against the VST3 build; it is intended as a cross-platform plug-in validator and tester [11]. Also exercise Steinberg's VST3 Plug-in Test Host/Validator, which simulates audio and event inputs in a small host environment [12]. Add a stress test that runs the plug-in at the smallest supported buffer, forces random block boundaries, automates correction and target parameters, and checks for NaNs, denormals, clicks, missed deadlines, and allocations.

Profile worst-case CPU, not average CPU. The expensive cases include low notes requiring long windows, rapid pitch modulation, FFT quality modes, and many simultaneous diagnostics. Add a deadline counter around `processBlock()` in development builds, but do not print from the callback. Use offline renders for numerical regression and real-time runs for deadline behavior.

### 10.4 Listening panel

A small expert listening set should include sustained /a/, /i/, and /u/ vowels; voiced-to-unvoiced syllables such as “see,” “tea,” and “key”; plosives; breathy singing; low and high registers; vibrato; slides; noisy rooms; and recordings with reverb. Compare dry, low correction, hard correction, and formant-preserving modes at matched loudness. Artifacts to label include octave flips, delayed note acquisition, vibrato flattening, lisping or smeared consonants, phasey vowels, grain clicks, and formant/chipmunk coloration.

## 11. Implementation risks and mitigations

| Risk | Why it happens | Mitigation |
|---|---|---|
| Stable octave error | Strong harmonic/formant wins local YIN decision | Retain candidates, harmonic score, continuity cost, octave alternatives |
| Tracker chatter | Confidence crosses one threshold repeatedly | Enter/hold/exit hysteresis and confidence smoothing |
| Late correction | Long frame, causal wait, or shifter history | Measure end-to-end delay; offer low-latency mode; expose behavior clearly |
| Vibrato flattened | Target is forced to a static note sample by sample | Smooth note center; preserve bounded fast F0 deviation |
| Grain clicks | Period/epoch changes or low overlap normalization | Tapered windows, equal-power crossfades, overlap normalization, reset handling |
| Phasey vowels | Arbitrary granular overlap or incoherent STFT phases | PSOLA for voiced mono, WSOLA alignment, phase locking, or dry blend |
| Chipmunk voice | Resampling shifts formants with F0 | Fixed-duration grains or envelope/residual processing |
| Consonants become dull | Pitch-shifter processes unvoiced/transient energy | Voiced/transient detector, dry transient layer, bandwidth preservation |
| LPC instability | Envelope fit sees noise, harmonics, or ill-conditioned frames | Gate by voicing, smooth envelope, regularize, bound filter coefficients |
| Click on note changes | Ratio changes at block boundary | Log-ratio smoothing per sample/sub-block and phase-continuous shifter |
| Host desynchronization | Latency changes without reporting or wrong measured delay | `setLatencySamples()` during initialization and on real changes; impulse tests |
| DAW glitch | Allocation, lock, logging, or rebuild in callback | Preallocate; atomic parameter reads; lock-free telemetry; offline rebuild/swap |
| State-dependent failures | Sample rate/block size/channel layout assumptions | Test arbitrary blocks, rates, layouts, reset, bypass, and restored state |
| Overconfident correction | Voicing is high but pitch certainty is low | Separate voicing and pitch confidence; reduce wetness on ambiguity |

## 12. Recommended development order

First build the ring buffer and detector with a diagnostic view. Confirm F0, confidence, octave rejection, and state transitions on synthetic and recorded tests before writing a shifter. Next implement a dry-safe TD-PSOLA or granular path with a fixed correction ratio and an explicit bypass for unvoiced input. Add log-domain correction smoothing, note hysteresis, and confidence-driven wetness. Then add consonant/transient protection and measure latency. Only after these are stable should formant-envelope processing or a phase-vocoder quality mode be added.

This order matters because formant correction cannot repair a wrong pitch candidate, and a sophisticated pitch shifter cannot hide a bad voiced/unvoiced state machine. Keep detector, tracker, shifter, and host wrapper interfaces separate so each can be tested offline without loading a DAW.

## References

[1]: https://docs.juce.com/master/classjuce_1_1AudioProcessor.html "JUCE AudioProcessor class reference"
[2]: https://docs.juce.com/master/classjuce_1_1AudioProcessorValueTreeState.html "JUCE AudioProcessorValueTreeState class reference"
[3]: https://pubmed.ncbi.nlm.nih.gov/12002874/ "YIN, a fundamental frequency estimator for speech and music"
[4]: https://arxiv.org/html/1912.12609 "A Comparative Study of Pitch Extraction Algorithms on a Large Variety of Singing Sounds"
[5]: https://dl.acm.org/doi/abs/10.1109/78.80814 "An autocorrelation pitch detector and voicing decision with confidence measures developed for noise-corrupted speech"
[6]: https://librosa.org/doc/0.11.0/generated/librosa.pyin.html "librosa.pyin documentation: probabilistic YIN and Viterbi tracking"
[7]: https://www.isca-archive.org/eurospeech_1989/charpentier89_eurospeech.html "Pitch-synchronous waveform processing techniques for text-to-speech synthesis using diphones"
[8]: https://www.ee.columbia.edu/~dpwe/papers/LaroD99-pvoc.pdf "New phase-vocoder techniques for pitch-shifting, harmonizing and other exotic effects"
[9]: https://breakfastquay.com/rubberband/technical.html "Rubber Band Library technical notes"
[10]: https://www.isca-archive.org/interspeech_2017/lenarczyk17_interspeech.pdf "Real time pitch shifting with formant structure preservation using the phase vocoder"
[11]: https://github.com/Tracktion/pluginval "Tracktion pluginval: cross-platform plugin testing and validation"
[12]: https://steinbergmedia.github.io/vst3_dev_portal/pages/What+is+the+VST+3+SDK/Plug-in+Test+Host.html "Steinberg VST3 Plug-in Test Host"
[13]: https://doi.org/10.1121/1.1458024 "YIN paper DOI"
[14]: https://doi.org/10.1109/ICASSP.2014.6853678 "pYIN paper DOI"
[15]: https://www.isca-archive.org/interspeech_2017/lenarczyk17_interspeech.html "ISCA record for real-time formant-preserving pitch shifting"

*Prepared as an engineering research brief. Parameter ranges are starting points to validate against the target singers, latency budget, and chosen pitch-shifter implementation.*
.
