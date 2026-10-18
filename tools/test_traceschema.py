"""Tests for traceschema.py, on synthetic records.

    uv run tools/test_traceschema.py
"""

import unittest
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


if __name__ == "__main__":
    unittest.main()
