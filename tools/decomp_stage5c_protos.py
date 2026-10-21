#!/usr/bin/env python3
"""Stage 5c (v3): prototype recovery with a hand-rolled PPC opcode access
table (capstone 5.0.7 PPC operands lack .access). Reads/writes per raw
word for the ~40 common integer forms — enough for arity/return recovery."""
import struct, pathlib, json, time, bisect
from collections import Counter

DOL = pathlib.Path('/home/joshua/Desktop/Forecast channel/extracted/contents/0000000d.app')
OUT = pathlib.Path('/home/joshua/Desktop/forecast-channel-web/decomp')

dol = DOL.read_bytes()
text_offs = struct.unpack('>7I', dol[0x00:0x1C])
text_addrs = struct.unpack('>7I', dol[0x48:0x64])
text_sizes = struct.unpack('>7I', dol[0x90:0xAC])
data_offs = struct.unpack('>11I', dol[0x1C:0x48])
data_addrs = struct.unpack('>11I', dol[0x64:0x90])
data_sizes = struct.unpack('>11I', dol[0xAC:0xD8])
entry = struct.unpack('>I', dol[0xE0:0xE4])[0]
sections = [(a, dol[o:o+s]) for o, a, s in zip(text_offs, text_addrs, text_sizes) if s]

def access(w):
    """return (reads:set, writes:set) of GPR numbers for a raw PPC word"""
    op = w >> 26
    rD = (w >> 21) & 31
    rA = (w >> 16) & 31
    rB = (w >> 11) & 31
    reads, writes = set(), set()
    if op in (14, 15):                      # addi/addis
        writes.add(rD)
        if rA: reads.add(rA)
    elif op == 7:                           # mulli
        writes.add(rD); reads.update((rA, rB))
    elif op == 8:                           # subfic
        writes.add(rD); reads.add(rA)
    elif op in (10, 11, 12):                # cmpli/cmpi/andi./ori
        reads.add(rA)
        if op == 12: writes.add(rA)         # ori writes rA
    elif op == 13:                          # subis... (xoris)
        reads.add(rA); writes.add(rA)
    elif 20 <= op <= 31:                    # integer arith immediate-ext
        if op == 20:                        # rlwimi
            reads.update((rA,)); writes.add(rA); reads.add(rD)
        elif op in (21, 23, 24, 26, 27, 29, 30, 31):
            # rlwinm/rlwnm/slw/srw/etc: write rD, read rA(+rB for rlwnm)
            writes.add(rD); reads.add(rA)
            if op in (23, 30): reads.add(rB)
        elif op in (25, 28):                # or/xor variants
            pass
    elif op in (32, 34, 40, 42, 44 - 0 + 32*0):  # lwz/lbz/lhz/lha
        writes.add(rD); 
        if rA: reads.add(rA)
    elif op in (33, 35, 41, 43):            # update forms: write rD and rA
        writes.update((rD, rA)); reads.add(rA)
    elif op in (36, 38, 44, 46):            # stw/stb/sth
        reads.add(rD)                       # rS
        if rA: reads.add(rA)
    elif op == 37 or op == 45 or op == 39:  # stwu/sthu/stbu
        reads.add(rD)
        reads.add(rA); writes.add(rA)
    elif op == 47:                          # stmw
        for r in range(rD, 32): reads.add(r)
        if rA: reads.add(rA)
    elif op == 31:                          # extended integer
        sub = (w >> 1) & 0x3FF  # extended opcode (bits 21-30)
        sub10 = (w >> 1) & 0x3FF
        if sub10 in (444, 284, 316, 476, 124, 60, 28, 92, 412):  # or/eqv...
            writes.add(rA); reads.add(rD)
            if sub10 not in (124, 28, 92, 412, 60): pass
        elif sub10 in (0, 32):              # cmp/cmpl
            reads.update((rA, rB))
        elif sub10 in (266, 40, 24, 1078, 1042, 151, 183, 235, 725, 894, 922, 1194, 660, 954):
            writes.add(rD); reads.update((rA, rB))
        elif sub10 in (278, 86, 246, 533, 597, 727, 857, 949, 1014, 54, 1010, 966, 341, 836, 339, 727):
            pass
        elif sub10 == 83:                   # mfcr
            writes.add(rD)
        elif sub10 == 19:                   # mfcr alt
            writes.add(rD)
        elif sub10 in (467, 54):            # mtspr/... reads rS
            reads.add(rD)
        elif sub10 == 339:                  # mfspr: writes rD
            writes.add(rD)
        elif sub10 in (23, 55):             # lwzx/lbz
            writes.add(rD); reads.update((rA, rB))
        elif sub10 in (151, 235):           # stwx/sthx
            reads.add(rD); reads.update((rA, rB))
        elif sub10 in (26, 598):            # lwzx variants / ecowx
            writes.add(rD); reads.update((rA, rB))
        elif sub10 == 20:                   # lwarx
            writes.add(rD); reads.update((rA, rB))
        elif sub10 == 150:                  # stwcx
            reads.add(rD); reads.update((rA, rB))
        elif sub10 in (725, 922, 954, 661, 660):  # srw/sraw etc
            writes.add(rD); reads.update((rA, rB))
        # else: leave unknown (safe default: no access recorded)
    elif op == 19:                          # mfcr etc + traps
        sub = (w >> 1) & 0x3FF
        if sub == 83: writes.add(rD)
    # op 16/18 branches, 3 twi, 4 ps, 59/63 fp: no GPR param signal
    return reads, writes

