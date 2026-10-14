"""Keep a lead or maintainer session inside its own worktrees (a PreToolUse hook).

Active only when BT_ROLE is set (docs/team.md). A lead writes only under
.worktrees/issue-*, a maintainer only under .worktrees/review-*; the clone
root stays a clean checkout of moddable. Paths outside the clone (scratch
directories, memory) are always allowed.

Reads the hook's JSON on stdin; exit 2 blocks the tool call and shows stderr.
"""

import json
import os
import re
import sys

OWN = {"lead": "issue-", "maintainer": "review-"}
GIT_WRITE = re.compile(
    r"\bgit\b(?:\s+-C\s+\S+)?\s+(?:commit|switch|checkout|reset|merge|rebase|stash|apply|am|"
    r"cherry-pick|restore|rm|mv|add|pull|push)\b"
)
TARGET = re.compile(r"(?:\bcd\s+|\s-C\s+)(\"[^\"]*\"|'[^']*'|\S+)")


def where(path: str, root: str, prefix: str) -> str:
    """'own', 'other' (the clone root or another worktree) or 'outside' the clone."""
    p = os.path.realpath(os.path.expanduser(path))
    if p != root and not p.startswith(root + os.sep):
        return "outside"
    parts = os.path.relpath(p, root).split(os.sep)
    if parts[0] == ".worktrees" and len(parts) > 1 and parts[1].startswith(prefix):
        return "own"
    return "other"


def block(role: str, prefix: str, what: str) -> None:
    sys.stderr.write(
        f"BT_ROLE={role}: {what}. Work only inside .worktrees/{prefix}<N>/ "
        f"(tools/worktree.sh{' --pr' if role == 'maintainer' else ''} <N> prints its path), "
        "and use that absolute path; the clone root stays a clean checkout of moddable.\n"
    )
    sys.exit(2)


def main() -> None:
    role = os.environ.get("BT_ROLE", "")
    if role not in OWN:
        return
    prefix = OWN[role]
    data = json.load(sys.stdin)
    root = os.path.realpath(os.environ.get("CLAUDE_PROJECT_DIR") or data.get("cwd") or os.getcwd())
    tool = data.get("tool_name", "")
    args = data.get("tool_input") or {}

    if tool in ("Edit", "Write", "NotebookEdit"):
        path = args.get("file_path") or args.get("notebook_path") or ""
        if path and where(path, root, prefix) == "other":
            block(role, prefix, f"{path} is outside your worktrees")
    elif tool == "Bash":
        cmd = args.get("command", "")
        if not GIT_WRITE.search(cmd):
            return
        targets = [t.strip("\"'") for t in TARGET.findall(cmd)]
        if any("worktree.sh" in t for t in targets):
            return
        places = [where(t, root, prefix) for t in targets if "$" not in t] or [
            where(data.get("cwd") or root, root, prefix)
        ]
        if "own" not in places and "outside" not in places:
            block(role, prefix, "this git command would change the clone root or another worktree")


main()
