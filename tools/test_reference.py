"""Tests for the trace name check in tools/reference.py.

    uv run tools/test_reference.py
"""

import contextlib
import io
import tempfile
import unittest
from pathlib import Path
from unittest import mock

import reference


class NameMismatchTest(unittest.TestCase):
    def test_equal_sets_pass(self):
        self.assertEqual(reference.name_mismatch({"a", "b"}, {"a", "b"}), "")

    def test_names_missing_and_extra(self):
        message = reference.name_mismatch({"a", "x"}, {"a", "b", "c"})
        self.assertIn("missing: b, c", message)
        self.assertIn("extra: x", message)

    def test_nothing_extra(self):
        self.assertIn("extra: none", reference.name_mismatch({"a"}, {"a", "b"}))


class StepsPerFrameTest(unittest.TestCase):
    def test_copies_end_with_the_key(self):
        with tempfile.TemporaryDirectory() as tmp:
            src = Path(tmp) / "src"
            src.mkdir()
            (src / "a.txt").write_text("ticks 5\nmap X")
            (src / "b.txt").write_text("ticks 6\nsteps_per_frame 2\n")
            copies = reference.with_steps_per_frame(
                {"a": src / "a.txt", "b": src / "b.txt"}, 5, Path(tmp) / "out")
            self.assertEqual(copies["a"].read_text(), "ticks 5\nmap X\nsteps_per_frame 5\n")
            self.assertEqual(copies["b"].read_text().splitlines()[-1], "steps_per_frame 5")
            self.assertEqual((src / "a.txt").read_text(), "ticks 5\nmap X")


class WithQuirksTest(unittest.TestCase):
    def rewrite(self, text: str) -> list[str]:
        with tempfile.TemporaryDirectory() as tmp:
            src = Path(tmp) / "a.txt"
            src.write_text(text)
            copies = reference.with_quirks({"a": src}, "3.1c +x", Path(tmp) / "out")
            self.assertEqual(src.read_text(), text)
            return copies["a"].read_text().splitlines()

    def test_line_goes_before_the_first_order(self):
        lines = self.rewrite("ticks 5\norder 1 0:A:1 move 1 2\norder 2 0:A:1 move 3 4\n")
        self.assertEqual(lines, ["ticks 5", "quirks 3.1c +x", "order 1 0:A:1 move 1 2", "order 2 0:A:1 move 3 4"])

    def test_line_goes_at_the_end_without_orders(self):
        self.assertEqual(self.rewrite("ticks 5\nmap X"), ["ticks 5", "map X", "quirks 3.1c +x"])


class CheckQuirksTest(unittest.TestCase):
    NAMES = ("eq", "diff", "bad")
    STORED = [(1, "a" * 16), (2, "b" * 16)]

    def run_check(self, header_extra: dict[str, str], diff_digests, failed_names=("bad",)):
        """check_traces(quirks=...) with play_all mocked; (exit status or None, output, spec lines played)."""
        header = {"game_seed": "1", "crt_seed": "1", "view_seed": "1", **header_extra}
        stored = ({"game_seed": "1", "crt_seed": "1", "view_seed": "1"}, self.STORED)
        results = {
            "eq": (header, self.STORED, 1.0, None),
            "bad": (header, self.STORED, 1.0, None),
            "diff": (header, diff_digests, 1.0, reference.ROOT / "build/scratch/check-diff-x"),
        }
        played = {}

        def fake_play_all(exe, scenarios, game_dir, parallel, expected, poison, failed):
            for name, path in scenarios.items():
                played[name] = path.read_text()
            for name in failed_names:
                failed[name] = "playtest.py ended with a crash"
            return results

        files = {}
        with tempfile.TemporaryDirectory() as tmp:
            for n in self.NAMES:
                files[n] = Path(tmp) / f"{n}.txt"
                files[n].write_text("ticks 2\norder 1 0:A:1 move 1 2\n")
            out = io.StringIO()
            status = None
            with mock.patch.object(reference, "scenario_files", lambda: files), \
                    mock.patch.object(reference, "TRACES", Path(tmp) / "traces"), \
                    mock.patch.object(reference.tracedigest, "parse_digest", lambda p: stored), \
                    mock.patch.object(reference, "play_all", fake_play_all), \
                    contextlib.redirect_stdout(out):
                (Path(tmp) / "traces").mkdir()
                for n in self.NAMES:
                    (Path(tmp) / "traces" / f"{n}.txt").write_text("")
                try:
                    reference.check_traces(Path("x.exe"), None, 1, quirks="recommended")
                except SystemExit as e:
                    status = e.code
        return status, out.getvalue(), played

    def test_header_quirks_line_alone_is_equal(self):
        status, out, played = self.run_check({"quirks": "0000abcd recommended"}, self.STORED, failed_names=())
        self.assertIn("equal   eq", out)
        self.assertIn("equal   diff", out)
        self.assertNotIn("differs", out)
        self.assertIn("3 equal, 0 differ, 0 failed", out)
        self.assertIsNone(status)
        self.assertIn("quirks recommended\norder", played["eq"])

    def test_difference_names_the_tick_and_is_not_a_failure(self):
        status, out, _ = self.run_check({}, [(1, "a" * 16), (2, "c" * 16)], failed_names=())
        self.assertIn("differs diff", out)
        self.assertIn("tick 2", out)
        self.assertIn("[kept: build/scratch/check-diff-x]", out)
        self.assertIn("2 equal, 1 differ, 0 failed", out)
        self.assertIsNone(status)

    def test_failed_scenario_exits_1(self):
        status, out, _ = self.run_check({}, self.STORED)
        self.assertIn("FAIL bad: playtest.py ended with a crash", out)
        self.assertIn("2 equal, 0 differ, 1 failed", out)
        self.assertEqual(status, 1)

    def test_usage_errors(self):
        for argv in (["--quirks", "recommended"],
                     ["--check", "--exe", __file__, "--quirks", "recommended", "--poison"]):
            with mock.patch("sys.argv", ["reference.py", *argv]), \
                    contextlib.redirect_stderr(io.StringIO()), self.assertRaises(SystemExit) as cm:
                reference.main()
            self.assertEqual(cm.exception.code, 2)


if __name__ == "__main__":
    unittest.main()
