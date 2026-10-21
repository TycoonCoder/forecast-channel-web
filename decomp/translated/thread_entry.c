/* Forecast Channel — stage 5b: thread_entry + alternate_path, decompiled.
 * The trampoline (0x801783f8) hands control here on a fresh stack.
 * Source: contents/0000000d.app. PPC -> C by GLaDOS, 2026-10-20. */

#include <stdint.h>
typedef uint32_t u32; typedef uint8_t u8; typedef int32_t s32;
typedef volatile uint32_t vu32; typedef volatile uint8_t  vu8;

/* r29 (stack frame base) resolves to 0x80198c18: the thread's local
 * message-queue storage. Three queues are initialized:
 *   queue_A at +0x08 (mode 2)  buffer 0x40 bytes
 *   queue_B at +0x20           buffer 0x40 bytes
 *   queue_C at +0x48 (mode 2) / +0x60 (mode 1)  — launch-dependent
 * 0x800e675c is the queue-init helper (cr1.eq cleared per the PPC
 * varargs convention used by Metrowerks-compiled OS calls). */

typedef struct { u8 buf[0x40]; } MsgQueueBuf;
typedef struct {
    MsgQueueBuf qA;   /* +0x08 */
    MsgQueueBuf qB;   /* +0x20 */
    MsgQueueBuf qC;   /* +0x48 (mode 2) or +0x60 (mode 1) */
} ThreadQueues;       /* base 0x80198c18 */

/* message handler table — the channel's command dispatch */
typedef struct {
    void* handler_0;   /* +0x00 */
    u32   flag;        /* +0x04 = 0 */
    void* handler_1;   /* +0x08 */
    void* handler_2;   /* +0x0c */
    void* handler_3;   /* +0x10 */
    void* handler_4;   /* +0x14 */
    void* handler_5;   /* +0x18 */
    void* handler_6;   /* +0x1c */
    void* handler_7;   /* +0x20 */
    void* handler_8;   /* +0x24 */
} DispatchTable;       /* base 0x801d6dc0 */

extern ThreadQueues  g_queues;       /* 0x80198c18 */
extern DispatchTable g_dispatch;     /* 0x801d6dc0 */
extern vu8 g_thread_ready;           /* 0x8032eff8 */
extern vu32 g_nand_status;           /* 0x8032eff0 */

/* hand-rolled handler sets recovered from the table stores:
 * mode 2 (from System Menu): handlers at 0x80178CA8, 0x80178CA0,
 *   0x80178C98, 0x80178C90, 0x80178C88, 0x80178C80, 0x80178C78,
 *   0x80178C70, 0x80178C68
 * mode 1 (returning from channel): handlers at 0x80178EE8, 0x80178EBC,
 *   0x80178EB4, 0x80178E00, 0x80178D8C, 0x80178E20, 0x80178D34,
 *   0x80178D68, 0x80178D44  (+ 0x80178CB0 head)
 * The two sets differ — the channel presents different behavior when
 * re-entered from another channel vs. started fresh. */

void init_queues_mode2(void);
void init_queues_mode1(void);
void fill_dispatch_mode2(void);
void fill_dispatch_mode1(void);

/* =====================================================================
 * thread_entry @ 0x80178a0c — the channel application's real main.
 * r3 = launch arg (0: from System Menu, 1: returning from a channel).
 * ===================================================================== */
s32 thread_entry(s32 launch_arg)
{
    init_queues_mode2();               /* qA, qB via 0x800e675c        */

    g_thread_ready = 0;                /* stb 0, -0x1008(r3=0x80330000)*/

    if (launch_arg == 2) {             /* cmpwi r30, 2                 */
        init_queues_mode2();           /* third queue at +0x48         */
        fill_dispatch_mode2();         /* table @ 0x801d6dc0, set A     */
        g_thread_ready = 1;           /* stb 1 -> 0x8032eff8           */
        goto main_loop;               /* b 0x80178bc4                  */
    }
    if (launch_arg == 1) {             /* cmpwi r30, 1                 */
        init_queues_mode1();           /* third queue at +0x60          */
        fill_dispatch_mode1();         /* table @ 0x801d6dc0, set B     */
        /* (mode-1 tail continues at 0x80178b98: same flag + loop)     */
    }
main_loop:
    /* 0x80178bc4 onward (not shown): the message pump — dequeue,
     * dispatch through g_dispatch, service the render loop.
     * See decomp/asm/ for the full listing. */
    return channel_main_loop();        /* 0x80178bc4                    */
}

/* =====================================================================
 * alternate_path @ 0x80178714 — settings-file recovery.
 * Pattern: check NAND state; store; on failure run the create/init
 * pair; re-check. This is the channel reading (or first-run creating)
 * of its NAND settings file.
 * ===================================================================== */
void load_or_create_settings(void)
{
    s32 st = nand_check_settings();    /* 0x80173e64                    */
    g_nand_status = st;                /* stw r3, -0x1010(r4)           */
    if (st == 0)                       /* file exists and is valid      */
        goto done;
    nand_init_defaults();             /* 0x80173e18                    */
    nand_create_settings();            /* 0x80173af8                    */
done:
    st = nand_finalize();              /* 0x80173e40                    */
    g_nand_status = st;
}

/* Hand-recovered dispatch handler sets (fill functions):
 * fill_dispatch_mode2 stores: [0]=0x80178CA8 [8]=0x80178CA0
 *   [0xc]=0x80178C78 [0x10]=0x80178C88 [0x14]=0x80178C80
 *   [0x18]=0x80178C98 [0x1c]=0x80178C90 [0x20]=0x80178C70
 *   [0x24]=0x80178C68 [4]=0
 * fill_dispatch_mode1 stores: [0]=0x80178EE8 (stwu head 0x80178CB0)
 *   [0x18]=0x80178EBC [0x1c]=0x80178EB4 [0x10]=0x80178E00
 *   [0x14]=0x80178D8C [8]=0x80178E20 [0xc]=0x80178D34
 *   [0x20]=0x80178D68 [0x24]=0x80178D44 [4]=0
 * Each of those nine targets is a distinct command handler; they are
 * the channel's actual event handlers (weather data arrival, button
 * input, globe motion, forecast render, etc.) — next translation pass.
 */
