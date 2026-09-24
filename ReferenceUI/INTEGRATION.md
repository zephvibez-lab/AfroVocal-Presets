# AfroVocal reference UI functional bridge

The supplied visual HTML/CSS remains untouched. The behavior bridge is in `afrovocal-runtime.js`.

Add exactly one script element immediately before the closing `</body>` tag of the complete supplied HTML:

```html
<script src="afrovocal-runtime.js"></script>
```

If the HTML file is in a different folder, adjust only the relative `src` path; do not change the new layout or CSS.

The bridge binds to the existing new-version DOM using the IDs/classes already present in the supplied code:

- `#knob-hpf`, `#knob-tune`, and `#knob-focus`
- Other knobs using the matching `#knob-*` IDs when present
- `.preset-item` preset rows
- `.dropdown-display` preset display
- `.btn-ai`, `#generate-ai`, or `[data-action="generate-ai"]`
- `.top-bar .hw-button` controls by their visible labels
- `.meter-fill` meter bars
- `.ai-status`, `#ai-status`, `[data-ai-status]`, or `.api-status`

Behavior included:

- Pointer-drag, wheel, keyboard, and accessible slider behavior for knobs
- Value display/ring updates without changing the stylesheet
- Factory preset loading
- 1,024 deterministic generated presets plus the factory bank
- Generate AI offline fallback
- Optional online AI event handoff when the new UI provides a prompt and API key
- A/B/C/D snapshots
- Copy/paste chain state
- Save state feedback
- Bypass state feedback
- Animated meter fill updates
- `afrovocal:*` CustomEvents for host integration

The source was syntax-checked with Node.js and passed validation.
