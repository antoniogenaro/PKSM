#!/usr/bin/env python3
"""Packs the sprites of the Pokémon that pkm_spritesheet.t3x lacks into a second sheet.

Usage: pack_sprites.py <sprite directory> <output file>

The sprite directory holds PKHeX's "Artwork Pokemon Sprites" (a_<species>.png, 68 x 56),
from PKHeX.Drawing.PokeSprite/Resources/img in https://github.com/kwsch/PKHeX (GPLv3).
The output is a .t3x sheet in the format the app reads for pkm_spritesheet.t3x: one entry
per species from FIRST_SPECIES to LAST_SPECIES, in order. Needs Pillow.
"""

import os
import struct
import sys

from PIL import Image

FIRST_SPECIES = 899
LAST_SPECIES = 1025
SPRITE_WIDTH = 68
SPRITE_HEIGHT = 56
SHEET_WIDTH = 1024
SHEET_HEIGHT = 512


def dimension_bits(size):
    return size.bit_length() - 1 - 3


def morton(x, y):
    return ((x & 1) | ((y & 1) << 1) | ((x & 2) << 1) | ((y & 2) << 2) | ((x & 4) << 2)
            | ((y & 4) << 3))


def tiled_texture(sheet):
    """The sheet as the 3DS keeps textures: 8 x 8 tiles in Morton order, bytes A, B, G, R."""
    pixels = sheet.load()
    data = bytearray(SHEET_WIDTH * SHEET_HEIGHT * 4)
    tiles_per_row = SHEET_WIDTH // 8
    for y in range(SHEET_HEIGHT):
        for x in range(SHEET_WIDTH):
            r, g, b, a = pixels[x, y]
            at = (((y // 8) * tiles_per_row + x // 8) * 64 + morton(x & 7, y & 7)) * 4
            data[at:at + 4] = bytes((a, b, g, r))
    return bytes(data)


def lz11(data):
    """LZ11, as the app's decodeLz11 reads it."""
    out = bytearray((0x11,)) + struct.pack("<I", len(data))[:3]
    assert len(data) < 1 << 24
    last = {}
    i = 0
    while i < len(data):
        flags_at = len(out)
        out.append(0)
        flags = 0
        for bit in range(8):
            if i >= len(data):
                break
            best_length = 0
            best_distance = 0
            for distance in (1, 4):
                if distance <= i:
                    length = 0
                    while (i + length < len(data) and length < 0x10110
                           and data[i + length] == data[i + length - distance]):
                        length += 1
                    if length > best_length:
                        best_length, best_distance = length, distance
            key = data[i:i + 3]
            if len(key) == 3 and key in last and i - last[key] <= 0x1000:
                distance = i - last[key]
                length = 0
                while (i + length < len(data) and length < 0x10110
                       and data[i + length] == data[i + length - distance]):
                    length += 1
                if length > best_length:
                    best_length, best_distance = length, distance
            if best_length >= 3:
                flags |= 0x80 >> bit
                d = best_distance - 1
                if best_length <= 0x10:
                    out += bytes((((best_length - 1) << 4) | (d >> 8), d & 0xFF))
                elif best_length <= 0x110:
                    n = best_length - 0x11
                    out += bytes((n >> 4, ((n & 0xF) << 4) | (d >> 8), d & 0xFF))
                else:
                    n = best_length - 0x111
                    out += bytes((0x10 | (n >> 12), (n >> 4) & 0xFF, ((n & 0xF) << 4) | (d >> 8),
                                  d & 0xFF))
                for at in range(i, min(i + best_length, len(data) - 2)):
                    last[data[at:at + 3]] = at
                i += best_length
            else:
                out.append(data[i])
                if len(key) == 3:
                    last[key] = i
                i += 1
        out[flags_at] = flags
    return bytes(out)


def main():
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    source, output = sys.argv[1], sys.argv[2]

    columns = SHEET_WIDTH // SPRITE_WIDTH
    count = LAST_SPECIES - FIRST_SPECIES + 1
    assert (count + columns - 1) // columns * SPRITE_HEIGHT <= SHEET_HEIGHT

    sheet = Image.new("RGBA", (SHEET_WIDTH, SHEET_HEIGHT), (0, 0, 0, 0))
    table = bytearray()
    for index in range(count):
        species = FIRST_SPECIES + index
        sprite = Image.open(os.path.join(source, f"a_{species}.png")).convert("RGBA")
        if sprite.size != (SPRITE_WIDTH, SPRITE_HEIGHT):
            sys.exit(f"a_{species}.png is {sprite.size[0]} x {sprite.size[1]}")
        x = index % columns * SPRITE_WIDTH
        y = index // columns * SPRITE_HEIGHT
        sheet.paste(sprite, (x, y))
        # Texture coordinates out of 1024, with y counted from the bottom of the sheet.
        to_u = 1024 // SHEET_WIDTH
        to_v = 1024 // SHEET_HEIGHT
        table += struct.pack("<6H", SPRITE_WIDTH, SPRITE_HEIGHT, x * to_u,
                             (SHEET_HEIGHT - y) * to_v, (x + SPRITE_WIDTH) * to_u,
                             (SHEET_HEIGHT - y - SPRITE_HEIGHT) * to_v)

    dimensions = dimension_bits(SHEET_WIDTH) | (dimension_bits(SHEET_HEIGHT) << 3)
    header = struct.pack("<HBBB", count, dimensions, 0, 0)
    with open(output, "wb") as file:
        file.write(header + table + lz11(tiled_texture(sheet)))
    print(f"{count} sprites, {os.path.getsize(output)} bytes")


if __name__ == "__main__":
    main()
