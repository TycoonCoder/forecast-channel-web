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
#include <stdint.h>
#include <emscripten.h>

#define MEM1_SIZE  0x8200000u   /* 24MB + guard, cached+uncached alias */
#define FC_MSR_FP  (1u << 18)   /* PPC MSR bit 18: floating-point available */
unsigned char *guest_mem;
CPUState fc_cpu;
static u32 fc_last_dispatch_pc;   /* last block address handed to the dispatcher */

/* Big-endian to host (little-endian) conversion */
static inline u32 be32(const unsigned char* p) {
    return ((u32)p[0] << 24) | ((u32)p[1] << 16) | ((u32)p[2] << 8) | (u32)p[3];
}

unsigned char* host_ptr(u32 addr) {
    u32 a = addr & 0x01FFFFFFu;  /* physical offset within MEM1 (24MB = 0x1800000) */
    if (a >= MEM1_SIZE) {
        EM_ASM({ console.error('[host_ptr] OOB:', $0, '->', $1); }, addr, a);
        return guest_mem;
    }
    return guest_mem + a;
}

/* ---- boot ---- */
int load_guest_image(const unsigned char* dol, unsigned len) {
    (void)len;
    guest_mem = (unsigned char*)malloc(MEM1_SIZE);
    if (!guest_mem) return -1;
    memset(guest_mem, 0, MEM1_SIZE);
    EM_ASM({ console.log('[chassis] guest_mem allocated at', $0, 'size', $1); }, (uintptr_t)guest_mem, MEM1_SIZE);
    
    u32 t_offs[7], t_addrs[7], t_sizes[7];
    u32 d_offs[11], d_addrs[11], d_sizes[11];
    
    /* DOL header is big-endian; byte-swap on little-endian host */
    for (int i = 0; i < 7; i++)  t_offs[i]  = be32(dol + 0x00 + i*4);
    for (int i = 0; i < 11; i++) d_offs[i]  = be32(dol + 0x1C + i*4);
    for (int i = 0; i < 7; i++)  t_addrs[i] = be32(dol + 0x48 + i*4);
    for (int i = 0; i < 11; i++) d_addrs[i] = be32(dol + 0x64 + i*4);
    for (int i = 0; i < 7; i++)  t_sizes[i] = be32(dol + 0x90 + i*4);
    for (int i = 0; i < 11; i++) d_sizes[i] = be32(dol + 0xAC + i*4);
    
    for (int i = 0; i < 7; i++)
        if (t_sizes[i]) {
            EM_ASM({ console.log('[load] text', $0, 'addr=0x', $1.toString(16), 'off=0x', $2.toString(16), 'sz=0x', $3.toString(16)); }, i, t_addrs[i], t_offs[i], t_sizes[i]);
            memcpy(host_ptr(t_addrs[i]), dol + t_offs[i], t_sizes[i]);
        }
    for (int i = 0; i < 11; i++)
        if (d_sizes[i]) {
            EM_ASM({ console.log('[load] data', $0, 'addr=0x', $1.toString(16), 'off=0x', $2.toString(16), 'sz=0x', $3.toString(16)); }, i, d_addrs[i], d_offs[i], d_sizes[i]);
            memcpy(host_ptr(d_addrs[i]), dol + d_offs[i], d_sizes[i]);
        }
    return 0;
}

/* ---- CPU hooks: permissive SPR access + instruction fallback ---- */

/* Mini timebase: Broadway TB ticks at ~60.75MHz; we advance per step slice. */
static u64 fc_tb_ticks;
#define FC_TB_PER_STEP 1012500u   /* ≈ 1/60s of ticks per fc_step slice */

static u32 fc_spr_read_hook(CPUState* cpu, u16 spr, u32 cia) {
    (void)cia;
    /* Runtime-called SPRs that cleared ppc_spr_access. Read as 0 — matches
     * unimplemented-register behavior for the channel's purposes. */
    EM_ASM({ console.log('[spr] read  spr=', $0); }, spr);
    (void)cpu;
    return 0;
}

static void fc_spr_write_hook(CPUState* cpu, u16 spr, u32 value, u32 cia) {
    (void)cpu; (void)value; (void)cia;
    EM_ASM({ console.log('[spr] write spr=', $0, 'val=0x', ($1>>>0).toString(16)); }, spr, value);
}

