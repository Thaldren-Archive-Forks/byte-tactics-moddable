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
    def fails(self, lost, allowed) -> str:
        err = io.StringIO()
        with self.assertRaises(SystemExit) as raised, contextlib.redirect_stderr(err):
            aliases.report_dropped_initialisers(lost, allowed)
        self.assertEqual(raised.exception.code, 1)
        return err.getvalue()

    def test_unlisted_drop_is_named(self):
        err = self.fails([("src/new.cpp", "_init", [])], {})
        self.assertIn("aliases: dropped dynamic initialiser _init in src/new.cpp", err)

    def test_allowed_drop_passes(self):
        aliases.report_dropped_initialisers([("src/old.cpp", "_init", ["_a", "_b"])],
                                            {("src/old.cpp", "_init"): {"_a", "_b"}})

    def test_allowance_is_per_file(self):
        self.fails([("src/new.cpp", "_init", [])], {("src/old.cpp", "_init"): set()})

    def test_extra_ref_fails_and_is_named(self):
        err = self.fails([("src/old.cpp", "_init", ["_a", "_rand"])], {("src/old.cpp", "_init"): {"_a"}})
        self.assertIn("src/old.cpp", err)
        self.assertIn("added: _rand", err)

    def test_missing_ref_fails_and_is_named(self):
        err = self.fails([("src/old.cpp", "_init", ["_a"])], {("src/old.cpp", "_init"): {"_a", "_b"}})
        self.assertIn("removed: _b", err)

    def test_no_drops_passes(self):
        aliases.report_dropped_initialisers([], {})

    def test_allowlist_file_parses(self):
        allowed = aliases.load_initialiser_allowlist()
        self.assertIn("_atexit", allowed[("src/game/economy.cpp", "__GLOBAL__sub_I_economy.cpp")])


class InitialiserRefsTest(unittest.TestCase):
    def test_follows_the_function_and_its_helper(self):
        secs = [Section(".text$a", CODE, b"\xc3", 1, [[0, 1, 0x06], [0, 2, 0x06]]),
                Section(".text$b", CODE, b"\xc3", 1, [[0, 3, 0x06]]),
                Section(".CRT$XCU", 0xC0300040, b"\0\0\0\0", 4, [[0, 0, 0x06]])]
        ext = aliases.IMAGE_SYM_CLASS_EXTERNAL
        syms = [Symbol("__GLOBAL__sub_I_x.cpp", 0, 1, 0x20, ext),
                Symbol("__cxx_global_var_init", 0, 2, 0x20, ext),
                Symbol("_rand", 0, 0, 0x20, ext),
                Symbol("?probe@@3_NB", 0, 0, 0, ext),
                Symbol(".text$a", 0, 1, 0, aliases.IMAGE_SYM_CLASS_STATIC)]
        secs[0].relocs.append([0, 4, 0x06])
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "o.obj"
            aliases.write(0, secs, syms, path)
            refs = aliases.initialiser_refs(path.read_bytes())
        self.assertEqual(refs, {"__GLOBAL__sub_I_x.cpp": ["?probe@@3_NB", "_rand"]})


if __name__ == "__main__":
    unittest.main()
