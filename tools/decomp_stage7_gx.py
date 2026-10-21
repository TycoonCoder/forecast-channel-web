#!/usr/bin/env python3
"""Stage 7: locate the GX/AX module tag strings and map the SDK function
blocks around them; find the channel's render-loop calls into GX."""
import struct, pathlib, re, json, bisect
from capstone import Cs, CS_ARCH_PPC, CS_MODE_32, CS_MODE_BIG_ENDIAN

DOL = pathlib.Path('/home/joshua/Desktop/Forecast channel/extracted/contents/0000000d.app')
dol = DOL.read_bytes()
text_offs = struct.unpack('>7I', dol[0x00:0x1C])
data_offs = struct.unpack('>11I', dol[0x1C:0x48])
text_addrs = struct.unpack('>7I', dol[0x48:0x64])
data_addrs = struct.unpack('>11I', dol[0x64:0x90])
text_sizes = struct.unpack('>7I', dol[0x90:0xAC])
data_sizes = struct.unpack('>11I', dol[0xAC:0xD8])
sections = [(a, dol[o:o+s]) for o, a, s in zip(text_offs, text_addrs, text_sizes) if s]

# locate SDK tags in data
tag_addrs = {}
for o, ad, s in zip(data_offs, data_addrs, data_sizes):
    if not s: continue
    blob = dol[o:o+s]
    for m in re.finditer(rb'<<\s*RVL_SDK\s*-\s*([A-Z0-9]+)\s*>>?', blob):
        tag_addrs.setdefault(m.group(1).decode(), []).append(ad + m.start())
print("SDK tags found:", {k: hex(v[0]) for k, v in tag_addrs.items()})

# GX module: functions that reference the GX tag string. SDK modules print
# their tag via OSReport in OSInit-style debug; the tag xref marks the
# module's init entry. Find lis/addi xrefs to each tag address.
xref_sites = {}
for base, code in sections:
    last_lis = {}
    n = len(code)//4
    for i in range(n):
        w = struct.unpack('>I', code[i*4:i*4+4])[0]
        op = w >> 26
        addr = base + i*4
        if op == 15 and ((w >> 16) & 0x1F) == 0:
            last_lis[(w >> 21) & 0x1F] = (w & 0xFFFF) << 16
        elif op == 14:
            rA = (w >> 16) & 0x1F
            if rA in last_lis:
                val = (last_lis[rA] + (w & 0xFFFF)) & 0xFFFFFFFF
                for tag, addrs in tag_addrs.items():
                    if val in addrs:
                        xref_sites.setdefault(tag, []).append(addr)
                del last_lis[rA]
for tag, sites in xref_sites.items():
    print(f"{tag}: xrefs at {[hex(s) for s in sites[:6]]}")

# The SDK function that references its own tag is the module init/debug fn.
# SDK function blocks are laid out in module order; each module's init is
# near its tag-xref. Dump the function containing each xref site.
starts = {int(a, 16) for a in json.loads(pathlib.Path('/home/joshua/Desktop/forecast-channel-web/decomp/prototypes.json').read_text()).__iter__().__next__()} if False else None
protos = json.loads(pathlib.Path('/home/joshua/Desktop/forecast-channel-web/decomp/prototypes.json').read_text())
starts = sorted(int(p['addr'], 16) for p in protos)

md = Cs(CS_ARCH_PPC, CS_MODE_32 | CS_MODE_BIG_ENDIAN)
md.skipdata = True

def func_of(addr):
    i = bisect.bisect_right(starts, addr) - 1
    return starts[i]

def dump(addr, size, label):
    base, code = next((b, c) for b, c in sections if b <= addr < b + len(c))
    print(f"===== {label} @ {addr:#x} =====")
    for insn in md.disasm(code[addr-base:addr-base+size], addr):
        raw = struct.unpack('>I', code[insn.address-base:insn.address-base+4])[0]
        print(f"{insn.address:#x}:  {raw:08x}  {insn.mnemonic} {insn.op_str}")
    print()

# show the tag-referencing function for GX and AX
for tag in ('GX', 'AX', 'OS', 'NWC24'):
    if tag not in xref_sites: continue
    fn = func_of(xref_sites[tag][0])
    print(f"{tag} module init candidate: function @ {fn:#x}")
    dump(fn, 0x50, f'{tag} init')

# Also: find calls to the GX-heavy neighborhood — functions whose bodies
# contain many 'bl' into the 0x800e0000-0x800f0000 SDK block are channel
# render calls. Top callers into that block:
callers_sdk = {}
for base, code in sections:
    n = len(code)//4
    for i in range(n):
        w = struct.unpack('>I', code[i*4:i*4+4])[0]
        if (w >> 26) == 18 and (w & 1):
            li = w & 0x03FFFFFC
            if li & 0x02000000: li -= 0x04000000
            target = base + i*4 + li
            if 0x800e0000 <= target < 0x800f0000:
                src = base + i*4
                callers_sdk.setdefault(func_of(src), set()).add(target)
top = sorted(callers_sdk.items(), key=lambda kv: -len(kv[1]))[:10]
print("functions calling most distinct SDK targets (0x800e0000-0x800f0000):")
for fn, targets in top:
    print(f"  {fn:#x}: {len(targets)} distinct SDK calls")