/* Fallback interpreter for instructions DolRecomp could not statically decode.
 * The generated code calls this instead of raising an illegal-instruction
 * exception; we emulate the common X-form system ops here.
 *
 * PPC mfspr/mtspr encoding note: the SPR number is split across the
 * instruction fields with the LOW 5 bits in bits 16-20 and the HIGH 5 bits
 * in bits 11-15 (verified against the corpus: mfspr r0,0x396 = 0x7c16e2a6
 * has rA-field=22, rB-field=28 → (28<<5)|22 = 918 = 0x396). */
static void fc_instruction_fallback(CPUState* cpu, u32 raw, u32 cia) {
    u32 opcode = raw >> 26;
    u32 ext = (raw >> 1) & 0x3FF;

    /* Generated blocks END at the fallback instruction and resume at
     * ctx->pc — which is still the fallback's own address. The fallback
     * handler is therefore responsible for advancing pc past the
     * instruction (verified against chunk_0000: `ctx->pc = addr;
     * ppc_fallback_instruction(...); return;`). */
    cpu->pc = cia + 4;

    if (opcode == 31) {
        u32 rd_rs = (raw >> 21) & 0x1F;
        u32 spr = (((raw >> 11) & 0x1F) << 5) | ((raw >> 16) & 0x1F);

        switch (ext) {
        case 339: /* mfspr rD, spr */
            if (spr == 1008 || spr == 1009) {
                /* HID0/HID1: report caches enabled, everything else off. */
                cpu->gpr[rd_rs] = (spr == 1008) ? 0x0011C000u : 0x80000000u;
            } else if (spr == 920) {
                cpu->gpr[rd_rs] = cpu->hid2;
            } else if (spr >= 912 && spr <= 919) {
                cpu->gpr[rd_rs] = cpu->gqr[spr - 912];
            } else if (spr == 22) {
                cpu->gpr[rd_rs] = 0; /* DEC */
            } else if (spr == 268 || spr == 269) {
                /* TBL/TBU direct SPR-style read */
                cpu->gpr[rd_rs] = (spr == 268) ? (u32)cpu->timebase
                                              : (u32)(cpu->timebase >> 32);
            } else {
                EM_ASM({ console.log('[fallback] mfspr r', $0, ',', $1, ' → 0'); }, rd_rs, spr);
                cpu->gpr[rd_rs] = 0;
            }
            return;
        case 467: /* mtspr spr, rS */
            if (spr == 920) {
                cpu->hid2 = cpu->gpr[rd_rs];
            } else if (spr >= 912 && spr <= 919) {
                cpu->gqr[spr - 912] = cpu->gpr[rd_rs];
            } else {
                EM_ASM({ console.log('[fallback] mtspr', $0, ', r', $1, '=0x', ($2>>>0).toString(16)); }, spr, rd_rs, cpu->gpr[rd_rs]);
            }
            return;
        case 371: /* mftb rD, tbr */
            if (spr == 268) cpu->gpr[rd_rs] = (u32)cpu->timebase;
            else if (spr == 269) cpu->gpr[rd_rs] = (u32)(cpu->timebase >> 32);
            else cpu->gpr[rd_rs] = 0;
            return;
        case 854: /* eieio */ return;
        case 598: /* sync */ return;
        case 982: /* icbi — cache invalidate, no-op on flat memory */ return;
        case 254: /* dcbt — prefetch hint */ return;
        case 54:  /* dcbst — store-back hint, no-op */ return;
        case 86:  /* dcbf — flush hint, no-op */ return;
        case 470: /* dcbi — invalidate, no-op */ return;
        case 278: /* tlbie — tlb invalidate, no-op */ return;
        case 17:  /* mfmsr — supervisor mode reports 0x0000.30? report MSR with FP */ 
            cpu->gpr[(raw >> 21) & 0x1F] = cpu->msr;
            return;
        case 146: /* mtmsr — accept and store (keep FP bit set) */
            cpu->msr = cpu->gpr[(raw >> 21) & 0x1F] | FC_MSR_FP;
            return;
        default:
            break;
        }
    }

    /* Unknown fallback instruction: log ONCE per unique address (these sit
     * inside hot loops), then treat as NOP so the guest keeps running. */
    static u32 seen_addr[64] = {0};
    static u32 seen_next = 0;
    for (u32 i = 0; i < seen_next && i < 64; i++)
        if (seen_addr[i] == cia) return;
    if (seen_next < 64) seen_addr[seen_next++] = cia;
    EM_ASM({ console.error('[fallback] unhandled insn raw=0x', ($0>>>0).toString(16), 'at 0x', ($1>>>0).toString(16)); }, raw, cia);
}

