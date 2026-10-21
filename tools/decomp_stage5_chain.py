#!/usr/bin/env python3
"""Stage 5 prep: dump the launch chain disassembly for hand translation."""
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
    print(f"===== {label} @ {addr:#x} ({size} bytes) =====")
    for insn in md.disasm(code[addr-base:addr-base+size], addr):
        raw = struct.unpack('>I', code[insn.address-base:insn.address-base+4])[0]
        print(f"{insn.address:#x}:  {raw:08x}  {insn.mnemonic} {insn.op_str}")
    print()

# launch chain from main's analysis:
dump(0x800041a8, 0x138, 'init_A')
dump(0x800042e0, 0x158, 'init_B')
dump(0x801783f8, 0x100, 'appMain_dispatch_target')
dump(0x80004028, 0x18, 'mode4_handler')
dump(0x80004238, 0xA0, 'init_C (called 3rd from main)')
