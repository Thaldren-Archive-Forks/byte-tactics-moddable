"""Propose a class (sim, view, platform or mixed) for every piece of the game's state, with evidence.

    uv run tools/stateclasses.py                 # writes data/state_classes.csv, keeping class and note
    uv run tools/stateclasses.py --out F         # write F instead (a dry run; class and note kept from the data file)
    uv run tools/stateclasses.py --check         # fail if anything but class and note differs
    uv run tools/stateclasses.py --unclassified  # fail if a row has an empty class
    uv run tools/stateclasses.py --unjoined      # list the globals no row of data/rwsets.csv names
    uv run tools/stateclasses.py --build-dir D   # use D/compile_commands.json (default build/rwsets)
    uv run tools/stateclasses.py --jobs N        # clang-cl processes at once (default 4)
    uv run tools/test_stateclasses.py            # tests, on hand-written functions and rows

This is the evidence for #125. The shared state block of #7 holds what game logic
owns, so every global, static, region of `struct Game` and heap allocation of game logic
gets a proposed class and the facts it rests on. `proposed` is the tool's answer by the
rules below; `class` is the hand-set answer of the review and starts empty; `note` is free
text. Regeneration keeps `class` and `note` of a row by (kind, name) and rewrites the rest.

Rows (kind), one per item, sorted by kind, then address, then name:

  global       a row of data/globals.csv (name, address)
  static       a variable the source defines that data/globals.csv does not list: a
               function-level static (`name@Function`, address of the function), a
               file-level or class static (`name@path`, `Owner::name`) and a global that
               exists only in the source (the test harness, the debug library); a
               file-level static only when it is not const or something reads it
  game-region  a top-level field of `struct Game` in include/ta_types.h (name, address is
               the offset): an array of structs such as `Player[10]` is one region, the
               fields of a bit-field storage unit are one region (`a|b|c`), and the
               `unknown_` and padding ranges are regions of their own
  heap         an allocation site in game logic, named `Function: allocator type-or-tag`
               (`#n` for the n-th identical site of a function); address is the allocating
               function's. Sites: `new` and `new[]` (not placement new), operator new,
               malloc, calloc, realloc, GlobalAlloc, GlobalAllocFixed, LocalAlloc, HeapAlloc,
               VirtualAlloc, GameAlloc, GameAllocIgnoreTag, GameAllocShared, GameAllocThunk,
               GameRealloc and PoolAlloc. The type is the one the result is cast or assigned
               to, else the `sizeof` operand, else the first string literal (the tag). A
               site whose result is returned is a wrapper and not a row, its callers are.
               Game logic is a function of data/game_logic.csv or one the walk below
               reaches from a phase root or a load root. A site elsewhere (the lobby and
               the frame handlers reach UnitSync, its map nodes and the packet manager
               only that way) is a row too when it allocates a record whose fields a
               function of the walk reads or writes, or it allocates a primitive
               (a raw buffer, which has no record to join on) and assigns it to a global or
               a `Type::field` that a function of the walk accesses (InitPacketManager's
               0x42a-byte `operator new`, stored in the packet manager's buffer field), and
               it is outside src/debug/ and src/harness/. A buffer held in a local, or in
               a global or field no walk touches, is no row.

Columns:

  name, address, kind  as above
  proposed             sim, view, platform or mixed by the rules below, or empty where the
                       evidence cannot say
  class                the reviewed class, kept by regeneration
  rule                 the rule that fired
  phases_write         phases of RunGameSteps whose functions write it (`|` between)
  phases_read          the same for reads
  presentation         `r`, `w` or `rw`: presentation code reads, writes or does both
  load_write           `yes` if code reachable from a load root writes it
  writers, readers     `count:first|three|names`, the functions that write and read it,
                       those phases reach first; a name ending `?` only maybe-writes or
                       takes the address
  note                 free text, kept by regeneration

Phases. The roots are those of hazards.ROOTS, each walked separately over the `call` rows
of data/rwsets.csv and the hand-resolved edges of data/hazard_edges.csv (hazards.Graph),
so a function carries the set of roots that reach it. HarnessPhase numbers them 0 to 8:

  0 HandleNetPackets   1 UpdateAllUnits   2 UpdateProjectiles   3 UpdateExplosions
  4 UpdatePlayers      5 UpdateFeatures   6 StepAllGafSequences and UpdateWind
  7 UpdateMeteors      8 UpdateResourceSharing
  d RunGameSteps itself (the tick counter and the step loop), not what it calls

Load. `load_write` follows the same walk from LOAD_ROOTS: LoadMatch, and
AssignStartPositions, which tools/crtcalls.py lists as game logic that only frame
handlers call.

Presentation. The functions the walk calls presentation, and so touching state as
presentation, are those reachable from PRESENTATION_ROOTS and from every function
defined under PRESENTATION_DIRS, without entering a phase root, RunGameSteps or a load
root. The walks from the phase roots and the load roots stop at such a function in turn:
the call graph reaches the dialogs and menus from game logic through the hand-resolved
edges (a unit selection that pops a screen), and what the dialog code does is presentation,
not a phase. The one exception is SIM_IN_PRESENTATION_DIRS: StepGafSequence, which phase 6
runs from src/graphics/gaf.cpp. A helper outside those directories that both walks reach
counts for both. The roots:

  the non-sim calls of RunGameSteps   UpdateCameraFollow, UpdateParticles, UpdateBlink,
                                      ExpireOldestMessage, ExpireEyeballs and the three
                                      EmptyPostSimStepHook functions
  the main loop                       MainLoopTick (pacing, keys, scrolling, drawing:
                                      its calls to RunGameSteps and HandleNetPackets stop
                                      the walk)
  the frame handlers installed in g_game->handler: MenuFrame, LoadingScreenFrame,
                                      PreBattleFrame, InitFrame, EndGameFrame,
                                      CampaignSetupFrame, BattleFrame, ReturnToMainMenuFrame
  the draw, menu and sound code       every function under src/graphics/, src/sound/,
                                      src/gui/, src/frontend/ and src/ingame/ (DrawBattleFrame,
                                      WindowProc, the GUI click handlers and the audio
                                      timer callbacks are among them)

Rules for `proposed`, first match wins:

  platform  the type, the name or the allocator is a device, file, window or DirectX
            handle (PLATFORM_TYPE, PLATFORM_NAME, PLATFORM_ALLOCATORS), or only functions
            of src/debug/ or src/harness/ touch it
  mixed     a phase writes it and presentation writes it too; or load and presentation
            write it and a phase reads it; or presentation writes it and a phase reads it
  sim       a phase writes it; or load writes it and a phase reads it, or nothing else
            touches it; or a phase reads it and nothing in game logic or presentation
            writes it (constants and tables)
  view      presentation writes it and no phase reads it; or load writes it and only
            presentation reads it; or only presentation reads it
  (empty)   only code outside every walk touches it, or nothing does

A write is a `*_write` row, a `*_maybe_written` row (a non-const pointer or reference
argument) or a `*_escape` row (address taken). The last two are weak: a weak write by a
phase or by load code counts as a write (sim is the safe side), but a weak write by
presentation counts only as a read, so that a table passed to a non-const API by a dialog
is not mixed. A weak write that is the only kind of write adds `(weak writes only)` to
the rule.

Where the evidence comes from. Reads and writes of globals and statics are the
`global_*` rows of data/rwsets.csv. A global of struct type, a Game region of struct
type, a static and a heap block also take the `Type::field` rows of the type and, by
value, of the structs inside it. That evidence is by type, not by instance: a struct used
in many places (Rect, Vec3) gives all of them the accesses of all uses. A pointer is
its own field only; what it points at is a heap row.

Names. rwsets.csv names globals as the source does. A target joins the row of
data/globals.csv with the same name, or with the address of a `DAT_<8 hex digits>` target
(the source names a global it never renamed that way, and a template static such as
`std::_Tree::_Nil` only that way). A target that joins nothing is a static row. The
globals no target joins have no evidence and an empty proposal unless their type or name
says platform; `--unjoined` lists them. They are the `unreferenced` rows, the GUID
constants of src/data/guids.cpp, some float and string constants of src/data/unused.cpp, and
tables the source declares `extern` inside a function (g_unitOrders, g_groundOrders), which
rwsets.py takes for a local, or uses only in inline assembly (g_sinTable, g_cosTable).

Game regions. rwsets.csv names a field `Game::name`, but each source file declares its own
partial view of `struct Game`, with the names it needs, so the name says nothing about the
offset. The tool runs clang-cl with `-Xclang -fdump-record-layouts-complete` on every file
and reads the layout of the file's own `struct Game`: each last-field access is then a byte
range, and a region takes the reads and writes whose range overlaps its own and the
maybe-written and address-taken accesses that start in it (a view often declares one
array over the unknown bytes after a field, and taking its address names only its start).
The regions are the layout of include/ta_types.h, from a probe file that includes it. Accesses reached through
a nested path (`g_game->players[i].hp`) are `Player::hp` in rwsets.csv, which is why
a region of structs takes the fields of the type.

A view field wider than one real field (a view that declares one array over the unknown bytes
after a field: settings.cpp's `password` is 71 KB) is attributed only what the access touches.
A subscript with a literal index of a primitive array is that one element. Any other subscript
is the whole array if it is at most OVERLAY_BYTES (64) long, else its first byte, because
the AST cannot tell which element a computed index reaches. Passing the array to a call or taking its
address names its start, as before. Only a file whose `g_game` is declared as a pointer to
its own `struct Game` counts: font.cpp declares a `struct Game` for the object `GetDisplay()`
returns and has no `g_game`, so its `Game::` accesses name no region.

Sub-objects. A row of data/globals.csv whose `defined` column says `in X+off` lies inside
another global. It has the accesses that name it and nothing of X: the parent's
accesses, by name or through its type, are never copied to it. A global whose address is
only compared with something (`while (p < (int)g_serialPortNumber)`, the end of a
table the loop walks) is not accessed by it: that escape is dropped. `DAT_00000000`, which
the source names for a read of absolute address 0 (UnitSync::ProcessSync reads it in the
`direct != 0` branch, a null address in the shipped exe too), is no variable and no row.

Limits. Inline assembly is not looked into (an `asm` function may touch more). A call
through a function pointer or a virtual method only follows the edges of
data/hazard_edges.csv. A callee's writes through a pointer argument are not looked into.
Evidence is by name, so overloads share their evidence. Dynamic initialisers of globals
are not rows of rwsets.csv.
"""

