#!/usr/bin/env python3
"""Stage 4+5: full disassembly corpus + automatic prototype recovery.

One detail-mode capstone pass over the whole text section, then per-function:
  - stack frame size (stwu r1, -X)
  - callee-saved registers (stw rX / stmw)
  - parameter arity (GPR reads-before-write on entry, r3-r10)
  - return value (r3 written before exit)
  - leaf detection (no calls)
Emits:
  decomp/asm/asm_partN.txt   (full corpus, chunked)
  decomp/prototypes.json     (9,573 recovered signatures)
"""
import struct, pathlib, json, re, bisect, sys, time
from capstone import Cs, CS_ARCH_PPC, CS_MODE_32, CS_MODE_BIG_ENDIAN, CS_AC_READ
from capstone.ppc import *

DOL = pathlib.Path('/home/joshua/Desktop/Forecast channel/extracted/contents/0000000d.app')
OUT = pathlib.Path('/home/joshua/Desktop/forecast-channel-web/decomp')
ASM = OUT / 'asm'
ASM.mkdir(parents=True, exist_ok=True)

dol = DOL.read_bytes()
text_offs = struct.unpack('>7I', dol[0x00:0x1C])
data_offs = struct.unpack('>11I', dol[0x1C:0x48])
text_addrs = struct.unpack('>7I', dol[0x48:0x64])
data_addrs = struct.unpack('>11I', dol[0x64:0x90])
text_sizes = struct.unpack('>7I', dol[0x90:0xAC])
data_sizes = struct.unpack('>11I', dol[0xAC:0xD8])
entry = struct.unpack('>I', dol[0xE0:0xE4])[0]

sections = [(a, dol[o:o+s]) for o, a, s in zip(text_offs, text_addrs, text_sizes) if s]

# strings (for name hints)
strings = {}
for o, ad, s in zip(data_offs, data_addrs, data_sizes):
    if not s: continue
    for m in re.finditer(rb'[\x20-\x7e]{5,}\x00', dol[o:o+s]):
        strings[ad + m.start()] = m.group()[:-1].decode('ascii', 'replace')

# ---- function starts (stage-1 algorithm, raw words) ----
t0 = time.time()
starts = {entry}
callers = {}
MFLR = 0x7c0802a6
BLR = 0x4e800020
NOP = 0x60000000
for base, code in sections:
    n = len(code) // 4
    prev_blr = False
    for i in range(n):
        w = struct.unpack('>I', code[i*4:i*4+4])[0]
        addr = base + i*4
        op = w >> 26
        if op == 18 and (w & 1):
            li = w & 0x03FFFFFC
            if li & 0x02000000: li -= 0x04000000
            callers.setdefault(addr + li, []).append(addr)
            starts.add(addr + li)
        if w == MFLR: starts.add(addr)
        if prev_blr and w != NOP and w != 0:
            starts.add(addr); prev_blr = False
        elif w == BLR: prev_blr = True
        else: prev_blr = False
starts = sorted(starts)
print(f"starts: {len(starts):,} ({time.time()-t0:.1f}s)")

# ---- one detail pass ----
GPR_ID = {}
for i in range(32):
    GPR_ID[getattr(__import__('capstone.ppc', fromlist=['x']), f'PPC_REG_R{i}')] = i

md = Cs(CS_ARCH_PPC, CS_MODE_32 | CS_MODE_BIG_ENDIAN)
md.skipdata = True
md.detail = True

insns = []   # (addr, mnemonic, op_str, reads(frozenset gpr#), writes(fset), word)
for base, code in sections:
    for insn in md.disasm(code, base):
        raw = struct.unpack('>I', code[insn.address-base:insn.address-base+4])[0]
        if insn.id == 0:  # SKIPDATA pseudo-instruction: no reg info
            insns.append((insn.address, insn.mnemonic, insn.op_str, frozenset(), frozenset(), raw))
            continue
        reads = frozenset(GPR_ID[r] for r in insn.regs_read if r in GPR_ID)
        writes = frozenset(GPR_ID[r] for r in insn.regs_write if r in GPR_ID)
        insns.append((insn.address, insn.mnemonic, insn.op_str, reads, writes, raw))
print(f"disassembled: {len(insns):,} instructions ({time.time()-t0:.1f}s)")

# index instructions by address for function slicing
addr_idx = {ins[0]: i for i, ins in enumerate(insns)}
max_end = max(base + len(c) for base, c in sections)
end_of = []
for i, s in enumerate(starts):
    end_of.append(starts[i+1] if i+1 < len(starts) else max_end)

