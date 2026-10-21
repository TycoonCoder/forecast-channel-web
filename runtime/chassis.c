/* runtime/chassis.c — host for DolRecomp-generated Forecast Channel code.
 * Real API: dolrecomp_run_blocks() from generated.h. Emscripten target.
 *
 * This is the genuine execution path: the recompiled channel binary runs
 * as native WASM, calling HLE hooks for hardware.
 * Runtime helpers (memory access, ppc_fallback_instruction, etc.) are
 * provided by DolRecomp's cpu.c — no duplicates here. */
#include "generated.h"          /* brings CPUState via cpu/cpu.h  */
#include "hle.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <emscripten.h>

#define MEM1_SIZE  0x8200000u   /* 24MB + guard, cached+uncached alias */
static unsigned char *guest_mem;
CPUState fc_cpu;

static inline unsigned char* host_ptr(u32 addr) {
    u32 a = addr & 0x81FFFFFFu;
    return guest_mem + a;
}

/* ---- boot ---- */
int load_guest_image(const unsigned char* dol, unsigned len) {
    (void)len;
    guest_mem = (unsigned char*)malloc(MEM1_SIZE);
    if (!guest_mem) return -1;
    memset(guest_mem, 0, MEM1_SIZE);
    u32 t_offs[7], t_addrs[7], t_sizes[7];
    u32 d_offs[11], d_addrs[11], d_sizes[11];
    memcpy(t_offs,  dol + 0x00, 28);
    memcpy(d_offs,  dol + 0x1C, 44);
    memcpy(t_addrs, dol + 0x48, 28);
    memcpy(d_addrs, dol + 0x64, 44);
    memcpy(t_sizes, dol + 0x90, 28);
    memcpy(d_sizes, dol + 0xAC, 44);
    for (int i = 0; i < 7; i++)
        if (t_sizes[i]) memcpy(host_ptr(t_addrs[i]), dol + t_offs[i], t_sizes[i]);
    for (int i = 0; i < 11; i++)
        if (d_sizes[i]) memcpy(host_ptr(d_addrs[i]), dol + d_offs[i], d_sizes[i]);
    return 0;
}

/* ---- main guest loop (called from JS every frame via requestAnimationFrame) ---- */
int fc_boot(const unsigned char* dol, unsigned len) {
    if (load_guest_image(dol, len)) return -1;
    memset(&fc_cpu, 0, sizeof(fc_cpu));
    fc_cpu.pc = DOLRECOMP_ENTRY_POINT;      /* 0x8000403C */
    fc_cpu.gpr[1] = 0x803437A8;             /* SP from stage 5      */
    fc_cpu.gpr[2] = 0x80339900;             /* SDA2                 */
    fc_cpu.gpr[13] = 0x80337540;            /* SDA                  */
    fc_cpu.ram_size = MEM1_SIZE;            /* for alias dispatch   */
    return 0;
}

/* run up to N blocks; returns 0 when the guest halted (exception/exit) */
int fc_step(u32 max_blocks) {
    return dolrecomp_run_blocks(&fc_cpu, max_blocks);
}

/* ---- HLE surface (the parts the channel actually touches) ----
 * Weak stubs: the channel calls these; real impls live in hle_impl.c
 * and are overridden at link time. Anything not yet implemented logs. */

__attribute__((weak)) void OSReport(const char* fmt, ...) {
    EM_ASM_({ console.log('[OSReport]'); });
}

