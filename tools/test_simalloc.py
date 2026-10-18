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

    def test_allowlist_skips_a_row(self):
        rows = [row("MakePlain", "0x401100", "new", "Widget")]
        checked, failures = simalloc.run(rows, self.index, {rows[0]["name"]})
        self.assertEqual((checked, failures), (0, []))


if __name__ == "__main__":
    unittest.main()