import argparse
import csv
import difflib
import io
import json
import re
import subprocess
import sys
import tempfile
import time
from collections import defaultdict
from concurrent.futures import ProcessPoolExecutor
from dataclasses import dataclass, field
from pathlib import Path

import hazards
import rwsets
from rwsets import ROOT, Analyzer, Walker, is_pointer, record_name, strip, type_text

GLOBALS = ROOT / "data/globals.csv"
GAME_LOGIC = ROOT / "data/game_logic.csv"
TYPES_HEADER = "ta_types.h"
OUT = ROOT / "data/state_classes.csv"
COLUMNS = ["name", "address", "kind", "proposed", "class", "rule", "phases_write", "phases_read",
           "presentation", "load_write", "writers", "readers", "note"]
KINDS = ["game-region", "global", "heap", "static"]
CLASSES = ("sim", "view", "platform", "mixed")
KEPT = ("class", "note")

PHASE_LABELS = {"HandleNetPackets": "0", "UpdateAllUnits": "1", "UpdateProjectiles": "2",
                "UpdateExplosions": "3", "UpdatePlayers": "4", "UpdateFeatures": "5",
                "StepAllGafSequences": "6", "UpdateWind": "6", "UpdateMeteors": "7",
                "UpdateResourceSharing": "8"}
DRIVER = "RunGameSteps"
DRIVER_LABEL = "d"
LOAD_ROOTS = ["LoadMatch", "AssignStartPositions"]
PRESENTATION_ROOTS = ["UpdateCameraFollow", "UpdateParticles", "UpdateBlink", "ExpireOldestMessage",
                      "ExpireEyeballs", "EmptyPostSimStepHook", "EmptyPostSimStepHook_B",
                      "EmptyPostSimStepHook_C", "MainLoopTick", "MenuFrame", "LoadingScreenFrame",
                      "PreBattleFrame", "InitFrame", "EndGameFrame", "CampaignSetupFrame", "BattleFrame",
                      "ReturnToMainMenuFrame"]
