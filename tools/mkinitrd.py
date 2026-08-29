#!/usr/bin/env python3
#
# Copyright (c) 2026 luke8086
# Distributed under the terms of GPL-2 License.
#
# File: mkinitrd.py - Create initial RAM disk
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
import re
import shutil
import struct

MAGIC        = b"IRD1"
NAME_LEN     = 31
ALIGN        = 4
HEADER_LEN   = 8                   # 4s magic + I count
ENTRY_LEN    = NAME_LEN + 9        # name + B type + I offset + I size

FILE_TYPE_UNKNOWN   = 0
FILE_TYPE_BITMAP    = 1
FILE_TYPE_SONG      = 2

FILE_TYPE_NAMES = {
    FILE_TYPE_UNKNOWN: "unknown",
    FILE_TYPE_BITMAP:  "bitmap",
    FILE_TYPE_SONG:    "song",
}

PALETTE_PATH = "misc/vga-256.gpl"
PALETTE_REX  = re.compile(r"^\s*(\d+)\s+(\d+)\s+(\d+)\s+\$([0-9a-fA-F]+)\s*$")

SPK_NOTE_REX   = re.compile(r"^\s*(\d+)\s*,\s*(\d+)\s*$")
SPK_META_REX   = re.compile(r"^\s*(\w+)\s*:\s*(.+?)\s*$")

INITRD_PATH  = "gentleos.rd"

SECTOR_LEN    = 512
FS_OFFSET     = 1048576


def die(msg):
    raise SystemExit(msg)


def align(n):
    return (n + ALIGN - 1) & ~(ALIGN - 1)


def split_ext(path):
    [base, ext] = os.path.splitext(os.path.basename(path))
    ext = ext.lower()[1:]
    return [base, ext]


def expand_paths(paths):
    ret = []

    for path in paths:
        if not glob.has_magic(path):
            ret.append(path)
            continue

        ret.extend(p for p in glob.glob(path, recursive=True) if os.path.isfile(p))

    return sorted(ret)


def load_palette(path):
    rgb = [None] * 256

    with open(path) as f:
        for line in f:
            m = PALETTE_REX.match(line)
            if not m:
                continue
            r, g, b, index = int(m[1]), int(m[2]), int(m[3]), int(m[4], 16)
            if rgb[index] is None:
                rgb[index] = (r, g, b)

    return [c if c is not None else (0, 0, 0) for c in rgb]


def process_image(path):
    try:
        from PIL import Image, ImageOps
    except ImportError:
        die("Error: mkinitrd.py requires 'pillow' library for Python")

    palette = getattr(process_image, "palette", None)

    if palette is None:
        palette = load_palette(PALETTE_PATH)
        process_image.palette = palette

    print("Importing %s... " % path, end="")

    img = Image.open(path).convert("RGB")
    width, height = img.size
    colors = img.getcolors(maxcolors=2)
    is_bw = colors and all(c in [(0, 0, 0), (255, 255, 255)] for (_, c) in colors)

    if is_bw:
        bpp = 1
        alpha = 0
        pitch = (width + 7) // 8
        img = ImageOps.invert(img.convert("1"))
    else:
        bpp = 8
        alpha = 0xfd
        pitch = width
        pal = Image.new("P", (1, 1))
        pal.putpalette([c for color in palette for c in color])
        img = img.quantize(palette=pal, dither=Image.Dither.FLOYDSTEINBERG)

    header = struct.pack("<7I", width, height, bpp, pitch, 0, alpha, 0)
    pixels = img.tobytes()

    print("ok (%dx%d, %d bpp)" % (width, height, bpp))

    return header + pixels


def read_spk(path):
    meta = {}
    segments = []

    with open(path) as f:
        lines = f.readlines()

    for num, line in enumerate(lines, 1):
        if not line.strip():
            continue

        if m := SPK_META_REX.match(line):
            meta[m[1]] = m[2]
            continue

        if m := SPK_NOTE_REX.match(line):
            pitch = min(0xffff, int(m[1]))
            duration = int(m[2])
        else:
            die("Error: %s:%d: invalid syntax" % (path, num))

        segments.append((pitch, max(1, min(0xffff, duration))))

    title = meta.get("title", "")
    if not title:
        die("Error: no title in %s" % path)

    if not segments:
        die("Error: no notes in %s" % path)

    return [title, segments]


def process_spk(path):
    print("Importing %s... " % path, end="")

    title, segments = read_spk(path)

    data = b""

    for pitch, duration in segments:
        data += struct.pack("<HH", pitch, duration)

    data += struct.pack("<HH", 0, 0)

    print("ok (\"%s\", %d notes, %d ms)" % (title, len(segments), sum(ms for _, ms in segments)))

    return title, data