t0 = time.time()
# ---- function starts ----
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
        if (w >> 26) == 18 and (w & 1):
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

max_end = max(base + len(c) for base, c in sections)
protos = []
for fi, s in enumerate(starts):
    e = starts[fi+1] if fi+1 < len(starts) else max_end
    # find containing section
    base = next((b for b, c in sections if b <= s < b + len(c)), None)
    if base is None: continue
    code = next(c for b, c in sections if b == base)
    n_ins = 0
    frame = 0
    params = []
    defined = {1, 2, 13}
    writes_r3 = False
    leaf = True
    i0 = (s - base) // 4
    i1 = (e - base) // 4
    for i in range(i0, min(i1, len(code)//4)):
        w = struct.unpack('>I', code[i*4:i*4+4])[0]
        n_ins += 1
        reads, writes = access(w)
        for r in reads:
            if r not in defined and 3 <= r <= 10:
                params.append(r); defined.add(r)
        for r in writes:
            defined.add(r)
            if r == 3: writes_r3 = True
        if (w >> 26) == 18 and (w & 1): leaf = False
        if frame == 0 and (w >> 26) == 37 and ((w >> 21) & 31) == 1 and ((w >> 16) & 31) == 1:
            imm = w & 0xFFFF
            frame = 0x10000 - imm if imm & 0x8000 else imm
    protos.append({
        'addr': hex(s), 'size': e - s, 'n_ins': n_ins,
        'n_callers': len(callers.get(s, [])),
        'params': sorted(set(params)), 'arity': len(set(params)),
        'returns': writes_r3, 'leaf': leaf, 'frame': frame,
    })

(OUT/'prototypes.json').write_text(json.dumps(protos, indent=0, separators=(',', ':')))
ar = Counter(p['arity'] for p in protos)
print(f"prototypes: {len(protos):,} ({time.time()-t0:.1f}s)")
print("arity:", dict(sorted(ar.items())))
print(f"returns r3: {sum(1 for p in protos if p['returns']):,}")
print(f"frames: {sum(1 for p in protos if p['frame']):,}")
print(f"leaves: {sum(1 for p in protos if p['leaf']):,}")
# known-function sanity checks
known = {
    0x800ea948: ('OSDisableInterrupts', 0, True),
    0x800ea970: ('OSRestoreInterrupts', 1, True),
    0x80004338: ('memmove', 3, True),
    0x80004028: ('SetSpecialBootFlag', 0, False),
    0x80004388: ('memset', 3, True),
}
for a, (name, exp_arity, exp_ret) in known.items():
    p = next((q for q in protos if int(q['addr'], 16) == a), None)
    if p:
        ok = (p['arity'] == exp_arity or (exp_arity==0 and p['arity']<=1)) and p['returns'] == exp_ret
        print(f"  {'OK ' if ok else '?? '}{name}: params={p['params']} arity={p['arity']} ret={p['returns']} frame={p['frame']}")
    else:
        print(f"  MISSING {name}")
