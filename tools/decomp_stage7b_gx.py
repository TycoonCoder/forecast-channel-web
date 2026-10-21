#!/usr/bin/env python3
"""Stage 7b: SDK tags live in TEXT (OSReport immediates); locate them by
scanning the whole DOL. Then dump the top SDK-orchestrators."""
import struct, pathlib, re, bisect, json
from capstone import Cs, CS_ARCH_PPC, CS_MODE_32, CS_MODE_BIG_ENDIAN

DOL = pathlib.Path('/home/joshua/Desktop/Forecast channel/extracted/contents/0000000d.app')
dol = DOL.read_bytes()
text_offs = struct.unpack('>7I', dol[0x00:0x1C])
text_addrs = struct.unpack('>7I', dol[0x48:0x64])
text_sizes = struct.unpack('>7I', dol[0x90:0xAC])
sections = [(a, dol[o:o+s]) for o, a, s in zip(text_offs, text_addrs, text_sizes) if s]

# scan whole file for SDK tags with file->address translation
tag_sites = {}
for base, code in sections:
    for m in re.finditer(rb'<< RVL_SDK - ([A-Z0-9]+)', code):
        tag_sites.setdefault(m.group(1).decode(), []).append(base + m.start())
print("SDK tags in text:", {k: [hex(x) for x in v[:3]] for k, v in sorted(tag_sites.items())})

protos = json.loads(pathlib.Path('/home/joshua/Desktop/forecast-channel-web/decomp/prototypes.json').read_text())
starts = sorted(int(p['addr'], 16) for p in protos)

md = Cs(CS_ARCH_PPC, CS_MODE_32 | CS_MODE_BIG_ENDIAN)
md.skipdata = True

def dump(addr, size, label):
    base, code = next((b, c) for b, c in sections if b <= addr < b + len(c))
    print(f"===== {label} @ {addr:#x} =====")
    for insn in md.disasm(code[addr-base:addr-base+size], addr):
        raw = struct.unpack('>I', code[insn.address-base:insn.address-base+4])[0]
        print(f"{insn.address:#x}:  {raw:08x}  {insn.mnemonic} {insn.op_str}")
    print()

# the function CONTAINING each tag is the SDK module's OSReport site.
for tag in ('GX', 'AX', 'NWC24', 'WPAD'):
    if tag not in tag_sites: continue
    site = tag_sites[tag][0]
    fn = starts[bisect.bisect_right(starts, site) - 1]
    print(f"{tag}: tag at {site:#x} -> inside function {fn:#x}")
    dump(fn, 0x40, f'{tag} module fn')

# top SDK orchestrator + the channel's own SDK-heavy function
dump(0x800e3f24, 0xa0, 'SDK orchestrator (39 calls)')
dump(0x8002f3d4, 0xc0, 'channel SDK-heavy function (12 calls)')
