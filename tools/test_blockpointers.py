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


def wild(holder, target="*", offsets="*", cls="not-a-pointer", note="an id"):
    return row(holder, target, offsets, cls, note)


class WildcardTest(unittest.TestCase):
    WORDS = [resolved("G", 0, "T1", "a", 5, 1, 2), resolved("G", 4, "T2", "b", 7, 3, 9),
             resolved("G", 8, "T1", "a", 2), resolved("H", 0x10, "T1"), resolved("H", 0x14, "T2"),
             resolved("K", 0, "T1")]

    def cells(self, rows):
        return [(r["holder"], r["target"], r["offsets"], r["scenarios"], r["ticks"], r["target_offsets"])
                for r in rows]

    def test_wildcard_target(self):
        old = [wild("H", "*", "0x10")]
        out = bp.merge_wild(self.WORDS, old)
        h = [r for r in out if r["holder"] == "H"]
        self.assertEqual(self.cells(h)[0], ("H", "*", "0x10", "1", "1", "0x0"))
        self.assertEqual(self.cells(h)[1][:3], ("H", "T2", "0x14"))

    def test_wildcard_offsets(self):
        out = bp.merge_wild(self.WORDS, [wild("G", "T1", "*")])
        self.assertEqual(self.cells(out)[:2], [("G", "T1", "*", "1", "5", "0x0-0x2"),
                                               ("G", "T2", "0x4", "1", "7", "0x3-0x9")])

    def test_both_and_removed_pairs(self):
        out = bp.merge_wild(self.WORDS, [wild("G")])
        self.assertEqual(self.cells(out)[0], ("G", "*", "*", "2", "12", "0x0-0x9"))
        self.assertEqual([(r["holder"], r["target"]) for r in out],
                         [("G", "*"), ("H", "T1"), ("H", "T2"), ("K", "T1")])

    def test_empty_pair_row_dropped(self):
        out = bp.merge_wild(self.WORDS, [wild("G", "T1", "*"), wild("G", "T2", "*")])
        self.assertEqual([(r["holder"], r["target"]) for r in out],
                         [("G", "T1"), ("G", "T2"), ("H", "T1"), ("H", "T2"), ("K", "T1")])

    def test_offsets_tokens_limit_coverage(self):
        out = bp.merge_wild(self.WORDS, [wild("G", "*", "0x0+0x4*k")])
        self.assertEqual(len(out), 4)
        out = bp.merge_wild(self.WORDS, [wild("G", "*", "0x0")])
        self.assertEqual(self.cells(out)[:2], [("G", "*", "0x0", "1", "5", "0x1-0x2"),
                                               ("G", "T1", "0x8", "1", "2", "0x0")])
        self.assertEqual(self.cells(out)[2][:2], ("G", "T2"))

    def test_nothing_covered_is_kept_with_zeros(self):
        out = bp.merge_wild(self.WORDS, [wild("Z", "*", "*")])
        self.assertEqual(self.cells(out)[-1], ("Z", "*", "*", "0", "0", "-"))
        self.assertEqual(bp.check_rows(bp.render(out[-1:])), [])

    def test_regeneration_keeps_hand_data(self):
        old = [wild("G", "*", "0x0 0x4", "outside", "grid, ids")]
        out = bp.merge_kept(bp.merge_wild(self.WORDS, old), old)
        w = out[0]
        self.assertEqual((w["holder"], w["offsets"], w["target"], w["class"], w["note"]),
                         ("G", "0x0 0x4", "*", "outside", "grid, ids"))
        self.assertEqual(bp.merge_wild(self.WORDS, out), out)

    def test_check_accepts(self):
        text = bp.render([wild("A"), wild("B", "*", "0x4+0x8*k 0x1"), wild("B", "T", "*", "outside", ""),
                          row("B", "U"), wild("C", "*", "*", "outside", "")])
        self.assertEqual(bp.check_rows(text), [])

    def test_check_rejects(self):
        text = bp.render([wild("A", cls="not-a-pointer", note=""), wild("B", "T", "* 0x4"), row("C*", "T"),
                          row("D", "T*"), row("E", "T", "0x4 *x"), wild("F", "T", "*", "bogus")])
        problems = "\n".join(bp.check_rows(text))
        self.assertIn("A -> *): a not-a-pointer row needs a note", problems)
        self.assertIn("offsets '* 0x4'", problems)
        self.assertIn("(C* -> T): `*` is only", problems)
        self.assertIn("(D -> T*): `*` is only", problems)
        self.assertIn("offsets '0x4 *x'", problems)
        self.assertIn("class 'bogus'", problems)

    def test_check_dash_only_on_wildcard(self):
        plain = dict(row("A", "T"), target_offsets="-")
        self.assertIn("target_offsets", "\n".join(bp.check_rows(bp.render([plain]))))

    def test_check_order_wildcard_first(self):
        self.assertEqual(bp.check_rows(bp.render([wild("A"), row("A", "T")])), [])
        self.assertIn("sorted", "\n".join(bp.check_rows(bp.render([row("A", "T"), wild("A")]))))

    def test_uncovered_by_wildcards(self):
        rows = [wild("G"), wild("H", "T1", "*"), wild("K", "*", "0x4")]
        scanned = [resolved("G", 0x99, "anything"), resolved("H", 7, "T1"), resolved("H", 7, "T2"),
                   resolved("K", 0x4, "T9"), resolved("K", 0x8, "T9")]
        missing = bp.uncovered(scanned, rows)
        self.assertEqual([(m.holder, m.offset, m.target) for m in missing], [("H", 7, "T2"), ("K", 8, "T9")])
        lines = bp.describe_uncovered(missing, rows)
        self.assertIn("pair not listed", lines[0])
        self.assertIn("offset not listed", lines[1])


if __name__ == "__main__":
    unittest.main()
