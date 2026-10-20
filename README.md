# Forecast Channel — Web

A from-scratch, browser-native recreation of the experience of the Wii's
Forecast Channel: a slowly rotating day/night globe you can grab and spin,
city dots with live forecasts, and a calm generative soundtrack. No emulator,
no console code — this is original HTML/CSS/JS/WebGL that runs as a plain
static site.

## Features

- **Spinnable WebGL globe** — drag to rotate with inertia, wheel/pinch/buttons to zoom, gentle auto-rotation when idle
- **Real-time day/night terminator** computed from the actual solar position (declination + equation of time), so the shading matches the real world right now
- **City lights on the night side**, atmospheric rim glow, drifting procedural clouds, starfield
- **Live weather** for 90+ world cities via the free Open-Meteo API: current conditions, today/tomorrow detail, 5-day strip, wind, humidity, precipitation odds, UV index, sunrise/sunset, and a laundry hint (a small tribute to the JP-only 洗濯 index)
- **"My City"** — geolocates you and forecasts the nearest city (or your exact spot)
- °C/°F toggle, generative ambient music toggle
- Everything is client-side: host it anywhere as static files

## Run it

Open `index.html` directly, or serve the folder:

```bash
python3 -m http.server 8080
# → http://localhost:8080
```

## Credits & data licenses

- Map boundaries: [Natural Earth](https://www.naturalearthdata.com/) via
  [world-atlas](https://github.com/topojson/world-atlas) TopoJSON — public domain
- Star positions: derived from the HYG star database as packaged by
  [d3-celestial](https://github.com/ofrohn/d3-celestial) — public domain
  (only the ~hundred brightest stars are embedded)
- Weather data: [Open-Meteo](https://open-meteo.com/) — free for
  non-commercial use, attribution required (see the footer link in the UI)
- All code here is original and MIT-licensed (see `LICENSE`)

This project contains **no Nintendo assets** — no channel binaries, fonts,
textures, or audio from the original channel. It is an independent fan
tribute, not affiliated with or endorsed by Nintendo.

## Live

Deployed on GitHub Pages: https://tycooncoder.github.io/forecast-channel-web/

To push updates: edit files, then run `bash deploy.sh` — it commits, pushes,
and re-checks Pages. Idempotent, safe to re-run.
