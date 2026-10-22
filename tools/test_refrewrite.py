"""Tests for refrewrite.py, on the fixtures in tools/testdata/refrewrite.

Needs clang-cl 22.1.8, so it runs where the modern build does (the
modern-compile job), not in the checks job.

    uv run tools/test_refrewrite.py
"""

import contextlib
import io
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

import refrewrite

DATA = Path(__file__).parent / "testdata/refrewrite"


class Tree:
    def __init__(self, test: unittest.TestCase, exceptions: str = "exceptions.csv"):
        self.dir = tempfile.TemporaryDirectory()
        test.addCleanup(self.dir.cleanup)
        self.root = Path(self.dir.name)
        (self.root / "src/units").mkdir(parents=True)
        (self.root / "data").mkdir()
        self.header = self.root / "src/units/sample.h"
        shutil.copy(DATA / "input.fixture", self.header)
        self.csv = self.root / "data/ref_exceptions.csv"
        shutil.copy(DATA / exceptions, self.csv)

    def run(self, *args: str) -> tuple[int, str, str]:
        out, err = io.StringIO(), io.StringIO()
        with contextlib.redirect_stdout(out), contextlib.redirect_stderr(err):
            code = refrewrite.main(["--plain", "--root", str(self.root), "--exceptions", str(self.csv), *args])
        return code, out.getvalue(), err.getvalue()


class RefRewriteTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        version = subprocess.run(["clang-cl", "--version"], capture_output=True, text=True).stdout
        if "22.1.8" not in version:
            raise unittest.SkipTest("clang-cl 22.1.8 is required")

    def test_rewrites_by_issue(self):
        tree = Tree(self)
        code, _, err = tree.run("--issue", "#349")
        self.assertEqual(code, 0)
        self.assertEqual(tree.header.read_text(), (DATA / "expected.fixture").read_text())
        self.assertEqual(tree.csv.read_text(), (DATA / "expected_exceptions.csv").read_text())
        self.assertIn("Holder::fixed (src/units/sample.h): not a plain pointer declarator", err)
        self.assertIn("Holder::viaMacro (src/units/sample.h): declared through a macro", err)
        self.assertIn("Holder::e is a pointer to pointer, now Ref<Target*>", err)

    def test_dry_run_writes_nothing(self):
        tree = Tree(self)
        before = (tree.header.read_text(), tree.csv.read_text())
        code, out, _ = tree.run("--struct", "Holder", "--dry-run")
        self.assertEqual(code, 0)
        self.assertEqual((tree.header.read_text(), tree.csv.read_text()), before)
        self.assertIn("-    Target *a;", out)
        self.assertIn("+    Ref<Target> a;", out)
        self.assertIn("+#include \"../../interfaces/ref.h\"", out)

    def test_struct_selects_other_issues_too(self):
        tree = Tree(self)
        tree.run("--struct", "Holder")
        self.assertIn("Ref<Target> kept;", tree.header.read_text())
        self.assertNotIn("Holder,kept", tree.csv.read_text())

    def test_other_issue_selects_only_its_rows(self):
        tree = Tree(self)
        tree.run("--issue", "#350")
        text = tree.header.read_text()
        self.assertIn("Ref<Target> kept;", text)
        self.assertIn("Target *a;", text)

    def test_second_run_changes_nothing(self):
        tree = Tree(self)
        tree.run("--issue", "#349")
        after = tree.header.read_text()
        code, out, _ = tree.run("--issue", "#349", "--dry-run")
        self.assertEqual(code, 0)
        self.assertEqual(out, "")
        self.assertEqual(tree.header.read_text(), after)

    def test_existing_include_is_kept(self):
        text = '#pragma once\n#include "../../interfaces/ref.h"\nstruct S;\n'
        self.assertEqual(refrewrite.add_include(text, Path("/r/src/units/s.h"), Path("/r/interfaces/ref.h")), text)

    def test_include_after_last_include(self):
        text = '#include "a.h"\n#include "b.h"\n\nstruct S;\n'
        out = refrewrite.add_include(text, Path("/r/src/s.cpp"), Path("/r/interfaces/ref.h"))
        self.assertEqual(out, '#include "a.h"\n#include "b.h"\n#include "../interfaces/ref.h"\n\nstruct S;\n')

    def test_comments_and_alignment_survive(self):
        tree = Tree(self)
        tree.header.write_text(
            "struct T { int v; };\n"
            "struct Holder {\n"
            "    T*     a;      // first\n"
            "    int    b;      // second\n"
            "    T *c, *d;      // pair\n"
            "};\n")
        tree.csv.write_text("struct,field,kind,file,issue\n"
                            "Holder,a,pointer,src/units/sample.h,#349\n"
                            "Holder,c,pointer,src/units/sample.h,#349\n")
        tree.run("--issue", "#349")
        self.assertEqual(
            tree.header.read_text(),
            '#include "../../interfaces/ref.h"\n'
            "struct T { int v; };\n"
            "struct Holder {\n"
            "    Ref<T>     a;      // first\n"
            "    int    b;      // second\n"
            "    Ref<T> c;\n"
            "    T *d;      // pair\n"
            "};\n")


if __name__ == "__main__":
    unittest.main()