static void fc_cache_control(CPUState* cpu, u8 op, u32 ea, u32 cia) {
    /* dcbst/dcbf/dcbi/icbi are no-ops on a flat memory model. */
    (void)cpu; (void)op; (void)ea; (void)cia;
}

/* ---------------- External (MMIO) hooks ----------------
 * The channel pokes real hardware. Address-aware responses below are
 * decoded from the channel's own polling loops:
 *   0xCC00500A (PI)  : bit5 polled-until-set (0x800e56d8) — report ready
 *   0xCC005004 (PI)  : combined-interrupt word polled-until-clear at
 *                      0x800e56a0, then bit15 polled-until-set at
 *                      0x800e576c. Real hardware: bit15 (reset-done)
 *                      transitions clear→set when the PI reset completes.
 *                      We model that: bit15 starts clear; once the channel
 *                      writes the PI reset command (0xCC005012 ← 0x43),
 *                      bit15 reads back set forever.
 *   0xCD00680C (DSP) : bit0 polled-until-clear — report idle/done
 *   0xCD006034/6834  : mailbox-full flags — report never-full
 *   0xCD006024 (Hollywood cfg) : status byte — report 0
 * Writes sink; reads default to 0. */
static bool fc_pi_reset_done = false;

static u64 fc_external_read(CPUState* cpu, u32 ea, u8 size) {
    (void)cpu;
    static u32 seen[128] = {0};
    static u32 seen_n = 0;
    bool fresh = true;
    for (u32 i = 0; i < seen_n; i++) if (seen[i] == ea) { fresh = false; break; }
    if (fresh && seen_n < 128) {
        seen[seen_n++] = ea;
        EM_ASM({ console.log('[mmio] R 0x', ($0>>>0).toString(16), ' sz=', $1); }, ea, size);
    }

    /* Address-aware responses. The channel reads 32-bit PI registers as TWO
     * separate 16-bit lhz accesses (0xCC005004 upper, 0xCC005006 lower).
     * We must return the correct half for the address + size. */
    if (ea == 0xCC00500Au) return 0x20u;       /* PI status: bit5 set = ready */
    if (ea == 0xCC005000u) return 0x46u;       /* PI: standard config */

    /* 0xCC005004: PI combined interrupt status (upper 16 bits of 32-bit reg).
         * Channel lhz reads upper half at 0x5004.
         *   - Loop at 0x800E56A0: lhz at pc=0x800E56A0 tests bit 0 (0x8000),
         *     branches WHILE set → exits when CLEAR. Return 0x0000.
         *   - Loop at 0x800E576C: lhz at pc=0x800E576C tests SAME bit 0,
         *     branches WHILE clear → exits when SET. Return 0x8000.
         * Both read via 16-bit lhz at address 0xCC005004. We distinguish by
         * cpu->pc (the load instruction's address). */
        if (ea == 0xCC005004u) {
            u64 ret;
            if (size == 2) {
                /* 16-bit read of upper half */
                if (cpu->pc == 0x800E576Cu) {
                    /* Second loop: needs bit 0 SET */
                    ret = fc_pi_reset_done ? 0x8000u : 0x0000u;
                } else {
                    /* First loop (pc=0x800E56A0 or 0x800E56A4): needs bit 0 CLEAR */
                    ret = 0x0000u;
                }
            } else {
                /* 32-bit read: upper half in upper 16, lower half in lower 16 */
                ret = 0x00000000u;
            }
            return ret;
        }

    /* 0xCC005006: PI interrupt mask / lower status half. Channel reads upper
     * 16 bits of this register via lhz at 0x5006. We report no interrupts
     * masked/enabled so the combined word test depends only on 0x5004. */
    if (ea == 0xCC005006u) {
        if (size == 2) return 0x0000u;
        return 0x00000000u;
    }

    return 0;
}

static void fc_external_write(CPUState* cpu, u32 ea, u64 value, u8 size) {
    (void)cpu; (void)size;
    /* PI reset command: writing 0x43 to 0xCC005012 resets the PI; after
     * the write, the reset-done bit (0x5004 bit15) reads back set. */
    if (ea == 0xCC005012u && (value & 0xFFu) == 0x43u)
        fc_pi_reset_done = true;
    static u32 seen[128] = {0};
    static u32 seen_n = 0;
    bool fresh = true;
    for (u32 i = 0; i < seen_n; i++) if (seen[i] == ea) { fresh = false; break; }
    if (fresh && seen_n < 128) {
        seen[seen_n++] = ea;
        EM_ASM({ console.log('[mmio] W 0x', ($0>>>0).toString(16), ' ← 0x', ($1>>>0).toString(16)); }, ea, (u32)value);
    }
}

