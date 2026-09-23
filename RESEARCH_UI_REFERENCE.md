# Afrobeat Vocal VST3 UI Reference Research

**Reference asset:** `/home/ubuntu/upload/45156.jpg` (4,080 × 3,060 px photograph of a monitor showing a dense channel-strip / vocal-processing plug-in inside a DAW).  
**Prepared for:** A modern, proprietary-brand-safe Afrobeat vocal VST3 plug-in.  
**Scope:** Visual analysis, layout geometry, color and typography direction, control conventions, meter treatment, spacing, hierarchy, and implementation notes. OCR was intentionally limited to labels that clarify the control groups.

## Executive direction

The photograph is most useful as a reference for **information architecture and tactile control language**, not as a bitmap to reproduce. It shows a narrow, vertically stacked console with clear processing blocks, strong color coding, illuminated status indicators, and a dedicated meter plus preset area. A faithful but original implementation should preserve those behaviors while using a cleaner grid, larger type, better scaling, and a distinct Afrobeat-inspired palette. Do not copy the visible product name, logo, exact panel artwork, exact knob geometry, or preset names.

The recommended design is a **single-screen vocal channel strip** with four processing lanes—Input/Control, Tone, Character, and Output/Space—plus a persistent center meter and a collapsible preset browser. The visual metaphor can remain hardware-like, but the interaction model should be modern: resizable, HiDPI-safe, automation-aware, keyboard-readable, and usable without relying on tiny labels.

## What the photograph contains

The reference appears to be a full channel-strip / console-style effect rather than a simple one-purpose compressor. The central plug-in body is surrounded by a dark host window and DAW chrome. Visible labels indicate families such as gate, compressor, EQ, high/low-pass filtering, harmonic or “THD” processing, output gain, solo, stereo mode, and a preset list. This label reading is approximate because the image is a perspective photograph, not a screen capture.

The design language combines **light gray metal-like modules**, **cyan/turquoise and blue accents**, **red warning or active-state controls**, a **black/cyan meter**, and a **very dark preset panel**. The result is deliberately dense and “console” oriented. It communicates that many processors are active in one signal path, but it also creates risks: small controls, low contrast on gray-on-gray captions, and a visual hierarchy that is stronger for experienced engineers than for first-time vocal users.

## Geometry and layout reconstruction

The following measurements are normalized estimates from the photograph. The monitor is photographed at an angle, so these are design proportions rather than pixel-accurate source dimensions.

| Region | Approximate position in the photographed frame | Proportion of plug-in canvas | Design interpretation |
|---|---:|---:|---|
| Plug-in canvas | x ≈ 12–96%, y ≈ 35–91% of photo | 100% | The host surrounds the plug-in with black DAW chrome. The product itself should not depend on the host chrome for identity. |
| Utility/header strip | top ≈ 7–10% of plug-in canvas | full width | Preset, A/B, undo/redo, bypass, resize, and help functions. |
| Processing body | ≈ 88–91% of plug-in canvas height | full width minus header | Main signal-path modules. |
| Left processing bank | ≈ 0–17% of plug-in width | 1 narrow column | Gate/expander and compressor-like controls. |
| Mid-left dynamics bank | ≈ 17–31% | 1 narrow column | A second dynamics or level-control section with threshold, attack/release, makeup, and mix-style controls. |
| Mid tone / character bank | ≈ 31–58% | 2–3 stacked sections | Filter, EQ/shape, harmonic/THD-style processing, and tone controls. |
| Meter bank | ≈ 58–73% | 1 medium column | Tall black display, scale, output/voice readouts, and status LEDs. |
| Output / utility bank | ≈ 73–82% | 1 narrow column | Large red control, polarity/utility button, mute, stereo mode, and output controls. |
| Preset browser | ≈ 82–100% | dark side panel | Preset list, categories, search/dropdown behavior, and a vertical fader-like control. |

The dense body is organized around **vertical panel seams**. Most sections have their own header, a small enable indicator, two or three rows of knobs, and a bottom row of mode buttons. The seams are important: they let a user scan the signal path left-to-right even when the individual control labels are small. In a new plug-in, preserve this scan path but use a consistent 8 px baseline grid and enough breathing room to avoid the cramped appearance of the reference.

### Recommended production geometry

