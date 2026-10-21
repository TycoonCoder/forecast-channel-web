/* Forecast Channel — stage 6: dispatch architecture, decompiled.
 * Source: contents/0000000d.app. PPC -> C by GLaDOS, 2026-10-20.
 *
 * CORRECTION to stage 5: launch args map as:
 *   mode 2 (normal start) -> thread_entry(0): SIX message queues, no handler
 *   mode 3 (returning)     -> thread_entry(1): handler table + WC24 checks
 * The "mode 2" stub table found in the binary is a DORMANT personality:
 * nine slots of li r3,-1 / li r3,0 stubs — never installed in the live
 * path (arg 2 is never passed by channel_main).
 */

#include <stdint.h>
typedef uint32_t u32; typedef uint8_t u8; typedef int32_t s32;
typedef volatile uint32_t vu32; typedef volatile uint8_t  vu8;

/* ---- queue init helper: 0x800e675c -------------------------------
 * Called with crclr cr1eq (PPC varargs convention: "no FP args").
 * r3 = queue object, r4 = size/message count. This is the RVL OS
 * queue-init family (OSInitMessageQueue-style). The channel thread
 * owns SIX queues at 0x80198c18 (thread-local block r29):            */

typedef struct {
    u8 q[0x18];        /* queue objects at +0x08, +0x20, +0x48,        */
} QSlot;               /* +0x60, +0x84, +0xb0, +0xe0 — six total      */

void queue_init(void* q, u32 nmsgs);        /* 0x800e675c              */

/* =====================================================================
 * thread_entry @ 0x80178a0c — corrected translation.
 *   arg 0 (started from System Menu):
 *     queue(q+0x08, arg); queue(q+0x20, 0x40); queue(q+0x48, arg);
 *     g_thread_ready = 0;
 *     queue(q+0x84, arg); queue(q+0xb0, ?); queue(q+0xe0, ?);
 *     return 1;
 *   arg 1 (returning from another channel):
 *     queue(q+0x08, 1); queue(q+0x20, 0x40); queue(q+0x60, 1);
 *     g_thread_ready = 0;
 *     fill handler table (9 slots) with the WC24 handler set;
 *     ... continues into shared tail (li r3, 1)
 * ===================================================================== */
s32 thread_entry(s32 launch_arg)
{
    queue_init((void*)0x80198c20, launch_arg);       /* +0x08 */
    queue_init((void*)0x80198c38, 0x40);             /* +0x20 */
    g_thread_ready = 0;                              /* 0x8032eff8 */

    if (launch_arg == 1) {
        queue_init((void*)0x80198c78, 1);            /* +0x60 */
        install_wc24_handler_table();                 /* 9 slots @ 0x801d6dc0 */
        /* shared tail */
        return 1;
    }
    if (launch_arg == 2) {                           /* DEAD PATH: no caller */
        queue_init((void*)0x80198c60, 1);            /* +0x48 */
        install_stub_table();                        /* nine return -1/0 stubs */
        g_thread_ready = 1;
        return 1;
    }
    /* launch_arg == 0 (normal start): three further queues */
    queue_init((void*)0x80198c9c, launch_arg);        /* +0x84 */
    queue_init((void*)0x80198cc8, 0);                /* +0xb0 */
    queue_init((void*)0x80198cf8, 0);                 /* +0xe0 */
    return 1;
}

/* =====================================================================
 * The dormant stub personality (arg 2, never dispatched by main):
 *   slot +0x00: 0x80178CA8  li r3,-1; blr
 *   slot +0x08: 0x80178CA0  li r3,-1; blr
 *   slot +0x0c: 0x80178C98  li r3,-1; blr
 *   slot +0x10: 0x80178C90  li r3,-1; blr
 *   slot +0x14: 0x80178C88  li r3, 0; blr
 *   slot +0x18: 0x80178C80  li r3, 0; blr
 *   slot +0x1c: 0x80178C78  li r3, 0; blr
 *   slot +0x20: 0x80178C70  li r3,-1; blr
 *   slot +0x24: 0x80178C68  li r3,-1; blr
 * Interleaved error/ok stubs — a "do nothing, reject commands" table.
 * ===================================================================== */

/* =====================================================================
 * The live (mode-3 / returning) handler set — WC24/network flavored:
 *
 * handler_0 @ 0x80178ee8:
 *     sub_80160874(dst=0x8032f508, src=0x8032f008);   // guarded by
 *     copy_worker(0x8032f508, 0x8032f008, 0x500);      // OSDisable/Restore
 *     return 0;   // 0x500-byte state snapshot copy between SDA globals
 *
 * sub_80160874 @ 0x80160874 (the guard):
 *     irq = OSDisableInterrupts();        // 0x800ea948
 *     *(u32*)dst = 0x8032f508-SDA... actually ctx ptr store
 *     g_saved_arg = arg;                  // stw r30, SDA-0x5ca0
 *     sub_80160e48();                     // real work under irq guard
 *     OSRestoreInterrupts(irq);           // 0x800ea970
 *
 * handler_1 @ 0x80178ebc (WC24 state check):
 *     if (*(u32*)SDA(-0x5c50) != 0)
 *         return -0x2715;                 // -10005: WiiConnect24 error
 *     ... continues (cut off in dump — full listing in decomp/asm/)
 *
 * -10005 is in the WC24/nwc24 error family — this is the channel
 * checking whether WiiConnect24 is available before proceeding on
 * the return-from-channel path.
 * ===================================================================== */
s32 wc24_check_available(void)
{
    if (g_wc24_flag != 0)
        return -10005;                       /* WC24 unavailable */
    return 0;
}

/* =====================================================================
 * Context trampoline twins — 0x801783f8 and 0x80178490.
 * Identical context-save prologues (stmw block at 0x8032eb10) with
 * DIFFERENT thread targets after the shared setup. Two threads:
 * the app thread and its sibling. The twin's call target was beyond
 * the stage-5b dump window; it is queued in decomp/asm/.
 *
 * OPEN QUESTION (flagged honestly): the trampoline tail does
 *     cmpwi r3, 1 ; bne settings_path
 *     lwz r4, 0x84(r3) ; mtlr r4 ; lmw r0, 0(r3) ; blr
 * i.e. r3==1 would treat r3 as a CONTEXT POINTER (resume), but
 * thread_entry returns literal 1. Either (a) my function-boundary
 * heuristic split thread_entry wrong and the live return value is
 * a context pointer, or (b) the r3==1 path is unreachable in
 * practice. Requires stage-6c boundary refinement to settle.
 * ===================================================================== */