# ---- per-function analysis ----
protos = []
asm_lines = []          # full corpus text
call_targets_count = 0
for fi, s in enumerate(starts):
    e = end_of[fi]
    i0 = addr_idx.get(s)
    if i0 is None: continue
    frame = 0
    saves = []
    params = set()
    defined = {1, 2, 13}        # SP, SDA, SDA2 never params
    writes_r3 = False
    calls = []
    leaf = True
    fn_lines = []
    i = i0
    while i < len(insns):
        addr, mn, ops, reads, writes, raw = insns[i]
        if addr >= e: break
        for r in reads:
            if r not in defined and 3 <= r <= 10:
                params.add(r)
        for r in writes:
            defined.add(r)
            if r == 3: writes_r3 = True
        # stack frame from stwu r1, -X(r1)
        if frame == 0 and (raw >> 26) == 37 and ((raw >> 21) & 0x1F) == 1 and ((raw >> 16) & 0x1F) == 1:
            imm = raw & 0xFFFF
            if imm & 0x8000: frame = 0x10000 - imm
        # callee saves: stw rX, off(r1) in first 32 insns, or stmw
        if len(fn_lines) < 64:
            if (raw >> 26) == 36 and ((raw >> 16) & 0x1F) == 1:  # stw rX, d(r1)
                saves.append(((raw >> 21) & 0x1F))
            if (raw >> 26) == 47:  # stmw rX..r31
                saves.extend(range((raw >> 21) & 0x1F, 32))
        if mn == 'bl':
            try: calls.append(int(ops, 16))
            except ValueError: calls.append(0)
            leaf = False
        fn_lines.append(f"{addr:#x}:  {raw:08x}  {mn} {ops}")
        i += 1
    asm_lines.extend(fn_lines)
    asm_lines.append("")
    arity = len([p for p in sorted(params) if p <= 3 + max(params, default=3) - 3]) if params else 0
    protos.append({
        'addr': hex(s), 'size': e - s, 'n_callers': len(callers.get(s, [])),
        'frame': frame, 'saves': sorted(set(saves) - {0, 1, 2, 13}),
        'params': sorted(params), 'arity': len(params),
        'returns': writes_r3, 'leaf': leaf,
        'calls': [hex(c) for c in calls[:24]],
    })

print(f"analyzed {len(protos):,} functions ({time.time()-t0:.1f}s)")

# ---- write corpus chunked ----
CHUNKS = 8
per = (len(asm_lines) + CHUNKS - 1) // CHUNKS
for ci in range(CHUNKS):
    part = asm_lines[ci*per:(ci+1)*per]
    (ASM / f'asm_part{ci+1}.txt').write_text("\n".join(part))
print(f"asm corpus: {CHUNKS} files, {len(asm_lines):,} lines")

(OUT/'prototypes.json').write_text(json.dumps(protos, indent=0, separators=(',', ':')))

# ---- stats ----
with_frames = sum(1 for p in protos if p['frame'])
with_params = sum(1 for p in protos if p['arity'])
leaves = sum(1 for p in protos if p['leaf'])
ret = sum(1 for p in protos if p['returns'])
print(f"\nframes recovered: {with_frames:,}/{len(protos):,}")
print(f"non-void (write r3): {ret:,}   leaves: {leaves:,}")
print(f"arity distribution:")
from collections import Counter
ar = Counter(p['arity'] for p in protos)
for k in sorted(ar): print(f"  {k} params: {ar[k]:,} functions")

# ---- dump functions for hand-translation ----
def dump(addr, size, label):
    base, code = next((b, c) for b, c in sections if b <= addr < b + len(c))
    md2 = Cs(CS_ARCH_PPC, CS_MODE_32 | CS_MODE_BIG_ENDIAN); md2.skipdata = True
    print(f"\n===== {label} @ {addr:#x} ({size} bytes) =====")
    for insn in md2.disasm(code[addr-base:addr-base+size], addr):
        raw = struct.unpack('>I', code[insn.address-base:insn.address-base+4])[0]
        print(f"{insn.address:#x}:  {raw:08x}  {insn.mnemonic} {insn.op_str}")

dump(entry, 0xB0, 'main entry')
dump(0x801783f8, 0x120, 'appMain (dispatch target from main)')
dump(0x800041a8, 0x100, 'init A (called first from main)')
dump(0x800042e0, 0x100, 'init B (called second from main)')
dump(0x80004028, 0x20, 'mode-4 handler')
print(f"\ntotal time {time.time()-t0:.1f}s")