Use a 1,440 × 820 logical-pixel reference layout, with a minimum supported size of about 1,080 × 620 and a maximum that can grow without changing the processing order. A practical proportional layout is:

- Header: 56 px, with 16 px outer padding.
- Main body: 748 px at the reference size.
- Four processing lanes: 18%, 18%, 24%, and 20% of the main body width.
- Meter/output lane: 20%, including a 160–220 px meter at the reference size.
- Preset browser: 260–300 px when open; collapse it to a 48 px rail when space is limited.
- Panel gutters: 8 px between adjacent modules; 16 px between major lanes.
- Internal panel padding: 12–16 px. Keep label-to-control spacing at 6–8 px.
- Primary knob diameter: 48–64 px. Secondary knob diameter: 32–40 px. Never make an important control smaller merely to retain the photographed density.

Use a responsive constraint system rather than hard-coded pixel positions. JUCE’s editor model places controls in `resized()` and recalculates bounds whenever the editor changes size; that is a good conceptual model even if a different UI framework is used.[1] VST3 explicitly supports a resizable editor, logical parameter organization, mouse-over parameter discovery, and context-menu integration, so these should be treated as product requirements rather than optional polish.[2] [3]

## Color and material direction

Color observations are approximate because the source is a photograph of a display with unknown white balance and reflections. A coarse sample of the image is dominated by near-black host/preset areas, neutral gray modules, and cyan/blue/red accents. Use the following as a starting token set, then tune it against an actual calibrated screenshot:

| Token | Approximate value | Role |
|---|---|---|
| `ink-950` | `#101316` | Main canvas, meter background, and deep chrome. |
| `ink-900` | `#1A1E22` | Preset browser and utility bars. |
| `metal-200` | `#D1D0C8` | Main light module face. |
| `metal-300` | `#B8B8B0` | Secondary module face and separators. |
| `metal-500` | `#777B7C` | Borders, inactive ticks, and secondary text. |
| `cyan-400` | `#8FD5D0` | Tone/air/filter family and active module accents. |
| `teal-500` | `#2A9C9D` | Primary tonal knobs and active meters. |
| `blue-500` | `#2A70C6` | Level, mix, or width controls. |
| `red-500` | `#D92732` | Attention, clipping, bypass/mute state, or character controls. |
| `amber-400` | `#D7A34A` | Warning, threshold, or secondary status. |
| `text-primary` | `#F2F4F3` on dark / `#172024` on light | High-priority labels and readouts. |
| `text-secondary` | `#4C5558` on light / `#AAB4B5` on dark | Supporting labels. |

A modern version should keep the **neutral metal + colored control** relationship, but avoid a literal brushed-metal texture. Use a mostly flat, subtly graduated panel with a 1 px inner highlight, a 1 px dark edge, and restrained shadows. This keeps the tactile console metaphor while producing sharper rendering at high DPI. Assign color by *function*, not by arbitrary decoration: teal for tone and musical color, blue for level/mix/spatial controls, red for attention or aggressive character, and amber for warnings.

Contrast should be tested in both inactive and active states. Light-gray labels on silver panels are the weakest legibility pattern in the photograph. For the new plug-in, reserve small gray text for nonessential captions, and use a dark ink color for all actionable labels.

## Typography and labeling

The photographed UI uses compact condensed, all-caps or title-case sans-serif labels with short abbreviations. That convention fits a console, but it appears optimized for an experienced user at a large monitor. Use a distinct, licensing-safe UI sans-serif such as Inter, IBM Plex Sans, Source Sans 3, or the platform system font. Do not imitate a proprietary typeface.

Recommended type scale at 1,440 × 820 logical pixels:

- Product name: 16–18 px, semibold.
- Major section title: 11–12 px, semibold, letter spacing 0.06–0.10 em.
- Control label: 10–11 px, medium, sentence case or short title case.
- Value readout: 12–14 px tabular numerals; use a slightly brighter color than labels.
- Meter tick labels: 9–10 px minimum; 11 px is preferable for the primary scale.
- Help/tooltip text: 12–13 px with a 1.35 line height.

