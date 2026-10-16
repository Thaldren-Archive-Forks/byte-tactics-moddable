"""Tests of the error-dialog matching and the retry decision: `uv run tools/test_playtest_dialogs.py`.

Pure functions only; no game, display or Wine is run.
"""

import playtest
import reference

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


if __name__ == "__main__":
    for name, fn in list(globals().items()):
        if name.startswith("test_"):
            fn()
    print("ok")
