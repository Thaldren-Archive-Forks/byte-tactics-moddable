"""Tests for the trace name check in tools/reference.py.

    uv run tools/test_reference.py
"""

import contextlib
import io
import os
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
    DIFFERS = [(1, "a" * 16), (2, "c" * 16)]

    def run_check(self, header_extra=None, diff_digests=None, failed_names=(), off_failed=None, off_digests=None):
        """check_traces(quirks=...) with play_all mocked, for a quirks run and the all-off rerun of "diff".

        Returns (exit status or None, output, scenario texts played, the names rerun).
        """
        header = {"game_seed": "1", "crt_seed": "1", "view_seed": "1", **(header_extra or {})}
        plain = {"game_seed": "1", "crt_seed": "1", "view_seed": "1"}
        stored = (plain, self.STORED)
        played, reruns = {}, []
        self.rerun_texts = {}

        with tempfile.TemporaryDirectory() as tmp:
            tmp = Path(tmp)

            def kept(name, phase_hashes):
                d = tmp / name
                d.mkdir()
                rows = [f"{t} {ph} {h}" for t, ph, h in phase_hashes]
                (d / "trace.txt").write_text("bt-trace 3\n" + "\n".join(rows) + "\n")
                return d

            quirk_rows = [(1, "units", "1" * 16), (2, "units", "2" * 16), (2, "players", "9" * 16)]
            off_rows = [(1, "units", "1" * 16), (2, "units", "2" * 16), (2, "players", "3" * 16)]

            def fake_play_all(exe, scenarios, game_dir, parallel, expected, poison, failed, keep_always=False):
                if keep_always:
                    reruns.extend(scenarios)
                    self.rerun_texts = {name: path.read_text() for name, path in scenarios.items()}
                    if off_failed:
                        failed["diff"] = off_failed
                        return {}
                    return {"diff": (plain, off_digests or self.STORED, 1.0, kept("off", off_rows))}
                for name, path in scenarios.items():
                    played[name] = path.read_text()
                for name in failed_names:
                    failed[name] = "playtest.py ended with a crash"
                return {
                    "eq": (header, self.STORED, 1.0, None),
                    "bad": (header, self.STORED, 1.0, None),
                    "diff": (header, diff_digests or self.STORED, 1.0, kept("quirks", quirk_rows)),
                }

            files = {}
            for n in self.NAMES:
                files[n] = tmp / f"{n}.txt"
                files[n].write_text("ticks 2\norder 1 0:A:1 move 1 2\n")
            (tmp / "traces").mkdir()
            for n in self.NAMES:
                (tmp / "traces" / f"{n}.txt").write_text("")
            out = io.StringIO()
            status = None
            with mock.patch.object(reference, "scenario_files", lambda: files), \
                    mock.patch.object(reference, "TRACES", tmp / "traces"), \
                    mock.patch.object(reference.tracedigest, "parse_digest", lambda p: stored), \
                    mock.patch.object(reference, "play_all", fake_play_all), \
                    contextlib.redirect_stdout(out):
                try:
                    reference.check_traces(Path("x.exe"), None, 1, quirks="recommended")
                except SystemExit as e:
                    status = e.code
        self.played_texts = played
        return status, out.getvalue(), played, reruns

    def test_header_quirks_line_alone_is_equal(self):
        status, out, played, reruns = self.run_check({"quirks": "0000abcd recommended"})
        self.assertIn("equal   eq", out)
        self.assertIn("equal   diff", out)
        self.assertNotIn("differs", out)
        self.assertIn("3 equal, 0 differ, 0 failed", out)
        self.assertIsNone(status)
        self.assertEqual(reruns, [])
        self.assertIn("quirks recommended\norder", played["eq"])

    def test_difference_names_the_tick_and_phase_and_is_not_a_failure(self):
        status, out, played, reruns = self.run_check(diff_digests=self.DIFFERS)
        self.assertIn("differs diff", out)
        self.assertIn("tick 2, phase players", out)
        self.assertIn("FUNCTION:", out)
        self.assertIn("2 equal, 1 differ, 0 failed", out)
        self.assertIsNone(status)
        self.assertEqual(reruns, ["diff"])
        self.assertIn("quirks recommended\norder", played["diff"])

    def test_all_off_rerun_plays_scenarios_without_the_quirks_line(self):
        self.run_check(diff_digests=self.DIFFERS)
        self.assertTrue(any(line.startswith("quirks") for line in self.played_texts["diff"].splitlines()))
        self.assertEqual(list(self.rerun_texts), ["diff"])
        self.assertFalse(any(line.startswith("quirks") for line in self.rerun_texts["diff"].splitlines()))

    def test_failed_scenario_exits_1(self):
        status, out, _, _ = self.run_check(failed_names=("bad",))
        self.assertIn("FAIL bad: playtest.py ended with a crash", out)
        self.assertIn("2 equal, 0 differ, 1 failed", out)
        self.assertEqual(status, 1)

    def test_failed_all_off_rerun_is_a_failure(self):
        status, out, _, _ = self.run_check(diff_digests=self.DIFFERS, off_failed="the exe hung")
        self.assertIn("FAIL diff: the all-off rerun failed: the exe hung", out)
        self.assertIn("2 equal, 0 differ, 1 failed", out)
        self.assertEqual(status, 1)

    def test_all_off_rerun_differing_from_stored_is_a_failure(self):
        status, out, _, _ = self.run_check(diff_digests=self.DIFFERS, off_digests=self.DIFFERS)
        self.assertIn("FAIL diff: all-off run differs from the stored trace", out)
        self.assertNotIn("differs diff", out)
        self.assertEqual(status, 1)

    def test_usage_errors(self):
        for argv in (["--quirks", "recommended"],
                     ["--check", "--exe", __file__, "--quirks", "recommended", "--poison"]):
            with mock.patch("sys.argv", ["reference.py", *argv]), \
                    contextlib.redirect_stderr(io.StringIO()), self.assertRaises(SystemExit) as cm:
                reference.main()
            self.assertEqual(cm.exception.code, 2)


