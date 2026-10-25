#!/usr/bin/env python3
"""Convert Nintendo TPL textures to PNG for browser-side inspection/use."""

from __future__ import annotations

import argparse
from pathlib import Path

from PIL import Image


def rgb565(value: int) -> tuple[int, int, int, int]:
    return (
        ((value >> 11) & 0x1F) * 255 // 31,
        ((value >> 5) & 0x3F) * 255 // 63,
        (value & 0x1F) * 255 // 31,
        255,
    )


def rgb555(value: int) -> tuple[int, int, int, int]:
    """RGB5A3 opaque layout: bit15 = flag, RGB in bits 14-0 (5-5-5)."""
    return (
        ((value >> 10) & 0x1F) * 255 // 31,
        ((value >> 5) & 0x1F) * 255 // 31,
        (value & 0x1F) * 255 // 31,
        255,
    )


def rgb5a3(value: int) -> tuple[int, int, int, int]:
    if value & 0x8000:
        return rgb555(value)
    return (
        ((value >> 8) & 0x0F) * 17,
        ((value >> 4) & 0x0F) * 17,
        (value & 0x0F) * 17,
        ((value >> 12) & 0x07) * 255 // 7,
    )


def rgba8_tile(tile: bytes, x: int, y: int) -> tuple[int, int, int, int]:
    ar = (y * 4 + x) * 2
    gb = 32 + ar
    return tile[ar + 1], tile[gb], tile[gb + 1], tile[ar]


def decode_texture(blob: bytes, texture_offset: int, palette_offset: int) -> Image.Image:
    height = int.from_bytes(blob[texture_offset : texture_offset + 2], "big")
    width = int.from_bytes(blob[texture_offset + 2 : texture_offset + 4], "big")
    fmt = int.from_bytes(blob[texture_offset + 4 : texture_offset + 8], "big")
    data_offset = int.from_bytes(blob[texture_offset + 8 : texture_offset + 12], "big")
    pixels = [(0, 0, 0, 0)] * (width * height)
    data = blob[data_offset:]

    palette: list[tuple[int, int, int, int]] = []
    if palette_offset:
        count = int.from_bytes(blob[palette_offset : palette_offset + 2], "big")
        palette_fmt = int.from_bytes(blob[palette_offset + 4 : palette_offset + 8], "big")
        palette_data = int.from_bytes(blob[palette_offset + 8 : palette_offset + 12], "big")
        for i in range(count):
            value = int.from_bytes(blob[palette_data + i * 2 : palette_data + i * 2 + 2], "big")
            palette.append(rgb565(value) if palette_fmt == 0 else rgb5a3(value))

    def put(x: int, y: int, color: tuple[int, int, int, int]) -> None:
        if x < width and y < height:
            pixels[y * width + x] = color

    if fmt in (0, 1, 2, 3, 4, 5, 6, 8, 9, 10):
        tile_w, tile_h = {
            0: (8, 8), 1: (8, 4), 2: (8, 4), 3: (4, 4),
            4: (4, 4), 5: (4, 4), 6: (4, 4), 8: (8, 8),
            9: (8, 4), 10: (4, 4),
        }[fmt]
        tile_bytes = {0: 32, 1: 32, 2: 32, 3: 32, 4: 32, 5: 32,
                      6: 64, 8: 32, 9: 32, 10: 32}[fmt]
        cursor = 0
        for ty in range(0, height, tile_h):
            for tx in range(0, width, tile_w):
                tile = data[cursor : cursor + tile_bytes]
                cursor += tile_bytes
                for py in range(tile_h):
                    for px in range(tile_w):
                        if fmt == 0:
                            value = tile[(py * 8 + px) // 2]
                            gray = ((value >> 4) if px % 2 == 0 else value & 0x0F) * 17
                            color = (gray, gray, gray, 255)
                        elif fmt == 1:
                            gray = tile[py * 8 + px]
                            color = (gray, gray, gray, 255)
                        elif fmt == 2:
                            value = tile[py * 8 + px]
                            color = ((value >> 4) * 17,) * 3 + ((value & 0x0F) * 17,)
                        elif fmt == 3:
                            value = tile[(py * 4 + px) * 2] << 8 | tile[(py * 4 + px) * 2 + 1]
                            color = (value >> 8, value >> 8, value >> 8, value & 0xFF)
                        elif fmt == 4:
                            i = (py * 4 + px) * 2
                            color = rgb565(tile[i] << 8 | tile[i + 1])
                        elif fmt == 5:
                            i = (py * 4 + px) * 2
                            color = rgb5a3(tile[i] << 8 | tile[i + 1])
                        elif fmt == 6:
                            color = rgba8_tile(tile, px, py)
                        elif fmt == 8:
                            value = tile[(py * 8 + px) // 2]
                            index = (value >> 4) if px % 2 == 0 else value & 0x0F
                            color = palette[index]
                        elif fmt == 9:
                            color = palette[tile[py * 8 + px]]
                        else:
                            i = (py * 4 + px) * 2
                            color = palette[((tile[i] << 8 | tile[i + 1]) & 0x3FFF)]
                        put(tx + px, ty + py, color)
    elif fmt == 14:
        # CMPR is four DXT1 4x4 blocks in each 8x8 tile, stored in
        # Dolphin-verified order: (0,0), (4,0), (0,4), (4,4).
        cursor = 0
        for ty in range(0, height, 8):
            for tx in range(0, width, 8):
                tile = data[cursor : cursor + 32]
                cursor += 32
                for block in range(4):
                    bx, by = (block % 2) * 4, (block // 2) * 4
                    c0 = int.from_bytes(tile[block * 8 : block * 8 + 2], "big")
                    c1 = int.from_bytes(tile[block * 8 + 2 : block * 8 + 4], "big")
                    colors = [rgb565(c0), rgb565(c1)]
                    if c0 > c1:
                        colors += [tuple((2 * colors[0][i] + colors[1][i]) // 3 for i in range(4)),
                                   tuple((colors[0][i] + 2 * colors[1][i]) // 3 for i in range(4))]
                    else:
                        # GCN: index 3 = AVERAGE color with alpha 0 (not black)
                        avg = tuple((colors[0][i] + colors[1][i]) // 2 for i in range(4))
                        colors += [avg, avg[:3] + (0,)]
                    bits = int.from_bytes(tile[block * 8 + 4 : block * 8 + 8], "big")
                    for py in range(4):
                        for px in range(4):
                            put(tx + bx + px, ty + by + py, colors[(bits >> (30 - (py * 4 + px) * 2)) & 3])
    else:
        raise ValueError(f"unsupported TPL format {fmt}")
    return Image.frombytes("RGBA", (width, height), bytes(c for p in pixels for c in p))


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("tpl", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--index", type=int, default=0)
    args = parser.parse_args()
    blob = args.tpl.read_bytes()
    if blob[:4] != b"\x00\x20\xaf\x30":
        raise SystemExit("not a TPL file")
    table = int.from_bytes(blob[8:12], "big") + args.index * 8
    texture = int.from_bytes(blob[table : table + 4], "big")
    palette = int.from_bytes(blob[table + 4 : table + 8], "big")
    image = decode_texture(blob, texture, palette)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    image.save(args.output)
    print(f"{args.tpl} texture {args.index} -> {args.output} {image.size}")


if __name__ == "__main__":
    main()
