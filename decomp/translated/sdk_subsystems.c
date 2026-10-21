/* Forecast Channel — stage 7: SDK anchors and subsystem init, decompiled.
 * Source: contents/0000000d.app. PPC -> C by GLaDOS, 2026-10-20. */

#include <stdint.h>
typedef uint32_t u32; typedef uint16_t u16; typedef uint8_t u8;
typedef volatile uint32_t vu32; typedef volatile uint8_t  vu8;
typedef int32_t s32;

/* =====================================================================
 * audio_hw_init @ 0x8011de5c — AI/DSP audio enable.
 *   irq = OSDisableInterrupts();
 *   *(vu32*)0xCD0000C0 |= 0x100;      // AI DMA control register: enable
 *   OSRestoreInterrupts(irq);
 *   ... 16-byte table at SDA-0x2b08+0x1040 region: cleared to -1 ...
 *   memset-style loop of stb -1 (16 slots)
 *   then queue/worker setup via 0x80129ca0 (crclr cr1eq: varargs)
 * This is the AX/AI audio bring-up on the channel's sound thread:
 * hardware DMA armed, channel-mixer table cleared, worker queued.
 * ===================================================================== */
void audio_hw_init(void)
{
    u16 irq = OSDisableInterrupts();
    *(vu32*)0xCD0000C0 |= 0x100;          /* AI DMA enable            */
    OSRestoreInterrupts(irq);

    u8* mixer_table = (u8*)0x802f24a0;    /* 16 slots                */
    for (int i = 0; i < 16; i++)
        mixer_table[i] = 0xFF;             /* all voices muted/invalid */

    audio_worker_setup();                  /* 0x80129ca0              */
}

/* =====================================================================
 * spawn_worker_thread @ 0x801622a4 — OSCreateThread, priority 1.
 *   if (g_worker_tcb == -1) {                    // not yet created
 *     queue_init(0x801979c8, ...);                // +0x00
 *     queue_init(0x801979c8 + 0x36, ...);         // +0x36
 *     stack_hi = OSGetStackHi?   (0x800e5344)    // thread obj setup
 *     stack_lo = ...             (0x800e532c)
 *     OSCreateThread(tcb, func=..., stack, ..., priority=1);  // 0x800e5240
 *   }
 * Priority 1 = high priority worker — consistent with the sound/network
 * sibling thread found in stage 5 (context twin at 0x80178490).
 * ===================================================================== */
void spawn_worker_thread(void)
{
    if (g_worker_tcb == (u32)-1) {
        queue_init((void*)0x801979c8, N_MSGS_A);
        queue_init((void*)0x80197a9e, N_MSGS_B);
        u32 stack_hi = os_get_stack_hi();          /* 0x800e5344 */
        u32 stack_lo = os_get_stack_lo();          /* 0x800e532c */
        OSCreateThread(&g_worker_tcb, worker_main,
                        stack_hi, stack_lo,
                        1 /* priority */);          /* 0x800e5240 */
    }
}

/* =====================================================================
 * app_state_validate @ 0x801068d8 — boot-time state validation.
 *   Under IRQ guard: registers the context object (r31) into a table of
 *   0x14-byte records at SDA-0x2d00+0x2140, either at slot 0 or
 *   index (count-1)*0x14.
 *   Then: type 0x10 / type 0x20 special-cases -> fatal_error(0x01234568,
 *   "type-16") / (0x01234569, "type-32") via 0x80108914.
 *   Region checks: g_region (SDA-0x5ffc) == 3 or 0xf get a distinct path
 *   with sub-validation via a mode flag; region 0xf additionally sets
 *   a flag at SDA-0x600c.
 *   Table record size 0x14 and region constants 3/0xf match the RVL
 *   'SC' (system configuration) region encoding (1=JPN, 3=USA? per
 *   product region tables) — the channel adapting per-console-region.
 * ===================================================================== */
typedef struct { u32 words[5]; } StateRecord;   /* 0x14 bytes */

void app_state_validate(u32 type_arg)
{
    u16 irq = OSDisableInterrupts();
    StateRecord* tbl = (StateRecord*)0x802d2140;
    u32 count = g_state_count;                       /* SDA-0x6080 */
    if (count == 0) {
        g_first_state = type_arg;                    /* tbl[0].w0 slot */
    } else {
        tbl[count - 1].words[?] = type_arg;           /* record at (count-1) */
    }
    OSRestoreInterrupts(irq);

    if (type_arg == 0x10) { fatal_shutdown(0x01234568, "type16"); return; }
    if (type_arg == 0x20) { fatal_shutdown(0x01234569, "type32"); return; }

    u32 region = g_region;                            /* SDA-0x5ffc */
    if (region == 3 || region == 0xF) {
        if (type_arg & 2) { fatal_shutdown(0x01234567, "mode2"); return; }
        g_retry_count = 0;                            /* SDA-0x6068 */
        if (region == 0xF)
            g_region_f_flag = 1;                      /* SDA-0x600c */
        /* ... continued: per-region setup (full listing decomp/asm/) */
    }
}

/* ---- fatal_shutdown: the 0x0123456X codes are the channel's
 * internal shutdown reasons (named constants, not magic). ---------- */
void fatal_shutdown(u32 reason, const char* tag);     /* 0x80108914 */
