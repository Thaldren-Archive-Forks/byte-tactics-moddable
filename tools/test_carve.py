"""Unit tests for the check that rejects references the carve link would misbind
(tools/carve.py: original_refs, reference_problem, target_problem).

    uv run tools/test_carve.py
"""

import struct
import unittest

from collections import Counter, defaultdict
from types import SimpleNamespace

from carve import Positional, marker_address, original_refs, reference_problem, target_problem
from place import REL_DIR32, REL_REL32, Obj, Sec, Sym

BASE = 0x400000
LO, HI = BASE + 0x1000, BASE + 0x200000
VA = 0x401000


def call(target_field: int) -> bytes:
    return b"\xe8" + struct.pack("<i", target_field)


def push_addr(addr: int) -> bytes:
    return b"\x68" + struct.pack("<I", addr)


ORIGINAL = push_addr(0x450000) + call(0x1000) + b"\x56" + call(0x2000) + b"\xc3"
SHIFTED = call(0x3000) + ORIGINAL


def relocs_of(refs):
    return list(refs)


class ReferenceCheck(unittest.TestCase):
    def test_original_refs(self):
        self.assertEqual(original_refs(ORIGINAL, VA, LO, HI), [(1, "abs"), (6, "rel"), (12, "rel")])

    def test_unshifted_passes(self):
        refs = original_refs(ORIGINAL, VA, LO, HI)
        for off, _ in refs:
            self.assertIsNone(reference_problem(refs, relocs_of(refs), off))

    def test_inserted_call_fails(self):
        refs = original_refs(ORIGINAL, VA, LO, HI)
        ours = original_refs(SHIFTED, VA, LO, HI)
        found = [reference_problem(refs, ours, off) for off, _ in ours]
        self.assertTrue(all(found))
        self.assertEqual(found[0][1], 1)

    def test_kind_mismatch_fails(self):
        refs = [(1, "abs")]
        self.assertIsNotNone(reference_problem(refs, [(1, "rel")], 1))

    def test_branch_inside_function_is_no_reference(self):
        code = b"\x90" + call(0) + b"\xc3"
        self.assertEqual(original_refs(code, VA, LO, HI), [])

    def test_target_kinds(self):
        text, slots = (0x401000, 0x1000), {0x4a0000}
        self.assertIsNone(target_problem(REL_REL32, "_f", 0x401800, text, slots))
        self.assertIsNotNone(target_problem(REL_REL32, "_f", 0x4a1000, text, slots))
        self.assertIsNone(target_problem(REL_DIR32, "__imp__Sleep@4", 0x4a0000, text, slots))
        self.assertIsNotNone(target_problem(REL_DIR32, "__imp__Sleep@4", 0x4a0004, text, slots))
        self.assertIsNotNone(target_problem(REL_DIR32, "?g@@3HA", 0x4a0000, text, slots))
        self.assertIsNotNone(target_problem(REL_DIR32, "?g@@3HA", 0x401800, text, slots))
        self.assertIsNone(target_problem(REL_DIR32, "?g@@3HA", 0x4b0000, text, slots))


class UncheckedSites(unittest.TestCase):
    def positional(self, code: bytes):
        img = SimpleNamespace(pristine=bytearray(b"\0" * 0x1000 + code), base=VA - 0x1000, size=0x200000)
        check = Positional.__new__(Positional)
        check.img, check.text, check.slots, check.absolutes, check.cache, check.problems = (
            img, (VA, 0x1000), set(), set(), {}, {})
        check.symbols = {}
        return check

    def test_reference_after_the_jump_table_cut_fails(self):
        code = call(0x1000) + b"\x90" * 3 + b"\xc3"
        own = Sym(0, "$L1", 0, 1, 3, False)
        far = Sym(1, "?F@@YAXXZ", 0, 0, 2, False)
        sec = Sec(1, ".text", 0x60000020, code, [(0, 0, REL_DIR32), (4, 0, REL_DIR32), (1, 1, REL_REL32)])
        obj = SimpleNamespace(syms={0: own, 1: far}, path=SimpleNamespace(name="x.obj"))
        check = self.positional(code)
        found = check.problem("F", obj, sec, 0, len(code), VA, 1, REL_REL32, far, VA + 0x100)
        self.assertIn("jump table", found)

    def test_reference_before_any_table_passes(self):
        code = call(0x1000) + b"\xc3"
        far = Sym(1, "?F@@YAXXZ", 0, 0, 2, False)
        sec = Sec(1, ".text", 0x60000020, code, [(1, 1, REL_REL32)])
        obj = SimpleNamespace(syms={1: far}, path=SimpleNamespace(name="x.obj"))
        check = self.positional(code)
        self.assertIsNone(check.problem("F", obj, sec, 0, len(code), VA, 1, REL_REL32, far, VA + 0x100))

    def shifted(self, symbols):
        original = call(0x1000) + b"\xc3"
        ours = b"\x90" * 5 + call(0x1000) + b"\xc3"
        func = Sym(1, "?F@@YGXXZ", 0, 0, 2, False)
        sec = Sec(1, ".text", 0x60000020, ours, [(6, 1, REL_REL32)])
        obj = SimpleNamespace(syms={1: func}, path=SimpleNamespace(name="x.obj"))
        check = self.positional(original)
        check.symbols = symbols
        return check, obj, sec, func, len(ours)

    def test_misaligned_site_of_an_unmarked_name_fails(self):
        check, obj, sec, func, size = self.shifted({})
        self.assertIsNotNone(check.problem("F", obj, sec, 0, size, VA, 6, REL_REL32, func, VA + 0x100))

    def test_misaligned_site_of_a_marked_name_passes(self):
        check, obj, sec, func, size = self.shifted({"F": VA + 0x100})
        self.assertIsNone(check.problem("F", obj, sec, 0, size, VA, 6, REL_REL32, func, VA + 0x900))

    def test_marker_names_only_plain_functions(self):
        symbols = {"F": 0x401100, "C::F": 0x401200}
        self.assertEqual(marker_address("?F@@YGXXZ", symbols), 0x401100)
        self.assertIsNone(marker_address("?F@C@@QAEXXZ", symbols))
        self.assertIsNone(marker_address("?F@?$T@H@std@@YGXXZ", symbols))


class MarkerBinding(unittest.TestCase):
    NAME = "?F@@YGXXZ"

    def setup(self, votes):
        check = Positional.__new__(Positional)
        check.symbols, check.problems = {"F": VA + 0x200}, {}
        placer = SimpleNamespace(ref_targets=defaultdict(Counter, votes), learned={})
        return check, placer

    def test_marked_callee_without_a_reference_binds_to_its_marker(self):
        check, placer = self.setup({})
        check.bind_markers(placer, {self.NAME})
        self.assertEqual(placer.learned, {self.NAME: VA + 0x200})
        self.assertEqual(placer.ref_targets[self.NAME].most_common(1)[0][0], VA + 0x200)
        self.assertEqual(check.problems, {})

    def test_marker_that_disagrees_with_the_positional_vote_fails(self):
        check, placer = self.setup({self.NAME: Counter({VA + 0x300: 2})})
        check.bind_markers(placer, {self.NAME})
        (message,) = check.problems
        self.assertIn(self.NAME, message)
        self.assertIn(hex(VA + 0x200), message)
        self.assertIn(hex(VA + 0x300), message)
        self.assertEqual(placer.learned, {})


if __name__ == "__main__":
    unittest.main()
