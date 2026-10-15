"""Build and run the bit-exact library's tests.

    uv run tools/bitexact.py

Generates the tables (tools/bitexact_tables.py) into build/bitexact/bx_tables.c
and checks the sine table and the angle doubles against the bytes of
orig/TotalA.exe. The exe's bytes also supply the original's g_sinTable,
g_cosTable, g_radToAngle16 and g_angle16ToRad, while the port uses the
generated tables. Compiles src/util/math_4b70a0.cpp as it is (the flags tools/check.py uses) and
lib/bitexact side by side into one MSVC 5 console exe under Wine, which calls
both over every 16-bit angle, a sweep of scales and fixed-seed random inputs,
and prints `name: N cases, M mismatches` for each entry point. The exit status
is non-zero on any mismatch. Everything is built under build/bitexact/.
"""

import argparse
import os
import struct
import subprocess
import sys
from pathlib import Path

import bitexact_tables
from check import ROOT, Original, compile_source, winpath

LIB = ROOT / "lib/bitexact"
BUILD = ROOT / "build/bitexact"
ORIGINAL = ROOT / "src/util/math_4b70a0.cpp"
TABLES = BUILD / "bx_tables.c"
ORIG_DATA = BUILD / "orig_data.c"
PORT_SOURCES = [LIB / "trig.c", TABLES, ORIG_DATA]
TEST = LIB / "test/bitexact_test.c"
CFLAGS = ["/nologo", "/c", "/O2", "/Op", "/MT", "/W3", "/TC", "/Zm1000"]
LIBS = ["LIBCMT.LIB", "KERNEL32.LIB"]


def compile_c(src: Path) -> Path | None:
    obj = BUILD / (src.stem + ".obj")
    deps = [src, *LIB.glob("*.h"), ROOT / "tools/bitexact_tables.py"]
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


def generate_and_check() -> bool:
    """Write bx_tables.c and orig_data.c, and compare the generated values with the exe."""
    BUILD.mkdir(parents=True, exist_ok=True)
    bitexact_tables.generate(TABLES)
    orig = Original()
    sin = bitexact_tables.sin_table()
    rad_to_angle16, angle16_to_rad = bitexact_tables.angle_doubles()
    exe_sin = orig.read(bitexact_tables.SIN_TABLE, 2 * len(sin))
    exe_cos = orig.read(bitexact_tables.SIN_TABLE + 256, 1024)
    exe_doubles = orig.read(bitexact_tables.RAD_TO_ANGLE16, 16)
    ok = True
    if struct.pack(f"<{len(sin)}h", *sin) != exe_sin:
        print("tables: generated sine table differs from orig/TotalA.exe", file=sys.stderr)
        ok = False
    if struct.pack("<512h", *sin[128:640]) != exe_cos:
        print("tables: generated cosine view differs from orig/TotalA.exe", file=sys.stderr)
        ok = False
    if struct.pack("<dd", rad_to_angle16, angle16_to_rad) != exe_doubles:
        print("tables: generated doubles differ from orig/TotalA.exe", file=sys.stderr)
        ok = False
    if not ok:
        return False
    exe_rad, exe_rad2 = struct.unpack("<dd", exe_doubles)
    assert struct.pack("<dd", float(f"{exe_rad:.17g}"), float(f"{exe_rad2:.17g}")) == exe_doubles
    words = lambda b: ", ".join(f"0x{w:04x}" for w in struct.unpack(f"<{len(b) // 2}H", b))
    ORIG_DATA.write_text(
        f"unsigned short g_sinTable[{len(sin)}] = {{{words(exe_sin)}}};\n"
        f"unsigned short g_cosTable[512] = {{{words(exe_cos)}}};\n"
        f"double g_radToAngle16 = {exe_rad:.17g};\n"
        f"double g_angle16ToRad = {exe_rad2:.17g};\n")
    print("tables: generated sine table and doubles match orig/TotalA.exe")
    return True


def build() -> Path | None:
    if not generate_and_check():
        return None
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
