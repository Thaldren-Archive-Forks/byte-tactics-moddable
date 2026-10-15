"""Generate the include directory the modern (clang-cl) build compiles against.

    python3 tools/modern_headers.py <msvc5 INCLUDE> <output dir>

clang-cl 22 cannot parse eight of MSVC 5's C++ library headers. This copies
them to <output dir>/include, applies the syntactic patches in PATCHES (each
must match exactly `count` times, or the script fails), and writes
<output dir>/overlay.json, a case-insensitive Clang VFS overlay that serves
the toolchain's upper-case INCLUDE tree with the patched copies in place.
The toolchain itself is never edited. Each PATCHES row is (header, search
text, replacement, expected matches, reason).
"""

import json
import os
import shutil
import sys
from pathlib import Path

DEFAULTED = "template<class _E, class _Tr = char_traits<_E> >"
UNDEFAULTED = "template<class _E, class _Tr>"

PATCHES = [
    ("UTILITY", DEFAULTED, UNDEFAULTED, 2,
     "IOSFWD already gave the default argument; clang rejects a second one"),
    ("STREAMBUF", DEFAULTED, UNDEFAULTED, 1, "default argument repeated after IOSFWD"),
    ("OSTREAM", DEFAULTED, UNDEFAULTED, 1, "default argument repeated after IOSFWD"),
    ("ISTREAM", DEFAULTED, UNDEFAULTED, 2, "default argument repeated after IOSFWD"),
    ("IOS", DEFAULTED, UNDEFAULTED, 1, "default argument repeated after IOSFWD"),
    ("IOSFWD", "struct _CRTIMP char_traits<wchar_t> {", "template<> struct _CRTIMP char_traits<wchar_t> {", 1,
     "explicit specialization needs template<>"),
    ("IOSFWD", "struct _CRTIMP char_traits<char> {", "template<> struct _CRTIMP char_traits<char> {", 1,
     "explicit specialization needs template<>"),
    ("XMEMORY", "class _CRTIMP allocator<void> {", "template<> class _CRTIMP allocator<void> {", 1,
     "explicit specialization needs template<>"),
    ("VECTOR", "class vector<_Bool, _Bool_allocator> {", "template<> class vector<_Bool, _Bool_allocator> {", 1,
     "explicit specialization needs template<>"),
    ("XLOCALE", "class _CRTIMP codecvt<wchar_t, char, mbstate_t> : public codecvt_base {",
     "template<> class _CRTIMP codecvt<wchar_t, char, mbstate_t> : public codecvt_base {", 1,
     "explicit specialization needs template<>"),
    ("XLOCALE", "class _CRTIMP ctype<char> : public ctype_base {",
     "template<> class _CRTIMP ctype<char> : public ctype_base {", 1,
     "explicit specialization needs template<>"),
    ("OSTREAM", "{if (flags() & unitbuf)", "{if (flags() & ios_base::unitbuf)", 1,
     "unqualified unitbuf finds the manipulator function, not the ios_base flag"),
    ("ISTREAM", "if (!_Noskip && flags() & skipws)", "if (!_Noskip && flags() & ios_base::skipws)", 1,
     "unqualified skipws finds the manipulator function, not the ios_base flag"),
]


def tree(directory: Path, patched: dict[str, Path]) -> list[dict]:
    """The overlay entries for a directory, with patched files substituted."""
    out = []
    for entry in sorted(directory.iterdir()):
        if entry.is_dir():
            out.append({"type": "directory", "name": entry.name, "contents": tree(entry, patched)})
        else:
            out.append({"type": "file", "name": entry.name,
                        "external-contents": str(patched.get(entry.name, entry))})
    return out


def main() -> None:
    include = Path(sys.argv[1]).resolve()
    out = Path(sys.argv[2]).resolve()
    copies = out / "include"
    shutil.rmtree(copies, ignore_errors=True)
    copies.mkdir(parents=True)
    names = sorted({p[0] for p in PATCHES})
    texts = {}
    for name in names:
        source = include / name
        if not source.is_file():
            raise SystemExit(f"modern_headers: {source} not found")
        texts[name] = source.read_text(encoding="latin-1")
    for name, search, replacement, count, reason in PATCHES:
        found = texts[name].count(search)
        if found != count:
            raise SystemExit(f"modern_headers: {name}: expected {count} match(es) of {search!r}, found {found}")
        texts[name] = texts[name].replace(search, replacement)
    patched = {}
    for name, text in texts.items():
        target = copies / name
        target.write_bytes(text.encode("latin-1"))
        patched[name] = target
    overlay = {"version": 0, "case-sensitive": False,
               "roots": [{"type": "directory", "name": str(include), "contents": tree(include, patched)}]}
    (out / "overlay.json").write_text(json.dumps(overlay))
    print(f"modern_headers: {len(names)} headers patched, overlay for {include}")


if __name__ == "__main__":
    main()
