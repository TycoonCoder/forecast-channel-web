#!/usr/bin/env python3
"""Stage 2: disassemble entry point + hottest functions; identify SDK signatures."""
import struct, pathlib
from capstone import Cs, CS_ARCH_PPC, CS_MODE_32, CS_MODE_BIG_ENDIAN

DOL = pathlib.Path('/home/joshua/Desktop/Forecast channel/extracted/contents/0000000d.app')
dol = DOL.read_bytes()
text_offs = struct.unpack('>7I', dol[0x00:0x1C])
text_addrs = struct.unpack('>7I', dol[0x48:0x64])
text_sizes = struct.unpack('>7I', dol[0x90:0xAC])
entry = struct.unpack('>I', dol[0xE0:0xE4])[0]

sections = [(a, dol[o:o+s]) for o, a, s in zip(text_offs, text_addrs, text_sizes) if s]
md = Cs(CS_ARCH_PPC, CS_MODE_32 | CS_MODE_BIG_ENDIAN)
md.skipdata = True

def disasm_range(addr, size, label=''):
    base, code = next(((b, c) for b, c in sections if b <= addr < b + len(c)), (None, None))
    assert base is not None, hex(addr)
    lines = []
    if label: lines.append(f"/* {label} @ {addr:#x}, {size} bytes */")
    for insn in md.disasm(code[addr-base:addr-base+size], addr):
        raw = struct.unpack('>I', code[insn.address-base:insn.address-base+4])[0]
        lines.append(f"{insn.address:#x}:  {raw:08x}  {insn.mnemonic} {insn.op_str}")
    return lines

print("=" * 62)
print("ENTRY POINT (main) — full disassembly:")
print("=" * 62)
print("\n".join(disasm_range(entry, 0xC0, 'main')))

print()
print("=" * 62)
print("HOTTEST FUNCTIONS — SDK signature candidates:")
print("=" * 62)
for addr, size, note in [
    (0x800ea970, 36, '922 callers'),
    (0x800ea948, 20, '751 callers'),
    (0x80004338, 44, '366 callers'),
    (0x800e675c, 4, '313 callers'),
    (0x801619c4, 24, '287 callers'),
    (0x80161a10, 24, '287 callers'),
    (0x8000443c, 4, '266 callers'),
    (0x8016184c, 40, '191 callers'),
]:
    print()
    print("\n".join(disasm_range(addr, size, f'hot: {note}')))
