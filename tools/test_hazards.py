"""Tests for hazards.py, on hand-written functions compiled with clang-cl.

    uv run tools/test_hazards.py
"""

import shutil
import tempfile
import unittest
from pathlib import Path

import hazards

SOURCE = """struct Unit { int hp; Unit* next; };
struct WithCtor { int a; WithCtor() { a = 0; } };
extern "C" {
void* malloc(unsigned);
void* calloc(unsigned, unsigned);
void* memset(void*, int, unsigned);
void* __stdcall GlobalAlloc(unsigned, unsigned);
unsigned long __stdcall GetTickCount();
unsigned long __stdcall CreateThread(void*, unsigned, unsigned long (__stdcall*)(void*), void*, unsigned, void*);
}
unsigned GetTicks();
int g_shared;
int g_other;
int g_calls;
void (*g_hook)(int);

int Less(Unit* a, Unit* b) { return a < b; }
int Distance(char* a, char* b) { return a - b; }
int Equal(Unit* a, Unit* b) { return a == b; }
int NullChecks(Unit* a) { return a != 0 && !(a == 0); }
int Offset(char* a, int n) { return (a + n) - a; }

int Hash(Unit* u) { return (int)u; }
int HashReinterpret(Unit* u) { return reinterpret_cast<unsigned>(u) >> 4; }
int HashLiteral() { return (int)"name"; }
int Widen(int x) { return (long)x; }
int PointerToBool(Unit* u) { return (bool)u; }

int Uninit(int c) { int x; if (c) x = 1; return x; }
int Initialised(int c) { int x = 0; if (c) x = 1; return x; }

void* Raw(unsigned n) { return malloc(n); }
void* Zeroed(unsigned n) { return calloc(1, n); }
void Stored(unsigned n) { Unit* u = (Unit*)malloc(n); u->hp = 1; }
void StoredAndCleared(unsigned n) { Unit* u = (Unit*)malloc(n); memset(u, 0, n); u->hp = 1; }
void ClearedOther(unsigned n) { Unit* u = (Unit*)malloc(n); char* v = (char*)malloc(n); memset(v, 0, n); u->hp = *v; }
void FieldStore(Unit* parent, unsigned n) { parent->next = (Unit*)malloc(n); }
void FieldCleared(Unit* parent, unsigned n) { parent->next = (Unit*)malloc(n); memset(parent->next, 0, n); }
void GlobalFlags(unsigned n) { void* a = GlobalAlloc(0, n); void* b = GlobalAlloc(0x40, n); (void)a; (void)b; }
void NewPlain() { Unit* u = new Unit; u->hp = 1; }
void NewArray(int n) { int* p = new int[n]; p[0] = 1; }
void NewInitialised() { Unit* u = new Unit(); int* p = new int(3); (void)u; (void)p; }
void NewWithConstructor() { WithCtor* w = new WithCtor; (void)w; }

unsigned Time() { return GetTickCount() + GetTicks(); }
unsigned NoTime() { return 5; }

unsigned long __stdcall Worker(void*) { g_shared = 1; return 0; }
void Start() { CreateThread(0, 0, Worker, 0, 0, 0); }
int ReadShared() { return g_shared; }

void Inline() { __asm { nop } }
void Indirect(int x) { g_hook(x); }

struct Base { virtual ~Base(); int v; };
struct Guard { ~Guard(); };
Guard MakeGuard();
void DeleteVirtual(Base* b) { delete b; }
void TemporaryGuard() { MakeGuard(); }
"""


def scanned(source):
    tmp = tempfile.TemporaryDirectory()
    root = Path(tmp.name)
    path = root / "src/a.cpp"
    path.parent.mkdir()
    path.write_text(source)
    try:
        return hazards.scan_source(path, root)
    finally:
        tmp.cleanup()


