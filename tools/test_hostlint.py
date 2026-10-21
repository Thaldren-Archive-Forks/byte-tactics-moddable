"""Tests for hostlint.py, on temporary directory trees.

    uv run tools/test_hostlint.py
"""

import contextlib
import io
import os
import stat
import tempfile
import unittest
from pathlib import Path

import hostlint


class HostLintTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)

    def write(self, path, text):
        p = self.root / path
        p.parent.mkdir(parents=True, exist_ok=True)
        p.write_text(text)

    def exceptions(self, *rows):
        self.write("data/host_exceptions.csv", "file,header,issue\n" + "".join(f"{r}\n" for r in rows))

    def lint(self, **kw):
        return hostlint.lint(str(self.root), **kw)

    def test_clean_tree_passes(self):
        self.write("src/a.cpp", '#include <stdio.h>\n#include "win_types.h"\n')
        self.assertEqual(self.lint(), [])

    def test_windows_h_outside_host_fails(self):
        self.write("tests/a.cpp", "#include <windows.h>\n")
        self.assertEqual(len(self.lint()), 1)
        self.assertIn("tests/a.cpp", self.lint()[0])

    def test_other_platform_headers_fail(self):
        for header in ["<shlobj.h>", "<MMSystem.h>", "<winsock2.h>", "<ddraw.h>", "<dsound.h>", "<dinput.h>",
                       "<d3d.h>", '"SDL.h"', "<SDL3/SDL.h>", "<imagehlp.h>"]:
            with self.subTest(header=header):
                self.write("src/a.cpp", f"# include {header}\n")
                self.assertEqual(len(self.lint()), 1)

    def test_listed_pair_passes(self):
        self.write("src/a.cpp", "#include <windows.h>\n")
        self.exceptions("src/a.cpp,windows.h,#302")
        self.assertEqual(self.lint(), [])

    def test_listed_file_with_another_header_fails(self):
        self.write("src/a.cpp", "#include <windows.h>\n#include <shlobj.h>\n")
        self.exceptions("src/a.cpp,windows.h,#302")
        found = self.lint()
        self.assertEqual(len(found), 1)
        self.assertIn("shlobj.h", found[0])

    def test_stale_row_fails(self):
        self.write("src/a.cpp", "#include <stdio.h>\n")
        self.exceptions("src/a.cpp,windows.h,#302", "src/gone.cpp,windows.h,#302")
        found = self.lint()
        self.assertEqual(len(found), 2)
        self.assertTrue(all("stale row" in line for line in found))

    def test_bad_issue_field_fails(self):
        self.write("src/a.cpp", "#include <windows.h>\n")
        for issue in ["", "302", "#", "#abc"]:
            with self.subTest(issue=issue):
                self.exceptions(f"src/a.cpp,windows.h,{issue}")
                found = self.lint()
                self.assertEqual(len(found), 1)
                self.assertIn("not #<number>", found[0])

    def test_host_files_pass(self):
        self.write("src/host/win32/a.cpp", "#include <windows.h>\n#include <ddraw.h>\n")
        self.assertEqual(self.lint(), [])

    def test_build_and_toolchain_are_skipped(self):
        self.write("build/a.cpp", "#include <windows.h>\n")
        self.write("toolchain/a.h", "#include <windows.h>\n")
        self.assertEqual(self.lint(), [])

    def test_open_issue_check(self):
        self.write("src/a.cpp", "#include <windows.h>\n")
        self.exceptions("src/a.cpp,windows.h,#302")
        for state, expect in [("OPEN", 0), ("CLOSED", 1), ("", 1)]:
            with self.subTest(state=state):
                gh = self.root / "fakegh"
                body = f'echo \'{{"state": "{state}"}}\'' if state else "exit 1"
                gh.write_text(f"#!/bin/sh\n{body}\n")
                gh.chmod(gh.stat().st_mode | stat.S_IXUSR)
                self.assertEqual(len(self.lint(open_issues=True, gh=str(gh))), expect)

    def test_main_exit_codes(self):
        self.write("tests/a.cpp", "#include <windows.h>\n")
        with contextlib.redirect_stdout(io.StringIO()) as out:
            self.assertEqual(hostlint.main(["--root", str(self.root)]), 1)
        self.assertIn("tests/a.cpp", out.getvalue())
        self.write("tests/a.cpp", "int a;\n")
        self.assertEqual(hostlint.main(["--root", str(self.root)]), 0)


if __name__ == "__main__":
    unittest.main()
