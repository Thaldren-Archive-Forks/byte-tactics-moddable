"""Tests for the trace name check in tools/reference.py.

    uv run tools/test_reference.py
"""

import os
import tempfile
import unittest
from pathlib import Path

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
