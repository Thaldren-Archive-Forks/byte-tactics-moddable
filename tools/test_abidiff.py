"""Tests for abidiff.py, on synthetic header pairs in a temporary directory.

    uv run tools/test_abidiff.py
"""

import tempfile
import unittest
from pathlib import Path

import abidiff

SLOT_A = "int (__stdcall* A)(Unit* unit);"
SLOT_B = "void (__stdcall* B)(void);"
SLOT_C = "short (__stdcall* C)(int x);"


def header(slots: list[str], major: int = 1, minor: int = 0, data: str = "int a; int b;") -> str:
    return """struct Unit;
struct Player;
struct Foo;
struct UnitDef;
#define T_VERSION_MAJOR %d
#define T_VERSION_MINOR %d
struct TSeam {
%s
};
struct Data { %s };
""" % (major, minor, "\n".join(slots), data)


def verdict(old: str, new: str, tagged: bool = True) -> abidiff.Result:
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        (root / "old").mkdir()
        (root / "new").mkdir()
        (root / "old/t.h").write_text(old)
        (root / "new/t.h").write_text(new)
        (result,) = abidiff.diff_dirs(root / "old", root / "new", tagged)
    return result


class NewSlotTest(unittest.TestCase):
    old = header([SLOT_A, SLOT_B])

    def test_minor_bump_passes(self):
        result = verdict(self.old, header([SLOT_A, SLOT_B, SLOT_C], minor=1))
        self.assertTrue(result.ok, result)
        self.assertEqual(result.summary, "ok (additive, minor 0->1)")

    def test_no_bump_fails(self):
        result = verdict(self.old, header([SLOT_A, SLOT_B, SLOT_C]))
        self.assertFalse(result.ok)
        self.assertIn("needs a minor bump", result.summary)

    def test_slot_inserted_before_the_end_is_breaking(self):
        result = verdict(self.old, header([SLOT_A, SLOT_C, SLOT_B], minor=1))
        self.assertFalse(result.ok)
        self.assertIn("major bump", result.summary)


class RemovedSlotTest(unittest.TestCase):
    old = header([SLOT_A, SLOT_B])

    def test_major_bump_needs_an_adapter(self):
        result = verdict(self.old, header([SLOT_A], major=2))
        self.assertFalse(result.ok)
        self.assertIn("adapter required (#9)", result.summary)

    def test_minor_bump_fails(self):
        result = verdict(self.old, header([SLOT_A], minor=1))
        self.assertFalse(result.ok)
        self.assertIn("needs a major bump", result.summary)

    def test_no_bump_fails(self):
        result = verdict(self.old, header([SLOT_A]))
        self.assertFalse(result.ok)
        self.assertIn("needs a major bump", result.summary)

    def test_reorder_is_breaking(self):
        result = verdict(self.old, header([SLOT_B, SLOT_A], minor=1))
        self.assertFalse(result.ok)
        self.assertIn("needs a major bump", result.summary)


class RetypedSlotTest(unittest.TestCase):
    def check(self, old_slot: str, new_slot: str) -> None:
        old = header([old_slot, SLOT_B])
        major = verdict(old, header([new_slot, SLOT_B], major=2))
        self.assertFalse(major.ok)
        self.assertIn("adapter required (#9)", major.summary)
        for new in (header([new_slot, SLOT_B], minor=1), header([new_slot, SLOT_B])):
            result = verdict(old, new)
            self.assertFalse(result.ok)
            self.assertIn("needs a major bump", result.summary)

    def test_unit_pointer_to_player_pointer_is_breaking_with_a_tag(self):
        self.check(SLOT_A, "int (__stdcall* A)(Player* unit);")

    def test_width_change(self):
        self.check("int (__stdcall* A)(int x);", "int (__stdcall* A)(short x);")

    def test_struct_pointer_to_another(self):
        self.check(SLOT_A, "int (__stdcall* A)(Player* unit);")

    def test_calling_convention(self):
        self.check(SLOT_A, "int (__fastcall* A)(Unit* unit);")

    def test_value_for_pointer(self):
        self.check("int (__stdcall* A)(int* x);", "int (__stdcall* A)(int x);")


