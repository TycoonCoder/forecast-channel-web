#!/usr/bin/env python3
"""ab_icons.py — A/B the old (buggy) vs new (fixed) RGB5A3 decode on the
actual sun TPL: count yellow pixels under each. Old bug: opaque pixels
decoded as RGB565 (reading the alpha-flag bit into red, 6-bit green)."""
import sys
from pathlib import Path
from PIL import Image

sys.path.insert(0, "/home/joshua/Desktop/Forecast channel/tools")
from tpl_to_png import decode_texture, rgb565  # noqa: E402

TPL = Path("/home/joshua/Desktop/Forecast channel/extracted/u8/banner/icon/arc/timg/icon_W_sun_00.tpl")


def old_rgb5a3(value):
    if value & 0x8000:
        return rgb565(value)  # the bug
    return (((value >> 8) & 0x0F) * 17, ((value >> 4) & 0x0F) * 17,
            (value & 0x0F) * 17, ((value >> 12) & 0x7) * 255 // 7)


def count_yellow(img):
    px = list(img.getdata())
    n = len(px)
    y = sum(1 for r, g, b, a in px if a > 128 and r > 150 and g > 120 and b < 120)
    red = sum(1 for r, g, b, a in px if a > 128 and r > 150 and g < 120 and b < 120)
    return y / n, red / n


blob = TPL.read_bytes()
table = int.from_bytes(blob[8:12], "big")
tex_off = int.from_bytes(blob[table:table + 4], "big")

# NEW decode (fixed)
img_new = decode_texture(blob, tex_off, 0)

# OLD decode: monkeypatch rgb5a3 inside decode_texture via module-level patch
import tpl_to_png
orig = tpl_to_png.rgb5a3
tpl_to_png.rgb5a3 = old_rgb5a3
img_old = decode_texture(blob, tex_off, 0)
tpl_to_png.rgb5a3 = orig

yo, ro = count_yellow(img_old)
yn, rn = count_yellow(img_new)
print(f"OLD (RGB565 opaque bug): yellow={yo:.2%} red-shifted={ro:.2%}")
print(f"NEW (RGB555 fixed):     yellow={yn:.2%} red-shifted={rn:.2%}")

# save side-by-side for eyeballing
W = Path("/home/joshua/Desktop/forecast-channel-web/assets/wicons")
img_old.resize((128, 128)).save(W / "_AB_sun_old_bug.png")
img_new.resize((128, 128)).save(W / "_AB_sun_new_fixed.png")
print(f"side-by-side: {W}/_AB_sun_old_bug.png vs _AB_sun_new_fixed.png")
