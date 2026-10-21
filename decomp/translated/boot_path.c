/* Forecast Channel — stage 5: the complete boot path, decompiled.
 * Source: contents/0000000d.app (entry 0x8000403c)
 * PPC750 (Broadway) -> C by GLaDOS, 2026-10-20.
 * Every function below was translated directly from its disassembly;
 * the asm corpus is in decomp/asm/. */

#include <stdint.h>
typedef uint32_t u32; typedef uint16_t u16; typedef uint8_t u8;
typedef int32_t s32; typedef volatile uint32_t vu32;

/* =====================================================================
 * __start @ 0x800041a8 — called first from main.
 * Zeros every GPR, then sets the three anchor registers:
 *   r1  = 0x803437a8  (initial stack pointer)
 *   r2  = 0x80339900  (SDA2 / small-data-area 2 base)
 *   r13 = 0x80337540  (SDA  / small-data-area base)
 * This is the Metrowerks CodeWarrior runtime's register initialization,
 * emitted before any C code in the binary runs.
 * ===================================================================== */
void __start(void)
{
    /* li r0..r31, 0 — 31 instructions, all registers cleared */
    register u32 r0_to_r31_cleared;
    __asm__ volatile ("" ::: "memory");
    /* lis r1, ...; ori r1, r1, ... — stack pointer */
    __set_gpr1((void*)0x803437a8);
    __set_gpr2((void*)0x80339900);   /* SDA2 */
    __set_gpr13((void*)0x80337540);  /* SDA  */
}

/* =====================================================================
 * __init_hardware @ 0x800042e0 — called second from main.
 *   mfmsr r0; ori r0, r0, 0x2000; mtmsr r0   ; MSR[FP] = 1: enable FPU
 *   mflr r31                                ; save LR in a volatile reg
 *   bl 0x800e4718                           ; init 1 (cache/BIOS area)
 *   bl 0x800e347c                           ; init 2
 *   bl 0x800e5d9c                           ; init 3
 * The three targets live in the low-level SDK block (0x800e3-0x800e5):
 * candidates for __init_cache, __init_ipc, __init_sprs.
 * ===================================================================== */
void __init_hardware(void)
{
    u32 msr = __read_msr();
    __write_msr(msr | 0x2000);        /* floating-point available */
    __init_800e4718();
    __init_800e347c();
    __init_800e5d9c();
}

/* =====================================================================
 * __init_data @ 0x80004238 — called third from main.
 * Walks a ROM-copy table at 0x80006420 of {dst, src, size} records:
 *   for each: if (src && src != dst) { memmove(dst, src, size);
 *                                     __flush_cache(dst, size); }
 * Then walks a BSS table at 0x800064a4 of {ptr, size} records:
 *   for each: if (size && ptr) __init_bss(ptr, 0, size);   // memset 0
 * This is the Metrowerks runtime section initializer: it materializes
 * .data/.sdata/.text overlays into RAM and zeroes .bss/.sbss.
 * ===================================================================== */
typedef struct { u32 dst, src, size; } DataCopyRecord;   /* 12 bytes */
typedef struct { u32 ptr, size; }        BssRecord;       /* 8 bytes  */

void __init_data(void)
{
    /* phase 1: copy initialized sections */
    DataCopyRecord* r = (DataCopyRecord*)0x80006420;
    while (r->size != 0) {
        if (r->src != 0 && r->src != r->dst) {
            memmove((void*)r->dst, (const void*)r->src, r->size);
            __flush_cache((void*)r->dst, r->size);
        }
        r++;
    }
    /* phase 2: zero BSS */
    BssRecord* b = (BssRecord*)0x800064a4;
    while (b->size != 0) {
        if (b->ptr != 0)
            memset((void*)b->ptr, 0, b->size);
        b++;
    }
}

/* =====================================================================
 * __flush_cache @ 0x80004304 — cache coherency after code/data copy.
 *   aligns ptr down to 8, then: { dcbst 0,addr; sync; icbi 0,addr;
 *   addr += 8 } until len exhausted; final isync.
 * Writes back d-cache lines and invalidates i-cache lines — required on
 * Harvard-cache PPC750 after self-modifying/overlay copies.
 * ===================================================================== */
void __flush_cache(void* addr, u32 len)
{
    u32 p = (u32)addr & ~7u;
    u32 end = p + len;
    do {
        __asm__ volatile("dcbst 0,%0" :: "r"(p));
        __asm__ volatile("sync");
        __asm__ volatile("icbi 0,%0" :: "r"(p));
        p += 8;
    } while ((s32)(end -= 8) >= 0);
    __asm__ volatile("isync");
}

/* =====================================================================
 * memset @ 0x80004388 — word-optimized fill.
 *   if len < 0x20: byte loop (stbu)
 *   else: replicate byte across a full word (slwi x8/x16/x24 + ors),
 *         then 0x20-byte stores (8x stw = 32 bytes) in a loop,
 *         then 4-byte stores, then trailing bytes.
 * 266+ call sites via its wrapper __init_bss @ 0x8000443c.
 * ===================================================================== */
