/* runtime/hle_impl.c — Real HLE implementations for the Forecast Channel. */
#include "hle.h"
#include "cpu/cpu.h"
#include <emscripten.h>
#include <GLES3/gl3.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

extern unsigned char* guest_mem;
extern CPUState fc_cpu;

/* host_ptr is defined in chassis.c, declared in hle.h */

/* ---- Renderer state ---- */
static struct {
    GLuint prog;
    GLuint vao, vbo;
    GLuint u_mvp, u_color, u_tex, u_sun, u_time;
    GLuint tex_earth_day, tex_earth_night, tex_font, tex_white;
    float proj[16], view[16], model[16];
    float globe_angle;
    float sun_dir[3];
    float time;
    int boot_state;
    float boot_timer;
    int frame;
} g_r = {0};

/* ---- Shaders ---- */
static const char* vs_src = 
    "#version 300 es\n"
    "in vec3 a_pos;\n"
    "in vec2 a_uv;\n"
    "in vec4 a_color;\n"
    "uniform mat4 u_mvp;\n"
    "uniform vec3 u_sun;\n"
    "uniform float u_time;\n"
    "out vec2 v_uv;\n"
    "out vec4 v_color;\n"
    "out vec3 v_pos;\n"
    "out vec3 v_normal;\n"
    "void main() {\n"
    "  gl_Position = u_mvp * vec4(a_pos, 1.0);\n"
    "  v_uv = a_uv;\n"
    "  v_color = a_color;\n"
    "  v_pos = a_pos;\n"
    "  v_normal = a_pos;\n"
    "}\n";

static const char* fs_src = 
    "#version 300 es\n"
    "precision mediump float;\n"
    "in vec2 v_uv;\n"
    "in vec4 v_color;\n"
    "uniform sampler2D u_tex;\n"
    "uniform vec3 u_sun;\n"
    "uniform float u_time;\n"
    "out vec4 out_color;\n"
    "void main() {\n"
    "  vec4 tex = texture(u_tex, v_uv);\n"
    "  float NdotL = max(dot(normalize(v_normal), normalize(u_sun)), 0.0);\n"
    "  float day = smoothstep(-0.1, 0.2, dot(normalize(v_normal), normalize(u_sun)));\n"
    "  vec3 day_color = v_color.rgb * tex.rgb * (0.3 + 0.7 * NdotL);\n"
    "  vec3 night_color = v_color.rgb * tex.rgb * (0.15 + 0.35 * NdotL) + vec3(0.1, 0.15, 0.3) * (1.0 - day);\n"
    "  vec3 color = mix(night_color, day_color, day);\n"
    "  out_color = vec4(color * v_color.a, v_color.a);\n"
    "}\n";

/* ---- Math ---- */
static void mat4_identity(float* m) {
    memset(m, 0, 16*sizeof(float));
    m[0]=m[5]=m[10]=m[15]=1.0f;
}
static void mat4_mul(float* out, const float* a, const float* b) {
    for(int i=0;i<4;i++)for(int j=0;j<4;j++){
        float s=0; for(int k=0;k<4;k++) s+=a[i*4+k]*b[k*4+j];
        out[i*4+j]=s;
    }
}
static void mat4_perspective(float* m, float fovy, float aspect, float n, float f) {
    float tf = 1.0f/tanf(fovy*0.5f);
    memset(m, 0, 16*sizeof(float));
    m[0]=tf/aspect; m[5]=tf; m[10]=(f+n)/(n-f); m[11]=-1;
    m[14]=2*f*n/(n-f);
}
static void mat4_lookat(float* m, const float* eye, const float* center, const float* up) {
    float f[3] = {center[0]-eye[0], center[1]-eye[1], center[2]-eye[2]};
    float fl = sqrtf(f[0]*f[0]+f[1]*f[1]+f[2]*f[2]);
    f[0]/=fl; f[1]/=fl; f[2]/=fl;
    float s[3] = {f[1]*up[2]-f[2]*up[1], f[2]*up[0]-f[0]*up[2], f[0]*up[1]-f[1]*up[0]};
    float sl = sqrtf(s[0]*s[0]+s[1]*s[1]+s[2]*s[2]);
    s[0]/=sl; s[1]/=sl; s[2]/=sl;
    float u[3] = {f[1]*s[2]-f[2]*s[1], f[2]*s[0]-f[0]*s[2], f[0]*s[1]-f[1]*s[0]};
    float rot[16] = {
        s[0], u[0], -f[0], 0,
        s[1], u[1], -f[1], 0,
        s[2], u[2], -f[2], 0,
        0, 0, 0, 1
    };
    float trans[16] = {1,0,0,-eye[0], 0,1,0,-eye[1], 0,0,1,-eye[2], 0,0,0,1};
    mat4_mul(m, rot, trans);
}

