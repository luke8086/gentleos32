# GentleOS/32

A hobby operating system for vintage 32-bit PCs,
built for tinkering with old hardware on the bare metal.

You can find more information on its [website](https://luke8086.dev/gentleos32).

It has a spin-off called
[GentleOS/16](https://github.com/luke8086/gentleos),
which targets even older, 16-bit PCs.

<img src="doc/t1900c.webp" width="400">

## Building

The only prerequisite is Docker & Docker Compose, supporting linux/amd64 platform.

To compile GentleOS/32, run:

```bash
docker compose run --rm dev make -j4
```

You will find the resulting binaries in `build/`.

To clean up docker artifacts, run:

```bash
docker compose down --rmi all
```

## Testing

The easiest way to test is to open `gentleos32-web.html` in a browser.

## Adding files

Additional assets like wallpapers and songs can be provided
using an initial RAM disk (initrd).

To create one and install in a disk image, run:

```bash
uv run tools/mkinitrd.py [FILES] --disk-image gentleos32-disk.img
```

## Adding wallpapers

To add a wallpaper, save it in PNG/JPG/GIF/BMP/PPM format
and add to initrd.

The image must match your screen resolution.

The image will be automatically converted to 256-color mode.

In planar video mode (the default one), it'll be displayed using only 16 colors.

For best results, in GIMP you can use an indexed mode with the provided
[256-color](misc/vga-256.gpl) and [16-color](misc/vga-16.gpl) palettes.

In the special case where the image is black and white, it'll be stored
in 1bpp mode to conserve memory, and it'll be rendered using colors
editable in settings.

## Adding songs

To add a song, save it as an uncompressed MusicXML file,
convert to the custom SPK format, and add to initrd:

```bash
uv run tools/mkspk.py -i song.musicxml -o song.spk
```

Only a very limited subset of MusicXML is supported:
plain notes, grace notes, ties, staccato dots and tempo markings.
Songs must only have a single staff with a single voice, and no chords.

Only files exported from MuseScore were tested, other tools may or may not work.

You can also edit SPK files directly, they're plain text consisting
of (pitch, milliseconds) pairs, and the song title metadata.

## Attributions

- Assets in [vendor/icons8](vendor/icons8) have been sourced from
  [Icons8](https://icons8.com/) using the
  [free license](https://web.archive.org/web/20260325111643/https://icons8.com/license)
  and modified

- Assets in [vendor/mona](vendor/mona) have been extracted from the
  [Mona Font](https://github.com/MonadABXY/mona-font) and modified
  ([LICENSE](vendor/mona/LICENSE.txt))

- Assets in [vendor/int10h](vendor/int10h) have been extracted from the
  [The Ultimate Oldschool PC Font Pack](https://int10h.org/oldschool-pc-fonts/)
  and modified ([LICENSE](vendor/int10h/LICENSE.txt))

## License

Except where otherwise noted, GentleOS/32 is licensed under [GPLv2](LICENSE).
