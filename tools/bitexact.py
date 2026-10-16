"""Build and run the bit-exact library's tests.

    uv run tools/bitexact.py [--compiler {both,msvc,clang}]

Generates the tables (tools/bitexact_tables.py) into build/bitexact/bx_tables.c
and checks the sine table and the angle doubles against the bytes of
orig/TotalA.exe. The exe's bytes also supply the original's g_sinTable,
g_cosTable, g_radToAngle16 and g_angle16ToRad, while the port uses the
generated tables. Compiles src/util/math_4b70a0.cpp as it is (the flags tools/check.py uses) and
lib/bitexact side by side into one MSVC 5 console exe under Wine, which calls
both over every 16-bit angle, a sweep of scales and fixed-seed random inputs,
and prints `name: N cases, M mismatches` for each entry point. A second exe
(test/crt_test.c, test/stl_shuffle.cpp) links LIBCMT's own rand, srand and
qsort and MSVC 5's std::random_shuffle beside the rand and qsort ports and
compares them. While the MSVC 5 exes run they also write what the originals
returned (build/bitexact/vectors/*.bin). Then the same tests are built with
clang-cl for i686 (-DBX_REPLAY, the library compiled by clang, no original
linked in) and run under Wine against those vectors, so the port is compared
with MSVC 5's results under both compilers; --compiler clang reruns only that
half on the vectors of the last MSVC run. The exit status is non-zero on any
mismatch. Everything is built under build/bitexact/.
"""

import argparse
import os
import shutil
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
PORT_SOURCES = [LIB / "trig.c", LIB / "math.c", LIB / "strtof.c", ROOT / "cmake/bitexact/sscanf.c", TABLES, ORIG_DATA]
MATH_TEST = LIB / "test/math_test.c"
TEST = LIB / "test/bitexact_test.c"
VECTORS_SOURCE = LIB / "test/vectors.c"
VECTORS = BUILD / "vectors"
MSVC5 = (ROOT / "toolchain/msvc5-sp3").resolve()
CLANG_BUILD = BUILD / "clang"
CRT_PORT_SOURCES = [LIB / "rand.c", LIB / "qsort.c"]
CRT_TESTS = [LIB / "test/crt_test.c", LIB / "test/stl_shuffle.cpp", VECTORS_SOURCE]
CFLAGS = ["/nologo", "/c", "/O2", "/Op", "/MT", "/W3", "/TC", "/Zm1000"]
LIBS = ["LIBCMT.LIB", "KERNEL32.LIB"]


def compile_c(src: Path) -> Path | None:
    obj = BUILD / (src.stem + ".obj")
    deps = [src, *LIB.glob("*.h"), ROOT / "tools/bitexact_tables.py"]
    if obj.exists() and obj.stat().st_mtime > max(d.stat().st_mtime for d in deps):
        return obj
    obj.unlink(missing_ok=True)
    flags = ["/nologo", "/c", "/O2", "/MT", "/W3", "/GX", "/TP", "/Zm1000"] if src.suffix == ".cpp" else CFLAGS
    cmd = [str(ROOT / "tools/wcl"), *flags, f"/I{winpath(LIB)}", f"/Fo{winpath(obj)}", winpath(src)]
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
    for name, negative, base in (("positive", False, bitexact_tables.POW10_POS),
                                 ("negative", True, bitexact_tables.POW10_NEG)):
        size = 12 * bitexact_tables.POW10_ENTRIES
        if bitexact_tables.pow10_table(negative) != orig.read(base, size):
            print(f"tables: generated {name} powers of ten differ from orig/TotalA.exe", file=sys.stderr)
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
    print("tables: generated sine table, doubles and powers of ten match orig/TotalA.exe")
    return True


def build() -> Path | None:
    if not generate_and_check():
        return None
    original, log = compile_source(ORIGINAL, out_dir="bitexact")
    if original is None:
        print(log, file=sys.stderr)
        return None
    return link("bitexact_test.exe", [original], [*PORT_SOURCES, TEST, MATH_TEST, VECTORS_SOURCE])


def build_crt() -> Path | None:
    BUILD.mkdir(parents=True, exist_ok=True)
    return link("crt_test.exe", [], [*CRT_PORT_SOURCES, *CRT_TESTS])


def link(name: str, objects: list[Path], sources: list[Path]) -> Path | None:
    objects = list(objects)
    for src in sources:
        obj = compile_c(src)
        if obj is None:
            return None
        objects.append(obj)
    exe = BUILD / name
    exe.unlink(missing_ok=True)
    rsp = BUILD / (name + ".rsp")
    rsp.write_text("\n".join(winpath(o) for o in objects) + "\n")
    cmd = [str(ROOT / "tools/wlink"), "/nologo", "/INCREMENTAL:NO", "/SUBSYSTEM:CONSOLE",
           f"/OUT:{winpath(exe)}", f"@{winpath(rsp)}", *LIBS]
    proc = subprocess.run(cmd, capture_output=True, text=True, cwd=ROOT)
    log = (proc.stdout + proc.stderr).replace("\r", "")
    if proc.returncode != 0 or not exe.exists():
        print(log, file=sys.stderr)
        return None
    return exe


def run(exe: Path, *args: Path) -> int:
    script = '. "$ROOT/tools/wineenv.sh"; exec wine "$@"'
    env = dict(os.environ, ROOT=str(ROOT))
    return subprocess.run(["bash", "-c", script, "wine", winpath(exe), *(winpath(a) for a in args)], env=env,
                          cwd=ROOT).returncode