@unittest.skipUnless(shutil.which("clang-cl"), "clang-cl is not installed")
class HazardsTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.functions = scanned(SOURCE)
        cls.by_name = {fn["function"]: fn for fn in cls.functions}

    def events(self, name, kind=None):
        return [(k, t) for k, t, line in self.by_name[name]["events"] if kind is None or k == kind]

    def test_pointer_ordering_and_difference_are_hazards(self):
        self.assertEqual(self.events("Less"), [("ptr_compare", "< Unit*")])
        self.assertEqual(self.events("Distance"), [("ptr_compare", "- char*")])

    def test_equality_and_null_checks_are_not(self):
        self.assertEqual(self.events("Equal"), [])
        self.assertEqual(self.events("NullChecks"), [])

    def test_pointer_plus_integer_is_not_a_hazard_but_the_difference_is(self):
        self.assertEqual(self.events("Offset", "ptr_compare"), [("ptr_compare", "- char*")])

    def test_pointer_to_integer_in_any_spelling(self):
        self.assertEqual(self.events("Hash"), [("ptr_to_int", "Unit * -> int")])
        self.assertEqual(self.events("HashReinterpret"), [("ptr_to_int", "Unit * -> unsigned int")])
        self.assertEqual([k for k, t in self.events("HashLiteral")], ["ptr_to_int"])

    def test_integer_casts_and_pointer_to_bool_are_not(self):
        self.assertEqual(self.events("Widen"), [])
        self.assertEqual(self.events("PointerToBool"), [])

    def test_uninitialised_use(self):
        self.assertEqual([k for k, t in self.events("Uninit")], ["uninit"])
        self.assertIn("x (", self.events("Uninit")[0][1])
        self.assertEqual(self.events("Initialised"), [])

    def test_allocation_is_a_hazard_unless_cleared(self):
        self.assertEqual(self.events("Stored"), [("heap_uncleared", "malloc")])
        self.assertEqual(self.events("StoredAndCleared"), [])
        self.assertEqual(self.events("Zeroed"), [])

    def test_clearing_another_block_does_not_clear_this_one(self):
        self.assertEqual(self.events("ClearedOther"), [("heap_uncleared", "malloc")])

    def test_returned_allocation_is_a_wrapper_not_a_row(self):
        self.assertEqual(self.events("Raw"), [])

    def test_allocation_stored_in_a_field(self):
        self.assertEqual(self.events("FieldStore"), [("heap_uncleared", "malloc")])
        self.assertEqual(self.events("FieldCleared"), [])

    def test_global_alloc_flags(self):
        self.assertEqual(self.events("GlobalFlags"), [("heap_uncleared", "GlobalAlloc")])

    def test_new_without_initialiser_is_a_hazard(self):
        self.assertEqual(self.events("NewPlain"), [("heap_uncleared", "new Unit")])
        self.assertEqual(self.events("NewArray"), [("heap_uncleared", "new int[]")])

    def test_initialised_new_and_constructors_are_not(self):
        self.assertEqual(self.events("NewInitialised"), [])
        self.assertEqual(self.events("NewWithConstructor"), [])

    def test_time_reads(self):
        self.assertEqual(sorted(self.events("Time")), [("time_read", "GetTickCount"), ("time_read", "GetTicks")])
        self.assertEqual(self.events("NoTime"), [])

    def test_thread_entry_points(self):
        self.assertEqual(self.events("Start"), [("thread_entry", "Worker")])

    def test_asm_and_indirect_call(self):
        self.assertEqual(self.events("Inline"), [("asm", "inline")])
        self.assertEqual([k for k, t in self.events("Indirect")], ["indirect_call"])
        self.assertTrue(self.events("Indirect")[0][1].startswith("indirect via"))

    def test_destructor_calls_carry_the_line(self):
        self.assertEqual(self.events("DeleteVirtual", "indirect_call"), [("indirect_call", "virtual Base::~Base")])
        lines = SOURCE.splitlines()
        for name in ("DeleteVirtual", "TemporaryGuard"):
            for kind, target, line in self.by_name[name]["events"]:
                self.assertIn(name, lines[line - 1])
        self.assertTrue(self.by_name["DeleteVirtual"]["events"])

    def test_events_carry_the_line(self):
        lines = SOURCE.splitlines()
        for name, fn in self.by_name.items():
            for kind, target, line in fn["events"]:
                self.assertIn(name, lines[line - 1], (name, kind))


RWSETS = [
    ["", "Root", "src/a.cpp", "call", "Middle"],
    ["", "Middle", "src/a.cpp", "call", "Leaf"],
    ["", "Middle", "src/a.cpp", "call", "Root"],
    ["", "Leaf", "src/a.cpp", "global_read", "g_shared"],
    ["", "Leaf", "src/a.cpp", "call", "external"],
    ["", "Orphan", "src/a.cpp", "call", "Leaf"],
    ["", "Worker", "src/a.cpp", "global_write", "g_shared"],
    ["", "Worker", "src/a.cpp", "call", "Helper"],
    ["", "Helper", "src/a.cpp", "global_maybe_written", "g_other"],
]


def fn(name, events=(), address="", start=1):
    return {"address": address, "function": name, "file": "src/a.cpp", "start": start, "events": list(events)}


