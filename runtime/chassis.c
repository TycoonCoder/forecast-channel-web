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

/* ---- main guest loop (called from JS every frame via requestAnimationFrame) ---- */
int fc_boot(const unsigned char* dol, unsigned len) {
    if (load_guest_image(dol, len)) return -1;
    memset(&fc_cpu, 0, sizeof(fc_cpu));
    
    /* Initialize DolRecomp CPU state - it manages its own RAM pointer */
    if (!cpu_init(&fc_cpu)) {
        EM_ASM({ console.error('[fc_boot] cpu_init failed'); });
        return -1;
    }
    
    /* Point DolRecomp's RAM to our guest memory */
    fc_cpu.ram = guest_mem;
    fc_cpu.ram_size = MEM1_SIZE;
    
    fc_cpu.pc = DOLRECOMP_ENTRY_POINT;      /* 0x8000403C */
    fc_cpu.gpr[1] = 0x803437A8;             /* SP from stage 5      */
    fc_cpu.gpr[2] = 0x80339900;             /* SDA2                 */
    fc_cpu.gpr[13] = 0x80337540;            /* SDA                  */
    return 0;
}

/* run up to N blocks; returns 0 when the guest halted (exception/exit) */
int fc_step(u32 max_blocks) {
    return dolrecomp_run_blocks(&fc_cpu, max_blocks);
}

/* ---- HLE surface is entirely implemented in hle_impl.c (strong defs) ---- */
/* No weak stubs here — hle_impl.c provides the only definitions. */