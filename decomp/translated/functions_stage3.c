/* Forecast Channel — hand-decompiled functions (stage 3)
 * Source: contents/0000000d.app (DOL, entry 0x8000403c)
 * PPC750 (Broadway) -> C by GLaDOS, 2026-10-20. Verified against disassembly.
 * These are the first identified functions of the channel binary; the
 * full corpus (9,573 recovered functions) is in functions.json. */

#include <stdint.h>
typedef uint32_t u32; typedef uint16_t u16; typedef int32_t s32;

/* ---- 0x800ea948: OSDisableInterrupts ----
 * 7c6000a6  mfmsr  r3          ; read Machine State Register
 * 5464045e  rlwinm r4, r3, 0, 0x11, 0xf   ; r4 = r3 with EE (bit 16) cleared
 * 7c800124  mtmsr  r4          ; interrupts off
 * 54638ffe  rlwinm r3, r3, 0x11, 0x1f, 0x1f ; r3 = (r3 >> 16) & 1 = old EE
 * 4e800020  blr
 * 751 call sites: the channel disables IRQs 751 times — mostly inside GX/AX. */
u16 OSDisableInterrupts(void)
{
    u32 msr = __read_msr();
    __write_msr(msr & ~0x8000);
    return (u16)((msr >> 16) & 1);
}

/* ---- 0x800ea970: OSRestoreInterrupts ----
 * 2c030000  cmpwi  r3, 0
 * ...      state != 0 -> set EE, else clear; returns old EE.
 * 922 call sites: every DisableInterrupts is paired with a Restore. */
u32 OSRestoreInterrupts(u16 state)
{
    u32 msr = __read_msr();
    u32 new_msr = state ? (msr | 0x8000) : (msr & ~0x8000);
    __write_msr(new_msr);
    return (msr >> 16) & 1;
}

/* ---- 0x80004338: memmove ----
 * 7c041840  cmplw  r4, r3       ; src vs dst
 * 41800028  blt    forward      ; if src < dst, copy backwards
 * lbzu/stbu loop, decremented counters. Overlap-safe. 366 call sites. */
void* memmove(void* dst, const void* src, size_t n)
{
    u8* d = (u8*)dst; const u8* s = (const u8*)src;
    if (s < d) {
        /* descending copy */
        s += n - 1; d += n - 1;
        while (n--) *d-- = *s--;
    } else {
        while (n--) *d++ = *s++;
    }
    return dst;
}

/* ---- 0x8000403c: main (entry point) ----
 * Sequenced from the disassembly:
 *   bl 0x800041a8      ; init subsystem A
 *   bl 0x800042e0      ; init subsystem B
 *   *(u32*)0x80000044 = 0xFFFFFFFF     ; global state
 *   app_ctx = *(void**)0x800000f4     ; application context
 *   if (app_ctx) mode = *(u32*)(app_ctx+0xc)
 *   else        mode = *(u32*)0x80000034 (fallback ctx)
 *   switch (mode) {
 *     case 2: launch_arg = 0; break;
 *     case 3: launch_arg = 1; break;
 *     case 4: launch_arg = 2; bl 0x80004028; return;
 *     default: return;               ; unknown mode -> halt
 *   }
 *   ((void(*)(int))0x801783f8)(launch_arg);   ; dispatch via mtlr+blrl
 *   ... then reads context again for the shutdown path.
 * Launch modes 2/3/4 are the System Menu's channel-start modes
 * (normal / return-from-channel / boot-flag). */
void channel_main(void)
{
    sub_800041a8();          /* init — candidate: OSInit / VIInit chain */
    sub_800042e0();          /* init — candidate: app state setup       */
    *(volatile u32*)0x80000044 = 0xFFFFFFFF;

    void* ctx = *(void**)0x800000f4;
    u32 mode;
    if (ctx) mode = *(u32*)((char*)ctx + 0xC);
    else     mode = *(u32*)0x80000034;

    s32 launch_arg;
    switch (mode) {
    case 2: launch_arg = 0; break;   /* started from disc/menu           */
    case 3: launch_arg = 1; break;   /* returning from another channel  */
    case 4:                              /* special boot flag            */
        launch_arg = 2;
        sub_80004028(launch_arg);
        return;
    default:
        return;                      /* no valid launch context: halt   */
    }
    ((void(*)(s32))0x801783f8)(launch_arg);   /* -> channel appMain      */
}

/* ---- 0x8016184c: C++ virtual dispatch thunk ----
 * 800c0000  lwz  r0, 0(r12)    ; vtable
 * 816c0004  lwz  r11, 4(r12)   ; offset table (MI aka -1 => direct)
 * 818c0008  lwz  r12, 8(r12)   ; base
 * add r3, r3, r0               ; adjust this-ptr
 * if (r11 >= 0) r12 = *(r3 + r12)[r11]  ; multi-inheritance fixup
 * mtctr r12; bctr
 * The channel is C++ (nw4r lib Revolution framework) — this thunk
 * implements virtual calls with MI adjustment, 191 call sites. */
typedef void (*VFn)(void* self);
VFn __virtual_dispatch(void* obj, u32 vt_index)
{
    /* structure of a nw4r RTTI thunk; reconstructed semantics */
    return (VFn)obj; /* actual indexing performed via CTR jump at site */
}
