"""Tests for typedupes.py, on fixture sources, mocked layouts and hand-written lists.

    uv run tools/test_typedupes.py
"""

import contextlib
import io
import sys
import tempfile
import unittest
from pathlib import Path
from unittest import mock

import typedupes as td

SOURCES = {
    "src/a.h": """struct Dup { int x; };
class Forward;
struct Forward2;
#if 0
struct Dead { int a; };
#endif
typedef struct { int q; } Aliased;
struct Wrap { struct Inner { int i; } in; };
""",
    "src/a.cpp": """#include "a.h"
struct Other { int a; };
struct Dead { int a; };
""",
    "src/b.cpp": """struct Dup { int x; };
struct Other { int b; float c; };
typedef struct { int q; } Aliased;
struct Forward { int f; };
""",
    "link/c.cpp": """struct Wrap { struct Inner { int i; } in; };
""",
}


def write_tree(root: Path, sources: dict) -> None:
    for rel, text in sources.items():
        path = root / rel
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(text)


def row(name, defs, files, layout="differs", in_scope="no", canonical=None):
    return {"name": name, "in_scope": in_scope, "defs": str(defs), "files": "|".join(files), "layout": layout,
            "canonical": canonical or files[0].split(":")[0]}


def definition(file, line=1, text="struct X { int a; };"):
    return {"file": file, "line": line, "id": "%s:%d" % (file, line), "norm": text}


def sig(size, *members):
    return size, tuple((o, t, n) for o, t, n in members)


class ScanTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.tmp = tempfile.TemporaryDirectory()
        cls.root = Path(cls.tmp.name)
        write_tree(cls.root, SOURCES)
        cls.files = td.collect(cls.root)
        cls.by_name = td.definitions(cls.files)

    @classmethod
    def tearDownClass(cls):
        cls.tmp.cleanup()

    def names(self, *, many=False):
        return sorted(n for n, ds in self.by_name.items() if (len(ds) > 1) == many)

    def test_a_name_with_two_bodies_is_listed(self):
        self.assertEqual(self.names(many=True), ["Aliased", "Dup", "Other", "Wrap", "Wrap::Inner"])

    def test_a_block_after_if_0_is_not_a_definition(self):
        self.assertEqual([d["id"] for d in self.by_name["Dead"]], ["src/a.cpp:3"])

    def test_a_forward_declaration_is_not_a_definition(self):
        self.assertEqual([d["id"] for d in self.by_name["Forward"]], ["src/b.cpp:4"])
        self.assertNotIn("Forward2", self.by_name)

    def test_a_typedef_struct_is_defined_under_the_typedef_name(self):
        self.assertEqual(sorted(d["id"] for d in self.by_name["Aliased"]), ["src/a.h:7", "src/b.cpp:3"])

    def test_a_nested_record_is_qualified_by_its_outer_one(self):
        self.assertEqual(sorted(d["id"] for d in self.by_name["Wrap::Inner"]), ["link/c.cpp:1", "src/a.h:8"])

    def test_the_definitions_of_a_name_keep_file_and_line(self):
        self.assertEqual(sorted(d["id"] for d in self.by_name["Dup"]), ["src/a.h:1", "src/b.cpp:1"])

    def test_includes_follow_the_header(self):
        self.assertIn("src/a.h", td.visible(self.files, "src/a.cpp"))
        self.assertNotIn("src/a.h", td.visible(self.files, "src/b.cpp"))