PRESENTATION_DIRS = ("src/graphics/", "src/sound/", "src/gui/", "src/frontend/", "src/ingame/")
NON_GAME_DIRS = ("src/debug/", "src/harness/")
SIM_IN_PRESENTATION_DIRS = ["StepGafSequence"]

PLATFORM_TYPE = re.compile(r"\b(HWND|HDC|HANDLE|HINSTANCE|HMODULE|HBITMAP|HPALETTE|HFONT|HICON|HCURSOR|HKEY|"
                           r"HGLOBAL|FILE|_?GUID|CRITICAL_SECTION|CritSec_\w+|LoadedImage|IDirect\w+|"
                           r"LPDIRECT\w+|DDSURFACEDESC\w*|DSBUFFERDESC\w*|WNDCLASS\w*|Surface|DirectDraw\w*)\b")
PLATFORM_NAME = re.compile(r"(^|_)(hwnd|hinst|handle|module|device|process|dll|pfn|gdperf|psapi|"
                           r"dplay|ddraw|dsound|guid|iid|clsid|dpaid|dplproperty)(_|$)|direct_(sound|draw|play)|"
                           r"windows_sound|window_(class|name)|blit_lock|lzss_lock|screen_lock|squash_thread|"
                           r"image_help")
PLATFORM_ALLOCATORS = {"VirtualAlloc"}
ALLOCATORS = (set(hazards.ALLOCATORS) | {"calloc", "GameAllocIgnoreTag", "GameAllocShared", "GameAllocThunk",
                                         "VirtualAlloc"})
PRIMITIVES = {"void", "char", "unsigned char", "signed char", "short", "unsigned short", "int", "unsigned int",
              "long", "unsigned long", "float", "double", "?"}
LAYOUT_FLAG = ["-Xclang", "-fdump-record-layouts-complete"]
LAYOUT_LINE = re.compile(r"^\s*(\d+)(?::(\d+)-(\d+))? \|( *)(.*?)\s*$")
RECORD_KEYWORD = re.compile(r"^(struct|union|class|enum)\s+")
UNKNOWN_RANGE = re.compile(r"^(unknown_|pad)")
WRITE_MODES = ("write", "maybe_written", "escape")
BIG = 1 << 40


@dataclass
class Field:
    """One member of a record in a layout dump, with its byte range."""
    name: str
    type: str
    start: int
    end: int = 0
    bits: bool = False


def layout_fields(text: str, record: str) -> tuple[int, list[Field]] | None:
    """The size and top-level members of `record` in a clang record-layout dump, or None.

    Members of an anonymous struct or union count as members of the record. A member's
    end is the next greater start (or the size), so union members share one end.
    """
    head = re.search(r"^ *0 \| (?:struct|class|union) %s\n" % re.escape(record), text, re.M)
    if not head:
        return None
    fields: list[Field] = []
    stack: list[tuple[int, bool]] = []
    size = 0
    for line in text[head.end():].split("\n"):
        if "[sizeof=" in line:
            size = int(re.search(r"sizeof=(\d+)", line).group(1))
            break
        m = LAYOUT_LINE.match(line)
        if not m:
            break
        start, low, _, indent, body = m.groups()
        depth = (len(indent) - 1) // 2
        while stack and stack[-1][0] >= depth:
            stack.pop()
        effective = depth - sum(1 for _, transparent in stack if transparent)
        transparent = body.endswith(")") and ("(anonymous" in body or "(unnamed" in body)
        stack.append((depth, transparent))
        if effective != 1 or transparent or " " not in body:
            continue
        name = body.rsplit(None, 1)[-1]
        type_ = RECORD_KEYWORD.sub("", body[:-len(name)].strip())
        fields.append(Field(name, type_, int(start), bits=low is not None))
    starts = sorted({f.start for f in fields} | {size})
    for f in fields:
        f.end = next(s for s in starts if s > f.start) if f.start < size else f.start
    return size, fields


def game_regions(fields: list[Field]) -> list[dict]:
    """The regions of a Game layout: a field each, but the fields of one bit-field unit together."""
    regions: list[dict] = []
    for f in fields:
        last = regions[-1] if regions else None
        if f.bits and last and last["bits"] and last["start"] == f.start:
            last["name"] += "|" + f.name
        else:
            regions.append({"name": f.name, "type": f.type, "start": f.start, "end": f.end, "bits": f.bits})
    return regions


def view_spans(found) -> dict[str, tuple[int, int]]:
    """For a file's own `struct Game`, each member name and the byte range it covers."""
    if not found:
        return {}
    return {f.name: (f.start, f.end) for f in found[1]}


def records_by_value(type_name: str, fields: dict[str, dict[str, str]]) -> list[str]:
    """The record `type_name` names, with the records held inside it by value, or none."""
    root = value_record(type_name, fields)
    if root is None:
        return []
    seen = [root]
    for name in seen:
        for inner in fields.get(name, {}).values():
            if inner in fields and inner not in seen:
                seen.append(inner)
    return seen


def value_record(type_name: str, fields: dict[str, dict[str, str]]) -> str | None:
    """The record a variable of this type holds by value (arrays allowed), else None."""
    text = re.sub(r"\[[^\]]*\]", "", type_name or "")
    text = rwsets.QUALIFIERS.sub("", text)
    if "*" in text or "&" in text or "(" in text:
        return None
    text = " ".join(text.split())
    return text if text in fields else None


def by_value_name(t: dict) -> str:
    """The record name of a field's type when it holds the record by value, else empty."""
    text = type_text(t)
    if "*" in text or "&" in text or "(" in text:
        return ""
    return record_name(t)


ELEMENT_SIZES = {"char": 1, "unsigned char": 1, "signed char": 1, "short": 2, "unsigned short": 2, "int": 4,
                 "unsigned int": 4, "long": 4, "unsigned long": 4, "float": 4}
OVERLAY_BYTES = 64
COMPARISONS = {"<", ">", "<=", ">=", "==", "!="}


def peel(node: dict) -> dict:
    """The node under casts and parentheses."""
    while node["kind"] in ("ImplicitCastExpr", "CStyleCastExpr", "ParenExpr") and node.get("inner"):
        node = node["inner"][0]
    return node


