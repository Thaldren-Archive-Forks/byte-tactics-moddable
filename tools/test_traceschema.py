"""Tests for traceschema.py, on synthetic records.

    uv run tools/test_traceschema.py
"""

import tempfile
import unittest
from pathlib import Path
from unittest import mock

import traceschema as ts

BODY = """    int head;
    union {
        struct {
            short lo;
            short hi;
        };
        int whole;
    };
    union {
        int fine;
        char coarse[4];
    };
    union { int a; char b[4]; };
    int tail;
"""


def leaves_of(name, body, declared):
    with mock.patch.object(ts.Schema, "__init__", lambda self: None):
        schema = ts.Schema()
    schema.cache = {}
    schema.errors = []
    return schema.parse(name, body, declared), schema.errors


class FlattenUnionsTest(unittest.TestCase):
    def test_non_unit_record_keeps_fine_fields(self):
        (leaves, size), errors = leaves_of("Widget", BODY, 0x14)
        self.assertEqual(errors, [])
        self.assertEqual(size, 0x14)
        self.assertEqual([(l["name"], l["offset"], l["size"]) for l in leaves],
                         [("head", 0, 4), ("lo", 4, 2), ("hi", 6, 2), ("fine", 8, 4),
                          ("a", 0xC, 4), ("tail", 0x10, 4)])

    def test_single_line_union_spans_its_widest_member(self):
        (leaves, size), errors = leaves_of("Widget", "    union { char a; int b; };\n    int c;\n", 8)
        self.assertEqual(errors, [])
        self.assertEqual([(l["name"], l["offset"], l["size"]) for l in leaves], [("a", 0, 4), ("c", 4, 4)])

    def test_record_without_unions_is_unchanged(self):
        (leaves, size), errors = leaves_of("Plain", "    int a;\n    int b;\n", 8)
        self.assertEqual(errors, [])
        self.assertEqual([l["name"] for l in leaves], ["a", "b"])


class InlineMethodTest(unittest.TestCase):
    BODY = """    int x;
    int y;

    Point operator+(const Point& o) const
    {
        Point r;
        if (o.x) {
            r.x = x;
        }
        return r;
    }
    int Sum() const { return x + y; }
    void Reset() {
        Point r;
        x = r.x;
    }
    int z;
"""

    def test_method_bodies_add_no_fields(self):
        (leaves, size), errors = leaves_of("Point", self.BODY, 12)
        self.assertEqual(errors, [])
        self.assertEqual(size, 12)
        self.assertEqual([(l["name"], l["offset"], l["size"]) for l in leaves],
                         [("x", 0, 4), ("y", 4, 4), ("z", 8, 4)])


class FunctionPointerTest(unittest.TestCase):
    def test_function_pointer_fields_are_pointer_sized(self):
        body = ("    int a;\n    void (__stdcall *handler)(void);\n"
                "    int (*plain)(int x, char* y);\n    char* (__cdecl *named)();\n    int b;\n")
        (leaves, size), errors = leaves_of("Widget", body, 0x14)
        self.assertEqual(errors, [])
        self.assertEqual([(l["name"], l["offset"], l["size"]) for l in leaves],
                         [("a", 0, 4), ("handler", 4, 4), ("plain", 8, 4), ("named", 0xC, 4), ("b", 0x10, 4)])


class FollowedHeadersTest(unittest.TestCase):
    def test_follows_includes_under_src_transitively_in_order(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp).resolve()
            (root / "include").mkdir()
            (root / "src").mkdir()
            (root / "include/top.h").write_text(
                '#include <stdio.h>\n#include "../src/a.h"\n#include "../src/b.h"\n#include "../src/a.h"\n'
                '#include "missing.h"\n')
            (root / "src/a.h").write_text('#include "c.h"\n')
            (root / "src/b.h").write_text("")
            (root / "src/c.h").write_text('#include "b.h"\n')
            with mock.patch.object(ts, "ROOT", root):
                found = ts.followed_headers(root / "include/top.h")
                moved = ts.moved_types(root / "include/top.h")
            self.assertEqual([p.name for p in found], ["a.h", "c.h", "b.h"])
            self.assertEqual(moved[0], root / "src/units/unit.h")

    def test_real_list_has_unit_and_game(self):
        names = [p.name for p in ts.MOVED_TYPES]
        self.assertEqual(len(names), len(set(names)))
        self.assertIn("unit.h", names)
        self.assertIn("game.h", names)


if __name__ == "__main__":
    unittest.main()