static u32 fc_external_read32(CPUState* cpu, u32 ea, u8 rid) {
    (void)rid;
    return (u32)fc_external_read(cpu, ea, 32);
}

static void fc_external_write32(CPUState* cpu, u32 ea, u32 value, u8 rid) {
    (void)rid;
    fc_external_write(cpu, ea, value, 32);
}

static void* fc_external_pointer(CPUState* cpu, u32 ea, u32 size) {
    /* Redirect MMIO pointer requests into a scratch page so vectorized
     * guest accesses don't walk off memory. */
    (void)cpu; (void)ea; (void)size;
    static unsigned char scratch[4096] = {0};
    return scratch;
}

/* ---- main guest loop (called from JS every frame via requestAnimationFrame) ---- */
int fc_boot(const unsigned char* dol, unsigned len) {
    if (load_guest_image(dol, len)) return -1;
    memset(&fc_cpu, 0, sizeof(fc_cpu));

    /* Allocate DolRecomp's own RAM, then point it at the loaded image.
     * cpu_init memsets the state and allocates RAM; we then swap in our
     * guest_mem (which already contains the loaded DOL image) and free
     * cpu_init's allocation to avoid double memory. */
    if (!cpu_init(&fc_cpu)) {
        EM_ASM({ console.error('[fc_boot] cpu_init failed'); });
        return -1;
    }
    unsigned char* init_ram = fc_cpu.ram;
    fc_cpu.ram = guest_mem;          /* 24MB+ MEM1 image already loaded */
    fc_cpu.ram_size = GC_MAIN_RAM_SIZE; /* 24MB — matches GC_RAM_BASE window */
    free(init_ram);

    /* The Wii BIOS hands the channel over with FP enabled and all hooks live.
     * cpu_init zeroed MSR (FPU disabled) and every callback — restore both. */
    fc_cpu.msr |= FC_MSR_FP;          /* FPU usable (channel uses FPU immediately) */
    fc_cpu.spr_read  = fc_spr_read_hook;
    fc_cpu.spr_write = fc_spr_write_hook;
    fc_cpu.instruction_fallback = fc_instruction_fallback;
    fc_cpu.cache_control = fc_cache_control;
    fc_cpu.external_read    = fc_external_read;
    fc_cpu.external_write   = fc_external_write;
    fc_cpu.external_read32  = fc_external_read32;
    fc_cpu.external_write32 = fc_external_write32;
    fc_cpu.external_pointer = fc_external_pointer;

    /* The Wii BIOS/apploader populates the boot-info page at 0x80000000
     * before the channel runs; our zeroed MEM1 lacks it. Decoded from the
     * channel's own init at 0x800e3f20:
     *   mem[0x800000F4] = BI2 pointer → channel reads BI2+0x0C (region,
     *     stored to 0x800030E8), BI2+0x24 (country, stored at -0x5d90(r13)),
     *     and uses BI2+0x0C as a "settings" struct whose +0x2C must be
     *     non-zero for the WC24 data subsystem to initialize (0x800e3610).
     *   mem[0x800000F8] = timer divisor (wait loop at 0x800e544c). */
    {
        /* All guest loads/stores go through read_be32/write_be32 (Wii is
         * big-endian) — seeds must be written BIG-ENDIAN or the channel
         * reads byte-swapped garbage. */
        const u32 BI2_ADDR = 0x80600000u;   /* free MEM1 page, away from code */
        unsigned char* bp = guest_mem;       /* 0x80000000 (offset 0) */
        unsigned char* bi2 = host_ptr(BI2_ADDR);
        u32 v;
        v = BI2_ADDR;  bi2[0]=v>>24; bi2[1]=v>>16; bi2[2]=v>>8; bi2[3]=v;  /* (unused but harmless) */
        v = 0u;        bi2[0x0C]=v>>24; bi2[0x0D]=v>>16; bi2[0x0E]=v>>8; bi2[0x0F]=v;  /* BI2+0xC = launch type.
                                                                     Decoded from 0x80004048 dispatcher:
                                                                     2/3/4 = special trampoline launches
                                                                     (WC24-return etc.), ANYTHING ELSE =
                                                                     normal start → main(r14,r15) → the
                                                                     channel's own render loop. 0 = normal. */
        v = 49u;       bi2[0x24]=v>>24; bi2[0x25]=v>>16; bi2[0x26]=v>>8; bi2[0x27]=v; /* country: USA */
        v = 1u;        bi2[0x2C]=v>>24; bi2[0x2D]=v>>16; bi2[0x2E]=v>>8; bi2[0x2F]=v; /* configured flag */
        v = BI2_ADDR;  bp[0xF4]=v>>24; bp[0xF5]=v>>16; bp[0xF6]=v>>8; bp[0xF7]=v;     /* BI2 pointer */
        v = 0x10000000u; bp[0xF8]=v>>24; bp[0xF9]=v>>16; bp[0xFA]=v>>8; bp[0xFB]=v;  /* timer divisor data */
        v = 1u;        bp[0xCC]=v>>24; bp[0xCD]=v>>16; bp[0xCE]=v>>8; bp[0xCF]=v;    /* boot type: normal */
        v = 0u;        bp[0xD0]=0; bp[0xD1]=0; bp[0xD2]=0; bp[0xD3]=0;               /* version */
        v = 2u;        bp[0x38]=v>>24; bp[0x39]=v>>16; bp[0x3A]=v>>8; bp[0x3B]=v;    /* NTSC-U boot flag */
        v = 1u;        bp[0x2C]=v>>24; bp[0x2D]=v>>16; bp[0x2E]=v>>8; bp[0x2F]=v;    /* apploader/BI flag (0x800e3610 gate) */
        /* Channel error/state word at boot+0x30E6 (16-bit BE): the WC24 poll
         * treats 0x8200 as "still waiting" and anything else as reportable.
         * 0x9000 is the channel's own connect-error code (written at
         * 0x800e42c4 when a global error flag is set). Seeding it makes the
         * channel take its native WC24-failure path instead of polling
         * forever — the same behavior as a Wii with no WC24 service. */
        bp[0x30E6]=0x90; bp[0x30E7]=0x00;
        /* mem[0x80003118] = MEM2 size. The channel's console-state function
         * (0x800e3610 → 0x800e37c8) compares it against 0x04000000 and
         * returns error 0x10001010 unless it matches — i.e., it requires a
         * real Wii (64MB MEM2). Without this, the WC24 poll never yields a
         * usable state and the app thread never advances. */
        v = 0x04000000u; bp[0x3118]=v>>24; bp[0x3119]=v>>16; bp[0x311A]=v>>8; bp[0x311B]=v;
    }

    fc_cpu.pc = DOLRECOMP_ENTRY_POINT;      /* 0x8000403C */
    fc_cpu.gpr[1] = 0x803437A8;             /* SP from stage 5      */
    fc_cpu.gpr[2] = 0x80339900;             /* SDA2                 */
    fc_cpu.gpr[13] = 0x80337540;            /* SDA                  */
    return 0;
}

