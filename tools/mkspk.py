#!/usr/bin/env python3
#
# Copyright (c) 2026 luke8086
# Distributed under the terms of GPL-2 License.
#
# File: mkspk.py - Convert from MusicXML to SPK
#

# /// script
# requires-python = ">=3.10"
# dependencies = [
# ]
# ///

import argparse
import xml.etree.ElementTree as ET

DEBUG = 0
MIN_REST_MS = 1
NOTE_NAMES = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"]
NOTE_TYPES = {
    "whole": 4.0,
    "half": 2.0,
    "quarter": 1.0,
    "eighth": 0.5,
    "16th": 0.25,
    "32nd": 0.125,
    "64th": 0.0625,
    "128th": 0.03125,
    "256th": 0.015625,
}
DEFAULT_PROPS = {
    "staccato_ratio": 0.5,
}

def die(msg):
    raise SystemExit(msg)


def ensure(cond, msg):
    if not cond:
        die(msg)


def note_name(note_idx):
    return f"{NOTE_NAMES[note_idx % 12]}{note_idx // 12}"


def note_pitch(note_idx):
    return max(19, min(0xffff, round(16.3515978313 * 2.0 ** (note_idx / 12.0))))


def merge_rests(notes):
    ret = []

    for idx, ms in notes:
        if idx is None:
            if not ret:
                continue
            if ret[-1][0] is None:
                ret[-1] = (None, ret[-1][1] + ms)
                continue
        ret.append((idx, ms))

    return ret


def split_repeated_notes(notes):
    ret = []
    count = 0

    for (cur_idx, cur_ms) in notes:
        (prev_idx, prev_ms) = ret[-1] if ret else (None, 0)

        if cur_idx is not None and prev_idx == cur_idx:
            ret[-1] = (prev_idx, max(1, prev_ms - MIN_REST_MS))
            ret.append((None, MIN_REST_MS))
            count += 1

        ret.append((cur_idx, cur_ms))

    if count:
        print(f"Split {count} repeated notes")

    return ret


def mxml_text(elem, path):
    child = elem.find(path)

    if child is None or child.text is None:
        return None

    return child.text.strip() or None


def mxml_props(root):
    props = {}

    for field in root.findall("identification/miscellaneous/miscellaneous-field"):
        if (field.get("name") or "").strip().lower() != "props":
            continue

        for item in (field.text or "").split(","):
            item = item.strip()

            if not item:
                continue

            (key, sep, value) = item.partition(":")
            ensure(sep, f"Error: invalid property {item!r}, expected key:value")

            props[key.strip()] = value.strip()

    return props


def mxml_note_idx(elem, where):
    steps = {"C": 0, "D": 2, "E": 4, "F": 5, "G": 7, "A": 9, "B": 11}

    pitch = elem.find("pitch")
    ensure(pitch is not None, f"Error: {where}: note has no pitch")

    step = mxml_text(pitch, "step")
    ensure(step in steps, f"Error: {where}: invalid pitch step {step!r}")

    octave = mxml_text(pitch, "octave")
    ensure(octave is not None, f"Error: {where}: note has no octave")

    alter = mxml_text(pitch, "alter") or "0"
    note_idx = int(octave) * 12 + steps[step] + round(float(alter))

    ensure(note_idx >= 0, f"Error: {where}: note is below C0")
    ensure(note_idx <= 119, f"Error: {where}: note is above B9")

    return note_idx


def mxml_append_note(notes, held, props):
    (note_idx, ms, staccato) = held

    if staccato and note_idx is not None and ms >= 2:
        note_ms = min(ms - 1, max(1, int(ms * props["staccato_ratio"])))
        notes.append((note_idx, note_ms))
        notes.append((None, ms - note_ms))
    else:
        notes.append((note_idx, ms))


def mxml_grace_ms(elem, tempo, where):
    note_type = mxml_text(elem, "type") or "16th"

    ensure(note_type in NOTE_TYPES, f"Error: {where}: invalid note type {note_type!r}")

    quarters = NOTE_TYPES[note_type]

    return max(1, round(quarters * 60000.0 / tempo))


def mxml_append_graces(notes, graces, where):
    need_ms = sum(ms for (_, ms) in graces)
    avail_ms = sum(ms for (_, ms) in notes)

    ensure(avail_ms > need_ms, f"Error: {where}: no time available for grace notes")

    while need_ms > 0:
        (prev_idx, prev_ms) = notes[-1]

        if prev_ms > need_ms:
            notes[-1] = (prev_idx, prev_ms - need_ms)
            need_ms = 0
        else:
            notes.pop()
            need_ms -= prev_ms

    notes.extend(graces)


def load_props(raw_props):
    ret = dict(**DEFAULT_PROPS)

    if "staccato_ratio" in raw_props:
        try:
            ret["staccato_ratio"] = max(0, min(1, float(raw_props["staccato_ratio"])))
        except ValueError:
            die(f"Error: invalid value for 'staccato_ratio' prop")

    return ret


