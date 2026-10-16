"""Tests of the error-dialog matching and the retry decision: `uv run tools/test_playtest_dialogs.py`.

Pure functions only; no game, display or Wine is run.
"""

import argparse
from pathlib import Path

import playtest
import reference
import tracediff

GAME = ("Total Annihilation", 640, 480)
BOX = ("Total Annihilation", 231, 77)


def test_classify() -> None:
    assert playtest.classify_dialog([GAME], False) is None
    assert playtest.classify_dialog([GAME, BOX], False) == ("dialog-directx", "Total Annihilation")
    assert playtest.classify_dialog([GAME, BOX], True) is None
    crt = ("Microsoft Visual C++ Runtime Library", 380, 150)
    assert playtest.classify_dialog([GAME, crt], True) == ("dialog-crt", crt[0])
    assert playtest.classify_dialog([GAME, crt], False)[0] == "dialog-crt"
    assert playtest.classify_dialog([("Cavedog", 300, 120)], True)[0] == "dialog-cavedog"
    assert playtest.classify_dialog([("tiny", 1, 1)], False) is None


def test_ticks_started() -> None:
    assert not playtest.ticks_started("bt-trace 1\ngame_seed 5\n")
    assert playtest.ticks_started("bt-trace 1\ngame_seed 5\n1 move 00ff12ab\n")


def test_retry() -> None:
    assert reference.should_retry("playtest.py failed: the game exited with code 1")
    assert reference.should_retry("the game shows an error dialog (dialog-directx, window title \"x\")")
    assert not reference.should_retry("the game shows an error dialog (dialog-crt, window title \"x\")")
    assert not reference.should_retry("the game shows an error dialog (dialog-cavedog, window title \"x\")")
    assert not reference.should_retry("the game still runs after 1470s")



def test_tracediff_poisons_only_b() -> None:
    calls = []

    def fake_play(exe, scenario, game_dir, dump, timeout=None, poison=False):
        calls.append((exe.name, dump, poison))
        return Path(exe.name)

    saved = tracediff.play, tracediff.need, tracediff.parse_trace, tracediff.diff_dumps, tracediff.print_diff
    tracediff.play = fake_play
    tracediff.need = lambda run, name: run / name
    tracediff.parse_trace = lambda path: ({}, [(1, "units", "aa")] if path.parent.name == "a.exe" else [(1, "units", "bb")])
    tracediff.diff_dumps = lambda *a: []
    tracediff.print_diff = lambda diffs: None
    try:
        args = argparse.Namespace(exe=["a.exe", "b.exe"], scenario=["s.txt"], game_dir=None,
                                  ignore_view_seed=False, poison_b=True)
        assert tracediff.compare(args) == 1
    finally:
        tracediff.play, tracediff.need, tracediff.parse_trace, tracediff.diff_dumps, tracediff.print_diff = saved
    assert calls == [("a.exe", None, False), ("b.exe", None, True), ("a.exe", 1, False), ("b.exe", 1, True)], calls


if __name__ == "__main__":
    for name, fn in list(globals().items()):
        if name.startswith("test_"):
            fn()
    print("ok")
