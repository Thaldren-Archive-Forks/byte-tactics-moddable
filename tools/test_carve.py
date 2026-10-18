"""Unit tests for the check that rejects references the carve link would misbind
(tools/carve.py: original_refs, reference_problem, target_problem).

    uv run tools/test_carve.py
"""

import struct
import unittest

from carve import original_refs, reference_problem, target_problem
from place import REL_DIR32, REL_REL32

BASE = 0x400000
LO, HI = BASE + 0x1000, BASE + 0x200000
VA = 0x401000


def call(target_field: int) -> bytes:
    return b"\xe8" + struct.pack("<i", target_field)


def push_addr(addr: int) -> bytes:
    return b"\x68" + struct.pack("<I", addr)


# push 0x450000; call far; push esi; call far; ret
ORIGINAL = push_addr(0x450000) + call(0x1000) + b"\x56" + call(0x2000) + b"\xc3"
# The same function with one call inserted at the top.
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


if __name__ == "__main__":
    unittest.main()
