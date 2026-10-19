"""Tests for the trace name check in tools/reference.py.

    uv run tools/test_reference.py
"""

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


if __name__ == "__main__":
    unittest.main()