class ReferenceEnvTest(unittest.TestCase):
    def test_pins_the_python_of_this_tree(self):
        with tempfile.TemporaryDirectory() as tmp:
            (Path(tmp) / ".python-version").write_text("3.14\n")
            env = reference.reference_env({"VIRTUAL_ENV": "x", "PATH": "/bin"}, Path(tmp))
        self.assertEqual(env, {"PATH": "/bin", "UV_PYTHON": "3.14"})

    def test_keeps_an_explicit_uv_python(self):
        with tempfile.TemporaryDirectory() as tmp:
            (Path(tmp) / ".python-version").write_text("3.14\n")
            env = reference.reference_env({"UV_PYTHON": "/usr/bin/python3.14"}, Path(tmp))
        self.assertEqual(env["UV_PYTHON"], "/usr/bin/python3.14")

    def test_without_the_pin_file_the_env_is_unchanged(self):
        with tempfile.TemporaryDirectory() as tmp:
            env = reference.reference_env({"PATH": "/bin"}, Path(tmp))
        self.assertEqual(env, {"PATH": "/bin"})

    def test_module_env_carries_the_pin(self):
        pin = (reference.ROOT / ".python-version").read_text().strip()
        self.assertEqual(reference.REF_ENV["UV_PYTHON"], os.environ.get("UV_PYTHON", pin))


class TracediffCommandTest(unittest.TestCase):
    def test_names_the_reference_side(self):
        cmd = reference.tracediff_command(Path("a/TotalA.exe"), Path("b/TotalA.exe"), Path("s.txt"), False, None)
        self.assertEqual(cmd[cmd.index("--reference") + 1], "a")
        self.assertEqual(cmd[2:4], ["a/TotalA.exe", "b/TotalA.exe"])
        self.assertNotIn("--poison-b", cmd)
        self.assertNotIn("--game-dir", cmd)

    def test_poison_and_game_dir(self):
        cmd = reference.tracediff_command(Path("a"), Path("b"), Path("s"), True, "/ta")
        self.assertIn("--poison-b", cmd)
        self.assertEqual(cmd[cmd.index("--game-dir") + 1], "/ta")


if __name__ == "__main__":
    unittest.main()
