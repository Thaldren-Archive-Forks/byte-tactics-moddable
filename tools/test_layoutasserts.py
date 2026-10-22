"""Tests for layoutasserts.py, on canned clang record-layout text.

    uv run tools/test_layoutasserts.py
"""

import unittest

import layoutasserts as la

LAYOUT = """*** Dumping AST Record Layout
         0 | struct Pair
         0 |   int a
         4 |   int b
         8:0-2 |   unsigned int flag
           | [sizeof=12, align=4]

*** Dumping AST Record Layout
         0 | class Poly
         0 |   (Poly vtable pointer)
         4 |   struct Pair base (base)
         4 |   int x
         8 |   union (anonymous at a.h:3:5)
         8 |     int u
         8 |     float f
        12 |   char[4] tail
           | [sizeof=16, align=4]

*** Dumping AST Record Layout
         0 | struct Empty (empty)
           | [sizeof=1, align=1]

*** Dumping AST Record Layout
         0 | struct Ignored
         0 |   int z
           | [sizeof=4, align=4]
"""


class ParseTests(unittest.TestCase):
    def setUp(self):
        self.records = la.parse_records(LAYOUT, {"Pair", "Poly", "Empty"})

    def test_only_wanted_records(self):
        self.assertEqual(sorted(self.records), ["Empty", "Pair", "Poly"])

    def test_sizes(self):
        self.assertEqual([self.records[n].size for n in ("Pair", "Poly", "Empty")], [12, 16, 1])

    def test_bitfields_are_skipped(self):
        self.assertEqual(self.records["Pair"].fields, [("a", 0), ("b", 4)])

    def test_vptr_base_and_anonymous_union_members(self):
        self.assertEqual(self.records["Poly"].fields, [("x", 4), ("u", 8), ("f", 8), ("tail", 12)])

    def test_empty_record_has_a_size_only(self):
        self.assertEqual(self.records["Empty"].fields, [])


class EmitTests(unittest.TestCase):
    def test_emission(self):
        text = la.emit(["../a/a.h"], [la.Record("B", 8, [("x", 4)]), la.Record("A", 4, [("y", 0)])])
        lines = text.split("\n")
        self.assertIn('#include "../a/a.h"', lines)
        self.assertLess(lines.index('#include "../a/a.h"'), lines.index('static_assert(sizeof(A) == 4, "");'))
        self.assertEqual(lines[-5:], ['static_assert(sizeof(A) == 4, "");', 'static_assert(offsetof(A, y) == 0, "");',
                                      'static_assert(sizeof(B) == 8, "");', 'static_assert(offsetof(B, x) == 4, "");',
                                      ""])

    def test_packing_comes_before_the_headers(self):
        text = la.emit(["../a/a.h"], [])
        self.assertLess(text.index("pragma pack(push, 1)"), text.index("a.h"))

    def test_include_order_puts_vec3_first(self):
        homes = {"A": "src/ai/a.h", "V": "src/util/vec3.h"}
        self.assertEqual(la.include_order(homes), ["../util/vec3.h", "../ai/a.h"])

    def test_split_keeps_ta_types_apart(self):
        parts = la.split_units({"A": "src/a.h", "T": "include/ta_types.h"})
        self.assertEqual(parts[la.OUT], {"A": "src/a.h"})
        self.assertEqual(parts[la.OUT_TA], {"T": "include/ta_types.h"})


class DropTests(unittest.TestCase):
    TEXT = 'a\nstatic_assert(offsetof(P, x) == 0, "");\nstatic_assert(sizeof(P) == 4, "");\n'

    def test_private_member_assert_is_dropped(self):
        diag = "unit.cpp:2:1: error: 'x' is a private member of 'P'\n"
        text, n, failed = la.drop_failing(self.TEXT, diag)
        self.assertEqual((n, failed), (1, []))
        self.assertNotIn("offsetof", text)
        self.assertIn("sizeof(P)", text)

    def test_wrong_value_is_reported_and_kept(self):
        diag = "unit.cpp:3:1: error: static assertion failed due to requirement 'sizeof(P) == 4'\n"
        text, n, failed = la.drop_failing(self.TEXT, diag)
        self.assertEqual((text, n), (self.TEXT, 0))
        self.assertEqual(len(failed), 1)

    def test_errors_in_other_files_are_ignored(self):
        diag = "/x/other.h:2:1: error: redefinition\n"
        self.assertEqual(la.drop_failing(self.TEXT, diag), (self.TEXT, 0, []))

    def test_settle_runs_until_clean(self):
        class Result:
            def __init__(self, err):
                self.stderr = err
        answers = [Result("unit.cpp:2:1: error: private\n"), Result("")]
        text, dropped = la.settle(self.TEXT, [], ".", run=lambda *a: answers.pop(0))
        self.assertEqual(dropped, 1)
        self.assertNotIn("offsetof", text)


if __name__ == "__main__":
    unittest.main()
