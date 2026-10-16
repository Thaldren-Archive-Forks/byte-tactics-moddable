"""Tests for data/trace_coverage.csv against data/state_classes.csv.

    uv run tools/test_trace_coverage.py
"""

import csv
import unittest
from pathlib import Path

DATA = Path(__file__).resolve().parent.parent / "data"
TRACED = ("yes", "partly", "no")
PLANS = ("add", "add-once", "argue-out", "none")
PRS = ("#136", "#137", "#138")


def read(name: str) -> list[dict]:
    with open(DATA / name, newline="") as f:
        return list(csv.DictReader(f))


def key(row: dict) -> tuple:
    return row["kind"], row["name"], row["address"]


class TraceCoverageTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.states = read("state_classes.csv")
        cls.rows = read("trace_coverage.csv")

    def test_every_sim_and_mixed_row_has_a_coverage_row(self):
        have = {key(r) for r in self.rows}
        missing = [key(r) for r in self.states if r["class"] in ("sim", "mixed") and key(r) not in have]
        self.assertEqual(missing, [])

    def test_every_coverage_row_is_a_sim_or_mixed_row(self):
        classes = {key(r): r["class"] for r in self.states}
        for r in self.rows:
            self.assertIn(classes.get(key(r)), ("sim", "mixed"), key(r))
            self.assertEqual(r["class"], classes[key(r)], key(r))

    def test_keys_are_unique(self):
        keys = [key(r) for r in self.rows]
        self.assertEqual(len(keys), len(set(keys)))

    def test_traced_and_plan_values_are_valid(self):
        for r in self.rows:
            self.assertIn(r["traced"], TRACED, key(r))
            self.assertIn(r["plan"], PLANS, key(r))

    def test_plan_is_empty_only_for_traced_rows(self):
        for r in self.rows:
            self.assertEqual(r["plan"] == "none", r["traced"] == "yes", key(r))

    def test_add_rows_name_the_pull_request_that_covers_them(self):
        for r in self.rows:
            if r["plan"] in ("add", "add-once"):
                self.assertIn(r["pr"], PRS, key(r))
            else:
                self.assertEqual(r["pr"], "", key(r))


if __name__ == "__main__":
    unittest.main()