Prefer full words for first exposure—“Threshold,” “Attack,” “Release,” “Makeup”—and reserve abbreviations for secondary annotations. A compact “THD” or “SC” label may remain, but its tooltip should expand to “harmonic drive” or “side-chain.” Each knob should have a stable label above or below it and a value that appears on hover, focus, or drag. Avoid using a color alone to distinguish controls; pair the color with a label, icon, or explicit state.

## Knob, button, and slider conventions

### Rotary controls

The reference uses analog-looking rotary knobs with a visible white pointer, tick marks, and a circular travel arc. Knob color is the primary quick-scan cue: neutral knobs appear general-purpose, turquoise knobs appear tone-related, blue knobs appear level or mix-related, and red knobs attract attention. This is an effective convention and should be retained in an original visual language.

Implement each primary knob with:

1. A 270–300° arc with a clear start and end. Use a 300° arc for continuous gain-like values and a smaller arc only when the range is intentionally constrained.
2. A high-contrast pointer line or wedge that remains visible at every value.
3. A thin active arc in the control’s functional color, with a low-contrast inactive arc.
4. A direct numeric readout on hover/drag and a double-click reset gesture.
5. Shift-drag or host-standard fine adjustment. Respect the VST3 host knob-mode indication where the host provides it.[2]
6. A right-click or context-menu path to parameter details, automation, and MIDI learn where supported.

The photographed controls use tick numbers around the perimeter. Use ticks sparingly. A continuous knob with five to nine meaningful markers is easier to read than a ring of tiny numbers. For threshold and frequency controls, place the unit in the value readout (`dB`, `Hz`, `ms`, `%`) instead of forcing the user to decode a dense perimeter.

### Buttons and switches

The reference uses small rectangular “IN,” “MUTE,” “LIMIT,” “CLIP,” and mode buttons, plus small circular LED indicators. Keep this language but modernize the state treatment:

- Use a minimum 28–32 px hit target, even if the visible button face is smaller.
- Provide three states where appropriate: off, on, and unavailable/disabled.
- Use a filled accent and a bright indicator for on-state; do not rely on red alone for “enabled.”
- Make momentary actions visually different from toggles.
- Put the label inside the button for short actions and outside for processing-state toggles.
- Add a tooltip and accessible name for icon-only controls.

### Fader and utility controls

The photographed right-hand strip contains a long vertical fader with a white rectangular handle on a dark track. Keep one output fader or vocal level fader as a strong visual anchor, but separate it from the preset browser so that users do not mistake the browser column for the audio path. Give the fader a visible 0 dB reference tick, clip region, and numeric value. If a vocal plug-in provides input trim and output trim, use compact knobs for input and a dedicated fader for output.

## Meter treatment

The central meter is one of the most successful visual anchors in the reference. It is tall, dark, and high contrast, with cyan/blue numeric markings and separate rows of colored LED-like indicators for gate/expander, compressor, and limiter activity. The meter creates a clear “is the processing working?” feedback loop without requiring the user to inspect every knob.

A modern Afrobeat vocal plug-in should make the meter more legible and more purposeful:

- Show **input peak** and **output peak** as two thin vertical bars, with a held peak marker.
- Show **gain reduction** as a separate bar or horizontal strip rather than conflating it with level.
- Use a conventional dBFS scale, with a clearly marked 0 dBFS ceiling and a red clip/true-peak state.
- Add a compact numerical readout for input, output, and gain reduction. Use tabular numerals so the display does not jump.
- Keep the reference’s three processor activity rows, but label them plainly: Gate, Comp, Limit. Use green for normal activity, amber for approaching a warning region, and red for clipping or excessive limiting.
- Consider a switchable “Vocal Focus” view that adds sibilance reduction or pitch-correction activity, but do not overload the main level meter with unrelated scales.

If the product claims loudness measurement, distinguish it from peak metering. ITU-R BS.1770-5 defines algorithms for programme loudness and true-peak audio level; the current recommendation was approved in November 2023.[4] EBU R 128 distinguishes Momentary (400 ms), Short-term (3 s), and Integrated measurements and describes a -23 LUFS broadcast normalization target.[5] Those standards are useful references, but a real-time vocal insert does not need to claim broadcast compliance. A safe implementation is a peak/GR meter by default, with optional Momentary and Short-term loudness readouts only if the DSP calculates them correctly. Do not place an “EBU R128” logo or claim compliance without implementing the relevant specification and usage requirements.