class WalkTest(unittest.TestCase):
    def test_reach_follows_call_rows_and_stops_at_cycles(self):
        graph = hazards.Graph(RWSETS)
        self.assertEqual(graph.reach(["Root"]), {"Root", "Middle", "Leaf", "external"})
        self.assertNotIn("Orphan", graph.reach(["Root"]))

    def test_reach_follows_hand_resolved_edges(self):
        graph = hazards.Graph(RWSETS, [("Leaf", "Orphan"), ("Orphan", "Worker")])
        self.assertEqual(graph.reach(["Root"]), {"Root", "Middle", "Leaf", "external", "Orphan", "Worker", "Helper"})
        self.assertNotIn("Worker", hazards.Graph(RWSETS).reach(["Root"]))

    def test_an_edge_makes_the_target_game_logic(self):
        functions = [fn("Root"), fn("Hooked", [("time_read", "GetTickCount", 4)])]
        graph = hazards.Graph(RWSETS + [["", "Hooked", "src/a.cpp", "function", ""]], [("Root", "Hooked")])
        rows = hazards.candidates(functions, graph, roots=["Root"], load_roots=[])
        self.assertEqual(rows, [["", "Hooked", "src/a.cpp", 4, "time_read", "GetTickCount", "tick"]])
        self.assertEqual(hazards.candidates(functions, hazards.Graph(RWSETS), roots=["Root"], load_roots=[]), [])

    def test_edges_are_read_and_a_stale_name_fails(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "e.csv"
            path.write_text("caller,target,reason\nRoot,Leaf,slot 1 (src/a.cpp:3)\n")
            self.assertEqual(hazards.read_edges(path, {"Root", "Leaf"}), [("Root", "Leaf")])
            with self.assertRaises(SystemExit):
                hazards.read_edges(path, {"Root"})
            path.write_text("caller,to,reason\n")
            with self.assertRaises(SystemExit):
                hazards.read_edges(path, {"Root"})

    def test_rows_only_for_game_logic(self):
        graph = hazards.Graph(RWSETS)
        functions = [fn("Root", [("asm", "inline", 3)]), fn("Leaf", [("time_read", "GetTickCount", 9)]),
                     fn("Orphan", [("time_read", "GetTickCount", 20)])]
        rows = hazards.candidates(functions, graph, roots=["Root"], load_roots=[])
        self.assertEqual(rows, [["", "Leaf", "src/a.cpp", 9, "time_read", "GetTickCount", "tick"],
                                ["", "Root", "src/a.cpp", 3, "asm", "inline", "tick"]])

    def test_load_roots_add_functions_the_tick_roots_do_not_reach(self):
        graph = hazards.Graph(RWSETS + [["", "Loader", "src/a.cpp", "call", "Helper"]])
        functions = [fn("Root", [("asm", "inline", 3)]), fn("Loader", [("asm", "inline", 5)]),
                     fn("Helper", [("time_read", "GetTickCount", 8)]), fn("Orphan", [("asm", "inline", 20)])]
        rows = hazards.candidates(functions, graph, roots=["Root"], load_roots=["Loader"])
        self.assertEqual([(r[1], r[6]) for r in rows], [("Helper", "load"), ("Loader", "load"), ("Root", "tick")])
        self.assertEqual(hazards.candidates(functions, graph, roots=["Root"], load_roots=[]),
                         [["", "Root", "src/a.cpp", 3, "asm", "inline", "tick"]])

    def test_a_function_both_walks_reach_is_tick(self):
        graph = hazards.Graph(RWSETS + [["", "Loader", "src/a.cpp", "call", "Leaf"]])
        functions = [fn("Leaf", [("asm", "inline", 9)]), fn("Loader")]
        rows = hazards.candidates(functions, graph, roots=["Root"], load_roots=["Loader"])
        self.assertEqual(rows, [["", "Leaf", "src/a.cpp", 9, "asm", "inline", "tick"]])

    def test_a_thread_global_read_only_by_load_code_is_load(self):
        graph = hazards.Graph(RWSETS + [["", "Loader", "src/a.cpp", "call", "Leaf"]])
        functions = [fn("Loader", [("thread_entry", "Worker", 3)]), fn("Leaf", start=7), fn("Worker")]
        rows = hazards.candidates(functions, graph, roots=["Idle"], load_roots=["Loader"])
        self.assertEqual(rows, [["", "Leaf", "src/a.cpp", 7, "thread_global", "g_shared <- Worker", "load"]])

    def test_the_default_load_roots_are_load_match_and_assign_start_positions(self):
        self.assertEqual(hazards.LOAD_ROOTS, ["LoadMatch", "AssignStartPositions"])
        self.assertTrue(set(hazards.LOAD_ROOTS).isdisjoint(hazards.ROOTS))

    def test_thread_globals_are_written_from_an_entry_point_and_read_by_game_logic(self):
        graph = hazards.Graph(RWSETS)
        functions = [fn("Root", [("thread_entry", "Worker", 3)]), fn("Leaf", start=7), fn("Worker"), fn("Helper")]
        rows = hazards.candidates(functions, graph, roots=["Root"], load_roots=[])
        self.assertEqual(rows, [["", "Leaf", "src/a.cpp", 7, "thread_global", "g_shared <- Worker", "tick"]])

    def test_a_global_only_the_main_thread_writes_is_not_a_thread_global(self):
        graph = hazards.Graph(RWSETS + [["", "Leaf", "src/a.cpp", "global_write", "g_main"],
                                        ["", "Root", "src/a.cpp", "global_read", "g_main"]])
        rows = hazards.candidates([fn("Root", [("thread_entry", "Worker", 3)]), fn("Leaf")], graph, roots=["Root"], load_roots=[])
        self.assertEqual([r[5] for r in rows if r[4] == "thread_global"], ["g_shared <- Worker"])


class WarningTest(unittest.TestCase):
    def test_both_diagnostic_formats_are_read(self):
        for text in ("/r/src/a.cpp(101,75) : warning: variable 'n' may be uninitialized when used here "
                     "[-Wconditional-uninitialized]",
                     "/r/src/a.cpp:101:75: warning: variable 'n' may be uninitialized when used here "
                     "[-Wconditional-uninitialized]"):
            m = hazards.WARNING.match(text)
            self.assertEqual((m.group(1), int(m.group(2) or m.group(3)), m.group(5)),
                             ("/r/src/a.cpp", 101, "-Wconditional-uninitialized"))
            self.assertIn(m.group(5), hazards.WARNING_FLAGS)
            self.assertEqual(hazards.VARIABLE.search(m.group(4)).group(1), "n")


    def test_other_warnings_are_not_uninit_rows(self):
        text = "/r/src/a.cpp(1,2) : warning: format specifies type 'int' [-Wformat]"
        self.assertNotIn(hazards.WARNING.match(text).group(5), hazards.WARNING_FLAGS)


class FileTest(unittest.TestCase):
    ROWS = [["0x401000", "A", "src/a.cpp", 10, "asm", "inline", "tick"],
            ["0x401000", "A", "src/a.cpp", 20, "asm", "inline", "tick"],
            ["", "B", "src/b.cpp", 5, "time_read", "time", "tick"]]

    def old(self, verdicts):
        """ROWS as read from the file: the verdict and note before the phase."""
        return [r[:6] + v + r[6:] for r, v in zip(self.ROWS, verdicts)]

    def test_rows_are_sorted_by_address_then_name_and_unaddressed_last(self):
        shuffled = [self.ROWS[2], self.ROWS[1], self.ROWS[0]]
        self.assertEqual(sorted(shuffled, key=hazards.order), self.ROWS)

    def test_verdicts_follow_the_occurrence_not_the_line(self):
        old = self.old([["real", "first"], ["false_positive", "second"], ["", ""]])
        moved = [["0x401000", "A", "src/a.cpp", 11, "asm", "inline", "tick"],
                 ["0x401000", "A", "src/a.cpp", 31, "asm", "inline", "tick"],
                 ["", "B", "src/b.cpp", 6, "time_read", "time", "tick"]]
        merged = hazards.merge_verdicts(moved, old)
        self.assertEqual([r[3:] for r in merged], [[11, "asm", "inline", "real", "first", "tick"],
                                                   [31, "asm", "inline", "false_positive", "second", "tick"],
                                                   [6, "time_read", "time", "", "", "tick"]])

    def test_verdicts_carry_over_when_the_phase_changes_or_is_missing(self):
        old = self.old([["real", "first"], ["", ""], ["", ""]])
        loaded = [r[:6] + ["load"] for r in self.ROWS]
        self.assertEqual([(r[6], r[7], r[8]) for r in hazards.merge_verdicts(loaded, old)][0], ("real", "first", "load"))
        legacy = [r[:8] for r in old]
        self.assertEqual(hazards.merge_verdicts(self.ROWS, legacy)[0][6:], ["real", "first", "tick"])

    def test_check_keys_include_the_phase_but_not_the_verdict(self):
        old = self.old([["real", "first"], ["", ""], ["", ""]])
        self.assertEqual(hazards.key_lines(old, 8), hazards.key_lines(self.ROWS, 6))
        loaded = [r[:6] + ["load"] for r in self.ROWS]
        self.assertNotEqual(hazards.key_lines(old, 8), hazards.key_lines(loaded, 6))

    def test_a_new_row_has_no_verdict(self):
        old = self.old([["real", "x"]])[:1]
        merged = hazards.merge_verdicts(self.ROWS[:2], old)
        self.assertEqual([(r[6], r[7]) for r in merged], [("real", "x"), ("", "")])

    def test_render_round_trips_through_read(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "h.csv"
            rows = self.old([["real", "has, a comma"]] * 3)
            path.write_text(hazards.render(rows))
            self.assertEqual(hazards.read_hazards(path), rows)
            self.assertEqual(rows[0][8], "tick")


if __name__ == "__main__":
    unittest.main()
