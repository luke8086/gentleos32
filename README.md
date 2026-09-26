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

Additional assets like wallpapers and songs can be provided using
an initial RAM disk (initrd).

To create an initrd and automatically install in a disk image, run:

```bash
uv run tools/mkinitrd.py [FILES] -d gentleos32-base.img
```

The initrd replaces the one already present in the image. Use option `-a`
to also include all the default assets shipped with the prebuilt images.

Certain emulators, including v86, only accept disk images which follow
geometry of physical disks. Use option `-p` to automatically pad the final
image with zeros to the right size.

If you already have GRUB installed on a physical disk, use option `-o PATH`
to save the initrd to a file, instead of installing it in a disk image.

## Adding wallpapers

To add a wallpaper, save it in PNG/JPG/GIF/BMP/PBM/PPM format,
[add to initrd](#adding-files) and select in settings.

Three kinds of wallpapers are supported:

- Monochrome - B&W images are stored as 1bpp to save memory and
  rendered using colors editable in settings. They're tiled,
  and their width must be a multiple of 8.

- Color - regular images are converted to 8bpp and tiled.

- Pixel art - images of exactly 64x43px are scaled and rendered in a grid.
  They're also converted to 8bpp and only supported in 640x480 video mode.

In planar video mode (the default one), color and pixel art
wallpapers are supported but only rendered using the first 16 colors.

For best results, in GIMP you can use an indexed mode with the provided
[256-color](misc/vga-256.gpl) and [16-color](misc/vga-16.gpl) palettes.

## Adding songs

To add a song, save it as an uncompressed MusicXML file,
convert to the custom SPK format, and [add to initrd](#adding-files):

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