def clang_tools() -> dict[str, str] | None:
    tools = {name: shutil.which(name) for name in ("clang-cl", "lld-link", "llvm-dlltool")}
    missing = [name for name, path in tools.items() if path is None]
    if missing:
        print(f"bitexact: --compiler clang needs {', '.join(missing)} (see CONTRIBUTING.md, Modern build)", file=sys.stderr)
        return None
    return tools


def clang_setup(tools: dict[str, str]) -> list[str] | None:
    """The patched headers' overlay and the KERNEL32 import library, made as the modern build does; the
    clang-cl arguments that compile against MSVC 5's headers."""
    CLANG_BUILD.mkdir(parents=True, exist_ok=True)
    if subprocess.run([sys.executable, str(ROOT / "tools/modern_headers.py"), str(MSVC5 / "INCLUDE"),
                       str(CLANG_BUILD)]).returncode != 0:
        return None
    kernel32_def = CLANG_BUILD / "KERNEL32.def"
    for cmd in (["uv", "run", "--project", str(ROOT), str(ROOT / "tools/aliases.py"), "--import-def",
                 str(MSVC5 / "LIB/KERNEL32.LIB"), "--out", str(kernel32_def)],
                [tools["llvm-dlltool"], "-m", "i386", "-k", "-d", str(kernel32_def), "-l",
                 str(CLANG_BUILD / "KERNEL32.lib")]):
        if subprocess.run(cmd, stdout=subprocess.DEVNULL).returncode != 0:
            return None
    return ["--target=i686-pc-windows-msvc", "-fms-compatibility", "-fms-extensions", "-fms-compatibility-version=11.00",
            "/clang:-ffp-contract=off", "-fno-strict-aliasing", "-fwrapv", "/O2", "/GS-", "-w",
            "-vfsoverlay", str(CLANG_BUILD / "overlay.json"), "-imsvc", str(MSVC5 / "INCLUDE"),
            "-DBX_REPLAY", f"-I{LIB}"]


def clang_link(tools: dict[str, str], flags: list[str], name: str, sources: list[Path]) -> Path | None:
    objects = []
    for src in sources:
        obj = CLANG_BUILD / (src.stem + ".obj")
        fpu = ["-msse2", "-mfpmath=sse"] if src.parent != LIB / "test" else ["-mno-sse"]
        proc = subprocess.run([tools["clang-cl"], *flags, *fpu, "/c", f"/Fo{obj}", str(src)], capture_output=True,
                              text=True)
        if proc.returncode != 0:
            print(proc.stdout + proc.stderr, file=sys.stderr)
            return None
        objects.append(obj)
    exe = CLANG_BUILD / name
    cmd = [tools["lld-link"], "/nologo", "/machine:x86", "/subsystem:console", "/entry:mainCRTStartup", "/fixed:no",
           "/safeseh:no", "/incremental:no", "/nodefaultlib", "/force:multiple", f"/out:{exe}", *map(str, objects),
           str(MSVC5 / "LIB/LIBCMT.LIB"), str(MSVC5 / "LIB/OLDNAMES.LIB"), str(CLANG_BUILD / "KERNEL32.lib")]
    proc = subprocess.run(cmd, capture_output=True, text=True)
    if proc.returncode != 0 or not exe.exists():
        print(proc.stdout + proc.stderr, file=sys.stderr)
        return None
    return exe


def run_clang() -> int:
    """The tests built with clang-cl for i686 against the vectors the MSVC 5 exes wrote; 2 if they cannot be built."""
    vectors = {name: VECTORS / f"{name}.bin" for name in ("bitexact", "crt")}
    for path in vectors.values():
        if not path.exists():
            print(f"bitexact: {path} is missing, run the MSVC 5 half first", file=sys.stderr)
            return 2
    tools = clang_tools()
    flags = clang_setup(tools) if tools else None
    if tools is None or flags is None:
        return 2
    exes = [(clang_link(tools, flags, "bitexact_test.exe",
                        [*PORT_SOURCES, TEST, MATH_TEST, VECTORS_SOURCE]), vectors["bitexact"]),
            (clang_link(tools, flags, "crt_test.exe", [*CRT_PORT_SOURCES, CRT_TESTS[0], VECTORS_SOURCE]),
             vectors["crt"])]
    code = 0
    for exe, vec in exes:
        if exe is None:
            return 2
        print(f"clang: {exe.name}")
        sys.stdout.flush()
        code = run(exe, vec) or code
    return code


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("--compiler", choices=("both", "msvc", "clang"), default="both",
                        help="MSVC 5 builds and writes the vectors, clang replays them (default: both)")
    args = parser.parse_args()
    code = 0
    if args.compiler != "clang":
        VECTORS.mkdir(parents=True, exist_ok=True)
        exe = build()
        if exe is None:
            return 2
        code = run(exe, VECTORS / "bitexact.bin")
        crt = build_crt()
        if crt is None:
            return 2
        code = run(crt, VECTORS / "crt.bin") or code
        print("bitexact (MSVC 5): " + ("OK" if code == 0 else f"FAILED (exit {code})"))
    if args.compiler != "msvc":
        clang = run_clang()
        print("bitexact (clang): " + ("OK" if clang == 0 else f"FAILED (exit {clang})"))
        code = clang or code
    return code


if __name__ == "__main__":
    sys.exit(main())