class RetypeBeforeTagTest(unittest.TestCase):
    old = header(["int (__stdcall* A)(void* weapon);", SLOT_B])
    new = ["int (__stdcall* A)(Foo* weapon);", SLOT_B]

    def test_minor_bump_passes(self):
        result = verdict(self.old, header(self.new, minor=1), tagged=False)
        self.assertTrue(result.ok, result)
        self.assertEqual(result.summary, "ok (additive, minor 0->1)")
        self.assertEqual(result.details, ["retype (before interfaces-v1), TSeam.A param 1: void * -> Foo *"])

    def test_no_bump_fails(self):
        self.assertFalse(verdict(self.old, header(self.new), tagged=False).ok)

    def test_major_bump_needs_an_adapter(self):
        result = verdict(self.old, header(self.new, major=2), tagged=False)
        self.assertIn("adapter required (#9)", result.summary)

    def test_after_the_tag_it_is_breaking(self):
        result = verdict(self.old, header(self.new, minor=1))
        self.assertFalse(result.ok)
        self.assertIn("needs a major bump", result.summary)
        self.assertIn("adapter required (#9)", verdict(self.old, header(self.new, major=2)).summary)

    def test_slot_keeping_retypes_pass(self):
        for a, b in (("Unit*", "Player*"), ("UnitDef*", "unsigned short"), ("int", "short"), ("char*", "int")):
            old = header(["int (__stdcall* A)(%s p);" % a])
            result = verdict(old, header(["int (__stdcall* A)(%s p);" % b], minor=1), tagged=False)
            self.assertTrue(result.ok, (a, b, result))

    def test_slot_changing_retypes_fail(self):
        for a, b in (("int", "double"), ("int", "Foo"), ("Foo*", "long long")):
            old = header(["int (__stdcall* A)(%s p);" % a])
            result = verdict(old, header(["int (__stdcall* A)(%s p);" % b], minor=1), tagged=False)
            self.assertFalse(result.ok, (a, b))
            self.assertIn("needs a major bump", result.summary)

    def test_calling_convention_and_return_type(self):
        old = header(["int (__stdcall* A)(Foo* p);"])
        self.assertFalse(verdict(old, header(["int (__fastcall* A)(Foo* p);"], minor=1), tagged=False).ok)
        self.assertFalse(verdict(old, header(["void (__stdcall* A)(Foo* p);"], minor=1), tagged=False).ok)
        self.assertTrue(verdict(old, header(["Foo* (__stdcall* A)(Foo* p);"], minor=1), tagged=False).ok)


class MovedFieldTest(unittest.TestCase):
    old = header([SLOT_A], data="int a; int b;")

    def test_moved_field_fails_with_a_bump(self):
        for bump in ({"minor": 1}, {"major": 2}):
            result = verdict(self.old, header([SLOT_A], data="short pad; int a; int b;", **bump))
            self.assertFalse(result.ok)
            self.assertIn("moved or changed size", result.summary)

    def test_resized_field_fails_with_a_bump(self):
        result = verdict(self.old, header([SLOT_A], data="short a; int b;", minor=1))
        self.assertFalse(result.ok)

    def test_appended_field_needs_a_minor_bump(self):
        new = "int a; int b; int c;"
        self.assertTrue(verdict(self.old, header([SLOT_A], data=new, minor=1)).ok)
        self.assertFalse(verdict(self.old, header([SLOT_A], data=new)).ok)


class VersionTest(unittest.TestCase):
    def test_no_change_passes_with_and_without_a_bump(self):
        old = header([SLOT_A])
        self.assertEqual(verdict(old, old).summary, "ok (no change)")
        self.assertEqual(verdict(old, header([SLOT_A], minor=1)).summary, "ok (no change, minor 0->1)")

    def test_version_going_down_fails(self):
        self.assertFalse(verdict(header([SLOT_A], minor=2), header([SLOT_A], minor=1)).ok)

    def test_virtual_methods_are_slots(self):
        base = "#define V_VERSION_MAJOR 1\n#define V_VERSION_MINOR %d\nclass V { public: virtual void A() = 0; %s };\n"
        old = base % (0, "")
        self.assertTrue(verdict(old, base % (1, "virtual void B() = 0;")).ok)
        self.assertFalse(verdict(old, base % (0, "virtual void B() = 0;")).ok)


if __name__ == "__main__":
    unittest.main()
