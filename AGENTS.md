# Instructions for coding agents

This is the moddable fork of TA: Byte Tactics. Byte Tactics is a matching
decompilation of Total Annihilation 3.1c: source that compiles, with the
original Visual C++ 5.0, to the shipped exe byte for byte. This fork turns that
source into a moddable engine. Every game-logic subsystem becomes a WASM
module, the 3.1c code compiled to WASM is the default module for each, and the
whole game runs as WASM on thin native and browser hosts.

The one rule above all others is **conformity**: with no mods loaded, the game
logic behaves exactly like 3.1c, bit for bit, tick for tick. The fork drops
byte-matching and network compatibility with 3.1c clients; it never drops
conformity.

The plan, its decisions and its order are in issue #2 and its sub-issues. Read
#2 before your first issue. #1 is the mod-support design (quirk flags, Lua
hooks) that the plan builds on.

## Words used here

- **Reference:** the matching build (this source compiled with Visual C++ 5.0)
  plus the test harness from #3. It is 3.1c's game logic, and it is what
  conformity is measured against.
- **Traces:** per-tick, per-phase hashes of the game state from a test-mode run
  (#3). Two builds conform when their traces are identical.
- **Gate:** the checks a change must pass before it merges. The issue says
  which; conformity is always one of them.
- **Seam:** the function table a subsystem is called through (#8). Subsystems
  reach each other only through seams.

## 1. Check the setup

Work in a clone of `ozgb/byte-tactics-moddable`, with `origin` pointing at it
(`git remote get-url origin`). The base branch is `moddable`:
`tools/worktree.sh` branches from `origin/moddable`, and pull requests target
it. Upstream Byte Tactics has no `moddable` branch.

```sh
gh auth status
ls toolchain/msvc5-sp3/BIN/CL.EXE orig/TotalA.exe
uv run tools/check.py 0x401070      # must print MATCH
```

A first-time setup follows `CONTRIBUTING.md`. If any check fails, stop and
tell the human.

## 2. Pick an issue

Take an open sub-issue of #2 with no open blockers and no assignee:

```sh
gh api --paginate repos/ozgb/byte-tactics-moddable/issues/2/sub_issues \
  --jq '.[] | select(.state=="open" and (.assignees | length) == 0) | .number'
gh api repos/ozgb/byte-tactics-moddable/issues/<N>/dependencies/blocked_by \
  --jq '[.[] | select(.state=="open") | .number]'      # must print []
```

Claim it, then work in your own copy:

```sh
gh issue edit <N> --add-assignee @me
gh issue comment <N> --body "Claimed by <tool> / <model>. Branch issue-<N>."
cd "$(tools/worktree.sh <N>)"
```

Keep scratch files in `build/scratch/<N>/`. If you stop before finishing,
comment `Released` on the issue and unassign yourself.

## 3. Do the issue

Each issue's tasks, CI checks and "Done when" are the specification. Do what
it asks, and finish every task or list it under "Left out" in the pull
request.

While you work, keep to the plan's boundaries. CI enforces each one as its
issue lands:

- Game logic is deterministic. It reads time only as ticks and draws random
  numbers only from the game-logic streams (#3).
- Game state lives in the shared state block (#7). A module keeps nothing
  between ticks outside it.
- Subsystems call each other through seams (#8).
- Presentation (rendering, UI, sound, UI modules) reads the player view and
  changes the game only by sending orders (#22).
- Platform calls go through the host interface (#24).
- A change in behaviour is either a bug or a quirk flag that is off by default
  (#1). With every flag off, traces match the reference.

When the plan looks wrong for your issue, say so in a comment on the issue
and let the maintainer decide (`docs/team.md`), rather than working around it.

## 4. Pass the gate

Run every check the issue names. Until #3 lands there are no traces yet, and
the gate for the game logic is the matching build:

```sh
uv run tools/checkall.py <every address in the files you touched>   # all MATCH
```

Once #3 lands, the gate is identical traces for the scenario suite, and the
issue gives the command.

**When traces diverge**, the divergence report names the first tick, phase and
field that differ. Find the smallest part of your change that causes it, and
fix that part. Only report a check as passing when you ran it and it passed.

**When a change touches the reference build** (the harness in #3, the
bit-exact maths in #4), the matching rules apply to it. The reference is a
build of a `moddable` commit (`uv run tools/reference.py HEAD`). The matching
guides were not carried over: read `docs/agent-guide.md` and
`docs/c2-regalloc.md` on `main` (frozen at upstream's 8aad1812) first.

## 5. Open a pull request

```sh
git fetch origin moddable && git rebase origin/moddable
# re-run the gate
git push -u origin issue-<N>
gh pr create --repo ozgb/byte-tactics-moddable --base moddable --title "<Type>: <Subject>" --body-file <file>
```

The body:

```
Closes #<N>

Model: <tool> / <model>

Checks:
- <each check the issue names>: <result>

Left out:
- what the issue asked for that you did not do, and why (or "none")
```

`Closes #<N>` is what unblocks the issues waiting on yours. Open one pull
request per issue.

## Comments in code

The code is its own documentation: names, types and tests carry what it does,
and the commit message and pull request carry why it changed. A comment takes
one of three forms:

1. `// TODO(#123): what is left to do`, where #123 is an open issue.
2. A one-line doc comment on each function and type of a published interface:
   the subsystem interface headers (#8) and the host interface (#24). Modders
   read these without reading the engine.
3. `// FUNCTION: 0x4b6c30` address markers, which tie a function to the
   reference so a trace divergence can be looked up there.

The lint also allows the `GLOBAL:`, `ENTRY:` and `FLAGS:` lines, because the
matching tools read them as build input, not commentary; they go when #89
removes the matching tools.

Code that looks wrong but is deliberate, such as a 3.1c quirk, is documented by
a quirk flag whose name says what it does (#1), or by a `TODO(#n)` pointing at
the issue that will add one.

Files from the decompilation still carry its matching notes. Leave them in
files your issue doesn't otherwise change; #35 removes them all in one
pass.

`tools/commentlint.py` checks this in CI, on the lines a pull request adds.

## Writing style

These apply to commit messages, pull requests, issue comments and the
published interface docs.

- Use commas, colons or parentheses where an em dash might go.
- Commit messages are `<Type>: <Subject>`, where Type is one of Add, Fix,
  Update, Bump, Remove, Optimize, Merge, Refactor, Reformat or Docs. The
  subject is imperative, capitalised, at most 50 characters, with no full
  stop.
- End every commit message with `Assisted-by: <agent>:<model>` (for example
  `Assisted-by: Claude:claude-opus-5-5`). This is the only attribution line;
  commits and pull requests carry no Co-Authored-By or "Generated with" lines.
- Run Python through `uv run`, never `python` directly.
