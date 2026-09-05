#!/usr/bin/env python3
#
# Copyright (c) 2026 luke8086
# Distributed under the terms of GPL-2 License.
#
# File: mkdata.py - Convert bitmaps and fonts to hardcoded C data
#

# /// script
# requires-python = ">=3.10"
# dependencies = [
#     "pillow>=10,<12",
# ]
# ///

import argparse
import glob
import os
import sys

from PIL import Image, ImageOps

import mkinitrd


TARGET = "build/data.c"
FONT_MAX_CHARS = 256
DEBUG = 0

FONTS = [
    {
        "path": "vendor/int10h/pc-8x16.pbm",
        "name": "PC 8x16",
        "width": 8,
        "height": 16,
        "pitch": 8,
    },
    {
        "path": "vendor/int10h/pc-8x8.pbm",
        "name": "PC 8x8",
        "width": 8,
        "height": 8,
        "pitch": 8,
    },
    {
        "path": "vendor/int10h/evxme58.pbm",
        "name": "Evx ME 5x8",
        "width": 5,
        "height": 8,
        "pitch": 8,
    },
]

PREFIXES = {
    "assets/icons": "icon_",
    "vendor/icons8": "icon_",
    "assets/player": "icon_player_",
    "assets/sprites": "sprite_",
    "assets/mahjong": "sprite_mj_",
    "vendor/mona": "glyph_mn_",
}


def hex_str(data):
    return "".join("\\x%02x" % b for b in data)


def load_pbm(path):
    img = Image.open(path)

    if img.mode != "1":
        raise SystemExit("%s: not a 1-bit PBM file" % path)

    if DEBUG:
        print("- %-32s  size: %dx%d" % (path, img.size[0], img.size[1]), end="")

    return ImageOps.invert(img)


def process_pbm(path):
    name = os.path.splitext(os.path.basename(path))[0]
    prefix = PREFIXES.get(os.path.dirname(path), "bitmap_")

    img = load_pbm(path)
    width, height = img.size
    pitch = (width + 7) // 8

    if DEBUG:
        print()

    img_bytes = img.tobytes()

    pixel_lines = [
        '        "%s" \\' % hex_str(img_bytes[y * pitch:(y + 1) * pitch])
        for y in range(height)
    ]

    lines = [
        "global bitmap_st %s%s = {" % (prefix, name),
        "    .size = { .width = %d, .height = %d }," % (width, height),
        "    .bpp = 1,",
        "    .pitch = %d," % pitch,
        "    .pixels = (uint8_t *)",
        *pixel_lines,
        "};",
        "",
    ]

    return "\n".join(lines)


def process_bitmaps():
    pbm_files = sorted([]
        + glob.glob("assets/icons/*.pbm")
        + glob.glob("assets/mahjong/*.pbm")
        + glob.glob("assets/patterns/*.pbm")
        + glob.glob("assets/player/*.pbm")
        + glob.glob("assets/sprites/*.pbm")
        + glob.glob("vendor/icons8/*.pbm")
        + glob.glob("vendor/mona/*.pbm")
        + ["vendor/misc/pattern_a.pbm"]
    )

    return "\n".join(process_pbm(f) for f in pbm_files)


def load_font(font):
    height = font["height"]
    pitch = font["pitch"]

    img = load_pbm(font["path"])

    cols = img.size[0] // pitch
    rows = img.size[1] // height
    num_chars = min(cols * rows, FONT_MAX_CHARS)
    max_bytes = FONT_MAX_CHARS * height

    if DEBUG:
        print("  grid: %dx%d  chars: %d" % (cols, rows, cols * rows))

    img_bytes = bytearray()

    for ch in range(num_chars):
        x = (ch % cols) * pitch
        y = (ch // cols) * height
        img_bytes += img.crop((x, y, x + pitch, y + height)).tobytes()

    img_bytes += bytes(max_bytes - len(img_bytes))

    return img_bytes


def process_font(font):
    width = font["width"]
    height = font["height"]
    name = font["name"]

    img_bytes = load_font(font)

    pixel_lines = [
        '            "%s" \\' % hex_str(img_bytes[i:i + height])
        for i in range(0, len(img_bytes), height)
    ]

    lines = [
        "    {",
        "        .size = { .width = %d, .height = %d }," % (width, height),
        '        .name = "%s",' % name,
        "        .pixels = (uint8_t *)",
        *pixel_lines,
        "    },",
    ]

    return lines


def process_fonts():
    lines = ["global font_st fonts[] = {"]

    for font in FONTS:
        lines += process_font(font)

    lines.append("};")

    return "\n".join(lines)


def process_builtin_file_data(index, data):
    data_lines = [
        '    "%s" \\' % hex_str(data[i:i + 16])
        for i in range(0, len(data), 16)
    ]
    if data_lines:
        data_lines[-1] = data_lines[-1].rstrip(" \\") + ";"
    else:
        data_lines = ['    "";']

    lines = [
        "static uint8_t builtin_file_data_%d[] __attribute__((aligned(4))) =" % index,
        *data_lines,
        "",
    ]

    return lines


def process_builtin_file_entry(index, f):
    return [
        "    {",
        '        .name = "%s",' % f["name"],
        "        .type = %s," % f["type"],
        "        .addr = builtin_file_data_%d," % index,
        "        .size = %d," % len(f["data"]),
        "    },",
    ]

def process_builtin_files():
    paths = sorted([]
        + glob.glob("build/assets/songs/*.spk")
        + ["vendor/misc/Sunset.png"]
    )

    if not paths:
        paths = ["/dev/null"]

    data_lines = []
    entry_lines = []

    for i, path in enumerate(paths):
        f = mkinitrd.load_file(path)
        data_lines += process_builtin_file_data(i, f["data"])
        entry_lines += process_builtin_file_entry(i, f)

    return "\n".join([
        *data_lines,
        "global file_st builtin_files[] = {",
        *entry_lines,
        "};",
        "",
        "global size_t builtin_files_count = %d;" % len(paths),
    ])


def main():
    parser = argparse.ArgumentParser(description="Convert bitmaps and fonts to C data")
    parser.add_argument("-d", "--debug", action="store_true", help="debug output")
    args = parser.parse_args()

    global DEBUG
    DEBUG = args.debug

    content = "\n".join([
        "#include <gui.h>",
        "",
        "#pragma GCC diagnostic push",
        '#pragma GCC diagnostic ignored "-Woverlength-strings"',
        "",
        process_bitmaps(),
        "",
        process_fonts(),
        "",
        process_builtin_files(),
        "",
        "#pragma GCC diagnostic pop",
        "",
    ])

    try:
        with open(TARGET) as f:
            current_content = f.read()
    except OSError:
        current_content = ""

    if current_content == content:
        print("%s: %s unchanged" % (sys.argv[0], TARGET))
        return

    with open(TARGET, "w") as f:
        f.write(content)

    print("%s: %s updated" % (sys.argv[0], TARGET))


main()
