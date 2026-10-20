"""Tests for blockpointers.py, on hand-written scans, maps, layouts and rows.

    uv run tools/test_blockpointers.py
"""

import unittest

import blockpointers as bp

SCAN = "\n".join([
    "bt-blockptrs 1",
    "commit\tabc",
    "ticks\t10",
    "holder\tholder_tag\toffset\ttarget\ttarget_tag\tticks\ttarget_offset_min\ttarget_offset_max",
    "0x00401010\t-\t0x4\t0x00401200\tNAMES\t10\t0x0\t0x0",
    "SimGlobals\t-\t0x14\t0x00401030\t-\t3\t0xa\t0x40",
    "",
])

MAP = """ Address         Publics by Value              Rva+Base               Lib:Object

 0001:00001000       ?Load@Loader@@QAEXXZ       0000000000401000     a.obj
 0001:00001000       ?Alias@@YAXXZ              0000000000401000     a.obj
 0001:00001020       _Setup@8                   0000000000401020     a.obj
 0001:00001100       ?Make@?$Vec@H@@QAEXXZ      0000000000401100     a.obj
 0001:00001150       ?catch$4@?0??Make@@4HA     0000000000401150     a.obj
 0001:00001200       ?Other@@YAXXZ              0000000000401200     a.obj
 0002:00000000       _Data                      0000000000500000     a.obj
"""

LAYOUT = """*** Dumping AST Record Layout
         0 | struct SimGlobals
         0 |   int a
         4 |   int[10] table
        44 |   struct Vec vec
        44 |     int begin
        48 |   int last
           | [sizeof=52, align=4,
           |  nvsize=52, nvalign=4]
"""


def resolved(holder, offset, target, scenario="s", ticks=1, low=0, high=0):
    return bp.Resolved(scenario, holder, offset, target, ticks, low, high)


def row(holder, target, offsets="0x0", cls="outside", note=""):
    return {"holder": holder, "offsets": offsets, "target": target, "scenarios": "1", "ticks": "1",
            "target_offsets": "0x0", "class": cls, "note": note}


class ScanTest(unittest.TestCase):
    def test_parse(self):
        scan = bp.parse_scan(SCAN, "eight")
        self.assertEqual((scan.commit, scan.ticks, len(scan.rows)), ("abc", 10, 2))
        r = scan.rows[1]
        self.assertEqual((r.holder, r.offset, r.target_min, r.target_max), ("SimGlobals", 0x14, 0xa, 0x40))
        self.assertEqual(scan.rows[0].target_tag, "NAMES")

    def test_parse_rejects(self):
        with self.assertRaises(ValueError):
            bp.parse_scan("bt-trace 3\n", "x")
        with self.assertRaises(ValueError):
            bp.parse_scan("bt-blockptrs 1\nticks\t1\na\tb\n", "x")
        with self.assertRaises(ValueError):
            bp.parse_scan("bt-blockptrs 1\ncommit\t-\n", "x")


class NamingTest(unittest.TestCase):
    def setUp(self):
        self.code = bp.parse_map(MAP)

    def test_map_names(self):
        self.assertEqual(self.code.function(0x401010), "Alias")
        self.assertEqual(self.code.function(0x401030), "Setup")
        self.assertEqual(self.code.function(0x401120), "Vec::Make")
        self.assertEqual(self.code.function(0x401160), "Vec::Make")
        self.assertEqual(self.code.function(0x401201), "Other")

    def test_names_and_numbers(self):
        sites = [(0x401010, ""), (0x401018, ""), (0x401012, "T"), (0x401014, "T"), (0x401030, "")]
        namer = bp.Namer(self.code, sites)
        self.assertEqual(namer.name("0x00401010", "-"), "Alias")
        self.assertEqual(namer.name("0x00401018", "-"), "Alias #2")
        self.assertEqual(namer.name("0x00401012", "T"), "Alias: T")
        self.assertEqual(namer.name("0x00401014", "T"), "Alias: T #2")
        self.assertEqual(namer.name("0x00401030", "-"), "Setup")
        self.assertEqual(namer.name("0x00000000", "-"), "unknown")

    def test_resolve_simglobals(self):
        members = bp.simglobals_members(LAYOUT)
        self.assertEqual([(m.name, m.start, m.end) for m in members],
                         [("a", 0, 4), ("table", 4, 44), ("vec", 44, 48), ("last", 48, 52)])
        out = bp.resolve([bp.parse_scan(SCAN, "s")], self.code, members)
        self.assertEqual((out[1].holder, out[1].offset), ("SimGlobals.table", 0x10))
        out = bp.resolve([bp.parse_scan(SCAN, "s")], self.code, None)
        self.assertEqual((out[1].holder, out[1].offset), ("SimGlobals+0x14", 0))


