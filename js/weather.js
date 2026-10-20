"use strict";
/* weather.js — Open-Meteo client, WMO code mapping, original SVG icons.
 * Fetches in metric and converts client-side for display. */

(function(){
  const cache = new Map();
  const TTL = 10 * 60 * 1000; // 10 minutes

  async function get(lat, lon){
    const key = lat.toFixed(2) + "," + lon.toFixed(2);
    const hit = cache.get(key);
    if (hit && Date.now() - hit.t < TTL) return hit.data;
    const url = "https://api.open-meteo.com/v1/forecast"
      + "?latitude=" + lat + "&longitude=" + lon
      + "&current=temperature_2m,relative_humidity_2m,apparent_temperature,is_day,precipitation,weather_code,wind_speed_10m"
      + "&daily=weather_code,temperature_2m_max,temperature_2m_min,sunrise,sunset,uv_index_max,precipitation_probability_max"
      + "&timezone=auto&forecast_days=6";
    const res = await fetch(url);
    if (!res.ok) throw new Error("Weather service returned " + res.status);
    const data = await res.json();
    const wrapped = {
      t: Date.now(),
      data: {
        current: data.current,
        daily: data.daily,
        utcOffset: data.utc_offset_seconds || 0,
        timezone: data.timezone || ""
      }
    };
    cache.set(key, wrapped);
    return wrapped.data;
  }

  /* ---------- WMO code -> text ---------- */
  function desc(code, isDay){
    if (code === 0) return isDay ? "Sunny" : "Clear";
    if (code === 1) return "Mostly Clear";
    if (code === 2) return "Partly Cloudy";
    if (code === 3) return "Cloudy";
    if (code === 45 || code === 48) return "Foggy";
    if (code >= 51 && code <= 57) return "Drizzle";
    if (code >= 61 && code <= 67) return "Rain";
    if (code >= 71 && code <= 77) return "Snow";
    if (code >= 80 && code <= 82) return "Showers";
    if (code === 85 || code === 86) return "Snow Showers";
    if (code >= 95) return "Thunderstorms";
    return "—";
  }

  /* ---------- original SVG icons ---------- */
  function sun(cx, cy, r, color){
    let rays = "";
    for (let i = 0; i < 8; i++){
      const a = i * Math.PI / 4;
      const x1 = cx + Math.cos(a) * (r + 3), y1 = cy + Math.sin(a) * (r + 3);
      const x2 = cx + Math.cos(a) * (r + 9), y2 = cy + Math.sin(a) * (r + 9);
      rays += `<line x1="${x1.toFixed(1)}" y1="${y1.toFixed(1)}" x2="${x2.toFixed(1)}" y2="${y2.toFixed(1)}" stroke="${color}" stroke-width="3.4" stroke-linecap="round"/>`;
    }
    return rays + `<circle cx="${cx}" cy="${cy}" r="${r}" fill="${color}"/>`;
  }
  function moon(cx, cy, r, color){
    return `<path d="M ${cx + r * 0.45} ${cy - r} a ${r} ${r} 0 1 0 ${r * 0.1} ${r * 2} a ${r * 0.86} ${r * 0.86} 0 1 1 ${-r * 0.1} ${-r * 2} z" fill="${color}"/>`;
  }
  function cloud(cx, cy, s, fill, stroke){
    return `<g transform="translate(${cx} ${cy}) scale(${s})">
      <path d="M -22 8 a 10 10 0 0 1 -1 -19.8 a 13 13 0 0 1 25 -3.4 a 9.5 9.5 0 0 1 14 8.7 a 8 8 0 0 1 -2 14.5 z"
        fill="${fill}" stroke="${stroke}" stroke-width="2"/></g>`;
  }
  function drops(x, y, color, n){
    let d = "";
    for (let i = 0; i < n; i++){
      const dx = x + i * 13;
      d += `<path d="M ${dx} ${y} l 0 9" stroke="${color}" stroke-width="3.6" stroke-linecap="round"/>
            <path d="M ${dx + 7} ${y + 5} l 0 9" stroke="${color}" stroke-width="3.6" stroke-linecap="round"/>`;
    }
    return d;
  }
  function flakes(x, y, color, n){
    let d = "";
    for (let i = 0; i < n; i++){
      const fx = x + i * 14, fy = y + (i % 2) * 5;
      for (let k = 0; k < 3; k++){
        const a = k * Math.PI / 3 + 0.3;
        d += `<line x1="${(fx - 5 * Math.cos(a)).toFixed(1)}" y1="${(fy - 5 * Math.sin(a)).toFixed(1)}" x2="${(fx + 5 * Math.cos(a)).toFixed(1)}" y2="${(fy + 5 * Math.sin(a)).toFixed(1)}" stroke="${color}" stroke-width="2.4" stroke-linecap="round"/>`;
      }
    }
    return d;
  }
  function bolt(x, y, color){
    return `<polygon points="${x},${y} ${x - 8},${y + 14} ${x - 1},${y + 14} ${x - 5},${y + 27} ${x + 9},${y + 11} ${x + 2},${y + 11} ${x + 8},${y}" fill="${color}"/>`;
  }

  function icon(code, isDay){
    const S = 'xmlns="http://www.w3.org/2000/svg" viewBox="0 0 64 64"';
    const sunC = "#f7b733", moonC = "#cfd8e6", clF = "#c9d6ea", clS = "#8fa3c0",
          rainC = "#4a90e2", snowC = "#9fc9ee", boltC = "#f7c744";
    let body = "";
    if (code === 0) body = isDay ? sun(32, 32, 12, sunC) : moon(33, 31, 14, moonC);
    else if (code === 1) body = (isDay ? sun(24, 22, 9, sunC) : moon(25, 21, 10, moonC)) + cloud(36, 40, 1.0, clF, clS);
    else if (code === 2) body = (isDay ? sun(22, 20, 8, sunC) : moon(23, 19, 9, moonC)) + cloud(36, 38, 1.15, clF, clS);
    else if (code === 3) body = cloud(32, 30, 1.5, clF, clS) + cloud(32, 40, 1.2, "#dde7f5", clS);
    else if (code === 45 || code === 48){
      body = cloud(32, 24, 1.15, "#dfe8f4", clS)
        + `<line x1="16" y1="44" x2="48" y2="44" stroke="${clS}" stroke-width="3.2" stroke-linecap="round"/>
           <line x1="20" y1="51" x2="44" y2="51" stroke="${clS}" stroke-width="3.2" stroke-linecap="round"/>`;
    }
    else if (code >= 51 && code <= 57) body = cloud(32, 24, 1.15, clF, clS) + drops(22, 42, rainC, 2);
    else if ((code >= 61 && code <= 67) || (code >= 80 && code <= 82))
      body = cloud(32, 24, 1.2, clF, clS) + drops(19, 42, rainC, 2);
    else if ((code >= 71 && code <= 77) || code === 85 || code === 86)
      body = cloud(32, 24, 1.2, clF, clS) + flakes(19, 43, snowC, 2);
    else if (code >= 95) body = cloud(32, 24, 1.2, "#b9c8de", clS) + bolt(32, 40, boltC);
    else body = cloud(32, 30, 1.3, clF, clS);
    return `<svg ${S}>${body}</svg>`;
  }

  /* ---------- laundry index (a nod to the channel's JP-only 洗濯 mode) ---------- */
  function laundry(day, i){
    if (!day) return null;
    const pop = day.precipitation_probability_max ? day.precipitation_probability_max[i] : 0;
    const wind = 0; // daily wind not fetched; derive from pop + temp spread
    const t = day.temperature_2m_max ? day.temperature_2m_max[i] : null;
    const score = Math.max(0, 100 - pop * 1.15) + (t !== null && t > 18 ? 15 : 0) - (pop > 60 ? 20 : 0);
    let label;
    if (score >= 75) label = "Good drying day";
    else if (score >= 45) label = "Fair drying day";
    else label = "Keep it indoors";
    return { label, score: Math.round(Math.min(100, score)) };
  }

  window.FC_WEATHER = { get, desc, icon, laundry };
})();
