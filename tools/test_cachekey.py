"""Tests of the object cache key (check.source_key): `uv run tools/test_cachekey.py`.

Uses a temporary tree for the edits; no tracked file is touched.
"""

import sys
import tempfile
from pathlib import Path

from check import ROOT, include_closure, source_key
import check

FLAGS = "/O2"


def reset() -> None:
    check._include_names.cache_clear()
    check.include_dir_hash.cache_clear()


def key(src: Path, root: Path) -> str:
    reset()
    return source_key(src, FLAGS, root=root)


def test_temp_tree() -> None:
    with tempfile.TemporaryDirectory() as t:
        root = Path(t).resolve()
        (root / "include").mkdir()
        (root / "include/sys.h").write_text("int sys;\n")
        (root / "src/map").mkdir(parents=True)
        (root / "src/ai").mkdir()
        (root / "src/map/mission.h").write_text('#include "inner.h"\nint mission;\n')
        (root / "src/map/inner.h").write_text("int inner;\n")
        a = root / "src/ai/a.cpp"
        a.write_text('#include "../map/mission.h"\n// #include "commented.h"\n#include "sys.h"\n#include "later.h"\n#include <win.h>\n')
        b = root / "src/ai/b.cpp"
        b.write_text('#include "sys.h"\nint b;\n')
        ka, kb = key(a, root), key(b, root)
        assert key(a, root) == ka

        (root / "src/map/mission.h").write_text('#include "inner.h"\nint mission;\n\n')
        assert key(a, root) != ka, "a header under src/ must change its includer's key"
        assert key(b, root) == kb, "an unrelated file's key must not change"
        ka = key(a, root)

        (root / "src/map/inner.h").write_text("int inner2;\n")
        assert key(a, root) != ka, "the closure is followed recursively"
        ka = key(a, root)

        (root / "src/ai/later.h").write_text("int later;\n")
        assert key(a, root) != ka, "creating a missing include changes the key"
        files, missing = include_closure(a, root=root)
        assert missing == [] and root / "src/ai/later.h" in files
        ka = key(a, root)

        (root / "src/ai/commented.h").write_text("int c;\n")
        assert key(a, root) == ka, "an include inside a comment is ignored"

        (root / "include/sys.h").write_text("int sys2;\n")
        assert key(a, root) != ka and key(b, root) != kb, "include/ still covers every file"


def test_nested_chain() -> None:
    with tempfile.TemporaryDirectory() as t:
        root = Path(t).resolve()
        (root / "include").mkdir()
        (root / "src/A/sub").mkdir(parents=True)
        a = root / "src/A/a.cpp"
        a.write_text('#include "sub/b.h"\n')
        (root / "src/A/sub/b.h").write_text('#include "c.h"\n')
        (root / "src/A/c.h").write_text("int c;\n")
        files, missing = include_closure(a, root=root)
        assert root / "src/A/c.h" in files and missing == [], "c.h is found through the includer's directory"
        ka = key(a, root)
        (root / "src/A/c.h").write_text("int c2;\n")
        assert key(a, root) != ka, "editing c.h must change a.cpp's key"


def test_real_tree() -> None:
    reset()
    mission = (ROOT / "src/map/mission.h").resolve()
    including = ROOT / "src/ai/player_ai.cpp"
    assert mission in include_closure(including)[0]
    unrelated = [p for p in sorted((ROOT / "src").rglob("*.cpp")) if mission not in include_closure(p)[0]]
    assert unrelated, "some file must not reach mission.h"


if __name__ == "__main__":
    test_temp_tree()
    test_nested_chain()
    test_real_tree()
    print("ok")
    sys.exit(0)
