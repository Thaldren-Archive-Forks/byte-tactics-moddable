"""Pull every numeric value string out of the TDF text files in the player's install.

    uv run tools/tdfnumbers.py [--game-dir DIR] [--out FILE]

Reads the .hpi, .ufo, .ccx and .gp3 archives of DIR (default ~/.ta) and loose
files below it, and for each .tdf, .fbi, .ota, .gui, .sht and .ini text file
collects the value of every `name=value;` assignment that is a number (an
optional sign, digits with an optional point, an optional exponent). The
distinct strings are written one per line to FILE (default build/tdf_numbers.txt),
which tools/bitexact.py feeds to the atof test. The strings come from the
player's files, so the output stays under build/ and is never committed.
"""

import argparse
import re
import struct
import sys
import zlib
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
ARCHIVES = (".hpi", ".ufo", ".ccx", ".gp3")
TEXT = (".tdf", ".fbi", ".ota", ".gui", ".sht", ".ini")
ASSIGN = re.compile(rb"([A-Za-z_][\w. ]*?)\s*=\s*([^;{}\r\n]*?)\s*;")
NUMBER = re.compile(rb"[+-]?(\d+\.?\d*|\.\d+)([eE][+-]?\d+)?")
CHUNK = 65536


def lz77(data: bytes, out_size: int) -> bytes:
    """Decompress one TA HPI LZ77 chunk."""
    window = bytearray(4096)
    pos = 1
    out = bytearray()
    i = 0
    while len(out) < out_size and i < len(data):
        tag = data[i]
        i += 1
        for _ in range(8):
            if tag & 1 == 0:
                b = data[i]
                i += 1
                out.append(b)
                window[pos] = b
                pos = (pos + 1) & 0xFFF
            else:
                packed = data[i] | data[i + 1] << 8
                i += 2
                offset = packed >> 4
                if offset == 0:
                    return bytes(out)
                for _ in range((packed & 0xF) + 2):
                    b = window[offset]
                    offset = (offset + 1) & 0xFFF
                    out.append(b)
                    window[pos] = b
                    pos = (pos + 1) & 0xFFF
            tag >>= 1
            if len(out) >= out_size:
                break
    return bytes(out)


class Hpi:
    """A read-only HAPI archive."""

    def __init__(self, path: Path):
        raw = bytearray(path.read_bytes())
        magic, version, _size, key, start = struct.unpack_from("<5I", raw)
        if magic != 0x49504148 or version != 0x00010000:
            raise ValueError("not a HAPI archive")
        if key:
            key = ~((key << 2) | (key >> 6)) & 0xFF
            for i in range(start, len(raw)):
                raw[i] = ((i ^ key) & 0xFF) ^ (~raw[i] & 0xFF)
        self.raw = bytes(raw)
        self.start = start

    def files(self):
        """Yield (path, offset, size, method) for every file."""
        yield from self._dir(self.start, "")

    def _dir(self, offset: int, prefix: str):
        count, entries = struct.unpack_from("<II", self.raw, offset)
        for n in range(count):
            name_off, data_off, flag = struct.unpack_from("<IIB", self.raw, entries + 9 * n)
            end = self.raw.index(b"\0", name_off)
            name = self.raw[name_off:end].decode("latin-1")
            if flag == 1:
                yield from self._dir(data_off, prefix + name + "/")
            else:
                file_off, size, method = struct.unpack_from("<IIB", self.raw, data_off)
                yield prefix + name, file_off, size, method

    def read(self, offset: int, size: int, method: int) -> bytes:
        if method == 0:
            return self.raw[offset:offset + size]
        chunks = (size + CHUNK - 1) // CHUNK
        sizes = struct.unpack_from(f"<{chunks}I", self.raw, offset)
        pos = offset + 4 * chunks
        out = bytearray()
        for csize in sizes:
            head = struct.unpack_from("<IBBBIII", self.raw, pos)
            _marker, _unk, kind, enc, packed, plain, _sum = head
            body = bytearray(self.raw[pos + 19:pos + 19 + packed])
            if enc:
                body = bytearray(((b - i) & 0xFF) ^ (i & 0xFF) for i, b in enumerate(body))
            if kind == 1:
                out += lz77(bytes(body), plain)
            elif kind == 2:
                out += zlib.decompress(bytes(body))
            else:
                out += body
            pos += csize
        return bytes(out)


def numbers_in(text: bytes, found: set[bytes]) -> None:
    for m in ASSIGN.finditer(text):
        value = m.group(2)
        if NUMBER.fullmatch(value):
            found.add(value)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("--game-dir", type=Path, default=Path.home() / ".ta")
    parser.add_argument("--out", type=Path, default=ROOT / "build/tdf_numbers.txt")
    args = parser.parse_args()
    found: set[bytes] = set()
    texts = 0
    for path in sorted(args.game_dir.rglob("*")):
        suffix = path.suffix.lower()
        if suffix in ARCHIVES and path.is_file():
            try:
                hpi = Hpi(path)
                for name, offset, size, method in hpi.files():
                    if name.lower().endswith(TEXT):
                        numbers_in(hpi.read(offset, size, method), found)
                        texts += 1
            except (ValueError, struct.error, zlib.error, IndexError) as err:
                print(f"tdfnumbers: skipped {path.name}: {err}", file=sys.stderr)
        elif suffix in TEXT and path.is_file():
            numbers_in(path.read_bytes(), found)
            texts += 1
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_bytes(b"".join(v + b"\n" for v in sorted(found)))
    print(f"tdfnumbers: {len(found)} distinct numeric strings from {texts} text files -> {args.out}")
    return 0 if found else 1


if __name__ == "__main__":
    sys.exit(main())
