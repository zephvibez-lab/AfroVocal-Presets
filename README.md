# AfroVocal Presets

**AfroVocal Presets** is a JUCE 8 vocal-effect plug-in focused on Afrobeat, Amapiano, emotional lead vocals, harmonies, and modern tuned vocal treatments.

## Current implementation

The processor now includes a causal, monophonic pitch-correction path: a 2,048-sample analysis frame with 256-sample hop, normalized autocorrelation/YIN-style candidate scoring, confidence and voicing gating, octave/continuity bias, key/scale target selection, smoothed correction ratio, and a bounded dual-ring granular-style correction blend that protects unvoiced material by returning toward the dry input. The rest of the chain provides high-pass filtering, low-mid cleanup, presence, air, compression, parallel compression, de-essing, warmth/saturation, plate and ambient reverb, delay, output gain, and ten Afrobeat/Amapiano-oriented factory presets.

The audio callback uses cached APVTS parameter pointers, preallocated scratch/ring buffers, allocation-free biquad coefficient updates, variable-block/channel guards, explicit reset behavior, a canonical host bypass parameter, and zero reported latency for the current causal implementation. The optional AI worker never calls host parameter APIs directly; it stages a validated JSON result for the editor/message thread to apply.

## Performance upgrade

The current development build adds **AfroFocus**, an adaptive vocal-lock macro that increases pitch correction authority only when the tracker has enough confidence. It is designed to preserve expressive transitions while tightening unstable notes. Retune Speed now controls the correction smoothing time, and the channel-strip EQ/air filters are actively applied in the audio path. The engine remains allocation-free in the callback, uses cached parameters and preallocated buffers, and reports zero host latency for the current causal design.

## Bold mixer UI and preset generation

The current interface uses a larger 1,540 × 900 default canvas, bold section hierarchy, enlarged value fields, two-row mixer modules, clear key/scale access, and a browser that exposes **1,034 total presets**: ten curated factory presets plus 1,024 deterministic AfroVocal variations across Afrobeat, Amapiano, Afro-R&B, emotional, harmony, Highlife, Dancehall, and Alté styles.

The **GENERATE AI** control always works offline: every click creates and loads a different new preset with coordinated tuning, key/scale, EQ, compression, de-essing, warmth, ambience, delay, and output values. When the user also supplies a prompt and API key, the same action starts the optional online AI variation worker; the offline preset remains available if internet access is unavailable.

## Reference-driven editor

The editor has been redesigned around the uploaded reference image’s information architecture without copying proprietary artwork or branding. It uses a dense light-metal channel-strip body, colored functional controls, active-state LEDs, a high-contrast central peak/gain-reduction meter, a dark preset browser, compact utility header, and an AI assistance footer. The logical target is approximately 1,320 × 760 with a resizable minimum of 1,080 × 620, following the research recommendation for larger knobs and readable labels rather than reproducing the photo’s cramped geometry.

## Native Windows VST3 build

Use the checked-in CMake presets to build the native Windows artifact with Visual Studio 2022 x64:

```bat
cmake --preset windows-release
cmake --build --preset windows-release
```

The release bundle is generated at:

```text
Build-Windows/AfroVocalPresets_artefacts/Release/VST3/AfroVocal Presets.vst3
```

Install the entire bundle, preserving its `Contents` folder, into:

```text
C:\Program Files\Common Files\VST3
```

Then open FL Studio’s Plug-in Manager, choose **Find installed plugins**, and run **Verify plugins** for the new entry. Do not flatten the bundle into a DLL and do not place it in the legacy VST2 folder.

The repository also contains `.github/workflows/build-windows.yml`, which builds the Release x64 artifact on `windows-2022`, checks the expected `Contents/x86_64-win` binary, and uploads a ZIP package.

## Linux verification

The local Linux build can be configured with:

```bash
cmake --preset linux-release
cmake --build --preset linux-release
```

The Linux VST3 is useful for local source validation but is not loadable by Windows FL Studio. A final native Windows build and FL Studio scan must be run on Windows or the configured Windows CI runner; this environment cannot honestly claim that Windows-host validation has already occurred.

## Research reports

- `RESEARCH_UI_REFERENCE.md` — close visual analysis of the uploaded reference and an original, brand-safe geometry/color/interaction specification.
- `RESEARCH_PITCH_DSP.md` — practical monophonic vocal pitch tracking, retuning, pitch shifting, consonant protection, and validation.
- `RESEARCH_JUCE_SAFETY.md` — source-specific real-time safety and host-contract audit.
- `RESEARCH_WINDOWS_FL.md` — native Windows VST3 bundle, validation, installation, signing, CI, and FL Studio checklist.