def constant_index(node: dict) -> int | None:
    """The value of an integer literal index, else None."""
    node = peel(node)
    if node["kind"] == "IntegerLiteral":
        return int(node["value"])
    return None


class RegionWalker(Walker):
    """The rwsets walker, recording `Game::name` accesses as the byte range they touch in this file's view.

    A plain access of a member is the member's whole range. A subscript of an array member
    is the one element when the index is a literal and the element a primitive. Any other
    index is the whole array if it is at most OVERLAY_BYTES long, else its first byte: a view
    often declares one big array over the unknown bytes after a field, and the access cannot
    be told from a loop or a computed offset. Taking the address or passing the array to a
    call names its start in the same way.
    """

    hint: tuple[str, int, int] | None = None

    def record(self, kind: str, target: str) -> None:
        if kind.startswith("field_") and target.startswith("Game::"):
            name = target[len("Game::"):]
            span = self.unit.view.get(name)
            if span:
                low, high = span
                if self.hint and self.hint[0] == name:
                    low, high = min(high, low + self.hint[1]), min(high, low + self.hint[2])
                kind, target = "region_" + kind[len("field_"):], "%d-%d" % (low, high)
        super().record(kind, target)

    def lvalue(self, node: dict, mode: str, path: bool = False) -> None:
        if node["kind"] == "ArraySubscriptExpr" and not path:
            base, index = node["inner"]
            member = strip(base)
            if base["kind"] == "ImplicitCastExpr" and member["kind"] == "MemberExpr" and member.get("inner") and \
                    record_name(member["inner"][0]["type"]) == "Game":
                size = ELEMENT_SIZES.get(type_text(node["type"]).replace("const ", "").strip())
                value = constant_index(index)
                span = self.unit.view.get(member.get("name") or "", (0, 0))
                if size and value is not None and value >= 0:
                    low, high = value * size, (value + 1) * size
                elif span[1] - span[0] <= OVERLAY_BYTES:
                    low, high = 0, span[1] - span[0]
                else:
                    low, high = 0, 1
                self.visit(index, "R")
                self.hint = (member.get("name") or "", low, high)
                try:
                    self.lvalue(member, mode)
                finally:
                    self.hint = None
                return
        super().lvalue(node, mode, path)

    def visit(self, node: dict, mode: str = "R") -> None:
        if node.get("kind") == "BinaryOperator" and node["opcode"] in COMPARISONS:
            for child in node["inner"]:
                target = peel(child)
                if target["kind"] == "DeclRefExpr" and target["referencedDecl"]["kind"] == "VarDecl" and \
                        target["referencedDecl"]["id"] not in self.locals:
                    name = self.unit.statics.get(target["referencedDecl"]["id"], target["referencedDecl"]["name"])
                    self.record("bound_global", name)
        super().visit(node, mode)


def call_name(node: dict) -> str | None:
    """The allocator a call expression names, else None."""
    name = hazards.callee_name(node)
    return name if name in ALLOCATORS and strip(node["inner"][0])["kind"] == "DeclRefExpr" else None


def context_type(node: dict, parents: list[dict]) -> str | None:
    """The type an allocation's result is cast or assigned to, or None."""
    child = node
    for parent in reversed(parents):
        kind = parent["kind"]
        if kind in hazards.WRAPPING:
            if kind != "ParenExpr" and is_pointer(parent.get("type", {})):
                name = record_name(parent["type"])
                if name != "void":
                    return name
        elif kind == "BinaryOperator" and parent["opcode"] == "=" and parent["inner"][1] is child:
            return record_name(parent["inner"][0]["type"])
        elif kind == "VarDecl":
            return record_name(parent["type"])
        else:
            return None
        child = parent
    return None


def stored_target(node: dict, parents: list[dict], locals_: dict[str, str]) -> str:
    """The rwsets target (`Type::field`, a global's name or a static) an allocation is assigned to, else empty."""
    child = node
    for parent in reversed(parents):
        kind = parent["kind"]
        if kind in hazards.WRAPPING:
            child = parent
        elif kind == "BinaryOperator" and parent["opcode"] == "=" and parent["inner"][1] is child:
            target = peel(parent["inner"][0])
            if target["kind"] == "MemberExpr":
                return "%s::%s" % (record_name(peel(target["inner"][0])["type"]), target.get("name") or "<anon>")
            if target["kind"] == "DeclRefExpr" and target["referencedDecl"]["kind"] == "VarDecl":
                decl = target["referencedDecl"]
                return locals_.get(decl["id"], decl["name"])
            return ""
        else:
            return ""
    return ""


def is_returned(node: dict, parents: list[dict]) -> bool:
    """Whether the allocation's result goes straight to a return (a wrapper)."""
    return hazards.holder_of(node, parents) == "return"


def argument_type(node: dict) -> str | None:
    """The `sizeof` operand type or the first string literal among a call's arguments."""
    found = None
    stack = list(node["inner"][1:])
    while stack:
        n = stack.pop(0)
        if n.get("kind") == "UnaryExprOrTypeTraitExpr" and n.get("name") == "sizeof":
            arg = n.get("argType")
            if arg:
                return record_name(arg)
        elif n.get("kind") == "StringLiteral" and found is None:
            found = n["value"].strip('"')
        stack = list(n.get("inner", [])) + stack
    return found


def local_variables(node: dict, function: str) -> dict[str, str]:
    """The variables declared in a function by id: "" for a local, `name@Function` for a static.

    A parameter is no VarDecl, and a local is not state."""
    found = {}
    stack = [node]
    while stack:
        n = stack.pop()
        if n.get("kind") == "VarDecl":
            found[n["id"]] = "%s@%s" % (n["name"], function) if n.get("storageClass") == "static" else ""
        stack.extend(n.get("inner", ()))
    return found


