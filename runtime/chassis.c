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
 * The channel pokes real hardware: DSP/audio DMA (0xCD006xxx), EXI
 * (0xCD8001xx), AI (0xCD0000C0), VI (0xCD0000xx), GX FIFO (0xCC000000).
 * On a Wii these carry data; in the browser we model the minimal
 * semantics the channel needs: reads report idle/done, writes sink. */
static u64 fc_external_read(CPUState* cpu, u32 ea, u8 size) {
    (void)cpu; (void)size;
    static u32 logged = 0;
    if (logged < 40) {
        EM_ASM({ console.log('[mmio] read', $0 ? '32' : '8', ' 0x', ($1>>>0).toString(16)); }, size == 32, ea);
        logged++;
    }
    /* DSP DMA status at 0xCD00680C: bit0 = busy → 0 means transfer done.
     * Everything else reads idle/absent. */
    return 0;
}

static void fc_external_write(CPUState* cpu, u32 ea, u64 value, u8 size) {
    (void)cpu; (void)value; (void)size;
    static u32 logged = 0;
    if (logged < 40) {
        EM_ASM({ console.log('[mmio] write 0x', ($0>>>0).toString(16), ' ← 0x', ($1>>>0).toString(16)); }, ea, value);
        logged++;
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

    fc_cpu.pc = DOLRECOMP_ENTRY_POINT;      /* 0x8000403C */
    fc_cpu.gpr[1] = 0x803437A8;             /* SP from stage 5      */
    fc_cpu.gpr[2] = 0x80339900;             /* SDA2                 */
    fc_cpu.gpr[13] = 0x80337540;            /* SDA                  */
    return 0;
}

/* run up to N blocks; returns 0 when the guest halted (exception/exit).
 * Custom loop over dolrecomp_call (instead of dolrecomp_run_blocks) so we can
 * watch pc per-block, detect spin loops, and keep the browser responsive. */
#define FC_SPIN_WINDOW 262144u  /* very lenient: legit SDK polling loops revisit pcs */

int fc_step(u32 max_blocks) {
    fc_cpu.timebase += FC_TB_PER_STEP;

    u32 blocks = 0;
    u32 spin_repeats = 0;
    u32 last_pc = 0;

    while (blocks < max_blocks) {
        if (!dolrecomp_call(&fc_cpu, fc_cpu.pc)) {
            EM_ASM({
                console.error('[fc_step] dispatch failed at pc=0x', ($0>>>0).toString(16));
            }, fc_cpu.pc);
            return 0;
        }
        if (fc_cpu.exception) {
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