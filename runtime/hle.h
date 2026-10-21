/* runtime/hle.h — High-Level Emulation runtime interface for hosting
 * recompiled Forecast Channel PPC code in a browser (WASM).
 *
 * The recompiled code calls these instead of real hardware. This header
 * defines the surface the channel binary actually exercises, derived from
 * the stage-7 decompilation of its SDK usage:
 *   OS: interrupts, threads, message queues, alarm
 *   GX: render-command submission (globe/UI draw)
 *   AX/AI: audio DMA + DSP mixer (16-voice table at 0x802f24a0)
 *   VI: video mode / retrace
 *   NAND: settings file read/create (load_or_create_settings)
 *   SO/NWC24: forecast data over HTTP(S) to WC24 replacement servers
 *   WPAD/KPAD: pointer input
 *
 * Memory map (Wii):
 *   0x80000000 app text      0x8032eb10 thread context block
 *   0x8000403c entry          0x80337540 SDA base (r13)
 *   0x801d6dc0 dispatch table 0x80339900 SDA2 (r2)
 *   0x802d2140 state records  0x803437a8 initial SP
 *   MEM1 = 24MB @ 0x80000000..0x817FFFFF
 *   IOS IPC: 0x0D000000 (ARM registers), 0xCD0000C0 = AI control
 */
#ifndef FC_HLE_H
#define FC_HLE_H

#include <stdint.h>
#include <stddef.h>

typedef uint32_t u32; typedef uint16_t u16; typedef uint8_t u8;
typedef int32_t s32; typedef int16_t s16;

/* ---------------- Shared state (defined in chassis.c) ---------------- */
#include "cpu/cpu.h"
extern unsigned char* guest_mem;
extern CPUState fc_cpu;

/* inline host pointer conversion - defined in chassis.c */
unsigned char* host_ptr(u32 addr);

/* ---------------- OS (kernel-equivalent) ---------------- */
u16  OSDisableInterrupts(void);
u32  OSRestoreInterrupts(u16 state);
void OSReport(const char* fmt, ...);
void OSSleepThread(void* thread);
void OSWakeupThread(void* thread);
void OSCreateThread(void* tcb, void* entry, void* stack_hi, void* stack_lo, u32 priority);
void OSResumeThread(void* tcb);
void OSYieldThread(void);
u32  OSGetSystemTime(void);
void* OSAllocFromHeap(void* heap, u32 size);
void* OSCreateHeap(void* start, void* end);
void OSInitMessageQueue(void* queue, void* buffer, u32 capacity);
s32  OSSendMessage(void* queue, void* msg, u32 flags);
s32  OSReceiveMessage(void* queue, void** msg, u32 flags);

/* ---------------- GX (render commands -> WebGL) ---------------- */
void GXBegin(u32 primitive, u32 vtxfmt, u32 count);
void GXEnd(void);
void GXSetTevOrder(u32 stage, u32 texmap, u32 texcoord, u32 color);
void GXSetVtxDesc(u32 attr, u32 type);
void GXLoadPosMtxImm(const void* matrix, u32 index);
void GXColor4u8(u8 r, u8 g, u8 b, u8 a);
void GXPosition3f32(float x, float y, float z);
void GXPosition2s16(s16 x, s16 y);
void GXDrawDone(void);
void GXCopyDisp(void* framebuffer, u32 clear);
void GXSetViewport(float x, float y, float w, float h, float zn, float zf);

/* ---------------- AX / AI (audio -> WebAudio) ---------------- */
void AXInit(void);
void AXPBAddrInOut(u32 voice, u32 addr);
void AIRegisterDMACallback(void (*cb)(void));

/* ---------------- VI ---------------- */
void VIInit(void);
void VIWaitForRetrace(void);
void VISetNextFramebuffer(void* fb);

/* ---------------- NAND (-> IndexedDB) ---------------- */
s32  NANDInit(void);
s32  NANDReadFile(const char* path, void* buf, u32* size);
s32  NANDWriteFile(const char* path, const void* buf, u32 size);
s32  NANDCreateFile(const char* path, u32 mode, u32 attrs);

/* ---------------- SO / NWC24 (-> fetch to WC24 servers) ---- */
s32  SOCInit(void);
s32  SOCinet_ntoa(u32 addr, char* out);
s32  NWC24Synchronize(void);

/* ---------------- WPAD / KPAD (-> pointer events) --------- */
void WPADInit(void);
s32  KPADInit(void);
s32  KPADRead(u32 controller, void* status, u32 length);

/* ---------------- IOS IPC ---------------- */
s32  IOS_Ioctl(u32 fd, u32 ioctl, const void* in, u32 inlen, void* out, u32 outlen);
s32  IOS_Open(const char* path, u32 mode);
s32  IOS_Close(u32 fd);

#endif /* FC_HLE_H */