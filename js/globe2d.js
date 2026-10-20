"use strict";
/* globe2d.js — Canvas-2D fallback globe for browsers that refuse WebGL.
 * Same experience as the WebGL engine: drag-to-spin with inertia, zoom,
 * real-time day/night terminator from the actual solar position, city
 * lights, stars, graticule. On success it installs itself as
 * window.FC_GLOBE so the rest of the app is engine-agnostic. Original code. */

(function(){
  const RAD = Math.PI/180;
  const smooth = (x,a,b)=>{const t=Math.max(0,Math.min(1,(x-a)/(b-a)));return t*t*(3-2*t);};

  function rotY(a){const c=Math.cos(a),s=Math.sin(a);return [c,0,-s, 0,1,0, s,0,c];}
  function rotX(a){const c=Math.cos(a),s=Math.sin(a);return [1,0,0, 0,c,s, 0,-s,c];}
  function mul(a,b){const o=new Array(9);for(let c=0;c<3;c++)for(let r=0;r<3;r++){o[c*3+r]=a[r]*b[c*3]+a[3+r]*b[c*3+1]+a[6+r]*b[c*3+2];}return o;}
  const app = (m,v)=>[m[0]*v[0]+m[3]*v[1]+m[6]*v[2], m[1]*v[0]+m[4]*v[1]+m[7]*v[2], m[2]*v[0]+m[5]*v[1]+m[8]*v[2]];
  function latLonToVec(lat,lon){const la=lat*RAD,lo=lon*RAD,cl=Math.cos(la);return [cl*Math.cos(lo),Math.sin(la),cl*Math.sin(lo)];}

  function sunDirection(date){
    const start = Date.UTC(date.getUTCFullYear(), 0, 1);
    const doy = (date.getTime() - start) / 86400000;
    const g = (2*Math.PI/365.24) * (doy - 1 + (date.getUTCHours()-12)/24);
    const dec = 0.006918 - 0.399912*Math.cos(g) + 0.070257*Math.sin(g)
      - 0.006758*Math.cos(2*g) + 0.000907*Math.sin(2*g)
      - 0.002697*Math.cos(3*g) + 0.00148*Math.sin(3*g);
    const eotMin = 229.18*(0.000075 + 0.001868*Math.cos(g) - 0.032077*Math.sin(g)
      - 0.014615*Math.cos(2*g) - 0.040849*Math.sin(2*g));
    const utcH = date.getUTCHours() + date.getUTCMinutes()/60 + date.getUTCSeconds()/3600;
    const lon = -15*(utcH - 12 + eotMin/60);
    return latLonToVec(dec/RAD, lon);
  }
  function gmstDeg(date){
    const jd = date.getTime()/86400000 + 2440587.5;
    return (280.46061837 + 360.98564736629*(jd - 2451545.0)) % 360;
  }

  const E = {
    _is2d: true,
    yaw: 15*(-(new Date().getTimezoneOffset()/60)) - 90,
    pitch: 18, yawVel: 0, pitchVel: 0,
    dist: 3.1, distTarget: 3.1,
    dragging: false, lastInteract: 0,
    fly: null, onMarkers: null, fps: 0
  };

  let canvas=null, ctx=null, W=0, H=0;
  let starData=null, glow=null;
  const SH=144;
  let shadeCv=null, shadeCtx=null, shadeImg=null;
  let frames=0, fpsT=0;

  function resize(){
    const dpr = Math.min(window.devicePixelRatio||1, 2);
    const w=Math.floor(canvas.clientWidth*dpr), h=Math.floor(canvas.clientHeight*dpr);
    if(canvas.width!==w||canvas.height!==h){ canvas.width=w; canvas.height=h; }
    W=w; H=h;
  }

  const radius = ()=>Math.min(W,H)*0.42*(3.1/E.dist);

  function project(lat, lon, rot){
    const p = app(rot, latLonToVec(lat,lon));
    if(p[2] < 0.015) return null;
    const cx=W/2, cy=H/2, r=radius();
    return [cx + p[0]*r, cy - p[1]*r];
  }

  function drawStars(rotStar){
    const cx=W/2, cy=H/2, r=Math.min(W,H)*0.92;
    ctx.fillStyle="#e6ecff";
    for(const s of starData){
      const v=app(rotStar, [s[0],s[1],s[2]]);
      if(v[2] < 0.15) continue;
      const x=cx+v[0]*r, y=cy-v[1]*r;
      if(x<-4||y<-4||x>W+4||y>H+4) continue;
      const b=1-s[3]/7;
      ctx.globalAlpha = 0.25 + 0.75*b;
      const sz = Math.max(0.7, (2.6 - s[3])*0.6);
      ctx.fillRect(x-sz/2, y-sz/2, sz, sz);
    }
    ctx.globalAlpha=1;
  }

  function drawGlobe(rot){
    const cx=W/2, cy=H/2, r=radius();
    let g=ctx.createRadialGradient(cx-r*0.25, cy-r*0.3, r*0.15, cx, cy, r);
    g.addColorStop(0,"#2a6fc4"); g.addColorStop(0.55,"#1c56a5"); g.addColorStop(1,"#0e3a7d");
    ctx.save();
    ctx.beginPath(); ctx.arc(cx,cy,r,0,Math.PI*2); ctx.clip();
    ctx.fillStyle=g; ctx.fillRect(cx-r, cy-r, r*2, r*2);

    // graticule
    ctx.strokeStyle="rgba(255,255,255,0.07)";
    ctx.lineWidth=Math.max(1, r*0.0016);
    for(let la=-60; la<=60; la+=30){
      ctx.beginPath(); let pen=false;
      for(let lo=-180; lo<=180; lo+=4){
        const p=app(rot, latLonToVec(la,lo));
        if(p[2]<0.01){pen=false;continue;}
        const x=cx+p[0]*r, y=cy-p[1]*r;
        pen?ctx.lineTo(x,y):ctx.moveTo(x,y); pen=true;
      }
      ctx.stroke();
    }
    for(let lo=-180; lo<180; lo+=30){
      ctx.beginPath(); let pen=false;
      for(let la=-85; la<=85; la+=3){
        const p=app(rot, latLonToVec(la,lo));
        if(p[2]<0.01){pen=false;continue;}
        const x=cx+p[0]*r, y=cy-p[1]*r;
        pen?ctx.lineTo(x,y):ctx.moveTo(x,y); pen=true;
      }
      ctx.stroke();
    }

    // land (Natural Earth polygons)
    function hash(s){let h=0;for(let i=0;i<s.length;i++)h=(h*31+s.charCodeAt(i))|0;return ((h>>>8)%13)/13;}
    for(const entry of (window.FC_WORLD||[])){
      const name=entry[0], polys=entry[1];
      const ice=/antartica/i.test(name)||/Greenland/i.test(name);
      const l=hash(name)*10-5;
      ctx.fillStyle = ice ? "#cfe0ea" : "hsl("+(126+l)+", 34%, "+(34+l)+"%)";
      ctx.strokeStyle = ice ? "#9db4c2" : "rgba(14,46,28,0.9)";
      ctx.lineWidth=Math.max(0.8, r*0.0013);
      for(const rings of polys){
        ctx.beginPath();
        for(const ring of rings){
          let pen=false;
          for(const q of ring){
            const p=app(rot, latLonToVec(q[1],q[0]));
            if(p[2]<0.01){ if(pen){ctx.closePath(); pen=false;} continue; }
            const x=cx+p[0]*r, y=cy-p[1]*r;
            pen?ctx.lineTo(x,y):ctx.moveTo(x,y); pen=true;
          }
          if(pen) ctx.closePath();
        }
        ctx.fill(); ctx.stroke();
      }
    }
    ctx.restore();

    // limb glow
    const lg=ctx.createRadialGradient(cx,cy,r*0.96, cx,cy, r*1.14);
    lg.addColorStop(0,"rgba(110,160,255,0)");
    lg.addColorStop(0.86,"rgba(110,160,255,0.02)");
    lg.addColorStop(0.96,"rgba(130,175,255,0.30)");
    lg.addColorStop(1,"rgba(130,175,255,0)");
    ctx.fillStyle=lg;
    ctx.beginPath(); ctx.arc(cx,cy,r*1.14,0,Math.PI*2); ctx.fill();
  }

  function drawNightShade(rot){
    const cx=W/2, cy=H/2, r=radius();
    const sunW = app(rot, sunDirection(new Date()));
    if(!shadeImg) shadeImg = shadeCtx.createImageData(SH,SH);
    const d=shadeImg.data, half=SH/2;
    for(let py=0;py<SH;py++){
      const ny=(half-py)/half;
      for(let px=0;px<SH;px++){
        const nx=(px-half)/half;
        const r2=nx*nx+ny*ny;
        const i=(py*SH+px)*4;
        if(r2>1){ d[i+3]=0; continue; }
        const nz=Math.sqrt(1-r2);
        const dot=nx*sunW[0] + ny*sunW[1] + nz*sunW[2];
        const dayness=smooth(dot,-0.16,0.22);
        d[i]=3; d[i+1]=8; d[i+2]=26;
        d[i+3]=Math.round((1-dayness)*205);
      }
    }
    shadeCtx.putImageData(shadeImg,0,0);
    ctx.save();
    ctx.beginPath(); ctx.arc(cx,cy,r,0,Math.PI*2); ctx.clip();
    ctx.imageSmoothingEnabled=true;
    ctx.drawImage(shadeCv, cx-r, cy-r, r*2, r*2);
    ctx.restore();
    return sunW;
  }

  function drawCityLights(rot, sunW){
    const cx=W/2, cy=H/2, r=radius();
    for(const c of (window.FC_CITIES||[])){
      const p=app(rot, latLonToVec(c[2],c[3]));
      if(p[2] < 0.04) continue;
      if(p[0]*sunW[0]+p[1]*sunW[1]+p[2]*sunW[2] > -0.05) continue; // day side
      const x=cx+p[0]*r, y=cy-p[1]*r;
      const s=Math.max(5, r*0.028);
      ctx.drawImage(glow, x-s/2, y-s/2, s, s);
    }
  }

  function frame(t){
    requestAnimationFrame(frame);
    const dt = Math.min(0.05, (t - (frame.last||t))/1000);
    frame.last = t;
    frames++; if(t - fpsT > 1000){ E.fps = frames; frames = 0; fpsT = t; }

    if(E.fly){
      const k = Math.min(1, (t - E.fly.t0)/E.fly.dur);
      const e = 1 - Math.pow(1-k, 3);
      E.yaw = E.fly.y0 + (E.fly.y1 - E.fly.y0)*e;
      E.pitch = E.fly.p0 + (E.fly.p1 - E.fly.p0)*e;
      if(k >= 1) E.fly = null;
    } else if(!E.dragging){
      E.yaw += E.yawVel*dt; E.pitch += E.pitchVel*dt;
      E.yawVel *= Math.pow(0.12, dt); E.pitchVel *= Math.pow(0.12, dt);
      if(t - E.lastInteract > 8000 && Math.abs(E.yawVel) < 0.02)
        E.yaw += 0.012*dt;
    }
    E.pitch = Math.max(-78, Math.min(78, E.pitch));
    E.dist += (E.distTarget - E.dist)*Math.min(1, dt*7);

    const rot = mul(rotX(E.pitch*RAD), rotY(E.yaw*RAD));
    const rotStar = rotY(gmstDeg(new Date())*RAD);

    resize();
    ctx.clearRect(0,0,W,H);
    drawStars(rotStar);
    drawGlobe(rot);
    const sunW = drawNightShade(rot);
    drawCityLights(rot, sunW);

    if(E.onMarkers && !E.dragging && t - (frame.mT||0) > 66){
      frame.mT = t;
      E.onMarkers(rot);
    }
  }

  function bindInput(){
    const pointers = new Map();
    let pinchD = 0;
    canvas.addEventListener("pointerdown", e=>{
      canvas.setPointerCapture(e.pointerId);
      pointers.set(e.pointerId, [e.clientX, e.clientY]);
      if(pointers.size===1){
        E.dragging = true; E.fly = null; E.lastInteract = performance.now();
        canvas.classList.add("dragging");
      } else if(pointers.size===2){
        const p=[...pointers.values()];
        pinchD = Math.hypot(p[0][0]-p[1][0], p[0][1]-p[1][1]);
      }
    });
    canvas.addEventListener("pointermove", e=>{
      if(!pointers.has(e.pointerId)) return;
      const prev = pointers.get(e.pointerId);
      pointers.set(e.pointerId, [e.clientX, e.clientY]);
      E.lastInteract = performance.now();
      if(pointers.size===1 && E.dragging){
        const dx=e.clientX-prev[0], dy=e.clientY-prev[1];
        const s = 0.0045 * (E.dist/3.1);
        E.yaw += dx*s; E.pitch += dy*s;
        E.yawVel = Math.max(-3, Math.min(3, dx*s*60));
        E.pitchVel = Math.max(-3, Math.min(3, dy*s*60));
      } else if(pointers.size===2){
        const p=[...pointers.values()];
        const d=Math.hypot(p[0][0]-p[1][0], p[0][1]-p[1][1]);
        if(pinchD>0) E.distTarget = Math.max(1.6, Math.min(6.5, E.distTarget*(pinchD/d)));
        pinchD = d;
      }
    });
    const up = e=>{
      pointers.delete(e.pointerId);
      if(pointers.size===0){ E.dragging=false; canvas.classList.remove("dragging"); }
    };
    canvas.addEventListener("pointerup", up);
    canvas.addEventListener("pointercancel", up);
    canvas.addEventListener("wheel", e=>{
      e.preventDefault();
      E.lastInteract = performance.now();
      E.distTarget = Math.max(1.6, Math.min(6.5, E.distTarget*(1+Math.sign(e.deltaY)*0.12)));
    }, {passive:false});
  }

  E.init = function(canvasEl){
    canvas = canvasEl;
    ctx = canvas.getContext("2d");
    if(!ctx) throw new Error("2D context refused");

    shadeCv = document.createElement("canvas");
    shadeCv.width = SH; shadeCv.height = SH;
    shadeCtx = shadeCv.getContext("2d");

    glow = document.createElement("canvas");
    glow.width = glow.height = 48;
    const gx = glow.getContext("2d");
    const gg = gx.createRadialGradient(24,24,0, 24,24,24);
    gg.addColorStop(0,"rgba(255,224,158,0.95)");
    gg.addColorStop(0.35,"rgba(255,182,92,0.5)");
    gg.addColorStop(1,"rgba(255,182,92,0)");
    gx.fillStyle = gg; gx.fillRect(0,0,48,48);

    starData = (window.FC_STARS||[]).map(s=>{
      const r=s[0]*RAD, d=s[1]*RAD;
      return [Math.cos(d)*Math.cos(r), Math.sin(d), Math.cos(d)*Math.sin(r), s[2]];
    });
    let seed=42; const rnd=()=>((seed=(seed*1103515245+12345)&0x7fffffff)/0x7fffffff);
    for(let i=0;i<380;i++){
      const u=rnd()*2-1, th=rnd()*Math.PI*2, rr=Math.sqrt(1-u*u);
      starData.push([rr*Math.cos(th), u, rr*Math.sin(th), 2.6+rnd()*3]);
    }

    bindInput();
    window.addEventListener("resize", resize);
    resize();
    window.FC_GLOBE = E;  // become the engine for the rest of the app
    requestAnimationFrame(frame);
  };

  E.flyTo = function(lat, lon){
    let target = lon - 90;
    const cur = E.yaw;
    target += 360*Math.round((cur-target)/360);
    E.fly = {
      t0: performance.now(), dur: 1100,
      y0: cur, y1: target,
      p0: E.pitch, p1: Math.max(-70, Math.min(70, lat))
    };
    E.yawVel = 0; E.pitchVel = 0;
    E.lastInteract = performance.now();
  };

  E.zoom = function(delta){
    E.distTarget = Math.max(1.6, Math.min(6.5, E.distTarget*(1+delta*0.18)));
    E.lastInteract = performance.now();
  };

  E.project = project;
  window.FC_GLOBE2D = E;
})();
