#!/usr/bin/env python3
"""Stage 1 (v2): raw-word PPC analysis of the Forecast Channel DOL.
No capstone dependency for coverage — decode opcodes directly so nothing
stops at bad words. Emits decomp/functions.json with real call graph,
string xrefs, and per-function disassembly (capstone, skipdata, optional)."""
import struct, pathlib, re, json, bisect

DOL = pathlib.Path('/home/joshua/Desktop/Forecast channel/extracted/contents/0000000d.app')
OUT = pathlib.Path('/home/joshua/Desktop/forecast-channel-web/decomp')
OUT.mkdir(parents=True, exist_ok=True)

dol = DOL.read_bytes()
text_offs = struct.unpack('>7I', dol[0x00:0x1C])
data_offs = struct.unpack('>11I', dol[0x1C:0x48])
text_addrs = struct.unpack('>7I', dol[0x48:0x64])
data_addrs = struct.unpack('>11I', dol[0x64:0x90])
text_sizes = struct.unpack('>7I', dol[0x90:0xAC])
data_sizes = struct.unpack('>11I', dol[0xAC:0xD8])
bss_addr, bss_size = struct.unpack('>II', dol[0xD8:0xE0])
entry = struct.unpack('>I', dol[0xE0:0xE4])[0]

sections = [(a, dol[o:o+s]) for o, a, s in zip(text_offs, text_addrs, text_sizes) if s]

# ---- strings at fixed addresses ----
strings = {}
for o, ad, s in zip(data_offs, data_addrs, data_sizes):
    if not s: continue
    blob = dol[o:o+s]
    for m in re.finditer(rb'[\x20-\x7e]{5,}\x00', blob):
        strings[ad + m.start()] = m.group()[:-1].decode('ascii', 'replace')

# ---- raw decode pass ----
starts = {entry}
callers = {}      # target -> [call sites]
xrefs = {}        # string addr -> [use sites]
MFLR = 0x7c0802a6
BLR = 0x4e800020
NOP = 0x60000000

for base, code in sections:
    n = len(code) // 4
    last_lis = {}
    prev_was_blr = False
    for i in range(n):
        w = struct.unpack('>I', code[i*4:i*4+4])[0]
        addr = base + i*4
        op = w >> 26
        if op == 18:  # b / bl
            li = w & 0x03FFFFFC
            if li & 0x02000000: li -= 0x04000000
            target = addr + li
            if w & 1:  # L bit = link
                callers.setdefault(target, []).append(addr)
                starts.add(target)
        elif op == 16:  # bc / bcl — PIC pointer thunk: bcl .+offset, cr0
            if (w & 0x0000FFFC) == 0x0000FFFC and (w & 3) == 1:
                pass  # bcl to next instr (get PC), not a function start
        elif op == 15:  # addis
            rD, rA = (w >> 21) & 0x1F, (w >> 16) & 0x1F
            if rA == 0 and rD != 0:
                last_lis[rD] = ((w & 0xFFFF) << 16, addr)
        elif op == 14:  # addi
            rD, rA = (w >> 21) & 0x1F, (w >> 16) & 0x1F
            if rA in last_lis and rA != 0:
                hi, _ = last_lis[rA]
                val = (hi + (w & 0xFFFF)) & 0xFFFFFFFF
                if val in strings:
                    xrefs.setdefault(val, []).append(addr)
                del last_lis[rA]  # consume
        elif op == 19:
            if w == MFLR>>26<<26: pass
        if w == MFLR:
            starts.add(addr)
        # function start heuristics: word after blr (skipping nops)
        if prev_was_blr and w != NOP and w != 0:
            starts.add(addr)
            prev_was_blr = False
        elif w == BLR:
            prev_was_blr = True
        else:
            prev_was_blr = False

starts = sorted(starts)
print(f"strings: {len(strings):,} | function starts: {len(starts):,} | string-xref sites: {len(xrefs):,}")

# ---- function table ----
max_end = max(base + len(code) for base, code in sections)
funcs = []
for i, s in enumerate(starts):
    end = starts[i+1] if i+1 < len(starts) else max_end
    funcs.append({'addr': s, 'end': end, 'size': end - s, 'callers': callers.get(s, [])})

# name functions by nearby strings
named = {}
for sa, refs in xrefs.items():
    for ref in refs:
        idx = bisect.bisect_right(starts, ref) - 1
        if idx >= 0:
            named.setdefault(starts[idx], []).append(strings[sa][:48])

total_code = max_end - sections[0][0]
print(f"total code span: {total_code:,} bytes; attributed to functions: {sum(f['size'] for f in funcs):,}")

# caller counts top
top_callers = sorted(funcs, key=lambda f: -len(f['callers']))[:12]
print("\nmost-called functions (hot SDK entry points):")
for f in top_callers:
    strs = named.get(f['addr'], [])
    print(f"  {f['addr']:#x}: {len(f['callers']):,} callers, {f['size']} bytes  {strs[:1]}")

out = {
    'meta': {
        'dol': 'contents/0000000d.app',
        'entry': hex(entry),
        'bss': hex(bss_addr), 'bss_size': bss_size,
        'sections': [
            {'kind': 'text' if k<7 else 'data', 'addr': hex(a), 'file_off': hex(o), 'size': s}
            for k,(o,a,s) in enumerate(list(zip(text_offs,text_addrs,text_sizes))+list(zip(data_offs,data_addrs,data_sizes))) if s
        ],
        'n_functions': len(funcs),
        'n_strings': len(strings),
    },
    'functions': [
        {
            'addr': hex(f['addr']), 'end': hex(f['end']), 'size': f['size'],
            'n_callers': len(f['callers']),
            'call_sites': [hex(c) for c in f['callers'][:20]],
            'strings_used': named.get(f['addr'], [])[:5],
        } for f in funcs
    ],
}
(OUT/'functions.json').write_text(json.dumps(out, indent=1))
(OUT/'symbols.json').write_text(json.dumps({hex(a): s for a, s in sorted(strings.items())}, indent=1))
print(f"\nwrote decomp/functions.json ({len(funcs):,} functions) and decomp/symbols.json ({len(strings):,} strings)")

# ---- per-function disassembly for the 40 most-referenced + 20 largest ----
from capstone import Cs, CS_ARCH_PPC, CS_MODE_32, CS_MODE_BIG_ENDIAN
md = Cs(CS_ARCH_PPC, CS_MODE_32 | CS_MODE_BIG_ENDIAN)
md.skipdata = True

def sec_of(addr):
    for base, code in sections:
        if base <= addr < base + len(code):
            return base, code
    return None, None

selected = sorted(funcs, key=lambda f: (-len(f['callers']), -f['size']))[:60]
asm_out = []
for f in selected:
    base, code = sec_of(f['addr'])
    if base is None: continue
    start = f['addr'] - base
    end = min(f['end'] - base, len(code))
    asm_out.append(f"/* {f['addr']:#x}  size={f['size']}  callers={len(f['callers'])}  strings={named.get(f['addr'],[])[:2]} */")
    for insn in md.disasm(code[start:end], f['addr']):
        raw = struct.unpack('>I', code[insn.address-base:insn.address-base+4])[0]
        asm_out.append(f"{insn.address:#x}:  {raw:08x}  {insn.mnemonic}\t{insn.op_str}")
    asm_out.append("")
(OUT/'hot_functions.asm').write_text("\n".join(asm_out))
print(f"wrote decomp/hot_functions.asm ({sum(1 for l in asm_out if ':' in l and '/*' not in l):,} instructions in top functions)")
