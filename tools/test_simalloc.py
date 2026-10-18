"""Tests for simalloc.py, on hand-written sources and rows.

    uv run tools/test_simalloc.py
"""

import tempfile
import unittest
from pathlib import Path

import simalloc

HEADER = """#include "block.h"
struct Widget { int a; };
"""

SOURCE = """#include "helpers.h"

// FUNCTION: 0x401000
Widget* MakeConverted()
{
    return new (kSim) Widget();
}

// FUNCTION: 0x401100
Widget* MakePlain()
{
    Widget* w = new Widget();
    return w;
}

// FUNCTION: 0x401200
char* MakeTagged(int n)
{
    return (char*)GameAllocIgnoreTag("BUFFER", n);
}

// FUNCTION: 0x401300
char* MakeSimTagged(int n)
{
    return (char*)SimAlloc(n);
}

// FUNCTION: 0x401400
void ViaHelper()
{
    Build(1);
}

// FUNCTION: 0x401500
void ViaBadHelper()
{
    BuildPlain();
}

// FUNCTION: 0x401600
void ViaNothing()
{
    int x = 0;
}

// FUNCTION: 0x401700
void OldOperator()
{
    void* p = operator new(16);
}

// FUNCTION: 0x401800
void ViaHeader()
{
    Deep();
}

// FUNCTION: 0x401900
void MixedTags()
{
    char* a = (char*)SimAlloc(8);
    char* b = (char*)GameAllocIgnoreTag("DRAW LIST", 8);
}

// FUNCTION: 0x401a00
void UnconvertedTag()
{
    char* a = (char*)SimAlloc(8);
    char* b = (char*)GameAllocIgnoreTag("STATE LIST", 8);
}

// FUNCTION: 0x401b00
void TempBuffer(char* path)
{
    char* a = (char*)SimAlloc(8);
    char* buf = (char*)GameAllocIgnoreTag(path, 8);
    GameFreeThunk(buf);
}

// FUNCTION: 0x401c00
void KeptBuffer(char* path)
{
    char* a = (char*)SimAlloc(8);
    char* buf = (char*)GameAllocIgnoreTag(path, 8);
    g_keep = buf;
}

static void Build(int n)
{
    Gadget* g = new (kSim) Gadget();
}

static void BuildPlain()
{
    Gadget* g = new Gadget();
}
"""

HELPERS = """inline void Deep()
{
    Gadget* g = new (kSim) Gadget[4];
}
"""


def row(function, address, allocator, what=""):
    name = f"{function}: {allocator} {what}".strip()
    return {"name": name, "address": address, "function": function, "allocator": allocator, "what": what}


class SimAllocTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.dir = tempfile.TemporaryDirectory()
        root = Path(cls.dir.name)
        (root / "helpers.h").write_text(HELPERS)
        path = root / "sample.cpp"
        path.write_text(SOURCE)
        cls.index = simalloc.Index([simalloc.Unit(path, "sample.cpp")])

    @classmethod
    def tearDownClass(cls):
        cls.dir.cleanup()

    def failures(self, *args):
        return simalloc.check_row(row(*args), self.index)

    def test_converted_site_passes(self):
        self.assertEqual(self.failures("MakeConverted", "0x401000", "new", "Widget"), [])

    def test_plain_new_fails(self):
        out = self.failures("MakePlain", "0x401100", "new", "Widget")
        self.assertEqual(len(out), 1)
        self.assertIn("sample.cpp:12 MakePlain: Widget* w = new Widget();", out[0])

    def test_game_alloc_fails(self):
        out = self.failures("MakeTagged", "0x401200", "GameAllocIgnoreTag", "BUFFER")
        self.assertEqual(len(out), 1)
        self.assertIn("GameAllocIgnoreTag", out[0])

    def test_sim_alloc_passes(self):
        self.assertEqual(self.failures("MakeSimTagged", "0x401300", "GameAllocIgnoreTag", "BUFFER"), [])

    def test_operator_new_fails(self):
        out = self.failures("OldOperator", "0x401700", "operator new", "Widget")
        self.assertEqual(len(out), 1)
        self.assertIn("operator new(16)", out[0])

    def test_helper_in_same_file_passes(self):
        self.assertEqual(self.failures("ViaHelper", "0x401400", "new", "Gadget"), [])

    def test_helper_with_plain_new_fails(self):
        out = self.failures("ViaBadHelper", "0x401500", "new", "Gadget")
        self.assertEqual(len(out), 1)
        self.assertIn("Gadget* g = new Gadget();", out[0])

    def test_helper_in_header_passes(self):
        self.assertEqual(self.failures("ViaHeader", "0x401800", "new[]", "Gadget"), [])

    def test_missing_allocation_is_reported(self):
        out = self.failures("ViaNothing", "0x401600", "new", "Widget")
        self.assertEqual(len(out), 1)
        self.assertIn("no new (kSim) Widget", out[0])

    def test_unknown_function_is_reported(self):
        out = self.failures("Nowhere", "", "new", "Widget")
        self.assertIn("no function found", out[0])

    def test_row_without_address_found_by_name(self):
        self.assertEqual(self.failures("Build", "", "new", "Gadget"), [])

    def test_row_names_parse(self):
        p = simalloc.parse_row({"name": "Order::Order: new[] Rec #2", "address": "0x1"})
        self.assertEqual((p["function"], p["allocator"], p["what"]), ("Order::Order", "new[]", "Rec"))
        p = simalloc.parse_row({"name": "LoadUnitInfo: GameAllocIgnoreTag UnitDef", "address": ""})
        self.assertEqual((p["allocator"], p["what"]), ("GameAllocIgnoreTag", "UnitDef"))

    def others(self, *tags):
        return frozenset(tags)

    def test_call_with_non_sim_row_tag_is_skipped(self):
        r = row("MixedTags", "0x401900", "GameAllocIgnoreTag", "Thing")
        self.assertEqual(simalloc.check_row(r, self.index, self.others("DRAW LIST")), [])

    def test_call_with_unlisted_tag_still_fails(self):
        r = row("UnconvertedTag", "0x401a00", "GameAllocIgnoreTag", "Thing")
        out = simalloc.check_row(r, self.index, self.others("DRAW LIST"))
        self.assertEqual(len(out), 1)
        self.assertIn('GameAllocIgnoreTag("STATE LIST", 8)', out[0])

    def test_non_sim_tag_does_not_hide_a_named_sim_call(self):
        r = row("UnconvertedTag", "0x401a00", "GameAllocIgnoreTag", "STATE LIST")
        self.assertEqual(len(simalloc.check_row(r, self.index, self.others("STATE LIST"))), 1)

    def test_non_literal_tag_freed_in_function_is_skipped(self):
        r = row("TempBuffer", "0x401b00", "GameAllocIgnoreTag", "Thing")
        self.assertEqual(simalloc.check_row(r, self.index), [])

    def test_non_literal_tag_kept_still_fails(self):
        r = row("KeptBuffer", "0x401c00", "GameAllocIgnoreTag", "Thing")
        out = simalloc.check_row(r, self.index)
        self.assertEqual(len(out), 1)
        self.assertIn("GameAllocIgnoreTag(path, 8)", out[0])

    def test_other_tags_read_from_non_sim_rows(self):
        with tempfile.TemporaryDirectory() as d:
            path = Path(d) / "classes.csv"
            path.write_text("name,address,kind,class\n"
                            "F: GameAllocIgnoreTag DRAW LIST,0x1,heap,view\n"
                            "F: GameAllocIgnoreTag KEEP,0x1,heap,sim\n"
                            "F: GameAllocIgnoreTag OTHER,0x2,heap,platform\n")
            self.assertEqual(simalloc.other_tags(path),
                             {("F", "0x1"): frozenset({"DRAW LIST"}), ("F", "0x2"): frozenset({"OTHER"})})

    def test_allowlist_skips_a_row(self):
        rows = [row("MakePlain", "0x401100", "new", "Widget")]
        checked, failures = simalloc.run(rows, self.index, {rows[0]["name"]})
        self.assertEqual((checked, failures), (0, []))


if __name__ == "__main__":
    unittest.main()
