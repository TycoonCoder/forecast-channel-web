"use strict";
/* globe.js — original WebGL globe: day/night terminator from the real solar
 * position (NOAA Fourier series for declination + equation of time),
 * stylized vector earth built from Natural Earth polygons, procedural
 * clouds, atmosphere rim, and an RA/Dec-correct star sphere. No libraries. */

(function(){
  const RAD = Math.PI / 180;

  /* ---------- tiny mat3 (column-major) ---------- */
  function rotY(a){ const c=Math.cos(a), s=Math.sin(a); return [c,0,-s, 0,1,0, s,0,c]; }
  function rotX(a){ const c=Math.cos(a), s=Math.sin(a); return [1,0,0, 0,c,s, 0,-s,c]; }
  function mul(a,b){
    const o=new Array(9);
    for(let c=0;c<3;c++)for(let r=0;r<3;r++){
      o[c*3+r]=a[r]*b[c*3]+a[3+r]*b[c*3+1]+a[6+r]*b[c*3+2];
    }
    return o;
  }

  function latLonToVec(lat, lon){
    const la=lat*RAD, lo=lon*RAD, cl=Math.cos(la);
    return [cl*Math.cos(lo), Math.sin(la), cl*Math.sin(lo)];
  }

  /* ---------- solar position (NOAA approximations) ---------- */
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

  /* ---------- GL helpers ---------- */
  function makeProgram(gl, vs, fs){
    function sh(type, src){
      const s = gl.createShader(type);
      gl.shaderSource(s, src); gl.compileShader(s);
      if(!gl.getShaderParameter(s, gl.COMPILE_STATUS))
        throw new Error(gl.getShaderInfoLog(s));
      return s;
    }
    const p = gl.createProgram();
    gl.attachShader(p, sh(gl.VERTEX_SHADER, vs));
    gl.attachShader(p, sh(gl.FRAGMENT_SHADER, fs));
    gl.linkProgram(p);
    if(!gl.getProgramParameter(p, gl.LINK_STATUS))
      throw new Error(gl.getProgramInfoLog(p));
    return p;
  }

  /* ---------- sphere geometry ---------- */
  function makeSphere(gl, rows, cols){
    const pos=[], uv=[], idx=[];
    for(let r=0;r<=rows;r++){
      const v=r/rows, lat=90 - v*180;
      for(let c=0;c<=cols;c++){
        const u=c/cols, lon=u*360-180;
        const p=latLonToVec(lat,lon);
        pos.push(p[0],p[1],p[2]); uv.push(u,v);
      }
    }
    for(let r=0;r<rows;r++)for(let c=0;c<cols;c++){
      const a=r*(cols+1)+c, b=a+1, d=a+cols+1, e=d+1;
      idx.push(a,d,b, b,d,e);
    }
    const vp=gl.createBuffer(); gl.bindBuffer(gl.ARRAY_BUFFER,vp);
    gl.bufferData(gl.ARRAY_BUFFER,new Float32Array(pos),gl.STATIC_DRAW);
    const vt=gl.createBuffer(); gl.bindBuffer(gl.ARRAY_BUFFER,vt);
    gl.bufferData(gl.ARRAY_BUFFER,new Float32Array(uv),gl.STATIC_DRAW);
    const ib=gl.createBuffer(); gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER,ib);
    gl.bufferData(gl.ELEMENT_ARRAY_BUFFER,new Uint16Array(idx),gl.STATIC_DRAW);
    return {vp,vt,ib,count:idx.length};
  }

  /* ---------- textures from 2D canvas ---------- */
  function drawWorldTexture(){
    const W=2048,H=1024, c=document.createElement("canvas");
    c.width=W; c.height=H; const x=c.getContext("2d");
    const grad=x.createLinearGradient(0,0,0,H);
    grad.addColorStop(0,"#123f7d"); grad.addColorStop(.5,"#17529b"); grad.addColorStop(1,"#123f7d");
    x.fillStyle=grad; x.fillRect(0,0,W,H);
    // graticule
    x.strokeStyle="rgba(255,255,255,0.055)"; x.lineWidth=1;
    for(let lo=-180;lo<=180;lo+=15){ x.beginPath(); x.moveTo((lo+180)/360*W,0); x.lineTo((lo+180)/360*W,H); x.stroke(); }
    for(let la=-75;la<=75;la+=15){ x.beginPath(); x.moveTo(0,(90-la)/180*H); x.lineTo(W,(90-la)/180*H); x.stroke(); }
    // land
    function hash(s){ let h=0; for(let i=0;i<s.length;i++) h=(h*31+s.charCodeAt(i))|0; return ((h>>>8)%13)/13; }
    for(const [name,polys] of window.FC_WORLD){
      for(const rings of polys){
        x.beginPath();
        for(const ring of rings){
          ring.forEach((p,i)=>{
            const px=(p[0]+180)/360*W, py=(90-p[1])/180*H;
            i===0?x.moveTo(px,py):x.lineTo(px,py);
          });
          x.closePath();
        }
        const ice = /antartica/i.test(name)||/Greenland/i.test(name);
        const l = hash(name)*10-5;
        x.fillStyle = ice ? "#cfe0ea" : `hsl(${126+l} 34% ${34+l}%)`;
        x.fill("evenodd");
        x.strokeStyle = ice ? "#9db4c2" : "rgba(16,52,34,0.85)";
        x.lineWidth = ice ? 2 : 1.6; x.stroke();
      }
    }
    return c;
  }

  function drawNightTexture(){
    const W=2048,H=1024, c=document.createElement("canvas");
    c.width=W; c.height=H; const x=c.getContext("2d");
    x.fillStyle="#01040c"; x.fillRect(0,0,W,H);
    // faint continents for context
    x.strokeStyle="rgba(60,90,130,0.35)"; x.lineWidth=1.2;
    for(const [,polys] of window.FC_WORLD)
      for(const rings of polys){
        x.beginPath();
        for(const ring of rings){
          ring.forEach((p,i)=>{
            const px=(p[0]+180)/360*W, py=(90-p[1])/180*H;
            i===0?x.moveTo(px,py):x.lineTo(px,py);
          });
          x.closePath();
        }
        x.stroke();
      }
    // city lights + halo clusters
    function dot(px,py,r,a,col){
      const g=x.createRadialGradient(px,py,0,px,py,r);
      g.addColorStop(0,col.replace("$",a)); g.addColorStop(1,col.replace("$",0));
      x.fillStyle=g; x.beginPath(); x.arc(px,py,r,0,Math.PI*2); x.fill();
    }
    let seed=20260704;
    const rnd=()=>((seed=(seed*1103515245+12345)&0x7fffffff)/0x7fffffff);
    for(const [,,lat,lon] of window.FC_CITIES){
      const px=(lon+180)/360*W, py=(90-lat)/180*H;
      dot(px,py,16,0.9,"rgba(255,196,110,$)");
      dot(px,py,5,1,"rgba(255,236,190,$)");
      const n=4+Math.floor(rnd()*5);
      for(let i=0;i<n;i++){
        const a=rnd()*Math.PI*2, d=(2+rnd()*9)*(1+rnd()*3);
        dot(px+Math.cos(a)*d, py+Math.sin(a)*d*0.6, 3+rnd()*5, 0.5, "rgba(255,190,100,$)");
      }
    }
    return c;
  }

  function uploadTex(gl, canvas){
    const t=gl.createTexture();
    gl.bindTexture(gl.TEXTURE_2D,t);
    gl.texImage2D(gl.TEXTURE_2D,0,gl.RGBA,gl.RGBA,gl.UNSIGNED_BYTE,canvas);
    gl.generateMipmap(gl.TEXTURE_2D);
    gl.texParameteri(gl.TEXTURE_2D,gl.TEXTURE_MIN_FILTER,gl.LINEAR_MIPMAP_LINEAR);
    gl.texParameteri(gl.TEXTURE_2D,gl.TEXTURE_MAG_FILTER,gl.LINEAR);
    gl.texParameteri(gl.TEXTURE_2D,gl.TEXTURE_WRAP_S,gl.CLAMP_TO_EDGE);
    gl.texParameteri(gl.TEXTURE_2D,gl.TEXTURE_WRAP_T,gl.CLAMP_TO_EDGE);
    const ext=gl.getExtension("EXT_texture_filter_anisotropic");
    if(ext) gl.texParameterf(gl.TEXTURE_2D,ext.TEXTURE_MAX_ANISOTROPY_EXT,
      Math.min(8,gl.getParameter(ext.MAX_TEXTURE_MAX_ANISOTROPY_EXT)));
    return t;
  }

  /* ---------- shaders ---------- */
  const VS_EARTH = `
    attribute vec3 aPos; attribute vec2 aUv;
    uniform mat3 uRot; uniform mat4 uVP;
    varying vec3 vN; varying vec2 vUv; varying vec3 vW;
    void main(){
      vec3 w = uRot * aPos;
      vN = w; vUv = aUv; vW = w;
      gl_Position = uVP * vec4(w, 1.0);
    }`;
  const FS_EARTH = `
    precision mediump float;
    varying vec3 vN; varying vec2 vUv; varying vec3 vW;
    uniform sampler2D uDay, uNight;
    uniform vec3 uSun; uniform vec3 uCam;
    void main(){
      vec3 n = normalize(vN);
      float d = dot(n, uSun);
      float dayness = smoothstep(-0.16, 0.22, d);
      vec3 day = texture2D(uDay, vUv).rgb;
      vec3 night = texture2D(uNight, vUv).rgb;
      vec3 lit = day * (0.22 + 0.9 * max(d, 0.0));
      vec3 col = mix(night * 1.15, lit, dayness);
      col += vec3(0.85, 0.38, 0.14) * exp(-pow(d * 5.5, 2.0)) * 0.16;
      vec3 view = normalize(uCam - vW);
      float fres = pow(1.0 - max(dot(view, n), 0.0), 3.0);
      col += vec3(0.25, 0.5, 0.9) * fres * (0.12 + 0.5 * dayness);
      gl_FragColor = vec4(col, 1.0);
    }`;
  const VS_CLOUD = `
    attribute vec3 aPos; attribute vec2 aUv;
    uniform mat3 uRot; uniform mat4 uVP; uniform float uScale;
    varying vec3 vN; varying vec2 vUv;
    void main(){
      vec3 w = uRot * (aPos * uScale); vN = w; vUv = aUv;
      gl_Position = uVP * vec4(w, 1.0);
    }`;
  const FS_CLOUD = `
    precision mediump float;
    varying vec3 vN; varying vec2 vUv;
    uniform vec3 uSun; uniform float uTime;
    float hash(vec2 p){ return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453); }
    float vnoise(vec2 p){
      vec2 i = floor(p), f = fract(p);
      vec2 u2 = f*f*(3.0-2.0*f);
      return mix(mix(hash(i), hash(i+vec2(1,0)), u2.x),
                 mix(hash(i+vec2(0,1)), hash(i+vec2(1,1)), u2.x), u2.y);
    }
    void main(){
      vec3 n = normalize(vN);
      float d = dot(n, uSun);
      float dayness = smoothstep(-0.15, 0.25, d);
      vec2 p = vUv * 7.0 + vec2(uTime * 0.006, uTime * 0.002);
      float f = 0.0, a = 0.5;
      for(int i=0;i<4;i++){
        f += a * vnoise(p); p = p * 2.05 + vec2(3.1, 1.7); a *= 0.5;
      }
      float cl = smoothstep(0.52, 0.78, f);
      vec3 col = mix(vec3(0.07, 0.09, 0.15), vec3(0.98) * (0.55 + 0.45 * max(d, 0.0)), dayness);
      float alpha = cl * (0.16 + 0.6 * dayness);
      gl_FragColor = vec4(col, alpha);
    }`;
  const VS_HALO = `
    attribute vec3 aPos;
    uniform mat3 uRot; uniform mat4 uVP; uniform float uScale;
    varying vec3 vN; varying vec3 vW;
    void main(){
      vec3 w = uRot * (aPos * uScale); vN = w; vW = w;
      gl_Position = uVP * vec4(w, 1.0);
    }`;
  const FS_HALO = `
    precision mediump float;
    varying vec3 vN; varying vec3 vW;
    uniform vec3 uSun; uniform vec3 uCam;
    void main(){
      vec3 n = normalize(vN);
      vec3 view = normalize(uCam - vW);
      float fres = pow(1.0 - max(dot(view, n), 0.0), 4.0);
      float d = dot(n, uSun);
      float dayness = smoothstep(-0.3, 0.35, d);
      vec3 col = mix(vec3(0.06, 0.1, 0.22), vec3(0.3, 0.55, 1.0), dayness);
      gl_FragColor = vec4(col * fres * 1.5, fres * 0.75);
    }`;
  const VS_STAR = `
    attribute vec3 aPos; attribute float aMag;
    uniform mat3 uRot; uniform mat4 uVP;
    varying float vB;
    void main(){
      vB = 1.0 - aMag / 7.0;
      gl_Position = uVP * vec4(aPos * 50.0, 1.0);
      gl_PointSize = max(1.5, (4.2 - aMag) * 1.15);
    }`;
  const FS_STAR = `
    precision mediump float; varying float vB;
    void main(){
      vec2 c = gl_PointCoord - 0.5;
      float a = smoothstep(0.5, 0.05, length(c));
      gl_FragColor = vec4(vec3(0.9, 0.93, 1.0) * vB, a * vB);
    }`;

  /* ---------- main object ---------- */
  const G = {
    yaw: 15*(-(new Date().getTimezoneOffset()/60)) - 90, // face the user's longitude
    pitch: 18, yawVel: 0, pitchVel: 0,
    dist: 3.1, distTarget: 3.1,
    dragging: false, lastInteract: 0,
    fly: null, onMarkers: null, fps: 0
  };
  let gl, progEarth, progCloud, progHalo, progStar;
  let sph, bufStarP, bufStarM, starCount = 0;
  let texDay, texNight;
  let canvas, camPos = [0,0,3.1], vp = new Float32Array(16);
  let frames = 0, fpsT = 0;

  function perspective(out, fovy, aspect, near, far){
    const f = 1/Math.tan(fovy/2), nf = 1/(near-far);
    out[0]=f/aspect; out[5]=f; out[10]=(far+near)*nf; out[11]=-1;
    out[14]=2*far*near*nf; out[15]=0;
  }

  function resize(){
    const dpr = Math.min(window.devicePixelRatio||1, 2);
    const w = Math.floor(canvas.clientWidth*dpr), h = Math.floor(canvas.clientHeight*dpr);
    if(canvas.width!==w||canvas.height!==h){ canvas.width=w; canvas.height=h; }
    gl.viewport(0,0,w,h);
    perspective(vp, 40*RAD, w/h, 0.1, 120);
  }

  function bindSphere(prog, geo, rot){
    gl.useProgram(prog);
    const vp_ = gl.getUniformLocation(prog,"uVP");
    const rotL = gl.getUniformLocation(prog,"uRot");
    gl.uniformMatrix4fv(vp_, false, vp);
    gl.uniformMatrix3fv(rotL, false, rot);
    gl.bindBuffer(gl.ARRAY_BUFFER, geo.vp);
    let loc = gl.getAttribLocation(prog,"aPos");
    gl.enableVertexAttribArray(loc);
    gl.vertexAttribPointer(loc,3,gl.FLOAT,false,0,0);
    const hasUv = gl.getAttribLocation(prog,"aUv");
    if(hasUv>=0){
      gl.bindBuffer(gl.ARRAY_BUFFER, geo.vt);
      gl.enableVertexAttribArray(hasUv);
      gl.vertexAttribPointer(hasUv,2,gl.FLOAT,false,0,0);
    }
    gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, geo.ib);
  }

  function project(lat, lon, rot){
    const p = latLonToVec(lat, lon);
    const w = [
      rot[0]*p[0]+rot[3]*p[1]+rot[6]*p[2],
      rot[1]*p[0]+rot[4]*p[1]+rot[7]*p[2],
      rot[2]*p[0]+rot[5]*p[1]+rot[8]*p[2]
    ];
    if(w[2] < 1/camPos[2] - 0.02) return null; // beyond the visible horizon
    const cz = camPos[2];
    const zc = w[2] - cz;            // view-space z (camera at +z looking at origin)
    const cx = w[0]*vp[0], cy = w[1]*vp[5];
    const czc = zc*vp[10] + vp[14];
    const cw = -zc;                   // clip w
    const ndcX = cx/cw, ndcY = cy/cw, ndcZ = czc/cw;
    if(ndcZ < -1 || ndcZ > 1) return null;
    return [
      (ndcX*0.5+0.5)*canvas.clientWidth,
      (1-(ndcY*0.5+0.5))*canvas.clientHeight
    ];
  }

  function frame(t){
    requestAnimationFrame(frame);
    const dt = Math.min(0.05, (t - (frame.last||t))/1000);
    frame.last = t;
    frames++; if(t - fpsT > 1000){ G.fps = frames; frames = 0; fpsT = t; }

    // motion
    if(G.fly){
      const k = Math.min(1, (t - G.fly.t0)/G.fly.dur);
      const e = 1 - Math.pow(1-k, 3);
      G.yaw = G.fly.y0 + (G.fly.y1 - G.fly.y0)*e;
      G.pitch = G.fly.p0 + (G.fly.p1 - G.fly.p0)*e;
      if(k >= 1) G.fly = null;
    } else {
      if(!G.dragging){
        G.yaw += G.yawVel*dt; G.pitch += G.pitchVel*dt;
        G.yawVel *= Math.pow(0.12, dt); G.pitchVel *= Math.pow(0.12, dt);
        if(t - G.lastInteract > 8000 && Math.abs(G.yawVel) < 0.02)
          G.yaw += 0.012*dt; // gentle idle drift (~0.7°/s)
      }
    }
    G.pitch = Math.max(-78, Math.min(78, G.pitch));
    G.dist += (G.distTarget - G.dist)*Math.min(1, dt*7);
    camPos[2] = G.dist;

    const rot = mul(rotX(G.pitch*RAD), rotY(G.yaw*RAD));
    const time = t/1000;
    // Subsolar point in earth-BODY coords (lat/lon where the sun is overhead).
    // Each drawn object gets the sun transformed into ITS frame so lighting
    // stays glued to the map when the user spins the globe.
    const sunB = sunDirection(new Date());
    const rotStar = rotY(gmstDeg(new Date())*RAD);
    const cloudRot = mul(rot, rotY(time*0.0035));
    const xform = (m,v)=>[
      m[0]*v[0]+m[3]*v[1]+m[6]*v[2],
      m[1]*v[0]+m[4]*v[1]+m[7]*v[2],
      m[2]*v[0]+m[5]*v[1]+m[8]*v[2]
    ];
    const sunW = xform(rot, sunB);      // earth + halo frame
    const sunC = xform(cloudRot, sunB); // cloud frame

    resize();
    gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);

    // stars
    gl.disable(gl.DEPTH_TEST);
    gl.enable(gl.BLEND);
    gl.blendFunc(gl.SRC_ALPHA, gl.ONE);
    gl.useProgram(progStar);
    gl.uniformMatrix4fv(gl.getUniformLocation(progStar,"uVP"), false, vp);
    gl.uniformMatrix3fv(gl.getUniformLocation(progStar,"uRot"), false, rotStar);
    gl.bindBuffer(gl.ARRAY_BUFFER, bufStarP);
    const lp = gl.getAttribLocation(progStar,"aPos");
    gl.enableVertexAttribArray(lp);
    gl.vertexAttribPointer(lp,3,gl.FLOAT,false,0,0);
    gl.bindBuffer(gl.ARRAY_BUFFER, bufStarM);
    const lm = gl.getAttribLocation(progStar,"aMag");
    gl.enableVertexAttribArray(lm);
    gl.vertexAttribPointer(lm,1,gl.FLOAT,false,0,0);
    gl.drawArrays(gl.POINTS, 0, starCount);

    // earth
    gl.enable(gl.DEPTH_TEST);
    gl.blendFunc(gl.ONE, gl.ZERO);
    bindSphere(progEarth, sph, rot);
    gl.activeTexture(gl.TEXTURE0); gl.bindTexture(gl.TEXTURE_2D, texDay);
    gl.uniform1i(gl.getUniformLocation(progEarth,"uDay"), 0);
    gl.activeTexture(gl.TEXTURE1); gl.bindTexture(gl.TEXTURE_2D, texNight);
    gl.uniform1i(gl.getUniformLocation(progEarth,"uNight"), 1);
    gl.uniform3fv(gl.getUniformLocation(progEarth,"uSun"), sunW);
    gl.uniform3fv(gl.getUniformLocation(progEarth,"uCam"), camPos);
    gl.drawElements(gl.TRIANGLES, sph.count, gl.UNSIGNED_SHORT, 0);

    // clouds (own slightly-larger sphere radius to avoid z-fighting)
    gl.enable(gl.BLEND);
    gl.blendFunc(gl.SRC_ALPHA, gl.ONE_MINUS_SRC_ALPHA);
    bindSphere(progCloud, sph, cloudRot);
    gl.uniform1f(gl.getUniformLocation(progCloud,"uScale"), 1.012);
    gl.uniform3fv(gl.getUniformLocation(progCloud,"uSun"), sunC);
    gl.uniform1f(gl.getUniformLocation(progCloud,"uTime"), time);
    gl.drawElements(gl.TRIANGLES, sph.count, gl.UNSIGNED_SHORT, 0);

    // halo
    gl.depthMask(false);
    gl.blendFunc(gl.SRC_ALPHA, gl.ONE);
    bindSphere(progHalo, sph, rot);
    gl.uniform1f(gl.getUniformLocation(progHalo,"uScale"), 1.13);
    gl.uniform3fv(gl.getUniformLocation(progHalo,"uSun"), sunW);
    gl.uniform3fv(gl.getUniformLocation(progHalo,"uCam"), camPos);
    gl.drawElements(gl.TRIANGLES, sph.count, gl.UNSIGNED_SHORT, 0);
    gl.depthMask(true);

    // markers (throttled)
    if(G.onMarkers && !G.dragging && t - (frame.mT||0) > 66){
      frame.mT = t;
      G.onMarkers(rot);
    }
  }

  /* ---------- input ---------- */
  function bindInput(){
    const pointers = new Map();
    let pinchD = 0;
    canvas.addEventListener("pointerdown", e=>{
      canvas.setPointerCapture(e.pointerId);
      pointers.set(e.pointerId, [e.clientX, e.clientY]);
      if(pointers.size===1){
        G.dragging = true; G.fly = null; G.lastInteract = performance.now();
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
      G.lastInteract = performance.now();
      if(pointers.size===1 && G.dragging){
        const dx=e.clientX-prev[0], dy=e.clientY-prev[1];
        const s = 0.0045 * (G.dist/3.1); // rad per pixel, scales with zoom
        G.yaw += dx*s; G.pitch += dy*s;
        G.yawVel = Math.max(-3, Math.min(3, dx*s*60));
        G.pitchVel = Math.max(-3, Math.min(3, dy*s*60));
      } else if(pointers.size===2){
        const p=[...pointers.values()];
        const d=Math.hypot(p[0][0]-p[1][0], p[0][1]-p[1][1]);
        if(pinchD>0) G.distTarget = Math.max(1.6, Math.min(6.5, G.distTarget*(pinchD/d)));
        pinchD = d;
      }
    });
    const up = e=>{
      pointers.delete(e.pointerId);
      if(pointers.size===0){ G.dragging=false; canvas.classList.remove("dragging"); }
    };
    canvas.addEventListener("pointerup", up);
    canvas.addEventListener("pointercancel", up);
    canvas.addEventListener("wheel", e=>{
      e.preventDefault();
      G.lastInteract = performance.now();
      G.distTarget = Math.max(1.6, Math.min(6.5, G.distTarget*(1+Math.sign(e.deltaY)*0.12)));
    }, {passive:false});
  }

  /* ---------- API ---------- */
  G.init = function(canvasEl){
    canvas = canvasEl;
    gl = canvas.getContext("webgl",{alpha:true, antialias:true, preserveDrawingBuffer:true})
      || canvas.getContext("experimental-webgl",{alpha:true, antialias:true, preserveDrawingBuffer:true});
    if(!gl) throw new Error("WebGL unavailable");
    gl.enable(gl.DEPTH_TEST);
    gl.clearColor(0,0,0,0);

    progEarth = makeProgram(gl, VS_EARTH, FS_EARTH);
    progCloud = makeProgram(gl, VS_CLOUD, FS_CLOUD);
    progHalo  = makeProgram(gl, VS_HALO,  FS_HALO);
    progStar  = makeProgram(gl, VS_STAR,  FS_STAR);
    sph = makeSphere(gl, 64, 96);

    texDay = uploadTex(gl, drawWorldTexture());
    texNight = uploadTex(gl, drawNightTexture());

    // stars: catalog bright + procedural faint
    const stars = (window.FC_STARS||[]).map(s=>{
      const [ra, dec, mag] = s;
      const r=ra*RAD, d=dec*RAD;
      return [Math.cos(d)*Math.cos(r), Math.sin(d), Math.cos(d)*Math.sin(r), mag];
    });
    let seed=42; const rnd=()=>((seed=(seed*1103515245+12345)&0x7fffffff)/0x7fffffff);
    for(let i=0;i<380;i++){
      const u=rnd()*2-1, th=rnd()*Math.PI*2, r=Math.sqrt(1-u*u);
      stars.push([r*Math.cos(th), u, r*Math.sin(th), 2.6+rnd()*3]);
    }
    starCount = stars.length;
    bufStarP = gl.createBuffer();
    gl.bindBuffer(gl.ARRAY_BUFFER, bufStarP);
    gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(stars.flatMap(s=>[s[0],s[1],s[2]])), gl.STATIC_DRAW);
    bufStarM = gl.createBuffer();
    gl.bindBuffer(gl.ARRAY_BUFFER, bufStarM);
    gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(stars.map(s=>s[3])), gl.STATIC_DRAW);

    bindInput();
    window.addEventListener("resize", resize);
    resize();
    requestAnimationFrame(frame);
  };

  G.flyTo = function(lat, lon){
    let target = lon - 90;
    const cur = G.yaw;
    target += 360*Math.round((cur-target)/360);
    G.fly = {
      t0: performance.now(), dur: 1100,
      y0: cur, y1: target,
      p0: G.pitch, p1: Math.max(-70, Math.min(70, lat))
    };
    G.yawVel = 0; G.pitchVel = 0;
    G.lastInteract = performance.now();
  };

  G.zoom = function(delta){
    G.distTarget = Math.max(1.6, Math.min(6.5, G.distTarget*(1+delta*0.18)));
    G.lastInteract = performance.now();
  };

  G.project = project;
  window.FC_GLOBE = G;
})();