class LayoutTest(unittest.TestCase):
    def test_a_dump_gives_size_and_members_of_each_record(self):
        dump = """*** Dumping AST Record Layout
         0 | struct Point
         0 |   int x
         4 |   int y
           | [sizeof=8, dsize=8, align=4,
           |  nvsize=8, nvalign=4]

*** Dumping AST Record Layout
         0 | class Empty (empty)
           | [sizeof=1, dsize=1, align=1,
           |  nvsize=0, nvalign=1]
"""
        recs = td.parse_layouts(dump)
        self.assertEqual(recs["Point"]["size"], 8)
        self.assertEqual(recs["Point"]["rows"], [(0, "int x"), (4, "int y")])
        self.assertEqual(recs["Empty"]["rows"], [])
        self.assertEqual(td.signature(recs["Point"]), (8, ((0, "int", "x"), (4, "int", "y"))))

    def test_identical_text_is_identical(self):
        ds = [definition("a.h", text="struct X { int a; };"), definition("b.cpp", text="struct X { int a; };")]
        self.assertEqual(td.layout_class(ds, [None, None]), "identical")

    def test_different_text_with_the_same_members_is_same_layout(self):
        ds = [definition("a.h", text="struct X { int a; };"), definition("b.cpp", text="struct X { int a; void F(); };")]
        same = sig(4, (0, "int", "a"))
        self.assertEqual(td.layout_class(ds, [same, same]), "same-layout")

    def test_a_different_member_differs(self):
        ds = [definition("a.h", text="a"), definition("b.cpp", text="b")]
        self.assertEqual(td.layout_class(ds, [sig(4, (0, "int", "a")), sig(4, (0, "int", "b"))]), "differs")

    def test_a_missing_layout_differs(self):
        ds = [definition("a.h", text="a"), definition("b.cpp", text="b")]
        self.assertEqual(td.layout_class(ds, [sig(4, (0, "int", "a")), None]), "differs")


class CanonicalTest(unittest.TestCase):
    def test_the_size_that_matches_3_1c_wins(self):
        ds = [definition("src/a.h"), definition("src/b.cpp", 5)]
        sigs = [sig(8, (0, "int", "a"), (4, "int", "b")), sig(4, (0, "int", "a"))]
        self.assertEqual(td.pick_canonical(ds, sigs, {4}), "src/b.cpp")

    def test_without_a_known_size_a_real_header_wins(self):
        ds = [definition("include/ta_types.h"), definition("src/x.h"), definition("src/y.cpp")]
        sigs = [sig(4, (0, "int", "a"))] * 3
        self.assertEqual(td.pick_canonical(ds, sigs, set()), "src/x.h")

    def test_among_headers_the_most_named_members_win(self):
        ds = [definition("src/a.h"), definition("src/b.h")]
        sigs = [sig(4, (0, "int", "a")), sig(8, (0, "int", "a"), (4, "int", "b"))]
        self.assertEqual(td.pick_canonical(ds, sigs, set()), "src/b.h")

    def test_a_method_only_header_keeps_its_place(self):
        ds = [definition("src/m.h"), definition("src/n.cpp")]
        sigs = [sig(1), sig(8, (0, "int", "a"), (4, "int", "b"))]
        self.assertEqual(td.pick_canonical(ds, sigs, {8}), "src/m.h")

    def test_a_method_only_class_outside_a_real_header_does_not_win(self):
        ds = [definition("src/m.cpp"), definition("src/n.cpp", 9)]
        sigs = [sig(1), sig(8, (0, "int", "a"), (4, "int", "b"))]
        self.assertEqual(td.pick_canonical(ds, sigs, {8}), "src/n.cpp")


