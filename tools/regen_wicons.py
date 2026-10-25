#!/usr/bin/env python3
"""regen_wicons.py — regenerate all weather-icon PNGs from the banner arcs
with the corrected TPL decoder (RGB5A3 opaque = RGB555; CMPR index-3 =
transparent average color; Dolphin-verified)."""
import sys
import glob
from pathlib import Path

BASE = Path("/home/joshua/Desktop/Forecast channel")
OUT = Path("/home/joshua/Desktop/forecast-channel-web/assets/wicons")

sys.path.insert(0, str(BASE / "tools"))
from tpl_to_png import decode_texture  # noqa: E402

SETS = {
    BASE / "extracted/u8/banner/icon/arc/timg": OUT,    # 32x32 small set
    BASE / "extracted/u8/banner/banner/arc/timg": OUT,  # big set
}


def main():
    count = 0
    OUT.mkdir(parents=True, exist_ok=True)
    for src, outdir in SETS.items():
        for tpl in sorted(src.glob("*.tpl")):
            blob = tpl.read_bytes()
            if blob[:4] != b"\x00\x20\xaf\x30":
                print(f"skip {tpl.name}: not TPL")
                continue
            nimg = int.from_bytes(blob[4:8], "big")
            table = int.from_bytes(blob[8:12], "big")
            for idx in range(nimg):
                entry = table + idx * 8
                tex_off = int.from_bytes(blob[entry : entry + 4], "big")
                pal_off = int.from_bytes(blob[entry + 4 : entry + 8], "big")
                if not tex_off:
                    continue
                h = int.from_bytes(blob[tex_off : tex_off + 2], "big")
                w = int.from_bytes(blob[tex_off + 2 : tex_off + 4], "big")
                if w == 0 or h == 0:
                    continue
                img = decode_texture(blob, tex_off, pal_off)
                suffix = f"_{idx}" if nimg > 1 else ""
                img.save(outdir / f"{tpl.stem}{suffix}.png")
                count += 1
    print(f"regenerated {count} PNGs -> {OUT}")


if __name__ == "__main__":
    main()
