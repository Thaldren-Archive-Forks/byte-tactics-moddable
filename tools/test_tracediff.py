"""Tests for tracediff.py's translation of the reference's leaf names, on synthetic schemas and dumps.

    uv run tools/test_tracediff.py
"""

import contextlib
import io
import tempfile
import unittest
from pathlib import Path
from unittest import mock

import tracediff as td
from traceschema import TRACE_FORMAT

REFERENCE_SCHEMA = """
static const TraceField kUnitFields[] = {
    { "f10", 0x0, 4, TF_POINTER, TT_HEAP, 0x0 },
    { "health", 0x4, 2, TF_SCALAR, TT_NONE, 0x0 },
    { "", 0x6, 2, TF_BYTES, TT_NONE, 0x0 },
    { "field_8", 0x8, 4, TF_SCALAR, TT_NONE, 0x0 },
};

static const TraceField kPlayerFields[] = {
    { "metal", 0x0, 4, TF_SCALAR, TT_NONE, 0x0 },
    { "energy", 0x4, 4, TF_SCALAR, TT_NONE, 0x0 },
    { "name", 0x8, 4, TF_BYTES, TT_NONE, 0x0 },
    { "", 0xc, 4, TF_BYTES, TT_NONE, 0x0 },
    { "tail", 0x10, 4, TF_SCALAR, TT_NONE, 0x0 },
    { "lo", 0x14, 4, TF_BYTES, TT_NONE, 0x0 },
    { "hi", 0x18, 4, TF_BYTES, TT_NONE, 0x0 },
    { "total", 0x1c, 8, TF_BYTES, TT_NONE, 0x0 },
};
"""

MODERN_SCHEMA = """
static const TraceField kUnitFields[] = {
    { "weapon", 0x0, 4, TF_POINTER, TT_HEAP, 0x0 },
    { "health", 0x4, 2, TF_SCALAR, TT_NONE, 0x0 },
    { "", 0x6, 2, TF_BYTES, TT_NONE, 0x0 },
    { "cargo", 0x8, 4, TF_SCALAR, TT_NONE, 0x0 },
};

static const TraceField kPlayerFields[] = {
    { "energy", 0x0, 4, TF_SCALAR, TT_NONE, 0x0 },
    { "metal", 0x4, 4, TF_SCALAR, TT_NONE, 0x0 },
    { "name", 0x8, 8, TF_BYTES, TT_NONE, 0x0 },
    { "tail", 0x10, 4, TF_SCALAR, TT_NONE, 0x0 },
    { "double", 0x14, 8, TF_SCALAR, TT_NONE, 0x0 },
    { "total", 0x1c, 8, TF_SCALAR, TT_NONE, 0x0 },
};
"""

REFERENCE_DUMP = """100 units units[3].f10 heap
100 units units[3].health 0x0064
100 units units[3].+0x6 hex:0102
100 units units[3].field_8 0x0000000a
100 players players[1].metal 0x00000001
100 players players[1].energy 0x00000002
100 players players[1].name hex:41424344
100 players players[1].+0xc hex:45464748
100 players players[1].tail 0x00000009
100 players players[1].lo hex:00000000
100 players players[1].hi hex:00004040
100 players players[1].total hex:0000000080662640
"""

MODERN_DUMP = """100 units units[3].weapon heap
100 units units[3].health 0x0064
100 units units[3].+0x6 hex:0102
100 units units[3].cargo 0x0000000a
100 players players[1].energy 0x00000001
100 players players[1].metal 0x00000002
100 players players[1].name hex:4142434445464748
100 players players[1].tail 0x00000009
100 players players[1].double 0x4040000000000000
100 players players[1].total 0x4026668000000000
"""


def write_dump(directory: Path, name: str, body: str) -> Path:
    path = directory / name
    path.write_text(f"bt-dump {TRACE_FORMAT}\n{body}")
    return path


class TableMapTest(unittest.TestCase):
    def setUp(self):
        self.maps = td.build_maps(REFERENCE_SCHEMA, MODERN_SCHEMA)

    def test_same_offset_and_size_is_renamed(self):
        renames = self.maps["kUnitFields"].renames
        self.assertEqual(renames["f10"], "weapon")
        self.assertEqual(renames["field_8"], "cargo")
        self.assertEqual(renames["health"], "health")
        self.assertEqual(renames["+0x6"], "+0x6")

    def test_swapped_names_are_each_mapped_to_the_other(self):
        renames = self.maps["kPlayerFields"].renames
        self.assertEqual((renames["metal"], renames["energy"]), ("energy", "metal"))

    def test_split_leaf_is_a_group_not_a_rename(self):
        table = self.maps["kPlayerFields"]
        self.assertNotIn("name", table.renames)
        self.assertNotIn("+0xc", table.renames)
        group = table.reference_groups["name"]
        self.assertIs(group, table.reference_groups["+0xc"])
        self.assertIs(group, table.modern_groups["name"])
        self.assertEqual(group.key, "name")


