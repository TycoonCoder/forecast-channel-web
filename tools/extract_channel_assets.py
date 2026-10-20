#!/usr/bin/env python3
"""Extract REAL channel assets: WiiNTLG font subset + banner boot jingle."""
import struct, pathlib, sys
import numpy as np
from PIL import Image

SRC = pathlib.Path('/home/joshua/Desktop/Forecast channel/extracted')
PROJ = pathlib.Path('/home/joshua/Desktop/forecast-channel-web')
FONTS = PROJ / 'fonts'
FONTS.mkdir(parents=True, exist_ok=True)

# ---------- 1) font ----------
from fontTools.ttLib import TTCollection
ttc = TTCollection(SRC/'u8'/'00000003'/'WiiNTLG-Regular.ttc')
font = ttc.fonts[0]
print("font full name:", font['name'].getDebugName(4), "| glyphs:", font['maxp'].numGlyphs)
try:
    from fontTools.subset import Subsetter, Options
    opt = Options()
    opt.flavor = 'woff2'
    text = ("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"
            "0123456789 .,;:!?-_~'\"()[]{}°%±·|/\\+*=<>#&@$€£¥©®™"
            "éèêëàáâäãçìíîïòóôõöùúûüñÉÈÊÀÁÂÄÃÇÌÍÎÏÒÓÔÕÖÙÚÛÜÑ")
    ss = Subsetter(opt)
    ss.populate(text=text)
    ss.subset(font)
    font.flavor = 'woff2'
    font.save(FONTS/'wiintlg.woff2')
    print("saved fonts/wiintlg.woff2:", (FONTS/'wiintlg.woff2').stat().st_size, "bytes")
except Exception as e:
    print("subset failed:", e, "-> saving full TTF")
    font.flavor = None
    font.save(FONTS/'wiintlg.ttf')
    print("saved fonts/wiintlg.ttf:", (FONTS/'wiintlg.ttf').stat().st_size, "bytes")

# ---------- 2) boot jingle from banner sound.bin (DSP-ADPCM) ----------
sb = (SRC/'u8'/'0000000c'/'banner'/'meta'/'sound.bin').read_bytes()
print("\nsound.bin:", len(sb), "bytes, magic:", sb[:4])
for off in range(0, 0x20, 16):
    print(f"  {off:02x}:", ' '.join(f'{b:02x}' for b in sb[off:off+16]))

# 'BNRL' header: 0x00 magic, ... DSP header typically at 0x60
DSP = 0x60
coefs = [struct.unpack('>h', sb[DSP+i*2:DSP+i*2+2])[0] for i in range(16)]
n_samples = struct.unpack('>I', sb[DSP+4:DSP+8])[0] or struct.unpack('>I', sb[DSP+0x1C:DSP+0x20])[0]
rate = struct.unpack('>I', sb[DSP+8:DSP+12])[0]
loop_start = struct.unpack('>I', sb[DSP+0x18:DSP+0x1C])[0]
print(f"DSP@0x60: rate={rate} n_samples={n_samples} loop_start={loop_start}")
print("coefs:", coefs)

data = sb[DSP+0x60:]
# proper Yamaha DSP-ADPCM decode: 8-byte frames (1 header + 7 data bytes = 14 samples)
def decode_dsp(buf, coefs, n_samples):
    out = np.zeros(n_samples, np.int16)
    h1 = h2 = 0
    si = 0
    frames = (n_samples + 13) // 14
    for f in range(frames):
        h = buf[f*8]
        scale = 1 << (h & 0xF)
        ci = (h >> 4) & 0xF
        c1, c2 = coefs[ci*2] if ci*2 < 16 else 0, coefs[ci*2+1] if ci*2+1 < 16 else 0
        for b in range(7):
            byte = buf[f*8+1+b]
            for half in range(2):
                if si >= n_samples: break
                nib = (byte >> 4) & 0xF if half == 0 else byte & 0xF
                s = nib - 16 if nib >= 8 else nib
                pred = ((h1 * c1 + h2 * c2) >> 11) + s * scale
                pred = max(-32768, min(32767, pred))
                out[si] = pred
                h2, h1 = h1, pred
                si += 1
    return out

samples = decode_dsp(data, coefs, n_samples)
peak = np.abs(samples.astype(int)).max()
print(f"decoded {n_samples} samples = {n_samples/rate:.2f}s @ {rate}Hz, peak={peak}")
if peak < 64:
    print("WARNING: peak suspiciously low; header guess may be off")
import wave
with wave.open(str(PROJ/'assets'/'boot_jingle.wav'), 'wb') as w:
    w.setnchannels(1); w.setsampwidth(2); w.setframerate(rate)
    w.writeframes(samples.tobytes())
print("saved assets/boot_jingle.wav:", (PROJ/'assets'/'boot_jingle.wav').stat().st_size, "bytes")

# trim trailing silence
arr = samples.astype(np.int32)
nz = np.where(np.abs(arr) > 40)[0]
if len(nz):
    trimmed = samples[:nz[-1]+rate//20]
    with wave.open(str(PROJ/'assets'/'boot_jingle.wav'), 'wb') as w:
        w.setnchannels(1); w.setsampwidth(2); w.setframerate(rate)
        w.writeframes(trimmed.tobytes())
    print(f"trimmed to {len(trimmed)/rate:.2f}s")
