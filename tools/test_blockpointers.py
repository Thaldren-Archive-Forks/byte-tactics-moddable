"""Tests for blockpointers.py, on hand-written scans, maps, layouts and rows.

    uv run tools/test_blockpointers.py
"""

import contextlib
import io
import tempfile
import unittest
from pathlib import Path
from unittest import mock

import blockpointers as bp

SCAN = "\n".join([
    "bt-blockptrs 2",
    "commit\tabc",
    "every\t8",
    "ticks\t10",
    "holder\tholder_tag\toffset\ttarget\ttarget_tag\tticks\ttarget_offset_min\ttarget_offset_max",
    "0x00401010\t-\t0x4\t0x00401200\tNAMES\t10\t0x0\t0x0",
    "SimGlobals\t-\t0x14\t0x00401030\t-\t3\t0xa\t0x40",
    "0x00401010\t-\t0x9e\t0x00401200\tNAMES\t10\t0x0\t0x0",
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
        self.assertEqual((scan.commit, scan.ticks, scan.every, len(scan.rows)), ("abc", 10, 8, 3))
        r = scan.rows[1]
        self.assertEqual((r.holder, r.offset, r.target_min, r.target_max), ("SimGlobals", 0x14, 0xa, 0x40))
        self.assertEqual(scan.rows[0].target_tag, "NAMES")
        self.assertEqual(scan.rows[2].offset, 0x9e)

    def test_parse_rejects(self):
        with self.assertRaises(ValueError):
            bp.parse_scan("bt-trace 3\n", "x")
        with self.assertRaises(ValueError):
            bp.parse_scan("bt-blockptrs 2\nevery\t1\nticks\t1\na\tb\n", "x")
        with self.assertRaises(ValueError):
            bp.parse_scan("bt-blockptrs 2\ncommit\t-\nevery\t1\n", "x")
        with self.assertRaises(ValueError):
            bp.parse_scan("bt-blockptrs 2\ncommit\t-\nticks\t1\n", "x")
        with self.assertRaises(ValueError):
            bp.parse_scan("bt-blockptrs 2\nevery\t0\nticks\t1\n", "x")
        with self.assertRaises(ValueError):
            bp.parse_scan("bt-blockptrs 1\ncommit\t-\nticks\t1\n", "x")


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
        text = bp.render([wild("A"), wild("A", "outside", ""), row("A", "T", "0x0+0x4*k 0x1"), wild("B"),
                          row("B", "T", cls="not-a-pointer", note="an id")])
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


def wild(holder, cls="not-a-pointer", note="words that only look like addresses"):
    return {"holder": holder, "offsets": "*", "target": "*", "scenarios": "0", "ticks": "0",
            "target_offsets": "-", "class": cls, "note": note}


class MatcherTest(unittest.TestCase):
    ROWS = [wild("H"), row("H", "T", "0x4+0x10*k 0x100"), wild("S.f", "outside", "n"), wild("S.f"),
            row("S.f", "U", "0x0"), row("P", "T")]

    def test_holder_with_wild_row_covers_any_word(self):
        scanned = [resolved("H", 0x4, "T"), resolved("H", 0x8, "T"), resolved("H", 0x4, "V"),
                   resolved("S.f", 0x99, "anything")]
        self.assertEqual(bp.uncovered(scanned, self.ROWS), [])

    def test_unknown_holder_and_holder_without_wild_row(self):
        scanned = [resolved("X", 0, "T"), resolved("P", 0, "T"), resolved("X", 0, "T"), resolved("H", 1, "T")]
        missing = bp.uncovered(scanned, self.ROWS)
        self.assertEqual([(m.holder, m.offset, m.target) for m in missing], [("X", 0, "T"), ("P", 0, "T")])
        self.assertIn("holder not listed", bp.describe_uncovered(missing)[0])

    def test_check_scans_missing_file(self):
        problems = bp.check_scans({"eight": None}, bp.ROOT / "none.map", bp.ROOT, [])
        self.assertIn("eight: no blockptrs.txt", problems[0])


class WildcardTest(unittest.TestCase):
    WORDS = [resolved("G", 0, "T1", "a", 5, 1, 2), resolved("G", 4, "T2", "b", 7, 3, 9),
             resolved("H", 0x10, "T1")]

    def test_wild_rows_are_kept_and_pair_rows_built(self):
        old = [wild("G", "outside", "n"), wild("G")]
        out = bp.with_wild(bp.merge(self.WORDS), old)
        self.assertEqual([(r["holder"], r["target"], r["class"]) for r in out],
                         [("G", "*", "not-a-pointer"), ("G", "*", "outside"), ("G", "T1", ""), ("G", "T2", ""),
                          ("H", "T1", "")])
        self.assertEqual(out[0]["scenarios"], "0")
        self.assertEqual(out[2]["scenarios"], "1")

    def test_key_is_holder_and_class(self):
        old = [wild("G", "outside", "pairs"), wild("G", "not-a-pointer", "ids"), row("G", "T1", cls="moves-in", note="m")]
        rows = bp.merge_kept(bp.with_wild(bp.merge(self.WORDS), old[:2]), old)
        self.assertEqual([(r["target"], r["class"], r["note"]) for r in rows[:3]],
                         [("*", "not-a-pointer", "ids"), ("*", "outside", "pairs"), ("T1", "moves-in", "m")])
        self.assertEqual(bp.with_wild(bp.merge(self.WORDS), rows)[:2], rows[:2])

    def test_order_by_class(self):
        self.assertEqual(bp.check_rows(bp.render([wild("A", "moves-in", ""), wild("A"), wild("A", "outside", "")])), [])
        self.assertIn("sorted", "\n".join(bp.check_rows(bp.render([wild("A"), wild("A", "moves-in", "")]))))
        self.assertIn("without repeats", "\n".join(bp.check_rows(bp.render([wild("A"), wild("A")]))))

    def test_check_wants_a_row_per_class(self):
        rows = [wild("A"), wild("A", "outside", ""), row("A", "T", cls="outside"), row("A", "U", cls="moves-in"),
                row("B", "T", cls="not-a-pointer", note="n")]
        problems = "\n".join(bp.check_rows(bp.render(rows)))
        self.assertIn("A: no `*` row of class moves-in", problems)
        self.assertIn("B: no `*` row of class not-a-pointer", problems)
        self.assertNotIn("A: no `*` row of class outside", problems)
        self.assertNotIn("A: no `*` row of class not-a-pointer", problems)
        self.assertEqual(bp.check_rows(bp.render(
            [wild("A", "moves-in", ""), wild("A"), wild("A", "outside", ""), row("A", "T", cls="outside"),
             row("A", "U", cls="moves-in")])), [])

    def test_check_rejects_half_wildcards(self):
        rows = [wild("A"), dict(row("A", "*", "0x4"), target_offsets="-"), dict(row("A", "T", "*"), target_offsets="0x0"),
                row("C*", "T")]
        problems = "\n".join(bp.check_rows(bp.render(rows)))
        self.assertIn("together", problems)
        self.assertIn("(C* -> T): `*` is only", problems)

    def test_check_dash_only_on_wildcard(self):
        plain = dict(row("A", "T"), target_offsets="-")
        self.assertIn("target_offsets", "\n".join(bp.check_rows(bp.render([wild("A", "outside", ""), plain]))))

    def test_wild_note_needed(self):
        self.assertIn("needs a note", "\n".join(bp.check_rows(bp.render([wild("A", note="")]))))


class SaveScansTest(unittest.TestCase):
    def test_saved_files_read_back(self):
        with tempfile.TemporaryDirectory() as tmp:
            target = Path(tmp) / "nested" / "scans"
            bp.save_scans({"b": SCAN, "a": SCAN, "none": None}, target)
            self.assertEqual(sorted(p.name for p in target.iterdir()), ["a.txt", "b.txt"])
            self.assertEqual([s.scenario for s in bp.read_scans([target])], ["a", "b"])


class PlaySuiteTest(unittest.TestCase):
    SCENARIOS = {"a": Path("a.txt"), "b": Path("b.txt"), "c": Path("c.txt")}

    def play(self, names, fail=()):
        import reference
        calls = []

        def fake_play_all(exe, scenarios, game_dir, parallel, failed=None, scans=None, retry_any=False, **kw):
            calls.append((sorted(scenarios), retry_any))
            for n in scenarios:
                if n in fail:
                    failed[n] = "boom"
                else:
                    scans[n] = SCAN

        failed = {}
        with mock.patch.object(reference, "play_all", fake_play_all), \
                mock.patch.object(reference, "scenario_files", lambda: self.SCENARIOS):
            texts = bp.play_suite(Path("x.exe"), None, 1, names, failed)
        return texts, failed, calls

    def test_finished_scans_are_kept_and_failures_listed(self):
        texts, failed, calls = self.play(None, fail=("b",))
        self.assertEqual(sorted(texts), ["a", "c"])
        self.assertEqual(failed, {"b": "boom"})
        self.assertEqual(calls, [(["a", "b", "c"], True)])

    def test_scenario_subset(self):
        texts, _, calls = self.play(["c", "a"])
        self.assertEqual(sorted(texts), ["a", "c"])
        self.assertEqual(calls[0][0], ["a", "c"])

    def test_unknown_scenario_exits(self):
        with self.assertRaises(SystemExit):
            self.play(["nope"])

    def test_scenario_without_save_scans_is_refused(self):
        with mock.patch("sys.argv", ["blockpointers.py", "--scenario", "a"]), \
                contextlib.redirect_stderr(io.StringIO()) as err, self.assertRaises(SystemExit) as cm:
            bp.main()
        self.assertEqual(cm.exception.code, 2)
        self.assertIn("--save-scans", err.getvalue())


if __name__ == "__main__":
    unittest.main()
