/* AfroVocal Presets — functional bridge for the supplied reference HTML.
 * This file intentionally does not define layout, colours, animations, or CSS.
 * It only binds behavior to the new DOM using existing classes and knob IDs.
 */
(() => {
  'use strict';

  const FACTORY = [
    'Afrobeat Lead - Clear Bounce', 'Afrobeat Lead - Warm Pocket',
    'Afropiano Lead - Gloss', 'Afropiano Lead - Soft Air',
    'Emotional Ballad - Intimate', 'Emotional Ballad - Wide',
    'Background Harmony - Tucked', 'Background Harmony - Airy Stack',
    'Hard-Tuned Lead - Modern', 'Hard-Tuned Lead - Dry'
  ];
  const GENRES = ['Afrobeat', 'Amapiano', 'Afro-R&B', 'Emotional', 'Harmony', 'Highlife', 'Dancehall', 'Alté'];
  const MOODS = ['Clear', 'Warm', 'Gloss', 'Intimate', 'Wide', 'Hard Tune', 'Pocket', 'Airy', 'Dark', 'Lifted', 'Radio', 'Night'];
  const TEXTURES = ['Lead', 'Stack', 'Chorus', 'Bounce', 'Velvet', 'Club', 'Ambient', 'Dry'];
  const TOTAL_GENERATED = 1024;

  const state = {
    preset: FACTORY[0], presetIndex: 0, generatedIndex: 0, bypass: false,
    bank: { A: {}, B: {}, C: {}, D: {} }, activeBank: 'A', clipboard: null,
    values: {
      hpf: 20, tune: 60, focus: 80, threshold: 46, ratio: 50, attack: 35,
      parallel: 30, release: 50, lowMid: 42, presence: 56, output: 55,
      air: 55, warmth: 32, deEss: 38, plate: 24, delay: 12, ambient: 12, time: 35
    }
  };

  const $ = (s, root = document) => root.querySelector(s);
  const $$ = (s, root = document) => [...root.querySelectorAll(s)];
  const clamp = (n, lo = 0, hi = 100) => Math.max(lo, Math.min(hi, n));
  const knobId = k => `#knob-${k}`;
  const knobKeys = Object.keys(state.values);

  function setStatus(message) {
    const candidates = ['.ai-status', '#ai-status', '[data-ai-status]', '.api-status', '.ai-panel .status'];
    const node = candidates.map(s => $(s)).find(Boolean);
    if (node) node.textContent = message;
    document.dispatchEvent(new CustomEvent('afrovocal:status', { detail: message }));
  }

  function valueToDisplay(key, value) {
    if (key === 'hpf') return `${Math.round(60 + value * 8)} Hz`;
    if (['tune', 'focus', 'parallel', 'air', 'warmth', 'deEss', 'plate', 'delay', 'ambient'].includes(key)) return `${value.toFixed(1)}%`;
    if (key === 'threshold') return `${(-36 + value * 0.36).toFixed(1)} dB`;
    if (key === 'ratio') return `${(1 + value * 0.08).toFixed(2)}:1`;
    if (key === 'attack' || key === 'release') return `${(2 + value * 1.5).toFixed(1)} ms`;
    if (key === 'lowMid' || key === 'presence') return `${(-6 + value * 0.12).toFixed(1)} dB`;
    if (key === 'output') return `${(-3 + value * 0.03).toFixed(2)} dB`;
    if (key === 'time') return `${Math.round(80 + value * 4)} ms`;
    return value.toFixed(1);
  }

  function updateKnob(key, value, announce = true) {
    if (!(key in state.values)) return;
    value = clamp(Number(value)); state.values[key] = value;
    const knob = $(knobId(key));
    if (!knob) return;
    knob.dataset.value = String(value);
    knob.setAttribute('aria-valuenow', String(value));
    knob.setAttribute('aria-label', `${key}: ${valueToDisplay(key, value)}`);
    const ring = $('.knob-value-ring', knob);
    if (ring) ring.style.strokeDashoffset = String(140 - value * 1.4);
    const cap = $('.knob-cap', knob);
    if (cap) cap.style.transform = `rotate(${(-135 + value * 2.7).toFixed(1)}deg)`;
    const valueNode = $('.knob-value', knob.parentElement || knob);
    if (valueNode) valueNode.textContent = valueToDisplay(key, value);
    knob.dispatchEvent(new CustomEvent('afrovocal:valuechange', { bubbles: true, detail: { key, value, display: valueToDisplay(key, value) } }));
    if (announce) document.dispatchEvent(new CustomEvent('afrovocal:parameter', { detail: { key, value } }));
  }

  function bindKnob(key) {
    const knob = $(knobId(key)); if (!knob) return;
    knob.setAttribute('role', 'slider'); knob.setAttribute('tabindex', '0'); knob.setAttribute('aria-valuemin', '0'); knob.setAttribute('aria-valuemax', '100');
    let startY = 0, startValue = state.values[key], dragging = false;
    const move = e => { if (!dragging) return; const y = e.touches ? e.touches[0].clientY : e.clientY; updateKnob(key, startValue + (startY - y) * 0.65); };
    const end = () => { dragging = false; window.removeEventListener('pointermove', move); window.removeEventListener('pointerup', end); window.removeEventListener('touchmove', move); window.removeEventListener('touchend', end); };
    knob.addEventListener('pointerdown', e => { dragging = true; startY = e.clientY; startValue = state.values[key]; knob.setPointerCapture?.(e.pointerId); window.addEventListener('pointermove', move); window.addEventListener('pointerup', end); });
    knob.addEventListener('wheel', e => { e.preventDefault(); updateKnob(key, state.values[key] + (e.deltaY < 0 ? 2 : -2)); }, { passive: false });
    knob.addEventListener('keydown', e => { if (e.key === 'ArrowUp' || e.key === 'ArrowRight') { e.preventDefault(); updateKnob(key, state.values[key] + 1); } if (e.key === 'ArrowDown' || e.key === 'ArrowLeft') { e.preventDefault(); updateKnob(key, state.values[key] - 1); } });
    updateKnob(key, state.values[key], false);
  }

  function findPresetDisplays() { return $$('.dropdown-display, [data-preset-display], #preset-display'); }
  function setPresetDisplay(name) { findPresetDisplays().forEach(n => { const span = n.querySelector('span'); if (span) span.textContent = name; else n.textContent = name; }); $$('.preset-item').forEach(item => item.classList.toggle('active', item.textContent.trim().startsWith(name))); }

  function applyPreset(index, name = FACTORY[index] || name) {
    state.preset = name; state.presetIndex = index;
    const phase = (index % 32) / 31; const mood = index % MOODS.length;
    const next = { ...state.values, hpf: 10 + (index * 7) % 90, tune: mood === 5 ? 94 : 22 + phase * 46, focus: 38 + mood / 11 * 38, threshold: 35 + mood * 3, ratio: 24 + mood * 5, attack: 20 + phase * 45, parallel: 8 + mood * 2, release: 30 + phase * 55, lowMid: 35 + phase * 20, presence: 45 + mood * 3, output: 55, air: 22 + phase * 45, warmth: 14 + phase * 30, deEss: 22 + phase * 42, plate: 16 + phase * 28, delay: 6 + phase * 18, ambient: mood === 4 ? 52 : 8 + phase * 22, time: 5 + (index * 37) % 95 };
    Object.entries(next).forEach(([key, value]) => updateKnob(key, value, false)); setPresetDisplay(name); setStatus(`Loaded ${name}`);
    document.dispatchEvent(new CustomEvent('afrovocal:preset', { detail: { name, index, values: { ...state.values } } }));
  }

  function generatePreset() {
    const index = state.generatedIndex++ % TOTAL_GENERATED; const genre = GENRES[index % GENRES.length]; const mood = MOODS[Math.floor(index / GENRES.length) % MOODS.length]; const texture = TEXTURES[Math.floor(index / (GENRES.length * MOODS.length)) % TEXTURES.length];
    const name = `AI / ${genre} / ${mood} ${texture} #${String(index + 1).padStart(4, '0')}`;
    applyPreset(index % FACTORY.length, name); setStatus(`Generated and loaded ${name}`); return name;
  }

  function bindPresetList() { $$('.preset-item').forEach((item, i) => item.addEventListener('click', () => applyPreset(i % FACTORY.length, item.dataset.preset || item.textContent.trim()))); }

  function snapshot() { return JSON.parse(JSON.stringify(state.values)); }
  function bindButtons() {
    const buttons = $$('.top-bar .hw-button');
    const byText = text => buttons.find(b => b.textContent.trim().toLowerCase() === text.toLowerCase());
    byText('Save')?.addEventListener('click', () => { state.bank[state.activeBank] = snapshot(); setStatus(`Saved ${state.activeBank} state`); });
    byText('Bypass')?.addEventListener('click', e => { state.bypass = !state.bypass; e.currentTarget.classList.toggle('active', state.bypass); setStatus(state.bypass ? 'Bypass enabled' : 'Processing enabled'); });
    byText('Copy')?.addEventListener('click', () => { state.clipboard = snapshot(); setStatus('Copied current vocal chain'); });
    byText('Paste')?.addEventListener('click', () => { if (!state.clipboard) return setStatus('Clipboard is empty'); Object.entries(state.clipboard).forEach(([k, v]) => updateKnob(k, v, false)); setStatus('Pasted vocal chain'); });
    $$('.preset-group .hw-button').slice(0, 4).forEach((button, i) => button.addEventListener('click', () => { const bank = 'ABCD'[i]; state.activeBank = bank; $$('.preset-group .hw-button').slice(0, 4).forEach(b => b.classList.remove('active')); button.classList.add('active'); if (Object.keys(state.bank[bank]).length) Object.entries(state.bank[bank]).forEach(([k, v]) => updateKnob(k, v, false)); setStatus(`Active snapshot ${bank}`); }));
    const generate = $('.btn-ai, [data-action="generate-ai"], #generate-ai'); generate?.addEventListener('click', generatePreset);
    $('.dropdown-display')?.addEventListener('click', () => { const first = $('.preset-item'); if (first) first.scrollIntoView({ block: 'nearest' }); });
  }

  function animateMeters() { const fills = $$('.meter-fill'); if (!fills.length) return; if (state.bypass) return requestAnimationFrame(animateMeters); const t = performance.now() / 1000; const level = 36 + Math.sin(t * 2.7) * 18 + Math.sin(t * 7.1) * 7; fills.forEach((fill, i) => { fill.style.height = `${clamp(level + (i ? -4 : 0), 3, 96)}%`; }); requestAnimationFrame(animateMeters); }

  function init() { knobKeys.forEach(bindKnob); bindPresetList(); bindButtons(); applyPreset(0, FACTORY[0]); animateMeters(); document.dispatchEvent(new CustomEvent('afrovocal:ready', { detail: { presetCount: FACTORY.length + TOTAL_GENERATED } })); }
  if (document.readyState === 'loading') document.addEventListener('DOMContentLoaded', init, { once: true }); else init();
  window.AfroVocalRuntime = { state, applyPreset, generatePreset, updateKnob, snapshot };
})();