def allocations(node: dict, function: str) -> list[tuple[str, str, int, str]]:
    """The (allocator, type or tag, line, where stored) allocation sites in one function definition.

    Where stored is the `Type::field` or global the result is assigned to, or empty."""
    if function in ALLOCATORS:
        return []
    found: list[tuple[str, str, int, str]] = []
    parents: list[dict] = []
    locals_ = local_variables(node, function)

    def walk(n: dict) -> None:
        kind = n.get("kind")
        if kind is None:
            return
        if kind == "CXXNewExpr" and not n.get("isPlacement") and not is_returned(n, parents):
            found.append(("new[]" if n.get("isArray") else "new", record_name(n["type"]), n.get("_line", 0),
                          stored_target(n, parents, locals_)))
        elif kind == "CallExpr" and call_name(n) and not is_returned(n, parents):
            context = context_type(n, parents)
            argument = argument_type(n)
            what = context if context and context not in PRIMITIVES else argument or context or "?"
            found.append((call_name(n), what, n.get("_line", 0), stored_target(n, parents, locals_)))
        if kind not in rwsets.UNEVALUATED:
            parents.append(n)
            for child in n.get("inner", ()):
                walk(child)
            parents.pop()

    for child in node.get("inner", ()):
        if child["kind"].endswith("Stmt") or child["kind"] == "CXXCtorInitializer":
            walk(child)
    return found


def function_statics(node: dict, function: str) -> list[tuple[str, str]]:
    """The (name@Function, type) of the static variables declared in a function body."""
    found = []
    stack = [node]
    while stack:
        n = stack.pop()
        if n.get("kind") == "VarDecl" and n.get("storageClass") == "static":
            found.append(("%s@%s" % (n["name"], function), n["type"].get("qualType", "")))
        stack.extend(n.get("inner", ()))
    return found


class StateAnalyzer(Analyzer):
    """The rwsets analyzer, also keeping statics and their types, record fields, allocations and Game ranges."""

    walker_class = RegionWalker

    def __init__(self, root: Path):
        super().__init__(root)
        self.view: dict[str, tuple[int, int]] = {}
        self.fields: dict[str, dict[str, str]] = defaultdict(dict)
        self.static_types: dict[str, str] = {}
        self.game_var = ""

    def declare(self, node: dict, owner: str | None) -> None:
        super().declare(node, owner)
        kind = node["kind"]
        if kind == "VarDecl" and node.get("name") == "g_game":
            self.game_var = node["type"].get("qualType", "")
        if kind == "VarDecl" and node["id"] in self.statics:
            self.static_types[self.statics[node["id"]]] = node["type"].get("qualType", "")
        elif kind == "CXXRecordDecl" and node.get("name") and node.get("completeDefinition"):
            self.collect_fields(node, owner + "::" + node["name"] if owner else node["name"])

    def collect_fields(self, node: dict, name: str) -> None:
        """Record the fields of a record and, folded into it, those of its anonymous members."""
        for child in node.get("inner", ()):
            if child["kind"] == "FieldDecl" and child.get("name"):
                self.fields[name][child["name"]] = by_value_name(child["type"])
            elif child["kind"] == "CXXRecordDecl" and not child.get("name"):
                self.collect_fields(child, name)

    def function(self, node: dict, name: str) -> None:
        before = len(self.functions)
        super().function(node, name)
        if len(self.functions) == before:
            return
        fn = self.functions[-1]
        fn["regions"] = [item for item in fn["items"] if item[0].startswith("region_")]
        fn["bounds"] = [item[1] for item in fn["items"] if item[0] == "bound_global"]
        fn["statics"] = function_statics(node, name)
        fn["allocs"] = allocations(node, name)


def set_view(unit: StateAnalyzer, layouts: str) -> None:
    """Take the Game ranges from the record layouts clang printed before the syntax tree."""
    unit.view = view_spans(layout_fields(layouts, "Game"))


def scan(job: tuple[Path, list[str], str]) -> dict:
    """Run clang-cl on one file (layouts and syntax tree) and return what the tool needs from it."""
    unit, _ = rwsets.analyse_unit(job, StateAnalyzer, set_view)
    owns_game = unit.game_var.replace("*", "").replace("const", "").strip() == "Game" and "*" in unit.game_var
    functions = [{key: fn[key] for key in ("address", "function", "file", "start", "regions", "bounds", "statics",
                                           "allocs")}
                 for fn in unit.functions]
    if not owns_game:
        for fn in functions:
            fn["regions"] = []
    return {"functions": functions, "fields": dict(unit.fields), "static_types": unit.static_types}


def scan_source(path: Path, root: Path) -> dict:
    """The scan of one source file outside the build, compiled with no headers (for tests)."""
    argv = ["clang-cl", "--target=i686-pc-windows-msvc", "/nologo", "-TP", "-fms-compatibility",
            "-fms-extensions", "-fsyntax-only"] + LAYOUT_FLAG + ["-Xclang", "-ast-dump=json", "--", str(path)]
    return scan((root, argv, str(root)))


def layout_argv(argv: list[str]) -> list[str]:
    """The AST-dump command line with the record-layout dump switched on."""
    i = argv.index("--")
    return argv[:i] + LAYOUT_FLAG + argv[i:]


def canonical_regions(argv: list[str], directory: str) -> list[dict]:
    """The regions of `struct Game` in include/ta_types.h, from a probe file that includes it."""
    i = argv.index("--")
    dump = argv.index("-ast-dump=json")
    flags = argv[:dump - 1] + argv[dump + 1:i]
    with tempfile.TemporaryDirectory() as tmp:
        probe = Path(tmp) / "probe.cpp"
        probe.write_text('#include "%s"\nstatic_assert(sizeof(Game) > 0, "");\n' % TYPES_HEADER)
        run = subprocess.run(flags + LAYOUT_FLAG + ["--", str(probe)], cwd=directory, capture_output=True,
                             text=True, errors="replace")
    found = layout_fields(run.stdout, "Game")
    if not found:
        raise SystemExit("stateclasses: no layout of struct Game from include/%s\n%s" % (TYPES_HEADER,
                                                                                           run.stderr[-1000:]))
    return game_regions(found[1])


def reach(graph: hazards.Graph, starts, stops: set[str]) -> set[str]:
    """The starts and the functions on a `call` path from them that does not enter a function of `stops`."""
    seen = set(starts) - stops
    todo = list(seen)
    while todo:
        for callee in graph.calls.get(todo.pop(), ()):
            if callee not in seen and callee not in stops:
                seen.add(callee)
                todo.append(callee)
    return seen


