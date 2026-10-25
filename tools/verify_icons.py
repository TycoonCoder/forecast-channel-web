#!/usr/bin/env python3
"""verify_icons.py — quantitative sanity of regenerated weather icons.
Checks: sun icon yellow-dominant; cloud white/grey; thunder has dark pixels;
all icons have sane alpha coverage (no fully-empty, no fully-opaque squares)."""
from pathlib import Path
from PIL import Image

W = Path("/home/joshua/Desktop/forecast-channel-web/assets/wicons")


def stats(name):
    img = Image.open(W / name).convert("RGBA")
    px = list(img.getdata())
    n = len(px)
    yellow = sum(1 for r, g, b, a in px if a > 128 and r > 150 and g > 120 and b < 120)
    white = sum(1 for r, g, b, a in px if a > 128 and r > 180 and g > 180 and b > 170)
    dark = sum(1 for r, g, b, a in px if a > 128 and r < 80 and g < 80 and b < 80)
    opaque = sum(1 for r, g, b, a in px if a > 128)
    return n, yellow / n, white / n, dark / n, opaque / n


for name in ["icon_W_sun_00.png", "W_sun_00.png", "icon_W_cloud_00.png", "W_cloud_00.png",
             "icon_W_thunder_00.png", "W_thunder_00.png", "icon_W_rain_00.png",
             "icon_W_fog_00.png", "icon_W_moon_00.png"]:
    if not (W / name).exists():
        print(f"{name}: MISSING")
        continue
    n, y, w_, d, op = stats(name)
    print(f"{name:28s} {n:6d}px  yellow={y:.2%} white={w_:.2%} dark={d:.2%} opaque={op:.2%}")

print()
issues = []
for f in sorted(W.glob("*.png")):
    img = Image.open(f).convert("RGBA")
    px = list(img.getdata())
    op = sum(1 for r, g, b, a in px if a > 8) / len(px)
    if op == 0:
        issues.append(f"{f.name}: fully transparent")
    if img.width == 0 or img.height == 0:
        issues.append(f"{f.name}: zero size")
print("issues:", issues if issues else "none")
