#!/usr/bin/env python3
"""Stage 6: dump the 9+9 dispatch handlers from thread_entry's tables."""
import struct, pathlib
from capstone import Cs, CS_ARCH_PPC, CS_MODE_32, CS_MODE_BIG_ENDIAN

DOL = pathlib.Path('/home/joshua/Desktop/Forecast channel/extracted/contents/0000000d.app')
dol = DOL.read_bytes()
text_offs = struct.unpack('>7I', dol[0x00:0x1C])
text_addrs = struct.unpack('>7I', dol[0x48:0x64])
text_sizes = struct.unpack('>7I', dol[0x90:0xAC])
sections = [(a, dol[o:o+s]) for o, a, s in zip(text_offs, text_addrs, text_sizes) if s]

md = Cs(CS_ARCH_PPC, CS_MODE_32 | CS_MODE_BIG_ENDIAN)
md.skipdata = True

def dump(addr, size, label):
    base, code = next((b, c) for b, c in sections if b <= addr < b + len(c))
    print(f"===== {label} @ {addr:#x} =====")
    for insn in md.disasm(code[addr-base:addr-base+size], addr):
        raw = struct.unpack('>I', code[insn.address-base:insn.address-base+4])[0]
        print(f"{insn.address:#x}:  {raw:08x}  {insn.mnemonic} {insn.op_str}")
    print()

# mode-2 (from System Menu) handler set
mode2 = [0x80178CA8, 0x80178CA0, 0x80178C98, 0x80178C90, 0x80178C88,
         0x80178C80, 0x80178C78, 0x80178C70, 0x80178C68]
for i, a in enumerate(mode2):
    dump(a, 0x30, f'mode2 handler {i} (slot {"%x" % [0,8,0xc,0x10,0x14,0x18,0x1c,0x20,0x24][i]})')

# mode-1 (returning) handler set
mode1 = [0x80178EE8, 0x80178EBC, 0x80178EB4, 0x80178E00, 0x80178D8C,
         0x80178E20, 0x80178D34, 0x80178D68, 0x80178D44]
for i, a in enumerate(mode1):
    dump(a, 0x40, f'mode1 handler {i}')