class Walks:
    """Which phases, load roots and presentation code reach each function, and where it is defined."""

    def __init__(self, rows: list[list[str]], graph: hazards.Graph):
        roots = set(PHASE_LABELS)
        if roots != set(hazards.ROOTS):
            raise SystemExit("stateclasses: PHASE_LABELS must name hazards.ROOTS exactly")
        for name in [DRIVER] + LOAD_ROOTS + PRESENTATION_ROOTS:
            if name not in graph.functions:
                raise SystemExit("stateclasses: %s is not a function of data/rwsets.csv" % name)
        self.files: dict[str, set[str]] = defaultdict(set)
        self.addresses: dict[str, str] = {}
        for address, function, file, kind, _ in rows:
            if kind == "function":
                self.files[function].add(file)
                if address and (function not in self.addresses or address < self.addresses[function]):
                    self.addresses[function] = address
        ui = {f for f, files in self.files.items() if any(x.startswith(PRESENTATION_DIRS) for x in files)}
        ui -= set(SIM_IN_PRESENTATION_DIRS)
        presentation_starts = ui | set(PRESENTATION_ROOTS)
        self.stops = presentation_starts
        self.phases: dict[str, set[str]] = defaultdict(set)
        for root, label in PHASE_LABELS.items():
            for name in reach(graph, [root], presentation_starts):
                self.phases[name].add(label)
        self.phases[DRIVER].add(DRIVER_LABEL)
        self.load = reach(graph, LOAD_ROOTS, presentation_starts)
        self.presentation = reach(graph, presentation_starts, roots | {DRIVER} | set(LOAD_ROOTS))
        self.sim = set(self.phases) | self.load

    def rank(self, function: str) -> tuple:
        """Sort key putting the functions game logic reaches first."""
        return (not self.phases.get(function), function not in self.load, function not in self.presentation,
                function)


@dataclass
class Access:
    """The functions that read and write one item (writers map to whether the write is certain)."""
    reads: set = field(default_factory=set)
    writes: dict = field(default_factory=dict)

    def add(self, function: str, mode: str) -> None:
        if mode in ("read", "escape"):
            self.reads.add(function)
        if mode in WRITE_MODES:
            strong = mode == "write"
            self.writes[function] = self.writes.get(function, False) or strong

    def merge(self, other: "Access") -> None:
        self.reads |= other.reads
        for function, strong in other.writes.items():
            self.writes[function] = self.writes.get(function, False) or strong


@dataclass
class Evidence:
    """What the walks say about the accesses of one item."""
    phases_write: set
    phases_read: set
    load_write: bool
    pres_write: bool
    pres_read: bool
    other_write: bool
    other_read: bool
    strong_write: bool
    has_write: bool
    files: set
    writers: list
    readers: list


def evidence(access: Access, walks: Walks) -> Evidence:
    """Fold an item's readers and writers into the facts the rules use."""
    phases_write, phases_read = set(), set()
    load_write = pres_write = pres_read = other_write = other_read = False
    for function, strong in access.writes.items():
        phases_write |= walks.phases.get(function, set())
        load_write |= function in walks.load
        pres_write |= strong and function in walks.presentation
        pres_read |= not strong and function in walks.presentation
        other_write |= function not in walks.sim and function not in walks.presentation
    for function in access.reads:
        phases_read |= walks.phases.get(function, set())
        pres_read |= function in walks.presentation
        other_read |= function not in walks.sim and function not in walks.presentation
    files = set()
    for function in access.reads | set(access.writes):
        files |= walks.files.get(function, set())
    return Evidence(phases_write, phases_read, load_write, pres_write, pres_read, other_write, other_read,
                    any(access.writes.values()), bool(access.writes), files,
                    sorted(access.writes, key=walks.rank), sorted(access.reads, key=walks.rank))


def snake_case(name: str) -> str:
    """A camelCase or snake_case name as lower-case words joined by underscores."""
    return re.sub(r"(?<=[a-z0-9])(?=[A-Z])|(?<=[A-Z])(?=[A-Z][a-z])", "_", name).lower()


def platform_reason(name: str, type_name: str, allocator: str | None = None) -> str | None:
    """Why the item is a device, file, window or DirectX handle by its allocator, type or name, else None."""
    if allocator in PLATFORM_ALLOCATORS:
        return "allocator %s" % allocator
    m = PLATFORM_TYPE.search(type_name or "")
    if m:
        return "type %s" % m.group()
    m = PLATFORM_NAME.search(snake_case(name.split("@")[0]))
    if m:
        return "name matches %s" % m.group().strip("_")
    return None


def classify(ev: Evidence, platform: str | None = None) -> tuple[str, str]:
    """The proposed class and the rule that fired, from the evidence (see the module docstring)."""
    if platform:
        return "platform", "platform: " + platform
    if ev.files and all(f.startswith(NON_GAME_DIRS) for f in ev.files):
        return "platform", "platform: only the debug library or the test harness touches it"
    weak = " (weak writes only)" if ev.has_write and not ev.strong_write else ""
    sim_read = bool(ev.phases_read)
    if ev.phases_write:
        if ev.pres_write:
            return "mixed", "mixed: a phase and presentation write it" + weak
        return "sim", "sim: a phase writes it" + weak
    if ev.load_write:
        if sim_read:
            if ev.pres_write:
                return "mixed", "mixed: load and presentation write it, a phase reads it" + weak
            return "sim", "sim: load writes it, a phase reads it" + weak
        if ev.pres_write or ev.pres_read:
            return "view", "view: load writes it, only presentation uses it" + weak
        return "sim", "sim: only load code touches it" + weak
    if ev.pres_write:
        if sim_read:
            return "mixed", "mixed: presentation writes it, a phase reads it" + weak
        return "view", "view: presentation writes it, no phase reads it" + weak
    if sim_read:
        if ev.other_write:
            return "sim", "sim: a phase reads it, only code outside the walks writes it" + weak
        return "sim", "sim: a phase reads it, nothing in game logic or presentation writes it"
    if ev.pres_read:
        return "view", "view: only presentation reads it, nothing writes it"
    if ev.other_write or ev.other_read:
        return "", "none: only code outside every walk touches it"
    return "", "none: no access found"


