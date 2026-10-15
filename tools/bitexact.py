"""Build and run the bit-exact library's tests.

    uv run tools/bitexact.py

Compiles src/util/math_4b70a0.cpp as it is (the flags tools/check.py uses) and
lib/bitexact side by side into one MSVC 5 console exe under Wine, which calls
both over every 16-bit angle, a sweep of scales and fixed-seed random inputs,
and prints `name: N cases, M mismatches` for each entry point. The exit status
is non-zero on any mismatch. Everything is built under build/bitexact/.
"""

import argparse
import os
import subprocess
import sys
from pathlib import Path

from check import ROOT, compile_source, winpath

LIB = ROOT / "lib/bitexact"
BUILD = ROOT / "build/bitexact"
ORIGINAL = ROOT / "src/util/math_4b70a0.cpp"
PORT_SOURCES = [LIB / "trig.c", LIB / "bx_tables.c"]
TEST = LIB / "test/bitexact_test.c"
CFLAGS = ["/nologo", "/c", "/O2", "/Op", "/MT", "/W3", "/TC", "/Zm1000"]
LIBS = ["LIBCMT.LIB", "KERNEL32.LIB"]


def compile_c(src: Path) -> Path | None:
    obj = BUILD / (src.stem + ".obj")
    deps = [src, *LIB.glob("*.h")]
    if obj.exists() and obj.stat().st_mtime > max(d.stat().st_mtime for d in deps):
        return obj
    obj.unlink(missing_ok=True)
    cmd = [str(ROOT / "tools/wcl"), *CFLAGS, f"/I{winpath(LIB)}", f"/Fo{winpath(obj)}", winpath(src)]
    proc = subprocess.run(cmd, capture_output=True, text=True, cwd=ROOT)
    log = (proc.stdout + proc.stderr).replace("\r", "")
    if proc.returncode != 0 or not obj.exists():
        print(log, file=sys.stderr)
        return None
    print(f"compiled {src.relative_to(ROOT)}", file=sys.stderr)
    return obj


def build() -> Path | None:
    BUILD.mkdir(parents=True, exist_ok=True)
    original, log = compile_source(ORIGINAL, out_dir="bitexact")
    if original is None:
        print(log, file=sys.stderr)
        return None
    objects = [original]
    for src in [*PORT_SOURCES, TEST]:
        obj = compile_c(src)
        if obj is None:
            return None
        objects.append(obj)
    exe = BUILD / "bitexact_test.exe"
    exe.unlink(missing_ok=True)
    rsp = BUILD / "objects.rsp"
    rsp.write_text("\n".join(winpath(o) for o in objects) + "\n")
    cmd = [str(ROOT / "tools/wlink"), "/nologo", "/INCREMENTAL:NO", "/SUBSYSTEM:CONSOLE",
           f"/OUT:{winpath(exe)}", f"@{winpath(rsp)}", *LIBS]
    proc = subprocess.run(cmd, capture_output=True, text=True, cwd=ROOT)
    log = (proc.stdout + proc.stderr).replace("\r", "")
    if proc.returncode != 0 or not exe.exists():
        print(log, file=sys.stderr)
        return None
    return exe


def run(exe: Path) -> int:
    script = '. "$ROOT/tools/wineenv.sh"; exec wine "$1"'
    env = dict(os.environ, ROOT=str(ROOT))
    return subprocess.run(["bash", "-c", script, "wine", winpath(exe)], env=env, cwd=ROOT).returncode


def main() -> int:
    argparse.ArgumentParser(description=__doc__.split("\n")[0]).parse_args()
    exe = build()
    if exe is None:
        return 2
    code = run(exe)
    print("bitexact: " + ("OK" if code == 0 else f"FAILED (exit {code})"))
    return code


if __name__ == "__main__":
    sys.exit(main())