/* ---- diagnostics for JS ---- */
u32 fc_get_pc(void) { return fc_cpu.pc; }
u32 fc_get_last_pc(void) { return fc_last_dispatch_pc; }

/* ---- dispatch-pc histogram (sampled) for init-progress diagnostics ---- */
#define FC_HIST_BUCKETS 1024u
static u32 fc_hist_pc[FC_HIST_BUCKETS];
static u32 fc_hist_n[FC_HIST_BUCKETS];
static u32 fc_hist_used = 0;

static void fc_hist_record(u32 pc) {
    for (u32 i = 0; i < fc_hist_used; i++) {
        if (fc_hist_pc[i] == pc) { fc_hist_n[i]++; return; }
    }
    if (fc_hist_used < FC_HIST_BUCKETS) {
        fc_hist_pc[fc_hist_used] = pc;
        fc_hist_n[fc_hist_used] = 1;
        fc_hist_used++;
    }
}

/* ---- region-transition trace: log every hop between 64KB regions ---- */
#define FC_TRACE_MAX 512u
static u32 fc_trace_from[FC_TRACE_MAX];
static u32 fc_trace_to[FC_TRACE_MAX];
static u32 fc_trace_used = 0;
static u32 fc_trace_prev_region = 0xFFFFFFFFu;

