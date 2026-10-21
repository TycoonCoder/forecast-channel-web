#!/usr/bin/env python3
"""Stage 6b: the real main loop (0x80178bc4), arg-0 path (0x80178b98),
and mode-1 handler callees."""
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

dump(0x80178b98, 0x2c, 'arg0_or_fallthrough')
dump(0x80178bc4, 0x100, 'MAIN_LOOP')
dump(0x801608d0, 0x40, 'callee of 0x80178cb0')
dump(0x80160874, 0x5c, 'mode1-handler-0 callee')
dump(0x80179180, 0x80, 'mode1-handler-0 worker (0x500-byte op)')