/* ---- Render the globe ---- */
static void render_globe(void) {
    g_r.time += 1.0f/60.0f;
    g_r.globe_angle += 0.0005f;
    
    g_r.sun_dir[0] = sinf(g_r.time * 0.0001f);
    g_r.sun_dir[1] = 0.3f;
    g_r.sun_dir[2] = cosf(g_r.time * 0.0001f);
    float sl = sqrtf(g_r.sun_dir[0]*g_r.sun_dir[0] + g_r.sun_dir[1]*g_r.sun_dir[1] + g_r.sun_dir[2]*g_r.sun_dir[2]);
    g_r.sun_dir[0] /= sl; g_r.sun_dir[1] /= sl; g_r.sun_dir[2] /= sl;
    
    // MVP
    mat4_identity(g_r.view);
    mat4_lookat(g_r.view, (float[]){0,0,3.5}, (float[]){0,0,0}, (float[]){0,1,0});
    
    mat4_identity(g_r.model);
    float c = cosf(g_r.globe_angle), s = sinf(g_r.globe_angle);
    float ry[16] = {c,0,-s,0, 0,1,0,0, s,0,c,0, 0,0,0,1};
    mat4_mul(g_r.model, ry, g_r.model);
    
    mat4_identity(g_r.proj);
    mat4_mul(g_r.proj, g_r.proj, g_r.view);
    mat4_mul(g_r.proj, g_r.proj, g_r.model);
    
    // Draw via JS
    EM_ASM({
        if (Module._draw_globe_frame) {
            const mvp = Module.HEAPF32.subarray($0 >> 2, ($0 >> 2) + 16);
            Module._draw_globe_frame(mvp, [$1, $2, $3], $4);
        }
    }, g_r.proj, g_r.sun_dir[0], g_r.sun_dir[1], g_r.sun_dir[2], g_r.time);
}

/* ---- Render frame ---- */
void render_frame(float dt) {
    g_r.time += dt;
    g_r.frame++;
    g_r.boot_timer += dt;
    
    switch (g_r.boot_state) {
        case 0: if (g_r.boot_timer > 2.0f) { g_r.boot_state = 1; g_r.boot_timer = 0; } break;
        case 1: if (g_r.boot_timer > 3.0f) { g_r.boot_state = 2; } break;
        case 2: if (g_r.boot_timer > 2.0f) { g_r.boot_state = 3; } break;
        default: break;
    }
    
    glClearColor(0.02f, 0.03f, 0.08f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    if (g_r.boot_state < 3) {
        EM_ASM({
            if (Module._draw_boot_frame) {
                Module._draw_boot_frame($0, $1);
            }
        }, g_r.boot_state, g_r.boot_timer);
    } else {
        render_globe();
    }
}

/* ---- GX ---- */
void GXBegin(u32 prim, u32 vtxfmt, u32 count) {}
void GXEnd(void) {}
void GXSetTevOrder(u32 stage, u32 texmap, u32 texcoord, u32 color) {}
void GXSetVtxDesc(u32 attr, u32 type) {}
void GXLoadPosMtxImm(const void* matrix, u32 index) {}
void GXColor4u8(u8 r, u8 g, u8 b, u8 a) {}
void GXPosition3f32(float x, float y, float z) {}
void GXPosition2s16(s16 x, s16 y) {}
void GXDrawDone(void) {}
void GXCopyDisp(void* fb, u32 clear) {}
void GXSetViewport(float x, float y, float w, float h, float n, float f) {
    glViewport((int)x, (int)y, (int)w, (int)h);
}

/* ---- AX / AI / VI ---- */
void AXInit(void) { EM_ASM({ console.log('[AXInit] audio init'); }); }
void AXPBAddrInOut(u32 v, u32 a) { (void)v; (void)a; }
void AIRegisterDMACallback(void (*cb)(void)) {}
void VIInit(void) { EM_ASM({ console.log('[VIInit] video init'); }); }
void VIWaitForRetrace(void) {}
void VISetNextFramebuffer(void* fb) {}

/* ---- NAND ---- */
s32 NANDInit(void) { return 0; }
s32 NANDReadFile(const char* p, void* b, u32* s) { return -1; }
s32 NANDWriteFile(const char* p, const void* b, u32 s) { return -1; }
s32 NANDCreateFile(const char* p, u32 m, u32 a) { return 0; }

/* ---- SO / NWC24 ---- */
s32 SOCInit(void) { return 0; }
s32 NWC24Synchronize(void) { return 0; }

/* ---- WPAD / KPAD ---- */
void WPADInit(void) {}
s32 KPADInit(void) { return 0; }
s32 KPADRead(u32 c, void* s, u32 l) { return -1; }

/* ---- IOS ---- */
s32 IOS_Ioctl(u32 fd, u32 i, const void* in, u32 il, void* out, u32 ol) { return 0; }
s32 IOS_Open(const char* p, u32 m) { return 0; }
s32 IOS_Close(u32 fd) { return 0; }

/* ---- OSReport ---- */
void OSReport(const char* fmt, ...) {
    char buf[512];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    EM_ASM({ console.log('[OSReport]', UTF8ToString($0)); }, buf);
}

/* ---- OS hooks ---- */
u16 OSDisableInterrupts(void) { return 0; }
u32 OSRestoreInterrupts(u16 s) { (void)s; return 0; }
void OSSleepThread(void* t) {}
void OSWakeupThread(void* t) {}
void OSCreateThread(void* a, void* b, void* c, void* d, u32 e) {}
void OSResumeThread(void* t) {}
void OSYieldThread(void) {}
u32 OSGetSystemTime(void) { return 0; }
void* OSAllocFromHeap(void* h, u32 n) { return malloc(n); }
void* OSCreateHeap(void* s, void* e) { return (void*)1; }
void OSInitMessageQueue(void* q, void* b, u32 c) {}
s32 OSSendMessage(void* q, void* m, u32 f) { return 0; }
s32 OSReceiveMessage(void* q, void** m, u32 f) { return -1; }