static void fc_trace_record(u32 pc) {
    u32 region = pc >> 16;
    if (region != fc_trace_prev_region) {
        if (fc_trace_used < FC_TRACE_MAX) {
            fc_trace_from[fc_trace_used] = (fc_trace_prev_region << 16) | 0xFFFFu;
            fc_trace_to[fc_trace_used] = pc;
            fc_trace_used++;
        }
        fc_trace_prev_region = region;
    }
}

u32 fc_trace_count(void) { return fc_trace_used; }
u32 fc_trace_from_at(u32 i) { return i < fc_trace_used ? fc_trace_from[i] : 0; }
u32 fc_trace_to_at(u32 i)   { return i < fc_trace_used ? fc_trace_to[i] : 0; }

/* JS reads: returns bucket count, or pc/n at index */
u32 fc_hist_count(void) { return fc_hist_used; }
u32 fc_hist_pc_at(u32 i) { return i < fc_hist_used ? fc_hist_pc[i] : 0; }
u32 fc_hist_n_at(u32 i)  { return i < fc_hist_used ? fc_hist_n[i] : 0; }
void fc_hist_reset(void) {
    fc_hist_used = 0;
    for (u32 i = 0; i < FC_HIST_BUCKETS; i++) fc_hist_n[i] = 0;
}

/* run up to N blocks; returns 0 when the guest halted (exception/exit).
 * Custom loop over dolrecomp_call (instead of dolrecomp_run_blocks) so we can
 * watch pc per-block, detect spin loops, and keep the browser responsive. */
#define FC_SPIN_WINDOW 262144u  /* very lenient: legit SDK polling loops revisit pcs */

int fc_step(u32 max_blocks) {
    u32 blocks = 0;
    u32 spin_repeats = 0;
    u32 last_pc = 0;

    while (blocks < max_blocks) {
        fc_last_dispatch_pc = fc_cpu.pc;
        if ((blocks & 63u) == 0) fc_hist_record(fc_cpu.pc);
        fc_trace_record(fc_cpu.pc);
        if (!dolrecomp_call(&fc_cpu, fc_cpu.pc)) {
            EM_ASM({
                console.error('[fc_step] dispatch failed at pc=0x', ($0>>>0).toString(16));
            }, fc_cpu.pc);
            return 0;
        }
        if (fc_cpu.exception) {
            /* `sc` (system-call) is NOT fatal on Wii: the RVL SDK uses it as
             * a cache-sync barrier (DCFlushRange etc. end in sc; the OS
             * handler at vector 0x00C00 just resumes). Simulate that: clear
             * the exception and resume where the handler would return. */
            if (fc_cpu.exception == PPC_EXC_SYSTEM_CALL) {
                fc_cpu.exception = 0;
                fc_cpu.pc = fc_cpu.srr0;   /* set by ppc_take_exception = sc addr */
                fc_cpu.pc += 4;            /* resume past the sc instruction */
                continue;
            }
            EM_ASM({
                console.error('[fc_step] guest halted:',
                    'pc=0x', ($0>>>0).toString(16),
                    'exception=0x', ($1>>>0).toString(16),
                    'prog_exc=0x', ($2>>>0).toString(16),
                    'dar=0x', ($3>>>0).toString(16));
            }, fc_cpu.pc, fc_cpu.exception, fc_cpu.program_exception, fc_cpu.dar);
            return 0;
        }
        blocks++;

        /* Advance the guest timebase WITH the block stream so mftb-based
         * waits (the channel paces init/render in 100ms ticks) complete
         * mid-slice exactly like real hardware. Debug: temporarily very
         * fast to discriminate "slow timer" from "broken wait math". */
        fc_cpu.timebase += 4096;

        /* Spin-loop watchdog: if the same block pc comes back repeatedly,
         * the guest is waiting on something our HLE never provides. */
        if (fc_cpu.pc == last_pc) {
            spin_repeats++;
            if (spin_repeats == FC_SPIN_WINDOW) {
                EM_ASM({
                    console.warn('[fc_step] spin suspected at pc=0x',
                        ($0>>>0).toString(16), '— likely HLE-blocked; breaking slice');
                }, fc_cpu.pc);
                /* Break the slice: return alive so JS can keep scheduling,
                 * but stop burning CPU on a wait loop this frame. */
                return 1;
            }
        } else {
            spin_repeats = 0;
            last_pc = fc_cpu.pc;
        }
    }
    return 1;
}

/* ---- HLE surface is entirely implemented in hle_impl.c (strong defs) ---- */
/* No weak stubs here — hle_impl.c provides the only definitions. */