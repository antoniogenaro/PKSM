#!/usr/bin/env python3
"""Imports from PKHeX the names that core's string lists lack, and the met locations.

Usage: import_pkhex_text.py <PKHeX text directory> <output directory>

The text directory is PKHeX.Core/Resources/text in https://github.com/kwsch/PKHeX (GPLv3).
The output directory is switch/i18n, which the build copies over core's strings in the
romfs, so it gets, for every language but English (core's English lists are complete):

- species.txt, moves.txt, abilities.txt and items.txt: one name a line, line N naming number
  N, as in core. PKHeX's lists hold the names of the latest games, up to Legends: Z-A.
- locations/<game>.txt: "number<TAB>name" lines, for the met and egg locations of the
  games whose Pokémon the app edits. Numbers without a name are left out.

Every language gets the location lists, English included.
"""

import os
import sys

# Core's language folder, then PKHeX's language code.
LANGUAGES = [
    ("eng", "en"),
    ("spa", "es"),
    ("fre", "fr"),
    ("ger", "de"),
    ("ita", "it"),
    ("jpn", "ja"),
    ("kor", "ko"),
    ("chs", "zh-Hans"),
    ("cht", "zh-Hant"),
]

# Core's list, then PKHeX's file, with {0} for the language code.
NAME_LISTS = [
    ("species.txt", "other/{0}/text_Species_{0}.txt"),
    ("moves.txt", "other/{0}/text_Moves_{0}.txt"),
    ("abilities.txt", "other/{0}/text_Abilities_{0}.txt"),
    ("items.txt", "items/text_Items_{0}.txt"),
]

# The app's list, then PKHeX's files: (first number, file), with {0} for the language code.
LOCATION_LISTS = [
    ("gen3", [(0, "locations/gen3/text_rsefrlg_00000_{0}.txt")]),
    ("lgpe", [(0, "locations/gen7/text_gg_00000_{0}.txt"),
              (40000, "locations/gen7/text_gg_40000_{0}.txt")]),
    ("swsh", [(0, "locations/gen8/text_swsh_00000_{0}.txt"),
              (30000, "locations/gen8/text_swsh_30000_{0}.txt"),
              (40000, "locations/gen8/text_swsh_40000_{0}.txt"),
              (60000, "locations/gen8/text_swsh_60000_{0}.txt")]),
    ("pla", [(0, "locations/gen8a/text_la_00000_{0}.txt"),
             (30000, "locations/gen8a/text_la_30000_{0}.txt"),
             (40000, "locations/gen8a/text_la_40000_{0}.txt"),
             (60000, "locations/gen8a/text_la_60000_{0}.txt")]),
    ("sv", [(0, "locations/gen9/text_sv_00000_{0}.txt"),
            (30000, "locations/gen9/text_sv_30000_{0}.txt"),
            (40000, "locations/gen9/text_sv_40000_{0}.txt"),
            (60000, "locations/gen9/text_sv_60000_{0}.txt")]),
    ("za", [(0, "locations/gen9a/text_za_00000_{0}.txt"),
            (30000, "locations/gen9a/text_za_30000_{0}.txt"),
            (40000, "locations/gen9a/text_za_40000_{0}.txt"),
            (60000, "locations/gen9a/text_za_60000_{0}.txt")]),
]


def read_lines(path):
    with open(path, encoding="utf-8-sig") as file:
        lines = file.read().replace("\r\n", "\n").split("\n")
    if lines and lines[-1] == "":
        lines.pop()
    return [line.strip() for line in lines]


def write_text(path, text):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w", encoding="utf-8", newline="\n") as file:
        file.write(text)


def main():
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    source, output = sys.argv[1], sys.argv[2]

    english = {name: read_lines(os.path.join(source, pattern.format("en")))
               for name, pattern in NAME_LISTS}

    for folder, code in LANGUAGES:
        if folder != "eng":
            for name, pattern in NAME_LISTS:
                lines = read_lines(os.path.join(source, pattern.format(code)))
                if len(lines) != len(english[name]):
                    sys.exit(f"{pattern.format(code)} has {len(lines)} names, English "
                             f"{len(english[name])}")
                # Core's lists end without a line break.
                write_text(os.path.join(output, folder, name), "\n".join(lines))

        for game, files in LOCATION_LISTS:
            entries = []
            for first, pattern in files:
                for index, line in enumerate(read_lines(os.path.join(source,
                                                                    pattern.format(code)))):
                    if line:
                        entries.append(f"{first + index}\t{line}")
            write_text(os.path.join(output, folder, "locations", game + ".txt"),
                       "\n".join(entries) + "\n")


if __name__ == "__main__":
    main()
