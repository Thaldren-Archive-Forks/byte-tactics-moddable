"""Check several functions at once and print one line each.

    uv run tools/checkall.py 0x401000 0x401070 0x4010b0

Compiles the files in parallel and prints the summary line tools/check.py
would print for each address. Run tools/check.py on one address for its
references and diff.
"""

import fcntl
import os
import sys
from contextlib import contextmanager
from concurrent.futures import ThreadPoolExecutor

from check import ROOT, Original, annotations, compare, compile_source, find_source, report
from coff import parse_object


@contextmanager
def address_lock(address: int):
    """Holds a lock on one address's object path for the compile and the read.

    The object's path decides the compiler's symbol ids, so two checkall runs
    cannot be given paths of their own: they take turns on each address instead,
    or one deletes or rewrites the object the other is about to read."""
    lock_dir = ROOT / "build" / "checkall"
    lock_dir.mkdir(parents=True, exist_ok=True)
    with open(lock_dir / f"{address:#x}.lock", "w") as fh:
        fcntl.flock(fh, fcntl.LOCK_EX)
        yield


def main() -> None:
    if len(sys.argv) < 2 or sys.argv[1] in ("-h", "--help"):
        sys.exit(__doc__)
    addresses = [int(a, 16) for a in sys.argv[1:]]
    orig = Original()

    def one(address: int) -> str:
        src = find_source(address)
        if src is None:
            return f"{address:#x}  no file under src/ has '// FUNCTION: {address:#x}'"
        qualname = next((q for a, q in annotations(src) if a == address), None)
        with address_lock(address):
            obj, log = compile_source(src, out_dir=f"checkall/{address:#x}")
            if obj is None:
                print(f"--- {address:#x} {src.name}: compiler output ---\n{log.rstrip()}\n---", file=sys.stderr, flush=True)
                return f"{address:#x}  compile failed ({src.name}); run tools/check.py {address:#x}"
            res = compare(orig, parse_object(obj.read_bytes(), obj.name), address, None, qualname)
        return report(res, verbose=False).splitlines()[0]

    with ThreadPoolExecutor(max_workers=os.cpu_count() or 4) as pool:
        for line in pool.map(one, addresses):
            print(line)


if __name__ == "__main__":
    main()