class FoldTest(unittest.TestCase):
    def test_progression(self):
        self.assertEqual(bp.offsets_text({0x10, 0x24, 0x38, 0x4c}), "0x10+0x14*k")

    def test_two_are_listed(self):
        self.assertEqual(bp.offsets_text({0x24, 0x10}), "0x10 0x24")

    def test_run_and_rest(self):
        self.assertEqual(bp.offsets_text({0, 8, 16, 100}), "0x0+0x8*k 0x64")

    def test_longest_wins(self):
        self.assertEqual(bp.fold({0, 4, 8, 12, 16}), [(0, 4)])

    def test_merge_groups(self):
        rows = [resolved("H", 0, "T", "a", 5, 0, 0), resolved("H", 4, "T", "a", 7, 8, 0x10),
                resolved("H", 8, "T", "b", 2, 0, 0), resolved("A", 0, "T", "a", 1)]
        merged = bp.merge(rows)
        self.assertEqual([(r["holder"], r["target"]) for r in merged], [("A", "T"), ("H", "T")])
        h = merged[1]
        self.assertEqual((h["offsets"], h["scenarios"], h["ticks"], h["target_offsets"]),
                         ("0x0+0x4*k", "2", "9", "0x0-0x10"))
        self.assertEqual(merged[0]["target_offsets"], "0x0")


class KeptTest(unittest.TestCase):
    def test_merge_kept(self):
        old = [row("H", "T", cls="moves-in", note="n")]
        new = bp.merge_kept([row("H", "T", cls="", note=""), row("H", "U", cls="", note="")], old)
        self.assertEqual([(r["class"], r["note"]) for r in new], [("moves-in", "n"), ("", "")])

    def test_differences_ignore_kept(self):
        a = [row("H", "T", cls="outside")]
        b = [row("H", "T", cls="")]
        self.assertEqual(bp.differences(a, b), [])


class CheckTest(unittest.TestCase):
    def test_valid(self):
        text = bp.render([row("A", "T", "0x0+0x4*k 0x1"), row("B", "T", cls="not-a-pointer", note="an id")])
        self.assertEqual(bp.check_rows(text), [])

    def test_header_only(self):
        self.assertEqual(bp.check_rows(bp.render([])), [])

    def test_problems(self):
        text = bp.render([row("B", "T", cls=""), row("A", "T", cls="not-a-pointer"), row("A", "T", "zz"),
                          row("C", "T", cls="bogus")])
        problems = "\n".join(bp.check_rows(text))
        self.assertIn("class '' is not one of", problems)
        self.assertIn("not-a-pointer row needs a note", problems)
        self.assertIn("offsets 'zz'", problems)
        self.assertIn("sorted", problems)
        self.assertIn("class 'bogus'", problems)

    def test_header(self):
        self.assertTrue(bp.check_rows("a,b\n"))


class MatcherTest(unittest.TestCase):
    ROWS = [row("H", "T", "0x4+0x10*k 0x100"), row("S.f", "U", "0x0")]

    def test_covered(self):
        scanned = [resolved("H", 0x4, "T"), resolved("H", 0x14, "T"), resolved("H", 0x100, "T"),
                   resolved("H", 0x1004, "T"), resolved("S.f", 0, "U")]
        self.assertEqual(bp.uncovered(scanned, self.ROWS), [])

    def test_uncovered_offset_and_pair(self):
        scanned = [resolved("H", 0x8, "T"), resolved("H", 0x4, "V"), resolved("X", 0, "T"),
                   resolved("H", 0x8, "T")]
        missing = bp.uncovered(scanned, self.ROWS)
        self.assertEqual([(m.holder, m.offset, m.target) for m in missing],
                         [("H", 8, "T"), ("H", 4, "V"), ("X", 0, "T")])
        lines = bp.describe_uncovered(missing, self.ROWS)
        self.assertIn("offset not listed", lines[0])
        self.assertIn("pair not listed", lines[1])

    def test_check_scans_missing_file(self):
        problems = bp.check_scans({"eight": None}, bp.ROOT / "none.map", bp.ROOT, [])
        self.assertIn("eight: no blockptrs.txt", problems[0])


if __name__ == "__main__":
    unittest.main()