class DiffDumpsTest(unittest.TestCase):
    def setUp(self):
        self.dir = tempfile.TemporaryDirectory()
        self.addCleanup(self.dir.cleanup)
        self.reference = write_dump(Path(self.dir.name), "reference.txt", REFERENCE_DUMP)
        self.modern = write_dump(Path(self.dir.name), "modern.txt", MODERN_DUMP)
        self.maps = td.build_maps(REFERENCE_SCHEMA, MODERN_SCHEMA)

    def diff(self, reference):
        a, b = (self.reference, self.modern) if reference != "b" else (self.modern, self.reference)
        return td.diff_dumps(a, b, 100, "units", reference, self.maps), td.diff_dumps(a, b, 100, "players", reference, self.maps)

    def test_without_reference_nothing_is_translated(self):
        units, players = self.diff(None)
        self.assertEqual([d[0] for d in units], ["units[3].f10", "units[3].field_8", "units[3].weapon", "units[3].cargo"])
        self.assertEqual([d[0] for d in players],
                         ["players[1].metal", "players[1].energy", "players[1].name", "players[1].+0xc",
                          "players[1].lo", "players[1].hi", "players[1].total", "players[1].double"])
        self.assertIn(("players[1].metal", "0x00000001", "0x00000002"), players)

    def test_reference_a_renames_and_swaps(self):
        units, players = self.diff("a")
        self.assertEqual(units, [])
        self.assertEqual(players, [])

    def test_reference_b_translates_the_other_side(self):
        units, players = self.diff("b")
        self.assertEqual((units, players), ([], []))

    def test_swapped_names_still_report_real_differences(self):
        self.reference.write_text(self.reference.read_text().replace("players[1].metal 0x00000001", "players[1].metal 0x00000007"))
        _, players = self.diff("a")
        self.assertEqual(players, [("players[1].energy", "0x00000007", "0x00000001")])

    def test_split_leaf_compares_on_its_bytes(self):
        self.reference.write_text(self.reference.read_text().replace("hex:45464748", "hex:45464749"))
        _, players = self.diff("a")
        self.assertEqual(players, [("players[1].name", "hex:4142434445464749", "hex:4142434445464748")])

    def test_one_modern_scalar_equals_two_reference_leaves_on_bytes(self):
        _, players = self.diff("a")
        self.assertEqual(players, [])
        self.reference.write_text(self.reference.read_text().replace("hex:00004040", "hex:00004041"))
        _, players = self.diff("a")
        self.assertEqual([d[0] for d in players], ["players[1].double"])

    def test_renamed_leaf_of_a_different_kind_compares_on_bytes(self):
        _, players = self.diff("a")
        self.assertNotIn("players[1].total", [d[0] for d in players])
        self.assertTrue(td.same_bytes("hex:0000000080662640", "0x4026668000000000"))
        self.assertFalse(td.same_bytes("hex:0000000080662640", "0x4026668000000001"))

    def test_wrong_side_leaves_names_untranslated(self):
        units = td.diff_dumps(self.reference, self.modern, 100, "units", "b", self.maps)
        self.assertIn("units[3].f10", [d[0] for d in units])

    def test_unknown_paths_pass_through(self):
        fields = {"mystery[0].x": "0x1", "units[3].f10": "heap"}
        out = td.translate_phase(fields, self.maps, True)
        self.assertEqual(out, {"mystery[0].x": "0x1", "units[3].weapon": "heap"})


class CommandLineTest(unittest.TestCase):
    def run_main(self, *argv):
        out, err = io.StringIO(), io.StringIO()
        with mock.patch("sys.argv", ["tracediff.py", *argv]), mock.patch.object(td, "load_maps", return_value=td.build_maps(REFERENCE_SCHEMA, MODERN_SCHEMA)), \
                contextlib.redirect_stdout(out), contextlib.redirect_stderr(err):
            code = td.main()
        return code, out.getvalue()

    def test_dumps_with_reference_option(self):
        with tempfile.TemporaryDirectory() as d:
            ref = write_dump(Path(d), "ref.txt", REFERENCE_DUMP)
            mod = write_dump(Path(d), "mod.txt", MODERN_DUMP)
            code, out = self.run_main("--dumps", str(ref), str(mod), "--at", "100", "players", "--reference", "a")
            self.assertEqual(code, 0)
            self.assertIn("no field differs", out)
            code, out = self.run_main("--dumps", str(ref), str(mod), "--at", "100", "players")
            self.assertEqual(code, 1)
            self.assertIn("players[1].metal", out)


if __name__ == "__main__":
    unittest.main()
