"""Tests for the dropped initialiser check in aliases.py, on synthetic objects.

    uv run tools/test_aliases.py
"""

import contextlib
import io
import tempfile
import unittest
from pathlib import Path

import aliases
from aliases import Section, Symbol

CODE = 0x60500020


def object_bytes(initialiser: str | None) -> bytes:
    secs = [Section(".text", CODE, b"\xc3", 1)]
    syms = [Symbol("_f", 0, 1, 0x20, aliases.IMAGE_SYM_CLASS_EXTERNAL)]
    if initialiser is not None:
        secs.append(Section(".CRT$XCU", 0xC0300040, b"\0\0\0\0", 4, [[0, 1, 0x06]]))
        syms.append(Symbol(initialiser, 0, 1, 0x20, aliases.IMAGE_SYM_CLASS_EXTERNAL))
    with tempfile.TemporaryDirectory() as tmp:
        path = Path(tmp) / "o.obj"
        aliases.write(0, secs, syms, path)
        return path.read_bytes()


def sections_kept(data: bytes) -> list[str]:
    _, secs, _ = aliases.read(data)
    return [s.name for s in secs if not s.chars & aliases.IMAGE_SCN_LNK_REMOVE]


class InitialiserSymbolsTest(unittest.TestCase):
    def test_names_the_target(self):
        self.assertEqual(aliases.initialiser_symbols(object_bytes("??__Eg_x@@YAXXZ")), ["??__Eg_x@@YAXXZ"])

    def test_none_without_section(self):
        self.assertEqual(aliases.initialiser_symbols(object_bytes(None)), [])

    def test_drop_removes_section(self):
        data = object_bytes("_init")
        self.assertEqual(sections_kept(data), [".text", ".CRT$XCU"])
        self.assertEqual(sections_kept(aliases.drop_initialisers(data)), [".text"])


class ReportTest(unittest.TestCase):
    def test_unlisted_drop_fails(self):
        with self.assertRaises(SystemExit) as raised, contextlib.redirect_stderr(io.StringIO()):
            aliases.report_dropped_initialisers([("src/new.cpp", "_init")], set())
        self.assertEqual(raised.exception.code, 1)

    def test_unlisted_drop_is_named(self):
        err = io.StringIO()
        with self.assertRaises(SystemExit), contextlib.redirect_stderr(err):
            aliases.report_dropped_initialisers([("src/new.cpp", "_init")], set())
        self.assertIn("aliases: dropped dynamic initialiser _init in src/new.cpp", err.getvalue())

    def test_allowed_drop_passes(self):
        aliases.report_dropped_initialisers([("src/old.cpp", "_init")], {("src/old.cpp", "_init")})

    def test_allowance_is_per_file(self):
        with self.assertRaises(SystemExit), contextlib.redirect_stderr(io.StringIO()):
            aliases.report_dropped_initialisers([("src/new.cpp", "_init")], {("src/old.cpp", "_init")})

    def test_no_drops_passes(self):
        aliases.report_dropped_initialisers([], set())

    def test_allowlist_file_parses(self):
        allowed = aliases.load_initialiser_allowlist()
        self.assertIn(("src/game/economy.cpp", "__GLOBAL__sub_I_economy.cpp"), allowed)


if __name__ == "__main__":
    unittest.main()
