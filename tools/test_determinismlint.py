"""Tests for determinismlint.py, on temporary git repositories.

    uv run tools/test_determinismlint.py
"""

import subprocess
import tempfile
import unittest
from pathlib import Path

import determinismlint

GIT = ["git", "-c", "user.name=t", "-c", "user.email=t@example.com", "-c", "commit.gpgsign=false"]

CSV = "address,function,file\n0x490c40,UpdateWind,src/game/a.cpp\n0x48a490,AlignUnitToGround,src/game/a.cpp\n"

BASE = """// FUNCTION: 0x490c40 UpdateWind
void UpdateWind() {
    int x = 0;
}

// FUNCTION: 0x500000 Unlisted
void Unlisted() {
    int x = 0;
}

// FUNCTION: 0x48a490 AlignUnitToGround
void AlignUnitToGround() {
    int y = 0;
}
"""


def put(text, marker, line):
    return text.replace(marker, marker + "\n" + line, 1)


class DeterminismLintTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        self.git("init", "-q")
        self.write("data/game_logic.csv", CSV)
        self.write("src/game/a.cpp", BASE)
        self.commit()

    def git(self, *args):
        subprocess.run([*GIT, *args], cwd=self.root, check=True, capture_output=True)

    def write(self, path, text):
        p = self.root / path
        p.parent.mkdir(parents=True, exist_ok=True)
        p.write_text(text)

    def commit(self):
        self.git("add", "-A")
        self.git("commit", "-q", "-m", "c")

    def lint(self):
        return determinismlint.lint(str(self.root), "HEAD~1...HEAD")

    def change(self, marker, line, path="src/game/a.cpp", base=BASE):
        self.write(path, put(base, marker, line))
        self.commit()
        return self.lint()

    def test_each_banned_call_flagged_in_listed_block(self):
        calls = ["GetTicks()", "GetTickCount()", "timeGetTime()", "time(0)", "std::time(nullptr)",
                 "QueryPerformanceCounter(&c)", "HarnessViewRand()", "::GetTickCount()"]
        for call in calls:
            with self.subTest(call=call):
                found = self.change("void UpdateWind() {", f"    int t = {call};")
                self.assertEqual([(f[0], f[1], f[2], f[3]) for f in found],
                                 [("src/game/a.cpp", 3, "UpdateWind", 0x490c40)], found)
                self.git("reset", "-q", "--hard", "HEAD~1")

    def test_unordered_containers_flagged(self):
        found = self.change("void UpdateWind() {", "    std::unordered_map<int, int> m;")
        self.assertEqual([f[1] for f in found], [3], found)
        self.assertIn("unordered_map", found[0][4])

    def test_unlisted_block_ignored(self):
        self.assertEqual(self.change("void Unlisted() {", "    int t = GetTickCount();"), [])

    def test_new_block_under_game_dir_flagged(self):
        text = BASE + "\n// FUNCTION: 0x501000 Fresh\nvoid Fresh() {\n    int t = GetTicks();\n}\n"
        self.write("src/units/b.cpp", text)
        self.commit()
        found = self.lint()
        self.assertEqual([(f[0], f[1], f[2], f[3]) for f in found],
                         [("src/units/b.cpp", 18, "Fresh", 0x501000)], found)

    def test_new_block_outside_game_dirs_ignored(self):
        self.write("src/graphics/b.cpp", "// FUNCTION: 0x501000 Fresh\nvoid Fresh() {\n    int t = GetTicks();\n}\n")
        self.commit()
        self.assertEqual(self.lint(), [])

    def test_new_line_in_old_unlisted_block_under_game_dir_ignored(self):
        self.write("src/units/b.cpp", BASE)
        self.commit()
        self.assertEqual(self.change("void Unlisted() {", "    int t = GetTicks();", "src/units/b.cpp"), [])

    def test_comment_and_string_ignored(self):
        for line in ["    // GetTicks();", "    /* GetTickCount() */", '    const char *s = "time(0)";',
                     "    int a = 0; // HarnessViewRand()", "    int GetTicksTotal = 1; int mytime(2);",
                     "    obj.time(1); p->GetTicks();"]:
            with self.subTest(line=line):
                self.assertEqual(self.change("void UpdateWind() {", line), [])
                self.git("reset", "-q", "--hard", "HEAD~1")

    def test_only_added_lines_checked(self):
        old = put(BASE, "void UpdateWind() {", "    int t = GetTicks();")
        self.write("src/game/a.cpp", old)
        self.commit()
        self.assertEqual(self.change("void Unlisted() {", "    int u = 1;", base=old), [])

    def test_allowlisted_site_passes(self):
        determinismlint.ALLOWLIST[(0x48a490, "GetTicks")] = "test"
        self.addCleanup(determinismlint.ALLOWLIST.pop, (0x48a490, "GetTicks"))
        self.assertEqual(self.change("void AlignUnitToGround() {", "    int t = GetTicks();"), [])
        self.git("reset", "-q", "--hard", "HEAD~1")
        found = self.change("void AlignUnitToGround() {", "    int t = GetTickCount();")
        self.assertEqual(len(found), 1, found)

    def test_certain_pointer_comparison_flagged(self):
        line = "    Unit *a = 0;\n    Unit *b = 0;\n    if (a < b) {}"
        found = self.change("void UpdateWind() {", line)
        self.assertEqual([f[1] for f in found], [5], found)
        self.assertIn("a < b", found[0][4])

    def test_uncertain_pointer_comparison_ignored(self):
        cases = [
            "    Unit *a = 0;\n    int n = 0;\n    if (a < n) {}",
            "    Unit *a = 0;\n    Unit *b = 0;\n    if (*a < *b) {}",
            "    Unit *a = 0;\n    Unit *b = 0;\n    if (a->x < b->x) {}",
            "    Unit *a = 0;\n    Unit *b = 0;\n    if (a < b + 1) {}",
            "    int a = 0; int b = 0;\n    if (a < b) {}",
            "    Unit *a = 0;\n    int b = 0;\n    Unit *c = 0;\n    if (a < b) {}",
            "    unsigned a, b;\n    if (a < b) {}",
            "    Unit *a = 0;\n    Unit *b = 0;\n    x = a << b;",
        ]
        for line in cases:
            with self.subTest(line=line):
                self.assertEqual(self.change("void UpdateWind() {", line), [])
                self.git("reset", "-q", "--hard", "HEAD~1")

    def test_pointer_redeclared_as_value_is_not_certain(self):
        line = "    Unit *a = 0;\n    Unit *b = 0;\n    { int a = 1; }\n    if (a < b) {}"
        self.assertEqual(self.change("void UpdateWind() {", line), [])

    def test_all_mode_checks_listed_blocks(self):
        text = put(BASE, "void UpdateWind() {", "    int t = GetTicks();")
        text = put(text, "void Unlisted() {", "    int t = GetTicks();")
        self.write("src/game/a.cpp", text)
        self.commit()
        found = determinismlint.lint(str(self.root), everything=True)
        self.assertEqual([(f[1], f[2]) for f in found], [(3, "UpdateWind")], found)

    def test_main_output_and_exit_status(self):
        import contextlib
        import io
        import os
        self.change("void UpdateWind() {", "    int t = GetTickCount();")
        cwd = os.getcwd()
        os.chdir(self.root)
        try:
            out = io.StringIO()
            with contextlib.redirect_stdout(out):
                status = determinismlint.main(["HEAD~1...HEAD"])
        finally:
            os.chdir(cwd)
        self.assertEqual(status, 1)
        self.assertRegex(out.getvalue(), r"^src/game/a\.cpp:3: UpdateWind 0x490c40: GetTickCount\(\)")


if __name__ == "__main__":
    unittest.main()