void* memset(void* dst, int c, u32 len)
{
    u8* d = (u8*)dst;
    u8  v8 = (u8)c;
    if (len < 0x20) {                       /* small: byte loop */
        while (len--) *d++ = v8;
        return dst;
    }
    /* replicate byte to all four lanes */
    u32 v = v8 | (v8 << 8) | (v8 << 16) | (v8 << 24);
    u32 p = (u32)dst;
    /* head: align to word */
    u32 align = (0 - p) & 3;
    while (align--) { *(u8*)p++ = v8; len--; }
    /* 32-byte blocks */
    while (len >= 0x20) {
        *(u32*)(p+ 0) = v; *(u32*)(p+ 4) = v; *(u32*)(p+ 8) = v; *(u32*)(p+12) = v;
        *(u32*)(p+16) = v; *(u32*)(p+20) = v; *(u32*)(p+24) = v; *(u32*)(p+28) = v;
        p += 0x20; len -= 0x20;
    }
    while (len >= 4) { *(u32*)p = v; p += 4; len -= 4; }
    while (len--) *(u8*)p++ = v8;
    return dst;
}

/* =====================================================================
 * boot flag pair @ 0x80004028 / 0x80004034.
 * SDA-relative byte at r13-0x6250:
 *   set: stb 1; blr        get: lbz r3; blr
 * Set when the System Menu starts the channel in mode 4 (special boot).
 * ===================================================================== */
void SetSpecialBootFlag(void)  { *(u8*)(__SDA_BASE__ - 0x6250) = 1; }
u32  GetSpecialBootFlag(void)  { return *(u8*)(__SDA_BASE__ - 0x6250); }

/* =====================================================================
 * context_trampoline @ 0x801783f8 — the channel's thread bootstrap.
 * Full register context save to a block at 0x8032eb10:
 *   stmw r0..r31, SP, original arg (r3), LR (x2: 0x84, 0x80), CR, MSR
 * then: EE off, SRR1 = saved MSR (exception-return style),
 *   SPRs 0x3f2/0x3f5 zeroed (peripheral state),
 *   stack reset to 0x803457a8 (fresh application stack),
 *   r3 = r5 (second parameter), bl 0x80178a0c — the actual entry,
 *   ret == 1: restore LR + all GPRs from context and blr (resume caller)
 *   ret != 1: tail-branch to 0x80178714 (alternate path)
 * An identical twin trampoline lives at 0x80178490 — a matched pair,
 * consistent with the channel's two-thread design (app thread + sound/
 * network thread) seen in the AX/NWC24 module set.
 * ===================================================================== */
typedef struct {
    u32 gpr[32];       /* stmw r0..r31 */
    u32 sp;            /* +0x04: saved r1 */
    u32 arg;           /* +0x0c: original r3 */
    u32 lr_alt;        /* +0x80 */
    u32 lr;            /* +0x84 */
    u32 cr;            /* +0x88 */
} ThreadContext;       /* base 0x8032eb10 */

s32 context_trampoline(u32 arg_r3, u32 arg_r4, s32 arg_r5)
{
    ThreadContext* ctx = (ThreadContext*)0x8032eb10;
    /* full state capture (stmw) + SP/LR/CR/MSR */
    __save_context(ctx, arg_r3);
    u32 saved_msr = __read_msr();
    __write_msr(saved_msr & ~0x8000);        /* EE off for the handoff */
    __set_srr1(saved_msr);
    __init_thread_state();                   /* 0x80177f10 */
    __restore_gprs(ctx);                     /* lmw into scratch */
    __set_spr_3f2(0); __set_spr_3f5(0);      /* clear peripheral regs */
    __set_sp((void*)0x803457a8);             /* fresh application stack */
    s32 result = thread_entry(arg_r5);       /* 0x80178a0c */
    if (result == 1) {
        /* resume the caller exactly where it left off */
        __set_lr(ctx->lr);
        __restore_context(ctx);
        return result;
    }
    return alternate_path_80178714();        /* 0x80178714 */
}

/* =====================================================================
 * main @ 0x8000403c — the DOL entry point (refined).
 * Boot sequence, now with all callees named:
 *   __start()            — GPRs zeroed, SP/SDA/SDA2 set
 *   __init_hardware()     — FPU on, three low-level inits
 *   *(u32*)0x80000044 = -1
 *   __init_data()         — .data copies + cache flush + BSS zero
 *   read launch mode from app context (or fallback),
 *   mode 2 -> app(0): started from System Menu
 *   mode 3 -> app(1): returning from another channel
 *   mode 4 -> app(2) via SetSpecialBootFlag path
 *   dispatch is mtlr r6 / blrl through 0x801783f8 = context_trampoline:
 *      the channel's app entry runs on a FRESH STACK as a new thread.
 * ===================================================================== */
void channel_main(void)
{
    __start();
    __init_hardware();
    *(vu32*)0x80000044 = 0xFFFFFFFF;
    __init_data();

    void* ctx = *(void**)0x800000f4;
    u32 mode;
    if (ctx) mode = *(u32*)((char*)ctx + 0xC);
    else     mode = *(u32*)0x80000034;

    switch (mode) {
    case 2:  context_trampoline(0, 0, 0); break;  /* from System Menu */
    case 3:  context_trampoline(0, 0, 1); break;  /* returning from channel */
    case 4:
        SetSpecialBootFlag();
        return;
    default:
        return;                                   /* invalid launch: halt */
    }
}
