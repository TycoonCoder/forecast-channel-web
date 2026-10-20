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
