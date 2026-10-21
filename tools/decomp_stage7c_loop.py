#!/usr/bin/env python3
"""Stage 7c: find the channel's per-frame render loop.
Signature: functions that call MANY distinct SDK targets AND reference the
VI-retrace / thread-yield family, or that are called from the main loop's
callees (0x801768d4, 0x8017886c, 0x8017674c, 0x8017889c)."""
import struct, pathlib, bisect, json
from capstone import Cs, CS_ARCH_PPC, CS_MODE_32, CS_MODE_BIG_ENDIAN

DOL = pathlib.Path('/home/joshua/Desktop/Forecast channel/extracted/contents/0000000d.app')
dol = DOL.read_bytes()
text_offs = struct.unpack('>7I', dol[0x00:0x1C])
text_addrs = struct.unpack('>7I', dol[0x48:0x64])
text_sizes = struct.unpack('>7I', dol[0x90:0xAC])
sections = [(a, dol[o:o+s]) for o, a, s in zip(text_offs, text_addrs, text_sizes) if s]
protos = json.loads(pathlib.Path('/home/joshua/Desktop/forecast-channel-web/decomp/prototypes.json').read_text())
starts = sorted(int(p['addr'], 16) for p in protos)
proto_of = {int(p['addr'], 16): p for p in protos}

def func_of(a):
    return starts[bisect.bisect_right(starts, a) - 1]

md = Cs(CS_ARCH_PPC, CS_MODE_32 | CS_MODE_BIG_ENDIAN)
md.skipdata = True

# 1) who do the main-loop callees call?
main_callees = [0x801768d4, 0x8017886c, 0x8017674c, 0x8017889c]
def calls_of(fn):
    p = proto_of.get(fn)
    if not p: return []
    return [int(c, 16) for c in p.get('calls', [])]

print("main-loop callee -> their callees:")
for c in main_callees:
    sub = calls_of(c)
    print(f"  {c:#x} ({proto_of[c]['n_ins']} ins): {[hex(s) for s in sub[:12]]}")

# 2) hunt for the VI/vblank wait idiom: writes to SDA-relative retrace flag
#    or calls to functions containing 'sc' (syscall) with heavy GQR use.
#    Cheaper: find the biggest functions in the 0x80110000-0x80170000
#    channel-code band that call >=8 distinct SDK targets.
callers_sdk = {}
for base, code in sections:
    n = len(code)//4
    for i in range(n):
        w = struct.unpack('>I', code[i*4:i*4+4])[0]
        if (w >> 26) == 18 and (w & 1):
            li = w & 0x03FFFFFC
            if li & 0x02000000: li -= 0x04000000
            t = base + i*4 + li
            if 0x800e0000 <= t < 0x800f0000:
                src = func_of(base + i*4)
                callers_sdk.setdefault(src, set()).add(t)
band = [(fn, ts) for fn, ts in callers_sdk.items()
        if 0x80100000 <= fn < 0x80170000]
band.sort(key=lambda kv: -len(kv[1]))
print("\nchannel-band functions by distinct SDK calls:")
for fn, ts in band[:14]:
    p = proto_of.get(fn, {})
    print(f"  {fn:#x}: {len(ts)} SDK calls, {p.get('n_ins',0)} ins, size {p.get('size',0)}, callers {p.get('n_callers',0)}")
