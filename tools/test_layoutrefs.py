"""Tests for layoutrefs.py, on hand-written globals and instruction bytes.

    uv run tools/test_layoutrefs.py
"""

import struct
import unittest

import layoutrefs as lr
from layoutrefs import Globals, Ref, Row

TABLE = Row(0x512370, "table", 0x3E8)
OTHER = Row(0x512758, "other", 8)
END = Row(0x512770, "end", 16)
FAR = Row(0x512900, "far", 4)
GLOBALS = Globals([TABLE, OTHER, END, FAR])


def code(*prefix: int) -> tuple[bytes, int]:
    """Instruction bytes ending in a 4-byte immediate, and the offset of the immediate."""
    return bytes(prefix) + struct.pack("<I", 0x512770) + b"\x90", len(prefix)


class UseTest(unittest.TestCase):
    def test_mov_immediate_is_address(self):
        self.assertEqual(lr.use_of(*code(0xB9)), "address")

    def test_compare_immediate(self):
        self.assertEqual(lr.use_of(*code(0x81, 0xFE)), "compare")
        self.assertEqual(lr.use_of(*code(0x3D)), "compare")

    def test_push_and_lea_are_arguments(self):
        self.assertEqual(lr.use_of(*code(0x68)), "argument")
        self.assertEqual(lr.use_of(*code(0x8D, 0x0D)), "argument")

    def test_load_and_store_are_memory(self):
        self.assertEqual(lr.use_of(*code(0x8B, 0x0D)), "memory")
        self.assertEqual(lr.use_of(*code(0xA1)), "memory")

    def test_offset_zero(self):
        self.assertEqual(lr.use_of(struct.pack("<I", 0), 0), "memory")


class PastEndTest(unittest.TestCase):
    def test_inside(self):
        self.assertFalse(lr.past_end(0, 8))
        self.assertFalse(lr.past_end(7, 8))

    def test_at_or_after_the_size(self):
        self.assertTrue(lr.past_end(8, 8))
        self.assertTrue(lr.past_end(0x400, 8))

    def test_negative(self):
        self.assertTrue(lr.past_end(-4, 8))

    def test_rows_name_the_neighbour(self):
        refs = [Ref(TABLE, 0x3E8, "memory"), Ref(TABLE, 4, "memory"), Ref(OTHER, -8, "memory")]
        rows = lr.rows_of("f", "0x401000", "src/f.cpp", refs, GLOBALS)
        self.assertEqual([(r[3], r[4], r[5], r[7]) for r in rows],
                         [("past-end", "table", 0x3E8, "other+0x0"), ("past-end", "other", -8, "table+0x3e0")])

    def test_neighbour_in_a_gap_is_relative_to_the_global_before(self):
        self.assertEqual(GLOBALS.neighbour(0x512780), "end+0x10")


class EndOfTest(unittest.TestCase):
    def test_address_at_the_end_of_an_array(self):
        refs = [Ref(TABLE, 0, "address"), Ref(END, 0, "compare")]
        self.assertEqual(lr.end_candidates(refs, GLOBALS), {("end", "table")})

    def test_a_global_between_means_it_is_not_the_end(self):
        refs = [Ref(TABLE, 0, "address"), Ref(OTHER, 0, "memory"), Ref(END, 0, "compare")]
        self.assertEqual(lr.end_candidates(refs, GLOBALS), {("end", "other")})

    def test_a_loaded_global_is_not_an_end(self):
        refs = [Ref(TABLE, 0, "address"), Ref(END, 0, "memory")]
        self.assertEqual(lr.end_candidates(refs, GLOBALS), set())

    def test_an_argument_is_an_end(self):
        refs = [Ref(TABLE, 0, "address"), Ref(END, 0, "argument")]
        self.assertEqual(lr.end_candidates(refs, GLOBALS), {("end", "table")})

    def test_a_nonzero_addend_is_not_an_end(self):
        refs = [Ref(TABLE, 0, "address"), Ref(END, 4, "compare")]
        self.assertEqual(lr.end_candidates(refs, GLOBALS), set())

    def test_too_far_after_the_array(self):
        refs = [Ref(TABLE, 0, "address"), Ref(FAR, 0, "compare")]
        self.assertEqual(lr.end_candidates(refs, GLOBALS), set())

    def test_before_the_array(self):
        refs = [Ref(END, 0, "memory"), Ref(TABLE, 0, "compare")]
        self.assertEqual(lr.end_candidates(refs, GLOBALS), set())

    def test_rows(self):
        refs = [Ref(TABLE, 0, "address"), Ref(END, 0, "compare")]
        self.assertEqual(lr.rows_of("f", "0x401000", "src/f.cpp", refs, GLOBALS),
                         [["f", "0x401000", "src/f.cpp", "end-of", "end", 0, "table", "", ""]])


if __name__ == "__main__":
    unittest.main()
