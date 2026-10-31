#!/usr/bin/env python3
"""Packs the Sword/Shield wonder cards of Project Pokémon's EventsGallery for the Switch app.

Usage: pack_events.py <EventsGallery directory> <output directory>

Writes swsh.bin, every card's 0x2D0 bytes one after another, and swsh.json, a list with the
name and game of each card in the same order, sorted by card ID. Cards with the same bytes are
kept once.
"""

import hashlib
import json
import os
import re
import struct
import sys

CARD_SIZE = 0x2D0
PREFIX = re.compile(r"^(\d{4}) (SWSH|SW|SH) - (.*)$")


def card_name(stem):
    match = PREFIX.match(stem)
    if not match:
        return None, None
    name = match.group(3).strip()
    if name.startswith("Item "):
        name = name[len("Item "):]
    return re.sub(r"\s+", " ", name), match.group(2)


def main():
    if len(sys.argv) != 3:
        print(__doc__)
        return 1
    gallery, out = sys.argv[1], sys.argv[2]
    root = os.path.join(gallery, "Released", "Gen 8", "SwSh", "Wondercards")
    if not os.path.isdir(root):
        print(f"'{root}' does not exist")
        return 2

    cards = []
    seen = set()
    for folder, _, files in os.walk(root):
        for file in sorted(files):
            if not file.endswith(".wc8"):
                continue
            with open(os.path.join(folder, file), "rb") as f:
                data = f.read()
            if len(data) != CARD_SIZE:
                continue
            name, game = card_name(file[: -len(".wc8")])
            if name is None:
                continue
            digest = hashlib.sha256(data).digest()
            if digest in seen:
                continue
            seen.add(digest)
            card_id = struct.unpack_from("<H", data, 0x08)[0]
            cards.append((card_id, name, game, data))

    cards.sort(key=lambda card: (card[0], card[1]))
    os.makedirs(out, exist_ok=True)
    with open(os.path.join(out, "swsh.bin"), "wb") as f:
        for card in cards:
            f.write(card[3])
    with open(os.path.join(out, "swsh.json"), "w", encoding="utf-8", newline="\n") as f:
        # One card a line, so a gallery update shows as a readable diff.
        lines = [json.dumps({"name": name, "game": game}, ensure_ascii=False)
            for _, name, game, _ in cards]
        f.write("[\n" + ",\n".join(lines) + "\n]\n")
    print(f"{len(cards)} cards")
    return 0


if __name__ == "__main__":
    sys.exit(main())
