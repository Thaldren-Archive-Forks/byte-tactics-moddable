"""Reduce a trace.txt to one digest per tick, the form stored in reference/traces/.

    uv run tools/tracedigest.py trace.txt --commit <sha> --scenario <path> > digest.txt

The digest of a tick is the 64-bit FNV-1a hash of that tick's phase hashes in
trace order, each as 8 little-endian bytes. The output holds the header lines of
the trace (the trace format, commit, scenario, seeds and fpu_control_word), then
one `<tick> <digest>` line per tick, the digest in 16 lowercase hex digits.

    bt-digest 1
    trace_format bt-trace 1
    commit <sha>
    scenario reference/scenarios/skirmish2.txt
    game_seed 1
    ...
    1 0123456789abcdef
"""

import argparse
import struct
import sys
from pathlib import Path

from tracediff import Error, parse_trace

FNV_OFFSET = 0xCBF29CE484222325
FNV_PRIME = 0x100000001B3
MASK = (1 << 64) - 1
HEADER_KEYS = ("commit", "scenario", "game_seed", "crt_seed", "view_seed", "fpu_control_word")


def fnv1a64(data: bytes) -> int:
    h = FNV_OFFSET
    for byte in data:
        h = ((h ^ byte) * FNV_PRIME) & MASK
    return h


def digests(rows: list[tuple[int, str, str]]) -> list[tuple[int, str]]:
    """The (tick, 16 hex digits) of each tick of a trace's (tick, phase, hash) rows."""
    out: list[tuple[int, str]] = []
    pending = b""
    current = None
    for tick, _phase, value in rows:
        if tick != current:
            if current is not None:
                out.append((current, f"{fnv1a64(pending):016x}"))
            current, pending = tick, b""
        pending += struct.pack("<Q", int(value, 16))
    if current is not None:
        out.append((current, f"{fnv1a64(pending):016x}"))
    return out


def render(header: dict[str, str], per_tick: list[tuple[int, str]]) -> str:
    """The digest file text for a trace's header and its per-tick digests."""
    lines = ["bt-digest 1", "trace_format bt-trace 1"]
    lines += [f"{k} {header[k]}" for k in HEADER_KEYS]
    lines += [f"{tick} {digest}" for tick, digest in per_tick]
    return "\n".join(lines) + "\n"


def parse_digest(path: Path) -> tuple[dict[str, str], list[tuple[int, str]]]:
    """The header lines as a dict, and the (tick, digest) lines of a digest file."""
    header: dict[str, str] = {}
    rows: list[tuple[int, str]] = []
    for n, line in enumerate(path.read_text().splitlines(), 1):
        words = line.split()
        if n == 1:
            if words != ["bt-digest", "1"]:
                raise Error(f"{path}: not a bt-digest 1 file")
        elif len(words) == 2 and words[0].isdigit() and len(words[1]) == 16:
            rows.append((int(words[0]), words[1]))
        elif rows or not words:
            raise Error(f"{path}:{n}: unexpected line")
        else:
            header[words[0]] = line[len(words[0]):].strip()
    return header, rows


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("trace", help="a trace.txt")
    ap.add_argument("--commit", help="replace the commit header")
    ap.add_argument("--scenario", help="replace the scenario header")
    args = ap.parse_args()
    try:
        header, rows = parse_trace(Path(args.trace))
        for key in ("commit", "scenario"):
            if getattr(args, key):
                header[key] = getattr(args, key)
        missing = [k for k in HEADER_KEYS if k not in header]
        if missing:
            raise Error(f"{args.trace} has no header line for {', '.join(missing)}")
    except (Error, OSError) as e:
        print(f"tracedigest: {e}", file=sys.stderr)
        return 2
    sys.stdout.write(render(header, digests(rows)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