def load_file(path):
    [basename, ext] = split_ext(path)

    if ext in  ["jpg", "jpeg", "png", "ppm", "gif", "bmp"]:
        name = basename
        file_type = FILE_TYPE_BITMAP
        data = process_image(path)
    elif ext == "spk":
        file_type = FILE_TYPE_SONG
        name, data = process_spk(path)
    else:
        name = f"{basename}.{ext}"
        file_type = FILE_TYPE_UNKNOWN
        with open(path, "rb") as f:
            data = f.read()

    return {
        "name": name[:NAME_LEN - 1],
        "type": file_type,
        "data": data,
    }


def build_initrd(files):
    count = len(files)
    offset = align(HEADER_LEN + count * ENTRY_LEN)
    table = b""
    blobs = b""

    for f in files:
        size = len(f["data"])
        ftype = f["type"]
        padded_size = align(size)

        print("- %s: %x (%u B, %s)" % (f["name"], offset, size, FILE_TYPE_NAMES[ftype]))

        name = f["name"].encode("latin-1")[:NAME_LEN - 1]
        table += struct.pack("<%dsBII" % NAME_LEN, name, ftype, offset, size)
        blobs += f["data"] + b"\0" * (padded_size - size)
        offset += padded_size

    return struct.pack("<4sI", MAGIC, count) + table + blobs


def get_kernel_offset_in_image(image):
    stage2_sectors_ofs = 5
    stage2_sectors, = struct.unpack_from("<H", image, stage2_sectors_ofs)

    return SECTOR_LEN * (2 + stage2_sectors)


def is_native_image(image):
    if len(image) < SECTOR_LEN:
        return False

    kernel_offset = get_kernel_offset_in_image(image)

    if len(image) < kernel_offset + 32:
        return False

    magic, = struct.unpack_from("<I", image, kernel_offset + 4)

    return magic == 0x1badb002


def install_initrd_native(disk_image_path, image, initrd):
    kernel_offset = get_kernel_offset_in_image(image)
    kernel_sectors, = struct.unpack_from("<H", image, SECTOR_LEN * 2)
    kernel_end = kernel_offset + kernel_sectors * SECTOR_LEN

    if kernel_sectors == 0 or len(image) < kernel_end:
        die("Error: no kernel found in the disk image")

    initrd_sectors = (len(initrd) + SECTOR_LEN - 1) // SECTOR_LEN

    if initrd_sectors * SECTOR_LEN > 15 * 1024 * 1024:
        die("Error: initrd too big to fit in 15MB of RAM")

    padding = b"\0" * (initrd_sectors * SECTOR_LEN - len(initrd))
    image = bytearray(image[:kernel_end]) + initrd + padding

    initrd_sectors_offset = SECTOR_LEN * 2 + 2
    struct.pack_into("<H", image, initrd_sectors_offset, initrd_sectors)

    with open(disk_image_path, "wb") as f:
        f.write(image)

    print("Initrd installed in %s" % disk_image_path)


def install_initrd_grub(disk_image_path, initrd_path):
    if not shutil.which("mcopy"):
        die("Error: mkinitrd.py requires 'mtools' package to install initrd in a disk image")

    cmd = "mcopy -D o -i '%s@@%d' %s ::gentleos.rd" % (disk_image_path, FS_OFFSET, initrd_path)
    print("Running %s" % cmd)
    os.system(cmd)


def install_initrd(disk_image_path, initrd, initrd_path):
    if not os.path.exists(disk_image_path):
        die("Error: disk image not found")

    with open(disk_image_path, "rb") as f:
        image = f.read()

    if is_native_image(image):
        install_initrd_native(disk_image_path, image, initrd)
    else:
        install_initrd_grub(disk_image_path, initrd_path)


def main():
    parser = argparse.ArgumentParser(description="Create initial RAM disk for GentleOS/32")
    parser.add_argument("files", nargs="*", help="files to add")
    parser.add_argument("--wallpaper", metavar="PATH", help="image to use as the wallpaper")
    parser.add_argument("--disk-image", metavar="PATH", help="disk image to install initrd into")
    parser.add_argument("-o", "--output", metavar="PATH", default=INITRD_PATH,
        help="path to save the initrd to (default: %s)" % INITRD_PATH)
    args = parser.parse_args()

    files = []

    for path in expand_paths(args.files):
        files.append(load_file(path))

    if args.wallpaper is not None:
        data = process_image(args.wallpaper)
        files.append({
            "name": "wallpaper",
            "type": FILE_TYPE_BITMAP,
            "data": data,
        })

    if not files:
        parser.print_usage()
        raise SystemExit(1)

    print("Generating initrd:")
    image = build_initrd(files)

    with open(args.output, "wb") as f:
        f.write(image)

    print(f"Initrd saved to {args.output}")

    if args.disk_image is not None:
        install_initrd(args.disk_image, image, args.output)


if __name__ == "__main__":
    main()
