"use strict";
/* recomp_host.js — loads the recompiled-channel WASM module, feeds it the
 * DOL, and drives execution. The channel's real PPC code runs as native
 * WebAssembly; HLE stubs provide the hardware surface. */

(async () => {
  const status = msg => {
    const el = document.getElementById("recomp-status");
    if (el) el.textContent = msg;
    console.log("[recomp]", msg);
  };

  let Module = null;
  let running = false;
  let lastTime = performance.now();

  // WebGL setup for HLE rendering
  let gl = null;
  let canvas = null;
  let drawGlobeFrame = null;
  let drawBootFrame = null;

  async function initGL() {
    if (gl) return gl;
    canvas = document.getElementById("recomp-canvas");
    if (!canvas) {
      canvas = document.createElement("canvas");
      canvas.id = "recomp-canvas";
      canvas.width = 800;
      canvas.height = 600;
      canvas.style.display = "block";
      canvas.style.marginTop = "16px";
      document.body.appendChild(canvas);
    }
    gl = canvas.getContext("webgl2", { antialias: true, preserveDrawingBuffer: true });
    if (!gl) {
      gl = canvas.getContext("webgl", { antialias: true, preserveDrawingBuffer: true });
    }
    if (!gl) throw new Error("WebGL not supported");
    
    // Load shaders for globe
    const vsSrc = `attribute vec3 a_pos;
    attribute vec2 a_uv;
    attribute vec4 a_color;
    uniform mat4 u_mvp;
    uniform vec3 u_sun;
    uniform float u_time;
    varying vec2 v_uv;
    varying vec4 v_color;
    varying vec3 v_pos;
    varying vec3 v_normal;
    void main() {
      gl_Position = u_mvp * vec4(a_pos, 1.0);
      v_uv = a_uv;
      v_color = a_color;
      v_pos = a_pos;
      v_normal = a_pos;
    }`;
    
    const fsSrc = `precision mediump float;
    varying vec2 v_uv;
    varying vec4 v_color;
    varying vec3 v_normal;
    uniform sampler2D u_tex;
    uniform vec3 u_sun;
    uniform float u_time;
    void main() {
      vec4 tex = texture2D(u_tex, v_uv);
      float NdotL = max(dot(normalize(v_normal), normalize(u_sun)), 0.0);
      float day = step(0.0, dot(normalize(v_normal), normalize(u_sun)));
      vec3 day_color = v_color.rgb * tex.rgb * (0.3 + 0.7 * NdotL);
      vec3 night_color = v_color.rgb * tex.rgb * 0.2 + vec3(0.1, 0.15, 0.3) * (1.0 - day);
      vec3 color = mix(night_color, day_color, day);
      gl_FragColor = vec4(color * v_color.a, v_color.a);
    }`;
    
    function compileShader(type, src) {
      const s = gl.createShader(type);
      gl.shaderSource(s, src);
      gl.compileShader(s);
      if (!gl.getShaderParameter(s, gl.COMPILE_STATUS)) {
        console.error("Shader error:", gl.getShaderInfoLog(s));
        console.error("Shader source:", src);
        gl.deleteShader(s);
        return null;
      }
      return s;
    }
    
    const vs = compileShader(gl.VERTEX_SHADER, vsSrc);
    const fs = compileShader(gl.FRAGMENT_SHADER, fsSrc);
    if (!vs || !fs) {
      throw new Error("Shader compilation failed - check console for details");
    }
    const prog = gl.createProgram();
    gl.attachShader(prog, vs);
    gl.attachShader(prog, fs);
    gl.linkProgram(prog);
    if (!gl.getProgramParameter(prog, gl.LINK_STATUS)) {
      console.error("Link error:", gl.getProgramInfoLog(prog));
      return null;
    }
    
    const u_mvp = gl.getUniformLocation(prog, "u_mvp");
    const u_tex = gl.getUniformLocation(prog, "u_tex");
    const u_color = gl.getUniformLocation(prog, "u_color");
    const u_sun = gl.getUniformLocation(prog, "u_sun");
    const u_time = gl.getUniformLocation(prog, "u_time");
    
    const vao = gl.createVertexArray();
    const vbo = gl.createBuffer();
    gl.bindVertexArray(vao);
    gl.bindBuffer(gl.ARRAY_BUFFER, vbo);
    const a_pos = gl.getAttribLocation(prog, "a_pos");
    const a_uv = gl.getAttribLocation(prog, "a_uv");
    const a_color = gl.getAttribLocation(prog, "a_color");
    gl.vertexAttribPointer(a_pos, 3, gl.FLOAT, false, 32, 0);
    gl.enableVertexAttribArray(a_pos);
    gl.vertexAttribPointer(a_uv, 2, gl.FLOAT, false, 32, 12);
    gl.enableVertexAttribArray(a_uv);
    gl.vertexAttribPointer(a_color, 4, gl.FLOAT, false, 32, 20);
    gl.enableVertexAttribArray(a_color);
    
    // White texture fallback
    const texWhite = gl.createTexture();
    gl.bindTexture(gl.TEXTURE_2D, texWhite);
    gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, 1, 1, 0, gl.RGBA, gl.UNSIGNED_BYTE, new Uint8Array([255,255,255,255]));
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR);
    
    // Load earth textures
    const texEarthDay = gl.createTexture();
    const texEarthNight = gl.createTexture();
    
    function loadTexture(gl, tex, url) {
      return new Promise((resolve, reject) => {
        const img = new Image();
        img.crossOrigin = "anonymous";
        img.onload = () => {
          gl.bindTexture(gl.TEXTURE_2D, tex);
          gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, gl.RGBA, gl.UNSIGNED_BYTE, img);
          gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR);
          gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR);
          resolve();
        };
        img.onerror = reject;
        img.src = url;
      });
    }
    
    await Promise.all([
      loadTexture(gl, texEarthDay, "assets/earth_day.jpg"),
      loadTexture(gl, texEarthNight, "assets/earth_night.jpg")
    ]).catch(e => console.warn("Texture load failed:", e));
    
    // Globe mesh generation
    function generateSphere(radius, segments) {
      const vertices = [];
      for (let lat = 0; lat <= segments; lat++) {
        const theta = lat * Math.PI / segments;
        const sinTheta = Math.sin(theta);
        const cosTheta = Math.cos(theta);
        for (let lon = 0; lon <= segments; lon++) {
          const phi = lon * 2 * Math.PI / segments;
          const sinPhi = Math.sin(phi);
          const cosPhi = Math.cos(phi);
          const x = cosPhi * sinTheta;
          const y = cosTheta;
          const z = sinPhi * sinTheta;
          const u = 1 - lon / segments;
          const v = 1 - lat / segments;
          vertices.push(radius * x, radius * y, radius * z, u, v, 1, 1, 1, 1);
        }
      }
      const indices = [];
      for (let lat = 0; lat < segments; lat++) {
        for (let lon = 0; lon < segments; lon++) {
          const first = lat * (segments + 1) + lon;
          const second = first + segments + 1;
          indices.push(first, second, first + 1);
          indices.push(second, second + 1, first + 1);
        }
      }
      return { vertices: new Float32Array(vertices), indices: new Uint16Array(indices) };
    }
    
    const sphere = generateSphere(1.0, 64);
    const indexBuffer = gl.createBuffer();
    gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, indexBuffer);
    gl.bufferData(gl.ELEMENT_ARRAY_BUFFER, sphere.indices, gl.STATIC_DRAW);
    gl.bufferData(gl.ARRAY_BUFFER, sphere.vertices, gl.STATIC_DRAW);
    const indexCount = sphere.indices.length;
    
    let globeAngle = 0;
    let sunDir = [1, 0.3, 0.5];
    let time = 0;
    let bootState = 0;
    let bootTimer = 0;
    let frameCount = 0;
    
    function mat4Identity(out) {
      out[0]=1; out[1]=0; out[2]=0; out[3]=0;
      out[4]=0; out[5]=1; out[6]=0; out[7]=0;
      out[8]=0; out[9]=0; out[10]=1; out[11]=0;
      out[12]=0; out[13]=0; out[14]=0; out[15]=1;
    }
    
    function mat4Mul(out, a, b) {
      for(let i=0;i<4;i++)for(let j=0;j<4;j++){
        let s=0; for(let k=0;k<4;k++) s+=a[i*4+k]*b[k*4+j];
        out[i*4+j]=s;
      }
    }
    
    function mat4Perspective(out, fovy, aspect, near, far) {
      const f = 1.0 / Math.tan(fovy * 0.5);
      out[0]=f/aspect; out[1]=0; out[2]=0; out[3]=0;
      out[4]=0; out[5]=f; out[6]=0; out[7]=0;
      out[8]=0; out[9]=0; out[10]=(far+near)/(near-far); out[11]=-1;
      out[12]=0; out[13]=0; out[14]=2*far*near/(near-far); out[15]=0;
    }
    
    function mat4LookAt(out, eye, center, up) {
      const f = [center[0]-eye[0], center[1]-eye[1], center[2]-eye[2]];
      const fl = Math.hypot(...f); f[0]/=fl; f[1]/=fl; f[2]/=fl;
      const s = [f[1]*up[2]-f[2]*up[1], f[2]*up[0]-f[0]*up[2], f[0]*up[1]-f[1]*up[0]];
      const sl = Math.hypot(...s); s[0]/=sl; s[1]/=sl; s[2]/=sl;
      const u = [f[1]*s[2]-f[2]*s[1], f[2]*s[0]-f[0]*s[2], f[0]*s[1]-f[1]*s[0]];
      const rot = [s[0],u[0],-f[0],0, s[1],u[1],-f[1],0, s[2],u[2],-f[2],0, 0,0,0,1];
      const trans = [1,0,0,-eye[0], 0,1,0,-eye[1], 0,0,1,-eye[2], 0,0,0,1];
      mat4Mul(out, rot, trans);
    }
    
    function drawGlobe(mvp, sun, t) {
      time = t;
      sunDir = sun;
      globeAngle += 0.0003;
      
      const proj = new Float32Array(16);
      const view = new Float32Array(16);
      const model = new Float32Array(16);
      
      mat4Perspective(proj, Math.PI/4, canvas.width/canvas.height, 0.1, 100);
      mat4LookAt(view, [0,0,3.5], [0,0,0], [0,1,0]);
      
      mat4Identity(model);
      const c = Math.cos(globeAngle), s = Math.sin(globeAngle);
      const ry = [c,0,-s,0, 0,1,0,0, s,0,c,0, 0,0,0,1];
      mat4Mul(model, ry, model);
      
      mat4Mul(mvp, proj, view);
      mat4Mul(mvp, mvp, model);
      
      gl.useProgram(prog);
      gl.bindVertexArray(vao);
      gl.uniformMatrix4fv(u_mvp, false, mvp);
      gl.uniform3fv(u_sun, sunDir);
      gl.uniform1f(u_time, time);
      gl.uniform1i(u_tex, 0);
      gl.uniform4f(u_color, 1, 1, 1, 1);
      
      gl.activeTexture(gl.TEXTURE0);
      gl.bindTexture(gl.TEXTURE_2D, texEarthDay);
      
      gl.enable(gl.DEPTH_TEST);
      gl.enable(gl.BLEND);
      gl.blendFunc(gl.SRC_ALPHA, gl.ONE_MINUS_SRC_ALPHA);
      gl.drawElements(gl.TRIANGLES, indexCount, gl.UNSIGNED_SHORT, 0);
    }
    
    function drawBoot(state, timer) {
      bootState = state;
      bootTimer = timer;
      
      gl.clearColor(0.02, 0.03, 0.08, 1);
      gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);
      
      if (state === 0) {
        // Nintendo logo
        gl.useProgram(prog);
        gl.bindVertexArray(vao);
        const mvp = new Float32Array(16);
        mat4Identity(mvp);
        gl.uniformMatrix4fv(u_mvp, false, mvp);
        gl.uniform4f(u_color, 1, 1, 1, 1);
        gl.bindTexture(gl.TEXTURE_2D, texWhite);
        // Draw simple quad for logo placeholder
      } else if (state === 1) {
        // "Checking the forecast..."
        // Just clear for now, JS overlay handles text
      } else if (state === 2) {
        // Loading globe
      }
    }
    
    drawGlobeFrame = drawGlobe;
    drawBootFrame = drawBoot;
    
    return gl;
  }

  async function boot() {
    status("fetching channel binary…");
    const res = await fetch("../assets/channel_0000000d.app");
    if (!res.ok) throw new Error("DOL fetch failed: " + res.status);
    const dol = new Uint8Array(await res.arrayBuffer());
    status(`binary loaded: ${dol.length} bytes; creating WASM runtime…`);

    await initGL();

    Module = await createChannelModule({
      locateFile: p => p,
      canvas: canvas,
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
      function frame(now) {
        // setTimeout doesn't pass timestamp; use performance.now() as fallback
        now = now || performance.now();
        console.log('[frame] callback at', now);
        if (!running) return;
        const dt = (now - lastTime) / 1000;
        lastTime = now;
      
      console.log('[frame] calling fc_step', FRAME_BLOCKS);
      // run a slice of recompiled blocks
      let alive = 0;
      try {
        alive = Module._fc_step(FRAME_BLOCKS);
      } catch(e) {
        console.error('[frame] fc_step threw:', e && e.message ? e.message : String(e));
        running = false;
        return;
      }
      steps++;
      if (steps % 60 === 0) {
        status(`running — ${(steps*FRAME_BLOCKS/1000).toFixed(0)}k blocks executed`);
      }
      if (alive === 0) {
        status("guest halted (exception or shutdown reached)");
        running = false;
        return;
      }
      
      // Call HLE render_frame if available
      if (Module._render_frame) {
        Module._render_frame(dt);
      }
      
      // Re-schedule with visibility awareness (RAF is throttled in hidden tabs)
      scheduleFrame();
    }
    function scheduleFrame() {
      if (document.hidden) {
        setTimeout(frame, 16);
      } else {
        requestAnimationFrame(frame);
      }
    }
    console.log('[frame] starting animation loop');
    scheduleFrame();
  }

  // Wait for DOM and Module factory
  document.addEventListener("DOMContentLoaded", () => {
    const btn = document.getElementById("recomp-boot");
    if (btn) btn.addEventListener("click", () => {
      btn.disabled = true;
      boot().catch(e => { status("FAILED: " + e.message); btn.disabled = false; });
    });
  });
})();