The iZotope Insight product page is a useful current example of separating loudness, levels, sound field, and spectrogram modules instead of hiding all measurements in one overloaded display.[6] For this vocal plug-in, copy the principle of **separate measurement roles**, not the appearance.

## Visual hierarchy and interaction flow

The photograph’s hierarchy is primarily spatial:

1. The processing body dominates the screen.
2. The meter provides the highest-contrast diagnostic feedback.
3. Red, cyan, and blue knobs create scan points inside gray modules.
4. The dark preset column is visually separate from the processor.
5. Small LEDs and mode buttons communicate whether a section is active.

Preserve this order, but add a stronger onboarding path. The first-time user should immediately find **preset, input/output, main vocal amount, and bypass**. Advanced controls can remain visible but should be visually subordinate. A recommended top bar is:

`Brand-safe product name | preset name + category | A/B | undo/redo | global bypass | resize/help`

The main body can read left-to-right as:

`Input / cleanup → Dynamics → Tone / character → Meter / output → Presets`

Within each module, use a stable top-to-bottom order: enable and mode at the top, high-impact primary knobs next, supporting controls below, and a small status line at the bottom. Avoid mixing unrelated controls in one column merely because they fit physically.

## Modern but faithful product concept

A strong original direction is a **warm-night console** rather than a replica of the photographed light-gray console. Use deep charcoal panels with pale warm-gray surfaces for active modules. Introduce a restrained coral-red accent for “presence/character,” turquoise for “tone/air,” and blue for “level/space.” Keep the central meter black with cool cyan lines so it remains the most diagnostic area.

The visual identity can reference Afrobeat through rhythm and color relationships—regular module cadence, warm/cool contrast, and a controlled sunset coral—without using flags, cultural clichés, copied names, or a literal imitation of any existing console. The brand mark should be original. Use a simple geometric wordmark or icon that does not resemble the photographed manufacturer mark.

### Suggested processing modules

| Module | Primary controls | Secondary controls | Visual accent |
|---|---|---|---|
| **Clean** | Input trim, gate/expander threshold, range | Attack, release, side-chain filter, listen | Blue-gray / amber status |
| **Shape** | Compressor threshold, ratio, makeup | Attack, release, mix, auto mode | Turquoise |
| **Color** | High-pass, low-pass, presence, air | Resonance, drive, tilt, de-ess amount | Coral-red for drive; cyan for air |
| **Space** | Width, send/mix, output trim | Mono check, polarity, stereo mode | Blue |
| **Meter** | Input/output, GR, clip | Momentary/short-term toggle, reset peak | Near-black + cyan |
| **Presets** | Category, search, load/save | Favorites, A/B, randomize if desired | Near-black; no audio-path ambiguity |

The labels and names above are generic implementation suggestions. They should not reuse visible preset names or proprietary branding from the photograph.

## Concrete implementation notes

### Rendering and scaling

Use vector primitives or scalable raster assets for knobs, arcs, ticks, and meters. Avoid a single fixed-resolution background image. The VST3 SDK supports a resizable editor, and its documentation also lists plug-in content scaling support in later revisions.[2] [3] Implement a logical coordinate system and render at the device pixel ratio. Test at 100%, 125%, 150%, and 200% display scaling, plus a narrow editor width where the preset browser collapses.

FabFilter’s current Pro-Q product page is a useful example of modern plugin expectations: resizable interface, customizable scaling, full-screen mode, interactive displays, MIDI learn, undo/redo, A/B, and parameter interpolation are explicitly treated as workflow features.[7] The new plug-in does not need all of these on day one, but resizability, A/B, safe automation, value readouts, and clear feedback should be in the first implementation.

### Parameter model and host behavior

Give every user-facing control a stable parameter ID and a meaningful unit. Group parameters semantically so the host can expose them as Input, Dynamics, Tone, Character, Meter, and Output rather than a flat list. The VST3 documentation describes logical parameter organization and host-facing parameter discovery; use that structure to make automation lanes understandable.[2] [3]

Use host-compatible begin-edit / perform-edit / end-edit behavior for every knob drag. Add parameter attachment or an equivalent binding layer so GUI state cannot drift from DSP state. Ensure that preset loading updates both DSP and UI, and that automation playback does not fight the editor’s visual state. Provide a parameter context menu where the host supports it, including automation and MIDI mapping options.

