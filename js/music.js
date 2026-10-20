"use strict";
/* music.js — generative ambient soundtrack (WebAudio, no samples).
 * Soft pads + sparse pentatonic plucks; brighter scale by day, moodier by night.
 * A distant, legally-unassailable cousin of the channel's BGM. */

(function(){
  let ctx = null, master = null, padBus = null, delay = null, playing = false;
  let chordTimer = null, melodyTimer = null;

  const DAY = [0, 2, 4, 7, 9], NIGHT = [0, 3, 5, 7, 10]; // pentatonic-ish, semitones from root
  const ROOT = 220; // A3
  const CHORD_PROG = [0, -5, -3, -7]; // descending drift, semitone offsets

  function ensureCtx(){
    if (ctx) return;
    ctx = new (window.AudioContext || window.webkitAudioContext)();
    master = ctx.createGain();
    master.gain.value = 0.0;
    master.connect(ctx.destination);

    const lp = ctx.createBiquadFilter();
    lp.type = "lowpass"; lp.frequency.value = 2600; lp.Q.value = 0.4;
    lp.connect(master);
    padBus = lp;

    delay = ctx.createDelay(2.0);
    delay.delayTime.value = 0.42;
    const fb = ctx.createGain(); fb.gain.value = 0.34;
    const wet = ctx.createGain(); wet.gain.value = 0.28;
    delay.connect(fb); fb.connect(delay);
    delay.connect(wet); wet.connect(master);
  }

  const noteHz = (semi) => ROOT * Math.pow(2, semi / 12);

  function pluck(semi, when, vol){
    const o = ctx.createOscillator();
    o.type = "triangle";
    o.frequency.value = noteHz(semi);
    const g = ctx.createGain();
    g.gain.setValueAtTime(0, when);
    g.gain.linearRampToValueAtTime(vol, when + 0.015);
    g.gain.exponentialRampToValueAtTime(0.0001, when + 2.1);
    o.connect(g); g.connect(delay); g.connect(master);
    o.start(when); o.stop(when + 2.3);
  }

  function padChord(semis, when){
    const dur = 10.5;
    semis.forEach((semi, i) => {
      [0, 4].forEach((detuneCents, k) => {
        const o = ctx.createOscillator();
        o.type = k === 0 ? "sine" : "triangle";
        o.frequency.value = noteHz(semi);
        o.detune.value = detuneCents + (i === 0 ? 0 : 2);
        const g = ctx.createGain();
        g.gain.setValueAtTime(0, when);
        g.gain.linearRampToValueAtTime(0.055 / (i + 1.2), when + 4.2);
        g.gain.setValueAtTime(0.055 / (i + 1.2), when + dur - 4.5);
        g.gain.linearRampToValueAtTime(0, when + dur);
        o.connect(g); g.connect(padBus);
        o.start(when); o.stop(when + dur + 0.1);
      });
    });
  }

  function isDaylight(){
    const h = new Date().getHours();
    return h >= 7 && h < 20;
  }

  let chordStep = 0;
  function scheduleChord(){
    if (!playing) return;
    const now = ctx.currentTime + 0.05;
    const scale = isDaylight() ? DAY : NIGHT;
    const base = CHORD_PROG[chordStep % CHORD_PROG.length];
    chordStep++;
    padChord([base, base + scale[2], base + scale[4], base + 12], now);
    chordTimer = setTimeout(scheduleChord, 9000);
  }

  function scheduleMelody(){
    if (!playing) return;
    const scale = isDaylight() ? DAY : NIGHT;
    const oct = Math.random() < 0.3 ? 12 : 0;
    if (Math.random() < 0.82){
      const semi = scale[Math.floor(Math.random() * scale.length)] + oct
        + CHORD_PROG[(chordStep + 3) % CHORD_PROG.length];
      pluck(semi, ctx.currentTime + 0.05, 0.16 + Math.random() * 0.06);
    }
    melodyTimer = setTimeout(scheduleMelody, 1300 + Math.random() * 2100);
  }

  function toggle(){
    ensureCtx();
    if (!playing){
      ctx.resume();
      playing = true;
      master.gain.cancelScheduledValues(ctx.currentTime);
      master.gain.setTargetAtTime(0.9, ctx.currentTime, 1.2);
      scheduleChord();
      scheduleMelody();
    } else {
      playing = false;
      clearTimeout(chordTimer); clearTimeout(melodyTimer);
      master.gain.setTargetAtTime(0.0, ctx.currentTime, 0.5);
    }
    return playing;
  }

  window.FC_MUSIC = { toggle };
})();