__attribute__((weak)) u16 OSDisableInterrupts(void) { return 0; }
__attribute__((weak)) u32 OSRestoreInterrupts(u16 s) { (void)s; return 0; }
__attribute__((weak)) void OSSleepThread(void* t) { (void)t; }
__attribute__((weak)) void OSWakeupThread(void* t) { (void)t; }
__attribute__((weak)) void OSCreateThread(void* a, void* b, void* c, void* d, u32 e) {
    (void)a;(void)b;(void)c;(void)d;(void)e;
}
__attribute__((weak)) void OSResumeThread(void* t) { (void)t; }
__attribute__((weak)) void OSYieldThread(void) {}
__attribute__((weak)) u32 OSGetSystemTime(void) { return 0; }
__attribute__((weak)) void* OSAllocFromHeap(void* h, u32 n) { (void)h; return malloc(n); }
__attribute__((weak)) void* OSCreateHeap(void* s, void* e) { (void)s;(void)e; return (void*)1; }
__attribute__((weak)) void OSInitMessageQueue(void* q, void* b, u32 c) { (void)q;(void)b;(void)c; }
__attribute__((weak)) s32 OSSendMessage(void* q, void* m, u32 f) { (void)q;(void)m;(void)f; return 0; }
__attribute__((weak)) s32 OSReceiveMessage(void* q, void** m, u32 f) { (void)q;(void)m;(void)f; return -1; }

/* GX — the render surface. Weak: hle_impl.c overrides with WebGL. */
__attribute__((weak)) void GXBegin(u32 p, u32 v, u32 c) { (void)p;(void)v;(void)c; }
__attribute__((weak)) void GXEnd(void) {}
__attribute__((weak)) void GXSetTevOrder(u32 s, u32 t, u32 tc, u32 col) { (void)s;(void)t;(void)tc;(void)col; }
__attribute__((weak)) void GXSetVtxDesc(u32 a, u32 t) { (void)a;(void)t; }
__attribute__((weak)) void GXLoadPosMtxImm(const void* m, u32 i) { (void)m;(void)i; }
__attribute__((weak)) void GXColor4u8(u8 r, u8 g, u8 b, u8 a) { (void)r;(void)g;(void)b;(void)a; }
__attribute__((weak)) void GXPosition3f32(float x, float y, float z) { (void)x;(void)y;(void)z; }
__attribute__((weak)) void GXPosition2s16(s16 x, s16 y) { (void)x;(void)y; }
__attribute__((weak)) void GXDrawDone(void) {}
__attribute__((weak)) void GXCopyDisp(void* fb, u32 c) { (void)fb;(void)c; }
__attribute__((weak)) void GXSetViewport(float x, float y, float w, float h, float n, float f) {
    (void)x;(void)y;(void)w;(void)h;(void)n;(void)f;
}

/* AX / AI / VI / NAND / SO / NWC24 / WPAD / KPAD / IOS stubs */
__attribute__((weak)) void AXInit(void) {}
__attribute__((weak)) void AXPBAddrInOut(u32 v, u32 a) { (void)v;(void)a; }
__attribute__((weak)) void AIRegisterDMACallback(void (*cb)(void)) { (void)cb; }
__attribute__((weak)) void VIInit(void) {}
__attribute__((weak)) void VIWaitForRetrace(void) {}
__attribute__((weak)) void VISetNextFramebuffer(void* fb) { (void)fb; }
__attribute__((weak)) s32 NANDInit(void) { return 0; }
__attribute__((weak)) s32 NANDReadFile(const char* p, void* b, u32* s) { (void)p;(void)b;(void)s; return -1; }
__attribute__((weak)) s32 NANDWriteFile(const char* p, const void* b, u32 s) { (void)p;(void)b;(void)s; return -1; }
__attribute__((weak)) s32 NANDCreateFile(const char* p, u32 m, u32 a) { (void)p;(void)m;(void)a; return 0; }
__attribute__((weak)) s32 SOCInit(void) { return 0; }
__attribute__((weak)) s32 NWC24Synchronize(void) { return 0; }
__attribute__((weak)) void WPADInit(void) {}
__attribute__((weak)) s32 KPADInit(void) { return 0; }
__attribute__((weak)) s32 KPADRead(u32 c, void* s, u32 l) { (void)c;(void)s;(void)l; return -1; }
__attribute__((weak)) s32 IOS_Ioctl(u32 fd, u32 i, const void* in, u32 il, void* out, u32 ol) {
    (void)fd;(void)i;(void)in;(void)il;(void)out;(void)ol; return 0;
}
__attribute__((weak)) s32 IOS_Open(const char* p, u32 m) { (void)p;(void)m; return 0; }
__attribute__((weak)) s32 IOS_Close(u32 fd) { (void)fd; return 0; }