def process_musicxml(root):
    notes = []
    divisions = None
    tempo = 120.0
    held = None
    graces = []
    props = load_props(mxml_props(root))

    parts = root.findall("part")
    ensure(parts, "Error: score contains no parts")
    ensure(len(parts) == 1, "Error: multiple parts are not supported")

    for measure in parts[0].findall("measure"):
        number = measure.get("number") or "?"
        loc = f"measure {number}"

        for elem in measure:
            if elem.tag == "attributes":
                if text := mxml_text(elem, "divisions"):
                    divisions = int(text)

                if text := mxml_text(elem, "staves"):
                    ensure(int(text) == 1, f"Error: {loc}: multiple staves are not supported")

            elif elem.tag in ("direction", "sound"):
                sound = elem if elem.tag == "sound" else elem.find("sound")

                if sound is not None and (text := sound.get("tempo")):
                    tempo = float(text)

            elif elem.tag in ("backup", "forward"):
                die(f"Error: {loc}: <{elem.tag}> is not supported")

            elif elem.tag == "note":
                ensure(elem.find("chord") is None, f"Error: {loc}: chords are not supported")

                if elem.find("grace") is not None:
                    ensure(elem.find("rest") is None, f"Error: {loc}: grace rests are not supported")

                    note_idx = mxml_note_idx(elem, loc)
                    ms = mxml_grace_ms(elem, tempo, loc)

                    if DEBUG:
                        print(f"{loc:10s}  {note_name(note_idx):4s}  {ms} (grace)")

                    graces.append((note_idx, ms))
                    continue

                if graces:
                    if held is not None:
                        mxml_append_note(notes, held, props)
                        held = None

                    mxml_append_graces(notes, graces, loc)
                    graces = []

                duration = mxml_text(elem, "duration")
                ensure(duration is not None, f"Error: {loc}: note has no duration")
                ensure(divisions, f"Error: {loc}: note before <divisions> was declared")

                note_idx = None if elem.find("rest") is not None else mxml_note_idx(elem, loc)
                ms = max(1, round(int(duration) / divisions * 60000.0 / tempo))

                ties = {tie.get("type") for tie in elem.findall("tie")}
                staccato = elem.find("notations/articulations/staccato") is not None

                if DEBUG:
                    name = "P" if note_idx is None else note_name(note_idx)
                    print(f"{loc:10s}  {name:4s}  {ms}")

                if held is not None and "stop" in ties and held[0] == note_idx:
                    held = (note_idx, held[1] + ms, held[2] or staccato)
                else:
                    if held is not None:
                        mxml_append_note(notes, held, props)
                    held = (note_idx, ms, staccato)

                if "start" not in ties:
                    mxml_append_note(notes, held, props)
                    held = None

    if held is not None:
        mxml_append_note(notes, held, props)

    notes.append((None, 2000))
    notes = merge_rests(notes)
    notes = split_repeated_notes(notes)
    notes = [(idx, min(ms, 0xffff)) for (idx, ms) in notes]

    return notes


def read_musicxml(path):
    try:
        tree = ET.parse(path)
    except ET.ParseError as e:
        die(f"Error: {path}: {e}")

    root = tree.getroot()

    for elem in root.iter():
        if isinstance(elem.tag, str) and "}" in elem.tag:
            elem.tag = elem.tag.split("}")[-1]

    ensure(root.tag == "score-partwise", f"Error: {path}: only score-partwise is supported")

    title = mxml_text(root, "work/work-title") or "Unnamed"
    notes = process_musicxml(root)

    return title, notes


def write_spk(path, title, notes):
    with open(path, "w") as f:
        f.write(f"title: {title}\n")
        for note_idx, ms in notes:
            pitch = 0 if note_idx is None else note_pitch(note_idx)
            f.write(f"{pitch}, {ms}\n")


def main():
    parser = argparse.ArgumentParser(description="Convert from MusicXML to SPK")
    parser.add_argument("-i", "--input", metavar="PATH", required=True, help="MusicXML file to read")
    parser.add_argument("-o", "--output", metavar="PATH", required=True, help="SPK file to write")
    parser.add_argument("-t", "--title", metavar="TITLE", help="Song title")
    parser.add_argument("-d", "--debug", action="store_true", help="Dump input events")
    args = parser.parse_args()

    global DEBUG
    DEBUG = args.debug

    (title, notes) = read_musicxml(args.input)

    ensure(notes, f"Error: no playable notes in {args.input}")

    write_spk(args.output, args.title or title, notes)

    total_ms = sum(ms for (_, ms) in notes)

    print(f"Saved {args.output}: \"{title}\" ({len(notes)} notes, {total_ms} ms)")

if __name__ == "__main__":
    main()