class RowsTest(unittest.TestCase):
    def test_rows_come_from_scan_layouts_and_scope(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            write_tree(root, SOURCES)
            files = td.collect(root)
            layouts = {
                "src/a.cpp": {"Dup": {"size": 4, "rows": [(0, "int x")]}, "Other": {"size": 4, "rows": [(0, "int a")]}},
                "src/b.cpp": {"Dup": {"size": 4, "rows": [(0, "int x")]},
                              "Other": {"size": 8, "rows": [(0, "int b"), (4, "float c")]}},
            }
            units = {"src/a.cpp": {"tu": "src/a.cpp"}, "src/b.cpp": {"tu": "src/b.cpp"}}
            state = [{"kind": "global", "name": "g_other", "class": "sim"},
                     {"kind": "global", "name": "g_view", "class": "view"}]
            globals_rows = [{"name": "g_other", "type": "Other", "size": "8", "size_from": "gap"},
                            {"name": "g_view", "type": "Dup", "size": "4", "size_from": "gap"}]
            rows = td.build_rows(files, units, state, globals_rows, 2, run=lambda e: layouts[e["tu"]])
        by_name = {r["name"]: r for r in rows}
        self.assertEqual(by_name["Dup"]["layout"], "identical")
        self.assertEqual(by_name["Dup"]["in_scope"], "no")
        self.assertEqual(by_name["Dup"]["files"], "src/a.h:1|src/b.cpp:1")
        self.assertEqual(by_name["Other"]["layout"], "differs")
        self.assertEqual(by_name["Other"]["in_scope"], "yes")
        self.assertEqual(by_name["Other"]["canonical"], "src/b.cpp")
        self.assertEqual([r["name"] for r in rows], sorted(by_name))

    def test_the_csv_has_the_columns_in_order(self):
        text = td.render([row("A", 2, ["a.h:1", "b.cpp:2"])])
        self.assertEqual(text, "name,in_scope,defs,files,layout,canonical\nA,no,2,a.h:1|b.cpp:2,differs,a.h\n")


class CheckTest(unittest.TestCase):
    committed = [row("A", 2, ["src/a.h:1", "src/b.cpp:2"]), row("B", 2, ["src/c.h:1", "src/d.cpp:2"])]

    def test_an_equal_list_passes_even_when_lines_move(self):
        moved = [row("A", 2, ["src/a.h:9", "src/b.cpp:40"]), self.committed[1]]
        self.assertEqual(td.compare(self.committed, moved), ([], []))

    def test_a_new_name_fails(self):
        fresh = self.committed + [row("C", 2, ["src/e.h:1", "src/f.cpp:2"])]
        growth, stale = td.compare(self.committed, fresh)
        self.assertEqual(len(growth), 1)
        self.assertIn("new name C", growth[0])

    def test_an_added_definition_fails(self):
        fresh = [row("A", 3, ["src/a.h:1", "src/b.cpp:2", "src/z.cpp:3"]), self.committed[1]]
        growth, stale = td.compare(self.committed, fresh)
        self.assertEqual(len(growth), 1)
        self.assertIn("A gained a definition", growth[0])
        self.assertIn("src/z.cpp", growth[0])

    def test_a_definition_moved_to_another_file_fails(self):
        fresh = [row("A", 2, ["src/a.h:1", "src/z.cpp:2"]), self.committed[1]]
        self.assertEqual(len(td.compare(self.committed, fresh)[0]), 1)

    def test_a_shrunk_list_is_stale(self):
        growth, stale = td.compare(self.committed, [self.committed[0]])
        self.assertEqual(growth, [])
        self.assertEqual(stale, ["B is gone"])

    def test_fewer_definitions_are_stale(self):
        fresh = [row("A", 1, ["src/a.h:1"]), self.committed[1]]
        growth, stale = td.compare(self.committed, fresh)
        self.assertEqual(growth, [])
        self.assertEqual(len(stale), 1)

    def test_a_changed_layout_is_stale(self):
        fresh = [row("A", 2, ["src/a.h:1", "src/b.cpp:2"], layout="identical"), self.committed[1]]
        self.assertEqual(td.compare(self.committed, fresh)[1], ["A changed: layout"])


class MainTest(unittest.TestCase):
    def run_main(self, committed, fresh):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "type_dupes.csv"
            path.write_text(td.render(committed))
            out = io.StringIO()
            with mock.patch.object(td, "OUT", path), mock.patch.object(td, "generate", return_value=fresh), \
                    mock.patch.object(sys, "argv", ["typedupes.py", "--check"]), contextlib.redirect_stdout(out):
                try:
                    td.main()
                    code = 0
                except SystemExit as e:
                    code = e.code
        return code, out.getvalue()

    def test_a_current_list_prints_the_counts(self):
        rows = [row("A", 2, ["a.h:1", "b.cpp:2"], in_scope="yes"), row("B", 2, ["c.h:1", "d.cpp:2"])]
        code, out = self.run_main(rows, rows)
        self.assertEqual(code, 0)
        self.assertEqual(out.strip(), "typedupes: 2 names with more than one definition (1 in scope), list up to date")

    def test_a_new_name_fails(self):
        rows = [row("A", 2, ["a.h:1", "b.cpp:2"])]
        code, out = self.run_main(rows, rows + [row("N", 2, ["n.h:1", "m.cpp:2"])])
        self.assertEqual(code, 1)
        self.assertIn("new name N", out)

    def test_an_added_definition_fails(self):
        code, out = self.run_main([row("A", 2, ["a.h:1", "b.cpp:2"])], [row("A", 3, ["a.h:1", "b.cpp:2", "c.cpp:3"])])
        self.assertEqual(code, 1)
        self.assertIn("gained a definition", out)

    def test_a_stale_list_says_to_rerun(self):
        code, out = self.run_main([row("A", 2, ["a.h:1", "b.cpp:2"])], [])
        self.assertEqual(code, 1)
        self.assertIn("run `uv run tools/typedupes.py`", out)


if __name__ == "__main__":
    unittest.main()