def format_functions(names: list[str], weak: set = frozenset()) -> str:
    """`count:a|b|c` for the first three names, a name ending `?` where the access is uncertain."""
    if not names:
        return ""
    shown = ["%s%s" % (n, "?" if n in weak else "") for n in names[:3]]
    return "%d:%s" % (len(names), "|".join(shown))


def make_row(kind: str, name: str, address: str, ev: Evidence, platform: str | None, writer_weak: set,
             unknown: bool = False) -> dict:
    """One output row, the proposal and rule from the evidence."""
    proposed, rule = classify(ev, platform)
    if unknown and rule == "none: no access found":
        rule = "none: unknown range, no access found"
    return {"name": name, "address": address, "kind": kind, "proposed": proposed, "class": "", "rule": rule,
            "phases_write": "|".join(sorted(ev.phases_write)), "phases_read": "|".join(sorted(ev.phases_read)),
            "presentation": ("r" if ev.pres_read else "") + ("w" if ev.pres_write else ""),
            "load_write": "yes" if ev.load_write else "",
            "writers": format_functions(ev.writers, writer_weak),
            "readers": format_functions(ev.readers), "note": ""}


class Items:
    """The accesses of every global, static and field in data/rwsets.csv, joined to the rows of data/globals.csv."""

    def __init__(self, rows: list[list[str]], globals_rows: list[dict], bounds: set | frozenset = frozenset()):
        self.by_name = {g["name"]: g for g in globals_rows}
        self.by_address = {int(g["address"], 16): g for g in globals_rows}
        self.access: dict[tuple, Access] = defaultdict(Access)
        for _, function, _, kind, target in rows:
            category, _, mode = kind.partition("_")
            if category == "global" and mode:
                key = self.join(target)
                if key and not (mode == "escape" and (function, target) in bounds):
                    self.access[key].add(function, mode)
            elif category == "field" and mode and not target.startswith("Game::"):
                self.access[("field", target)].add(function, mode)

    def join(self, target: str) -> tuple | None:
        """The key of the row a global target belongs to: ("global", name) or ("static", target).

        None for `DAT_00000000`, which is no variable: a read of absolute address 0.
        """
        if target in self.by_name:
            return ("global", target)
        m = re.fullmatch(r"DAT_([0-9a-f]{8})", target)
        if m and int(m.group(1), 16) == 0:
            return None
        if m and int(m.group(1), 16) in self.by_address:
            return ("global", self.by_address[int(m.group(1), 16)]["name"])
        return ("static", target)

    def for_type(self, type_name: str, fields: dict[str, dict[str, str]]) -> Access:
        """The accesses of the fields of the record a type holds by value, and of the records inside it."""
        total = Access()
        for record in records_by_value(type_name, fields):
            for name in fields.get(record, {}):
                total.merge(self.access.get(("field", "%s::%s" % (record, name)), Access()))
        return total


def region_access(region: dict, spans: dict) -> Access:
    """The accesses to the region: a read or write whose byte range overlaps it, or a weak one starting in it.

    A file's view may declare one big array over the unknown bytes after a field. Taking
    the address of that array (a weak access) names the start of it, not all of it.
    """
    total = Access()
    for (start, end, weak), access in spans.items():
        if (region["start"] <= start < region["end"]) if weak else (start < region["end"] and end > region["start"]):
            total.merge(access)
    return total


def rows_of(rwsets_rows: list[list[str]], graph: hazards.Graph, globals_rows: list[dict], scans: list[dict],
            regions: list[dict], logic_addresses: set[str]) -> list[dict]:
    """Every row of data/state_classes.csv (class and note empty), sorted."""
    walks = Walks(rwsets_rows, graph)
    fields: dict[str, dict[str, str]] = defaultdict(dict)
    static_types: dict[str, str] = {}
    functions: dict[tuple, dict] = {}
    spans: dict[tuple, Access] = defaultdict(Access)
    for scan_result in scans:
        for record, members in scan_result["fields"].items():
            fields[record].update(members)
        static_types.update(scan_result["static_types"])
        for fn in scan_result["functions"]:
            functions.setdefault((fn["function"], fn["file"], fn["start"]), fn)
    bounds = {(fn["function"], name) for fn in functions.values() for name in fn.get("bounds", ())}
    items = Items(rwsets_rows, globals_rows, bounds)
    for fn in functions.values():
        for kind, target in fn["regions"]:
            start, end = map(int, target.split("-"))
            mode = kind[len("region_"):]
            spans[(start, end, mode in ("maybe_written", "escape"))].add(fn["function"], mode)
    out: list[dict] = []

    def add(kind, name, address, access, type_name, allocator=None, unknown=False):
        total = Access()
        total.merge(access)
        total.merge(items.for_type(type_name, fields))
        weak = {f for f, strong in total.writes.items() if not strong}
        out.append(make_row(kind, name, address, evidence(total, walks), platform_reason(name, type_name, allocator),
                            weak, unknown))

    for g in globals_rows:
        add("global", g["name"], g["address"], items.access.get(("global", g["name"]), Access()), g["type"])
    static_rows: dict[str, tuple[str, str]] = {}
    for fn in functions.values():
        for name, type_name in fn["statics"]:
            static_rows.setdefault(name, (walks.addresses.get(fn["function"], fn["address"]), type_name))
    for name, type_name in static_types.items():
        if name not in static_rows and (("static", name) in items.access or not type_name.startswith("const")):
            static_rows[name] = ("", type_name)
    for kind, name in list(items.access):
        if kind == "static" and name not in static_rows:
            static_rows[name] = ("", static_types.get(name, ""))
    for name, (address, type_name) in static_rows.items():
        add("static", name, address, items.access.get(("static", name), Access()), type_name)
    for region in regions:
        add("game-region", region["name"], "0x%x" % region["start"], region_access(region, spans), region["type"],
            unknown=bool(UNKNOWN_RANGE.match(region["name"])))
    sim_types = {target.split("::")[0] for _, function, _, kind, target in rwsets_rows
                 if kind.startswith("field_") and "::" in target and function in walks.sim}
    sim_targets = {target for _, function, _, kind, target in rwsets_rows
                   if kind.startswith(("field_", "global_")) and function in walks.sim}
    sites = []
    for fn in functions.values():
        address = walks.addresses.get(fn["function"], fn["address"])
        in_logic = fn["function"] in walks.sim or address in logic_addresses
        for allocator, what, line, stored in fn["allocs"]:
            outside = fn["file"].startswith(NON_GAME_DIRS)
            reached = what in sim_types and what not in PRIMITIVES and not outside
            held = what in PRIMITIVES and stored in sim_targets and not outside
            if in_logic or reached or held:
                sites.append((fn["function"], address, fn["file"], line, allocator, what))
    count: dict[str, int] = defaultdict(int)
    ordered = sorted(set(sites), key=lambda s: (s[2], s[3], s[4], s[5], s[0]))
    for function, address, file, line, allocator, what in ordered:
        base = "%s: %s %s" % (function, allocator, what)
        count[base] += 1
        access = Access()
        access.add(function, "write")
        add("heap", base if count[base] == 1 else "%s #%d" % (base, count[base]), address, access, what, allocator)
    return sorted(out, key=row_order)


