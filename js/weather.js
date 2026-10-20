"use strict";
/* weather.js — Open-Meteo client + the Forecast Channel's REAL icons,
 * extracted from the channel's TPL archives (assets/wicons/). */

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
    if (code === 56 || code === 57 || code === 66 || code === 67) return "Freezing Rain";
    if (code >= 51 && code <= 57) return "Drizzle";
    if (code >= 61 && code <= 67) return "Rain";
    if (code >= 71 && code <= 77) return "Snow";
    if (code >= 80 && code <= 82) return "Showers";
    if (code === 85 || code === 86) return "Snow Showers";
    if (code >= 95) return "Thunderstorms";
    return "—";
  }

  /* ---------- the channel's actual icons ---------- */
  const ICON_DIR = "assets/wicons/";
  function icon(code, isDay){
    let file;
    if (code === 0)        file = isDay ? "W_sun_00" : "W_moon_00";
    else if (code === 1)   file = isDay ? "W_sun_00" : "W_cloud_02";
    else if (code === 2)   file = "W_cloud_01";
    else if (code === 3)   file = "W_cloud_00";
    else if (code === 45 || code === 48) file = "W_fog_00";
    else if (code === 56 || code === 57 || code === 66 || code === 67) file = "W_sleet_all";
    else if ((code >= 51 && code <= 55) || (code >= 61 && code <= 65) || (code >= 80 && code <= 82)) file = "W_rain_00";
    else if ((code >= 71 && code <= 77) || code === 85 || code === 86) file = "W_snow_all";
    else if (code >= 95)   file = "W_thunder_00";
    else if (code === 77)  file = "W_hail_all";
    else                   file = "W_cloud_00";
    return '<img class="wicon" src="' + ICON_DIR + file + '.png" alt="' + desc(code, isDay) + '" loading="lazy">';
  }

  /* ---------- laundry index (a nod to the channel's JP-only 洗濯 mode) ---------- */
  function laundry(day, i){
    if (!day) return null;
    const pop = day.precipitation_probability_max ? day.precipitation_probability_max[i] : 0;
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
