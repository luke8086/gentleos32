#!/usr/bin/env python3
#
# Copyright (c) 2026 luke8086
# Distributed under the terms of GPL-2 License.
#
# File: mkemu.py - Make webpage with v86 emulator
#

import base64
import json
import sys

def die(msg):
    raise SystemExit(msg)


def read_text(path):
    with open(path, "r") as f:
        return f.read()


def read_b64(path):
    with open(path, "rb") as f:
        data = f.read()

    return base64.b64encode(data).decode()


def main():
    if len(sys.argv) != 3:
        die(f"usage: {sys.argv[0]} <disk-image> <output-html>")

    assets = {
        "wasm": read_b64("vendor/v86/v86.wasm"),
        "bios": read_b64("vendor/v86/bios/seabios.bin"),
        "vga_bios": read_b64("vendor/v86/bios/vgabios.bin"),
        "hda": read_b64(sys.argv[1]),
    }

    libv86 = read_text("vendor/v86/libv86.js")

    html = read_text("misc/emu-tpl.html")
    html = html.replace("__ASSETS__", json.dumps(assets, indent=2))
    html = html.replace("__LIBS__", libv86)

    with open(sys.argv[2], "w") as f:
        f.write(html)

    print(f"{sys.argv[0]}: created {sys.argv[2]}")


main()
