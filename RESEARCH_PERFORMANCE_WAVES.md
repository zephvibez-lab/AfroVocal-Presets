# AfroVocal Presets performance direction — Waves-style workflow research

The target is a professional real-time vocal workflow, not a copy of Waves source code, branding, or artwork.

Official Waves Tune Real-Time describes the expected feature bar as automatic real-time vocal tuning, transparent-to-creative correction, ultra-low latency, formant correction, natural vibrato handling, scale editing, tolerance/range control, and optional MIDI pitch control. Waves Tune additionally emphasizes correction speed, note transition time, correction amount, scale conformity, natural vibrato preservation, formant-corrected shifting, and hard-tuned creative modes.

The AfroVocal performance upgrade therefore prioritizes: a causal pitch/voicing tracker, fast retune response, key/scale targeting, confidence-aware correction authority, transient/consonant protection, a visible peak/gain-reduction meter, a dedicated adaptive AfroFocus macro, and a fixed offline-safe audio path. The UI remains an original channel-strip implementation inspired by the uploaded reference rather than a copy of Waves or any other product.

Sources:

- https://www.waves.com/plugins/waves-tune-real-time
- https://www.waves.com/plugins/waves-tune
- https://docs.juce.com/master/classjuce_1_1AudioProcessor.html
- https://github.com/Tracktion/pluginval

Implementation note: the current causal pitch stage reports zero samples of latency to the host. Any future formant-preserving PSOLA or phase-vocoder mode must measure and report its actual latency with `setLatencySamples()` before shipping.
