"use strict";
/* recomp_host.js — loads the recompiled-channel WASM module, feeds it the
 * DOL, and drives execution. The channel's real PPC code runs as native
 * WebAssembly; HLE stubs provide the hardware surface. */

(function(){
  const status = msg => {
    const el = document.getElementById("recomp-status");
    if (el) el.textContent = msg;
    console.log("[recomp]", msg);
  };

  let Module = null;
  let running = false;

  async function boot() {
    status("fetching channel binary…");
    // The channel DOL, served from the repo (user-owned dump, this repo)
    const res = await fetch("assets/channel_0000000d.app");
    if (!res.ok) throw new Error("DOL fetch failed: " + res.status);
    const dol = new Uint8Array(await res.arrayBuffer());
    status(`binary loaded: ${dol.length} bytes; creating WASM runtime…`);

    Module = await createChannelModule({
      locateFile: p => p,
    });
    status("WASM instantiated; loading guest image…");

    const dolPtr = Module._malloc(dol.length);
    Module.HEAPU8.set(dol, dolPtr);
    const rc = Module._load_guest_image(dolPtr, dol.length);
    if (rc !== 0) throw new Error("guest image load failed");

    status("booting channel PPC code (entry 0x8000403C)…");
    Module._fc_boot(dolPtr, dol.length);
    running = true;

    let steps = 0;
    const FRAME_BLOCKS = 20000;
    function frame() {
      if (!running) return;
      // run a slice of recompiled blocks
      const alive = Module._fc_step(FRAME_BLOCKS);
      steps++;
      if (steps % 60 === 0) {
        status(`running — ${(steps*FRAME_BLOCKS/1000).toFixed(0)}k blocks executed`);
      }
      if (alive === 0) {
        status("guest halted (exception or shutdown reached)");
        running = false;
        return;
      }
      requestAnimationFrame(frame);
    }
    requestAnimationFrame(frame);
  }

  window.FC_RECOMP_HOST = { boot, stop: ()=>{ running = false; } };

  document.addEventListener("DOMContentLoaded", () => {
    const btn = document.getElementById("recomp-boot");
    if (btn) btn.addEventListener("click", () => {
      btn.disabled = true;
      boot().catch(e => { status("FAILED: " + e.message); btn.disabled = false; });
    });
  });
})();