def row_order(row: dict) -> tuple:
    """The sort key of a row: kind, then address (rows without one last), then name."""
    address = int(row["address"], 16) if row["address"] else BIG
    return KINDS.index(row["kind"]), address, row["name"]


def render(rows: list[dict]) -> str:
    out = io.StringIO()
    writer = csv.writer(out, lineterminator="\n")
    writer.writerow(COLUMNS)
    writer.writerows([[row[c] for c in COLUMNS] for row in rows])
    return out.getvalue()


def read_rows(path: Path = OUT) -> list[dict]:
    """The rows of data/state_classes.csv, or none when it does not exist."""
    if not path.exists():
        return []
    with path.open(newline="") as f:
        return list(csv.DictReader(f))


def merge_kept(rows: list[dict], old: list[dict]) -> list[dict]:
    """The rows with the class and note the old file had for the same (kind, name)."""
    kept = {(r["kind"], r["name"]): r for r in old}
    for row in rows:
        before = kept.get((row["kind"], row["name"]))
        if before:
            for column in KEPT:
                row[column] = before[column]
    return rows


def read_globals(path: Path = GLOBALS) -> list[dict]:
    with path.open(newline="") as f:
        return list(csv.DictReader(f))


def read_logic_addresses(path: Path = GAME_LOGIC) -> set[str]:
    with path.open(newline="") as f:
        return {r["address"] for r in csv.DictReader(f)}


def scan_all(build_dir: Path, jobs: int) -> tuple[list[dict], list[dict], int]:
    """The scan of every file under src/, the regions of Game and the number of files compiled."""
    commands = rwsets.compile_commands(build_dir, [])
    regions = canonical_regions(commands[0][1], commands[0][2])
    with ProcessPoolExecutor(max_workers=jobs) as pool:
        scans = list(pool.map(scan, [(ROOT, layout_argv(argv), directory) for _, argv, directory in commands],
                              chunksize=4))
    return scans, regions, len(commands)


def build(scans: list[dict], regions: list[dict]) -> tuple[list[dict], list[str]]:
    """The rows and the globals no row of data/rwsets.csv names, from the scans."""
    rwsets_rows, graph = hazards.load_graph()
    globals_rows = read_globals()
    rows = rows_of(rwsets_rows, graph, globals_rows, scans, regions, read_logic_addresses())
    touched = {r["name"] for r in rows if r["kind"] == "global" and (r["writers"] or r["readers"])}
    return rows, [g["name"] for g in globals_rows if g["name"] not in touched]


def differences(have: list[dict], want: list[dict]) -> list[str]:
    """Unified diff lines of the rows with class and note left out."""
    def lines(rows):
        return [",".join(r[c] for c in COLUMNS if c not in KEPT) for r in rows]
    return list(difflib.unified_diff(lines(have), lines(want), "data/state_classes.csv", "regenerated",
                                     lineterm="", n=0))


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--check", action="store_true", help="fail if anything but class and note differs")
    parser.add_argument("--unclassified", action="store_true", help="fail if a row has no class")
    parser.add_argument("--unjoined", action="store_true", help="list the globals no row of rwsets.csv names")
    parser.add_argument("--build-dir", type=Path, default=rwsets.DEFAULT_BUILD_DIR)
    parser.add_argument("--out", type=Path, default=OUT, help="write the rows here instead of the data file")
    parser.add_argument("--jobs", type=int, default=4)
    args = parser.parse_args()
    sys.setrecursionlimit(100000)
    old = read_rows()
    if args.unclassified:
        empty = [r for r in old if not r["class"]]
        bad = [r for r in old if r["class"] and r["class"] not in CLASSES]
        for r in bad:
            print("bad class %r: %s %s" % (r["class"], r["kind"], r["name"]))
        for r in empty[:20]:
            print("no class: %s %s (proposed %s: %s)" % (r["kind"], r["name"], r["proposed"] or "-", r["rule"]))
        print("stateclasses: %d of %d rows have no class" % (len(empty), len(old)))
        sys.exit(1 if empty or bad else 0)
    started = time.time()
    scans, regions, files = scan_all(args.build_dir, args.jobs)
    rows, unjoined = build(scans, regions)
    elapsed = time.time() - started
    if args.unjoined:
        print("\n".join(unjoined))
        print("stateclasses: %d globals have no rwsets rows" % len(unjoined))
        return
    if args.check:
        diff = differences(old, rows)
        if diff:
            print("\n".join(diff[:40]))
            if len(diff) > 40:
                print("... %d more lines" % (len(diff) - 40))
            print("stateclasses: data/state_classes.csv is out of date; run `uv run tools/stateclasses.py` and "
                  "commit the result")
            sys.exit(1)
        print("stateclasses: up to date (%d files, %d rows, %.0fs)" % (files, len(rows), elapsed))
        return
    merged = merge_kept(rows, old)
    now = {(r["kind"], r["name"]) for r in merged}
    lost = [r for r in old if (r["class"] or r["note"]) and (r["kind"], r["name"]) not in now]
    args.out.write_text(render(merged))
    print("%d rows, %d files, %.0fs" % (len(merged), files, elapsed))
    if lost:
        print("warning: %d rows with a class or note no longer exist and were dropped" % len(lost))
        for r in lost[:20]:
            print("  %s %s: %s %s" % (r["kind"], r["name"], r["class"], r["note"]))


if __name__ == "__main__":
    main()
