# Forecast Channel — Web

A browser-native recreation of the Wii's Forecast Channel: a spinnable
day/night globe with live weather for 90+ cities, rendered with the
**channel's actual iconography and typeface**, extracted from a real
Forecast Channel WAD.

Unofficial fan project. Not affiliated with or endorsed by Nintendo.

## What comes straight from the channel

Extracted from `Forecast Channel (Wii) (USA) (WiiLink).wad` with custom
format decoders (written for this repo — see `tools/`):

- **Weather icons** (`assets/wicons/W_*.png`) — the channel's twelve forecast
  icons (sun, moon, clouds, fog, rain, sleet, snow, hail, thunder, wind),
  decoded from the banner TPL archives (Wii texture formats RGB5A3 / IA8),
  used throughout the UI
- **Small icon set** (`assets/wicons/icon_W_*.png`) — the channel's compact
  icon variants
- **Typeface** (`fonts/wiintlg.woff2`) — a subset of *Wii NTLG Gothic Regular*,
  the channel's system font, converted from the channel's TTC and served as
  woff2
- **Globe texture tiles** (`assets/earth_*.png`) — all fourteen 1024×1024
  CMPR-compressed earth tiles plus regional zooms (Japan, South Pole, EU, USA)
  decoded from the channel's `earth.brres`. Shipped as reference assets; the
  on-screen globe currently maps NASA Blue Marble imagery because the channel
  tiles are octahedron-face UVs whose exact mesh parametrization (MDL0) is
  still being reconstructed
- Decoders live in `tools/extract_channel_assets.py` (TPL/BRRES/CMPR/RGB5A3/
  IA8, BRRES section walking, TEX0 extraction)

## The actual decompilation (decomp/)

This repo now contains a **real, in-progress decompilation** of the channel's
actual executable, not just a tribute site:

- `decomp/functions.json` — **9,573 recovered functions** with the full call
  graph (call sites per target) and string cross-references, from the 1.5 MB
  PowerPC text section of `contents/0000000d.app` (entry `0x8000403c`)
- `decomp/symbols.json` — 1,574 recovered string literals at fixed addresses
- `decomp/named_symbols.json` — functions identified from PPC idiom
  signatures: `OSDisableInterrupts`/`OSRestoreInterrupts` (751/922 call
  sites), `memmove`, Metrowerks save/restore GPR helpers, and the C++
  virtual-dispatch thunk
- `decomp/translated/functions_stage3.c` — the first hand-decompiled C:
  the entry-point launch sequencer (`channel_main`, which switches on the
  System Menu's launch-mode 2/3/4 and dispatches the app via function
  pointer) plus the identified hot functions
- `decomp/hot_functions.asm` — disassembly of the most-referenced functions
- `tools/decomp_stage*.py` — the full pipeline: DOL parsing, raw-word
  analysis (function starts, BL call graph, lis/addi string xrefs), capstone
  disassembly, and translation
- `tools/decrypt_wad.py` — WAD decryption (title key from ticket, AES-CBC
  per content, SHA1-verified against the TMD)

The binary statically links 19 RVL SDK modules (OS, GX, AX, AI, VI, SI, PAD,
WPAD, KPAD, EXI, DVD, NAND, SC, SO, NWC24, CNT, HBM, NCD, DSP) — full
browser-native execution additionally requires stages 4-8 (see
`decomp/STATE.json`): complete C translation at scale, an HLE runtime
(GX→WebGL, AX→WebAudio, NAND→IndexedDB), static recompilation to WASM, and
WC24 forecast data from the RiiConnect24/WiiLink replacement servers.

## Features

- Spinnable WebGL globe — drag with inertia, wheel/pinch/button zoom, gentle idle drift; automatic Canvas-2D fallback engine for browsers without WebGL; plain city-list fallback if both fail
- Real-time day/night terminator computed from the actual solar position (NOAA declination + equation of time)
- City lights on the night side, atmosphere rim, drifting procedural clouds, RA/Dec-correct starfield
- Live weather for 90+ world cities via Open-Meteo: current conditions, today/tomorrow, 5-day strip, wind, humidity, rain chance, UV index, sunrise/sunset, and a laundry hint (tribute to the JP-only 洗濯 index)
- "My City" geolocation, °C/°F toggle, generative ambient soundtrack toggle

## Run it

Open `index.html` directly, or serve the folder:

```bash
python3 -m http.server 8080
# → http://localhost:8080
```

## Credits & licenses

- Channel assets (icons, font subset, texture tiles): extracted by the
  maintainer from a legally obtained channel dump for personal,
  non-commercial study. All rights to those assets remain with their owners
  (Nintendo / Weathernews).
- Globe base imagery: NASA Blue Marble & Earth Lights (public domain)
- Weather data: [Open-Meteo](https://open-meteo.com/) — free for
  non-commercial use, attribution required
- All code in this repository is original and MIT-licensed (see `LICENSE`)

## Live

https://tycooncoder.github.io/forecast-channel-web/

To push updates: `bash deploy.sh` — commits, pushes, verifies Pages.
Idempotent, safe to re-run.