### Accessibility and usability

Provide a visible focus ring, keyboard increments, and a textual value overlay for every control. Include a high-contrast mode that darkens text on the light panels and preserves the accent semantics. Do not encode “active” only through a tiny LED or color. At minimum, pair LED state with a label change, filled button, or accessible state text.

The reference’s density is appropriate for experienced mixing engineers but not for a casual singer or producer. Add short tooltips that explain purpose and units without turning into a manual. A “simple” view can expose only Input, Main, Color, Space, and Output; an “advanced” view can reveal the full photographed-style grid. This preserves the console’s depth without forcing every user to parse it.

### Meter performance and perceptual behavior

Meters should be updated on the UI timer, not drawn on every audio sample. Apply a fast attack, a musically readable release, and a peak-hold decay. Keep DSP measurement and UI smoothing separate so the readout remains truthful. Make clip indicators latch until reset or until the user explicitly dismisses them. If gain reduction is shown, use a different direction or color from the level bars so users cannot mistake “more processing” for “more level.”

### Preset browser behavior

Keep the browser visually detached from the audio path. Use a dark column with a clear header, search/filter affordance, category chips, and a scrollable list. Use generic preset names such as `Afro Lead - Forward`, `Chorus Lift`, or `Soft Verse` only as product-content examples; do not reuse names visible in the photograph. Display the current preset in the header even when the browser is collapsed. Include unsaved-state indication and an explicit save action.

## Proprietary-brand and trade-dress avoidance checklist

The final UI should not reproduce the photograph’s manufacturer name, logo, wordmark, exact product name, exact preset list, or distinctive proprietary artwork. Do not trace the same panel outlines, use the same typography, or copy the exact combination and placement of every control. It is acceptable to use broad industry conventions such as rotary knobs, gray module panels, colored status LEDs, a central meter, a preset list, and a vertical fader. Make the new identity distinct through its color tokens, spacing, typography, module names, iconography, meter arrangement, and responsive behavior.

## Acceptance criteria for a first UI build

1. At a glance, users can identify the signal path, bypass state, preset, output level, and current processing activity.
2. At 100–200% scale, all labels and values remain legible and all important controls have at least a 28 px effective hit target.
3. The UI remains usable when the preset browser is collapsed and when the editor is resized narrower than the reference.
4. Knob values appear during interaction, reset on double-click, support fine adjustment, and remain synchronized with automation and preset loading.
5. The meter distinguishes level, gain reduction, and clipping. If loudness is shown, its timing and units are explicitly labeled.
6. The visual language feels console-like and tactile but does not copy visible brand marks, exact typography, exact artwork, or exact preset names from the reference.
7. The plug-in passes screenshot review at 100%, 125%, 150%, and 200% display scaling on both dark and light host backgrounds.

## References

[1]: https://juce.com/tutorials/tutorial_code_basic_plugin/ "JUCE: Create a basic Audio/MIDI plugin, Part 2"
[2]: https://steinbergmedia.github.io/vst3_dev_portal/pages/Technical+Documentation/Index.html "Steinberg VST 3 Developer Portal: Technical Documentation"
[3]: https://github.com/steinbergmedia/vst3sdk "Steinberg VST 3 Plug-In SDK"
[4]: https://www.itu.int/rec/R-REC-BS.1770 "ITU-R Recommendation BS.1770: Algorithms to measure audio programme loudness and true-peak audio level"
[5]: https://tech.ebu.ch/loudness "EBU Loudness and R 128 guidance"
[6]: https://www.izotope.com/products/insight "iZotope Insight 2: Intelligent metering for music & post"
[7]: https://www.fabfilter.com/products/pro-q-4-equalizer-plug-in "FabFilter Pro-Q 4: Interface workflow, scaling, and metering features"
[8]: https://developer.native-instruments.com/komplete-ui/ "Native Instruments Komplete UI: Scalable, efficient UI framework"

## Source note

The external references above were consulted for implementation conventions and standards. The geometry, colors, hierarchy, and control observations are an original analysis of `/home/ubuntu/upload/45156.jpg`; they are approximate because the reference is a perspective photograph of a screen rather than a lossless UI capture.
