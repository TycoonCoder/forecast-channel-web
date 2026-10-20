"use strict";
/* main.js — UI orchestration: markers, forecast panel, units, clock,
 * geolocation, music button. Original code. */

(function(){
  // capture runtime errors for diagnostics
  window.FC_ERRORS = [];
  window.addEventListener("error", e=>{
    window.FC_ERRORS.push(String(e.message||e));
  });

  const $ = id => document.getElementById(id);
  const cities = window.FC_CITIES.map(c=>({name:c[0],country:c[1],lat:c[2],lon:c[3]}));
  const markerEls = [];
  let unit = "F";
  try { unit = localStorage.getItem("fc_unit") || "F"; } catch(e){}
  let selected = -1, selectedDay = 0, weatherData = null;

  /* ---------- toast ---------- */
  let toastTimer = null;
  function toast(msg){
    const t = $("toast");
    t.textContent = msg; t.hidden = false;
    clearTimeout(toastTimer);
    toastTimer = setTimeout(()=>{ t.hidden = true; }, 2600);
  }

  /* ---------- clock ---------- */
  function tickClock(){
    const now = new Date();
    $("clock-time").textContent =
      String(now.getHours()).padStart(2,"0") + ":" + String(now.getMinutes()).padStart(2,"0");
    $("clock-date").textContent = now.toLocaleDateString(undefined,
      { weekday:"short", month:"short", day:"numeric" });
  }
  tickClock(); setInterval(tickClock, 10000);

  /* ---------- markers ---------- */
  const markersDiv = $("markers");
  function makeMarker(i){
    const m = document.createElement("div");
    m.className = "marker"; m.dataset.i = i;
    const label = document.createElement("span");
    label.className = "m-label";
    label.textContent = cities[i].name;
    m.appendChild(label);
    m.addEventListener("click", ev=>{
      ev.stopPropagation();
      selectCity(i, true);
    });
    markerEls.push(m);
    markersDiv.appendChild(m);
    return m;
  }
  cities.forEach((c,i)=>makeMarker(i));

  function updateMarkers(){
    const rot = window.FC_GLOBE._lastRot || null;
    if(!rot) return;
    for(let i=0;i<cities.length;i++){
      const p = window.FC_GLOBE.project(cities[i].lat, cities[i].lon, rot);
      const el = markerEls[i];
      if(!p){ el.style.display = "none"; continue; }
      el.style.display = "";
      el.style.left = p[0].toFixed(1) + "px";
      el.style.top = p[1].toFixed(1) + "px";
    }
  }
  // FC_GLOBE calls onMarkers with the current rotation every render throttle
  function hookGlobe(){
    const G = window.FC_GLOBE;
    G.onMarkers = rot=>{
      G._lastRot = rot;
      updateMarkers();
    };
  }

  /* ---------- weather panel ---------- */
  const cToF = c => c*9/5+32;
  function fmtT(c){
    if(c===null||c===undefined) return "–";
    const v = unit==="F" ? cToF(c) : c;
    return Math.round(v) + "°";
  }
  function fmtSpeed(k){
    return unit==="F" ? Math.round(k*0.6214)+" mph" : Math.round(k)+" km/h";
  }
  function cityTime(offsetSec){
    // UTC time shifted by the location's UTC offset
    const u = new Date(Date.now() + offsetSec*1000);
    return String(u.getUTCHours()).padStart(2,"0") + ":" + String(u.getUTCMinutes()).padStart(2,"0");
  }
  function hhmm(iso){
    return iso ? iso.slice(11,16) : "–";
  }

  async function selectCity(i, fly){
    selected = i; selectedDay = 0;
    const c = cities[i];
    markerEls.forEach((m,j)=>m.classList.toggle("selected", j===i));
    $("panel").classList.remove("hidden");
    $("city-name").textContent = c.name;
    $("city-country").textContent = c.country;
    $("cond-text").textContent = "Loading…";
    $("temp").textContent = "–";
    $("daily-detail").innerHTML = "";
    $("strip").innerHTML = "";
    $("details").innerHTML = "";
    if(fly) window.FC_GLOBE.flyTo(c.lat, c.lon);
    try{
      weatherData = await window.FC_WEATHER.get(c.lat, c.lon);
      if(selected !== i) return; // user moved on
      render();
    }catch(err){
      window.FC_ERRORS.push("weather: "+err);
      $("cond-text").textContent = "Weather unavailable";
      toast("Could not reach the weather service.");
    }
  }

  function render(){
    const c = cities[selected];
    const w = weatherData;
    if(!w) return;
    const cur = w.current;
    $("city-local-time").textContent = "Local time " + cityTime(w.utcOffset)
      + (w.timezone ? " · " + w.timezone : "");
    $("temp").textContent = fmtT(cur.temperature_2m);
    $("cond-icon").innerHTML = window.FC_WEATHER.icon(cur.weather_code, cur.is_day === 1);
    $("cond-text").textContent = window.FC_WEATHER.desc(cur.weather_code, cur.is_day === 1);
    $("feels").textContent = "Feels like " + fmtT(cur.apparent_temperature);

    // tabs
    document.querySelectorAll("#tabs button").forEach(b=>{
      b.classList.toggle("active", +b.dataset.day === Math.min(selectedDay,1));
    });

    // strip (5 days)
    const strip = $("strip"); strip.innerHTML = "";
    const d = w.daily;
    for(let k=0;k<5;k++){
      const cell = document.createElement("div");
      cell.className = "day-cell" + (k===selectedDay ? " active" : "");
      const date = new Date(d.time[k] + "T12:00:00");
      cell.innerHTML =
        `<div class="dc-name">${k===0?"Today":date.toLocaleDateString(undefined,{weekday:"short"})}</div>` +
        `<div class="dc-icon">${window.FC_WEATHER.icon(d.weather_code[k], true)}</div>` +
        `<div class="dc-hi">${fmtT(d.temperature_2m_max[k])}</div>` +
        `<div class="dc-lo">${fmtT(d.temperature_2m_min[k])}</div>`;
      cell.addEventListener("click", ()=>{ selectedDay = k; render(); });
      strip.appendChild(cell);
    }

    // detail for selectedDay
    const k = selectedDay;
    const dateStr = k===0 ? "Today" :
      k===1 ? "Tomorrow" :
      new Date(d.time[k]+"T12:00:00").toLocaleDateString(undefined,{weekday:"long"});
    const laun = window.FC_WEATHER.laundry(d, k);
    $("daily-detail").innerHTML =
      `<div class="dd-icon">${window.FC_WEATHER.icon(d.weather_code[k], true)}</div>` +
      `<div class="dd-main"><div class="dd-desc">${dateStr} — ` +
      window.FC_WEATHER.desc(d.weather_code[k], true) +
      (laun ? `<br>Laundry: ${laun.label}` : "") +
      `</div><div class="dd-range">High ${fmtT(d.temperature_2m_max[k])} · Low ${fmtT(d.temperature_2m_min[k])}</div></div>`;

    // details grid
    const rows = [
      ["Wind", fmtSpeed(cur.wind_speed_10m)],
      ["Humidity", cur.relative_humidity_2m + "%"],
      ["Rain chance", (d.precipitation_probability_max[k] ?? 0) + "%"],
      ["UV index", Math.round(d.uv_index_max[k] ?? 0)],
      ["Sunrise", hhmm(d.sunrise[k])],
      ["Sunset", hhmm(d.sunset[k])]
    ];
    $("details").innerHTML = rows.map(r=>
      `<div class="det"><div class="k">${r[0]}</div><div class="v">${r[1]}</div></div>`).join("");
  }

  document.querySelectorAll("#tabs button").forEach(b=>{
    b.addEventListener("click", ()=>{ selectedDay = +b.dataset.day; render(); });
  });
  $("panel-close").addEventListener("click", ()=>{
    $("panel").classList.add("hidden");
    markerEls.forEach(m=>m.classList.remove("selected"));
    selected = -1;
  });

  /* ---------- controls ---------- */
  $("units").textContent = "°" + unit;
  $("units").addEventListener("click", ()=>{
    unit = unit==="F" ? "C" : "F";
    try { localStorage.setItem("fc_unit", unit); } catch(e){}
    $("units").textContent = "°" + unit;
    render();
  });
  $("music").addEventListener("click", ()=>{
    const on = window.FC_MUSIC.toggle();
    $("music").textContent = on ? "Music On" : "Music Off";
  });
  $("zoom-in").addEventListener("click", ()=>window.FC_GLOBE.zoom(-1));
  $("zoom-out").addEventListener("click", ()=>window.FC_GLOBE.zoom(1));

  /* ---------- My City ---------- */
  $("locate").addEventListener("click", ()=>{
    if(!navigator.geolocation){ toast("Geolocation not available."); return; }
    toast("Finding you…");
    navigator.geolocation.getCurrentPosition(pos=>{
      const {latitude:lat, longitude:lon} = pos.coords;
      let best = -1, bestD = 1e9;
      cities.forEach((c,i)=>{
        const d = Math.hypot(c.lat-lat, c.lon-lon);
        if(d < bestD){ bestD = d; best = i; }
      });
      if(best >= 0 && bestD < 3){ selectCity(best, true); return; }
      // exact location as a new marker
      cities.push({name:"My Location", country:"", lat:lat, lon:lon});
      makeMarker(cities.length-1);
      toast("Marked your location.");
      selectCity(cities.length-1, true);
    }, ()=>{
      toast("Could not get your location.");
    }, {timeout: 12000});
  });

  /* ---------- keyboard ---------- */
  window.addEventListener("keydown", e=>{
    if(e.key==="+"||e.key==="=") window.FC_GLOBE.zoom(-1);
    if(e.key==="-") window.FC_GLOBE.zoom(1);
    if(e.key==="Escape") $("panel-close").click();
  });

  /* ---------- boot ---------- */
  function boot(){
    const canvas = $("scene");
    try{
      window.FC_GLOBE.init(canvas);
      hookGlobe();
      updateMarkers();
    }catch(err){
      window.FC_ERRORS.push("globe: "+err);
      $("nogl").hidden = false;
    }
  }
  if(document.readyState === "loading")
    document.addEventListener("DOMContentLoaded", boot);
  else boot();
})();
