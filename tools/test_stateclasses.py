"""Tests for stateclasses.py, on hand-written layouts, functions and rows.

    uv run tools/test_stateclasses.py
"""

import shutil
import sys
import tempfile
import unittest
from pathlib import Path

import hazards
import stateclasses as sc

LAYOUT = """*** Dumping AST Record Layout
         0 | struct Game
         0 |   char[4] unknown_0
         4 |   int tick
         8 |   struct Settings settings
         8 |     int speed
        12 |     int volume
        16 |   Player[10] players
       176 |   union Game::(anonymous at a.cpp:3:5)
       176 |     int whole
       176 |     struct Game::(anonymous union)::(anonymous at a.cpp:4:9)
       176 |       int half
       180 |   unsigned short bit0
       180 |   unsigned short bit1
 180:0-0 |   unsigned short flag_a
 180:1-3 |   unsigned short flag_b
 182:0-7 |   unsigned char flag_c
       184 |   char * name
           | [sizeof=188, align=1,
           |  nvsize=188, nvalign=1]
"""

SOURCE = """struct Settings { int speed; int volume; };
struct Unit { int hp; Settings settings; Unit* next; int pad[2]; };
struct Game { char unknown_0[4]; int tick; int more; char blob[100]; char small[4]; };
extern "C" { void* malloc(unsigned); void* calloc(unsigned, unsigned); }
void* operator new(unsigned, void* where) { return where; }
void* GameAllocIgnoreTag(char* name, unsigned int size);
void* GameAlloc(unsigned n) { return malloc(n); }
struct SimHeap {};
extern const SimHeap kSim;
void* operator new(unsigned, const SimHeap&);
void* operator new[](unsigned, const SimHeap&);
void* SimAlloc(unsigned n);
void* SimAllocTagged(const char* tag, unsigned n);
void* SimRealloc(void* p, unsigned n);
extern Game* g_game;
int counter;

int ReadTick() { return g_game->tick; }
void BumpMore() { g_game->more++; }
int Count() { static int calls; calls++; static const int kLimit = 3; return calls < kLimit; }
void Plain() { Unit* u = new Unit; Unit* v = new Unit[4]; (void)u; (void)v; }
void Sized() { void* p = GameAllocIgnoreTag("SOME TAG", sizeof(Unit) * 3); (void)p; }
void Cast() { Unit* u = (Unit*)GameAllocIgnoreTag("CAST TAG", 16); (void)u; }
void Tag() { char* b = (char*)malloc(32); (void)b; }
char* Wrapper(unsigned n) { return (char*)malloc(n); }
void Placement(void* where) { Unit* u = new (where) Unit; (void)u; }
void SimNew() { Unit* u = new (kSim) Unit; Unit* v = new (kSim) Unit[4]; (void)u; (void)v; }
void SimCalls(void* old) { Unit* u = (Unit*)SimAlloc(16); void* t = SimAllocTagged("SIM TAG", 8);
                           void* r = SimRealloc(old, 32); (void)u; (void)t; (void)r; }
void Zeroed() { void* z = calloc(1, 8); (void)z; }
void LiteralIndex() { g_game->blob[3] = 1; }
int LiteralRead() { return g_game->blob[5]; }
void VariableIndex(int i) { g_game->blob[i] = 1; }
int SmallVariable(int i) { return g_game->small[i]; }
void WholeArray() { char* p = g_game->blob; (void)p; }
struct Pool { char* buffer; };
Pool g_pool;
char* g_held;
void StoreField(Pool* p) { p->buffer = (char*)malloc(32); }
void StoreGlobal() { g_held = (char*)malloc(32); }
void StoreStatic() { static char* s; s = (char*)malloc(32); }
void StoreLocal() { char* b; b = (char*)malloc(32); (void)b; }
char g_end[4];
long Bound() { char* p = 0; return (long)p < (long)g_end; }
"""


def functions(source):
    tmp = tempfile.TemporaryDirectory()
    root = Path(tmp.name)
    path = root / "src/a.cpp"
    path.parent.mkdir()
    path.write_text(source)
    try:
        return sc.scan_source(path, root)
    finally:
        tmp.cleanup()


class LayoutTest(unittest.TestCase):
    def setUp(self):
        self.size, self.fields = sc.layout_fields(LAYOUT, "Game")
        self.by_name = {f.name: f for f in self.fields}

    def test_size_and_top_level_members(self):
        self.assertEqual(self.size, 188)
        self.assertEqual([f.name for f in self.fields],
                         ["unknown_0", "tick", "settings", "players", "whole", "half", "bit0", "bit1", "flag_a",
                          "flag_b", "flag_c", "name"])

    def test_members_of_a_nested_struct_are_not_top_level(self):
        self.assertNotIn("speed", self.by_name)

    def test_members_of_anonymous_unions_are_members_of_the_record(self):
        self.assertEqual((self.by_name["whole"].start, self.by_name["half"].start), (176, 176))
        self.assertEqual(self.by_name["whole"].end, 180)

    def test_a_member_ends_where_the_next_greater_start_begins(self):
        self.assertEqual((self.by_name["settings"].start, self.by_name["settings"].end), (8, 16))
        self.assertEqual(self.by_name["players"].end, 176)
        self.assertEqual(self.by_name["name"].end, 188)

    def test_types_lose_their_keyword(self):
        self.assertEqual(self.by_name["settings"].type, "Settings")
        self.assertEqual(self.by_name["players"].type, "Player[10]")
        self.assertEqual(self.by_name["name"].type, "char *")

    def test_a_record_that_is_not_in_the_dump_is_none(self):
        self.assertIsNone(sc.layout_fields(LAYOUT, "Other"))

    def test_bit_fields_of_one_unit_are_one_region(self):
        names = [r["name"] for r in sc.game_regions(self.fields)]
        self.assertIn("flag_a|flag_b", names)
        self.assertIn("flag_c", names)
        self.assertEqual(names.count("players"), 1)

    def test_a_view_maps_each_member_name_to_its_range(self):
        self.assertEqual(sc.view_spans((self.size, self.fields))["tick"], (4, 8))
        self.assertEqual(sc.view_spans(None), {})


@unittest.skipUnless(shutil.which("clang-cl"), "clang-cl is not installed")
class ScanTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.scan = functions(SOURCE)
        cls.by_name = {fn["function"]: fn for fn in cls.scan["functions"]}

    def test_a_game_access_is_the_byte_range_of_the_member_in_the_files_own_view(self):
        self.assertEqual(self.by_name["ReadTick"]["regions"], [("region_read", "4-8")])
        self.assertEqual(sorted(self.by_name["BumpMore"]["regions"]),
                         [("region_read", "8-12"), ("region_write", "8-12")])

    def test_a_literal_index_is_one_element_and_any_other_index_the_start_of_the_array(self):
        self.assertEqual(self.by_name["LiteralIndex"]["regions"], [("region_write", "15-16")])
        self.assertEqual(self.by_name["LiteralRead"]["regions"], [("region_read", "17-18")])
        self.assertEqual(self.by_name["VariableIndex"]["regions"], [("region_write", "12-13")])
        self.assertEqual(self.by_name["SmallVariable"]["regions"], [("region_read", "112-116")])
        self.assertEqual(self.by_name["WholeArray"]["regions"], [("region_escape", "12-112")])

    def test_a_global_compared_as_an_address_is_a_bound(self):
        self.assertEqual(self.by_name["Bound"]["bounds"], ["g_end"])

    def test_function_statics_are_named_with_their_function(self):
        self.assertEqual(self.by_name["Count"]["statics"], [("kLimit@Count", "const int"), ("calls@Count", "int")])

    def test_record_fields_are_by_value_names_or_empty(self):
        self.assertEqual(self.scan["fields"]["Unit"], {"hp": "int", "settings": "Settings", "next": "",
                                                       "pad": "int"})

    def test_new_and_new_array_sites(self):
        self.assertEqual([(a, w) for a, w, *_ in self.by_name["Plain"]["allocs"]], [("new", "Unit"), ("new[]", "Unit")])

    def test_the_type_is_the_sizeof_operand_or_the_cast_type_or_the_tag(self):
        self.assertEqual([(a, w) for a, w, *_ in self.by_name["Sized"]["allocs"]],
                         [("GameAllocIgnoreTag", "Unit")])
        self.assertEqual([(a, w) for a, w, *_ in self.by_name["Cast"]["allocs"]],
                         [("GameAllocIgnoreTag", "Unit")])
        self.assertEqual([(a, w) for a, w, *_ in self.by_name["Tag"]["allocs"]], [("malloc", "char")])

    def test_the_target_an_allocation_is_assigned_to_is_a_field_or_a_global_and_not_a_local(self):
        stored = {name: self.by_name[name]["allocs"][0][3]
                  for name in ("StoreField", "StoreGlobal", "StoreStatic", "StoreLocal", "Tag")}
        self.assertEqual(stored, {"StoreField": "Pool::buffer", "StoreGlobal": "g_held", "StoreStatic": "s@StoreStatic",
                                  "StoreLocal": "", "Tag": ""})

    def test_a_wrapper_placement_new_and_the_allocator_itself_are_not_sites(self):
        for name in ("Wrapper", "Placement", "GameAlloc"):
            self.assertEqual(self.by_name[name]["allocs"], [], name)

    def test_sim_block_forms_are_sites_named_as_the_forms_they_replace(self):
        self.assertEqual([(a, w) for a, w, *_ in self.by_name["SimNew"]["allocs"]],
                         [("new", "Unit"), ("new[]", "Unit")])
        self.assertEqual([(a, w) for a, w, *_ in self.by_name["SimCalls"]["allocs"]],
                         [("operator new", "Unit"), ("GameAllocIgnoreTag", "SIM TAG"), ("realloc", "void")])

    def test_calloc_is_a_site(self):
        self.assertEqual([a for a, w, *_ in self.by_name["Zeroed"]["allocs"]], ["calloc"])


class NoSyntaxTreeTest(unittest.TestCase):
    def test_a_run_that_prints_no_syntax_tree_exits_with_clangs_stderr(self):
        argv = [sys.executable, "-c", "import sys; print('layouts'); print('oops', file=sys.stderr)"]
        with self.assertRaises(SystemExit) as caught:
            sc.scan((Path("."), argv, "."))
        self.assertIn("oops", str(caught.exception))


class OwnGameTest(unittest.TestCase):
    def test_a_file_whose_game_is_not_the_type_of_g_game_has_no_regions(self):
        font = functions("struct Game { int tick; };\nGame* GetDisplay();\nint F() { return GetDisplay()->tick; }\n")
        self.assertEqual(font["functions"][0]["regions"], [])
        game = functions("struct Game { int tick; };\nextern Game* g_game;\nint F() { return g_game->tick; }\n")
        self.assertEqual(game["functions"][0]["regions"], [("region_read", "0-4")])


def ev(**kw):
    values = dict(phases_write=set(), phases_read=set(), load_write=False, pres_write=False, pres_read=False,
                  other_write=False, other_read=False, strong_write=True, has_write=False, files=set(),
                  writers=[], readers=[])
    values.update(kw)
    values["has_write"] = values["has_write"] or bool(values["phases_write"] or values["load_write"]
                                                      or values["pres_write"] or values["other_write"])
    return sc.Evidence(**values)


class RuleTest(unittest.TestCase):
    def walked(self):
        rows = ModelTest().rows()
        return sc.Walks(rows, hazards.Graph(rows)), rows

    def proposed(self, platform=None, **kw):
        return sc.classify(ev(**kw), platform)[0]

    def test_written_by_a_phase_is_sim(self):
        self.assertEqual(self.proposed(phases_write={"1"}), "sim")
        self.assertEqual(self.proposed(phases_write={"1"}, pres_read=True), "sim")

    def test_written_by_a_phase_and_by_presentation_is_mixed(self):
        self.assertEqual(self.proposed(phases_write={"1"}, pres_write=True), "mixed")

    def test_presentation_writing_what_a_phase_reads_is_mixed(self):
        self.assertEqual(self.proposed(pres_write=True, phases_read={"2"}), "mixed")

    def test_touched_only_by_presentation_is_view(self):
        self.assertEqual(self.proposed(pres_write=True), "view")
        self.assertEqual(self.proposed(pres_read=True), "view")
        self.assertEqual(self.proposed(pres_write=True, pres_read=True), "view")

    def test_load_writes_what_a_phase_reads_is_sim(self):
        self.assertEqual(self.proposed(load_write=True, phases_read={"1"}), "sim")
        self.assertEqual(self.proposed(load_write=True, phases_read={"1"}, pres_write=True), "mixed")

    def test_load_writes_what_only_presentation_uses_is_view(self):
        self.assertEqual(self.proposed(load_write=True, pres_read=True), "view")

    def test_load_alone_is_sim(self):
        self.assertEqual(self.proposed(load_write=True), "sim")

    def test_a_table_a_phase_reads_and_nothing_writes_is_sim(self):
        self.assertEqual(self.proposed(phases_read={"1"}), "sim")

    def test_platform_by_type_name_or_allocator_beats_the_phases(self):
        self.assertEqual(self.proposed(platform="type HWND", phases_write={"1"}), "platform")

    def test_only_debug_or_harness_files_is_platform(self):
        self.assertEqual(self.proposed(phases_write={"1"}, files={"src/debug/debug_lib.cpp"}), "platform")
        self.assertEqual(self.proposed(phases_write={"1"}, files={"src/debug/a.cpp", "src/units/b.cpp"}), "sim")

    def test_nothing_or_only_unreached_code_is_unclassified(self):
        self.assertEqual(sc.classify(ev())[0], "")
        self.assertEqual(sc.classify(ev(other_write=True))[0], "")
        self.assertIn("outside every walk", sc.classify(ev(other_read=True))[1])

    def test_a_weak_presentation_write_is_not_a_presentation_write(self):
        walks, rows = self.walked()
        access = sc.Access()
        access.add("Draw", "maybe_written")
        access.add("Helper", "write")
        fact = sc.evidence(access, walks)
        self.assertEqual((fact.pres_write, fact.pres_read, fact.phases_write), (False, True, {"1"}))
        self.assertEqual(sc.classify(fact)[0], "sim")

    def test_a_weak_write_is_said_in_the_rule(self):
        self.assertIn("weak", sc.classify(ev(phases_write={"1"}, strong_write=False))[1])
        self.assertNotIn("weak", sc.classify(ev(phases_write={"1"}))[1])

    def test_the_rule_names_the_rule_that_fired(self):
        self.assertTrue(sc.classify(ev(phases_write={"1"}))[1].startswith("sim:"))
        self.assertTrue(sc.classify(ev(pres_read=True))[1].startswith("view:"))

    def test_platform_reasons(self):
        self.assertEqual(sc.platform_reason("g_x", "HWND"), "type HWND")
        self.assertEqual(sc.platform_reason("lock@GetAllocLock", "CritSec_004da780"), "type CritSec_004da780")
        self.assertEqual(sc.platform_reason("g_windowClassName", "int"), "name matches window_class")
        self.assertEqual(sc.platform_reason("IID_IDirectDraw", ""), "name matches iid")
        self.assertEqual(sc.platform_reason("Foo: VirtualAlloc ?", "?", "VirtualAlloc"), "allocator VirtualAlloc")
        self.assertIsNone(sc.platform_reason("g_unitCount", "int"))
        self.assertIsNone(sc.platform_reason("g_cdPlayer", "int"))
        self.assertIsNone(sc.platform_reason("g_modemCount", "int"))
        for ui in ("g_directXCheckDone", "g_cdPlayerDialogShown", "g_helpDialogBaseGadgetCount",
                   "g_shareDialogPlayerNetIds", "g_lzssPresetWindow"):
            self.assertIsNone(sc.platform_reason(ui, "int"), ui)
        self.assertEqual(sc.platform_reason("g_pfnStackWalk", "int"), "name matches pfn")
        self.assertEqual(sc.platform_reason("g_noDirectSound", "int"), "name matches direct_sound")


def function_rows(*names):
    return [["", name, "src/%s%s.cpp" % ("gui/" if name == "Draw" else "", name), "function", ""] for name in names]


class ModelTest(unittest.TestCase):
    GLOBALS = [{"name": "g_count", "address": "0x500000", "type": "int"},
               {"name": "DAT_00500010", "address": "0x500010", "type": "int"},
               {"name": "Tmpl::_Nil", "address": "0x500020", "type": "int"},
               {"name": "g_hwnd", "address": "0x500030", "type": "int"},
               {"name": "g_untouched", "address": "0x500040", "type": "int"}]

    def rows(self, extra=()):
        roots = list(sc.PHASE_LABELS) + [sc.DRIVER] + sc.LOAD_ROOTS + sc.PRESENTATION_ROOTS
        rows = function_rows(*roots, "Helper", "Draw", "Init")
        rows += [["", "UpdateAllUnits", "src/UpdateAllUnits.cpp", "call", "Helper"],
                 ["", "Helper", "src/Helper.cpp", "global_write", "g_count"],
                 ["", "Helper", "src/Helper.cpp", "global_read", "DAT_00500010"],
                 ["", "Draw", "src/gui/Draw.cpp", "global_write", "s_view@src/draw.cpp"],
                 ["", "Draw", "src/gui/Draw.cpp", "global_read", "g_count"],
                 ["", "UpdateAllUnits", "src/UpdateAllUnits.cpp", "global_write", "DAT_00500020"],
                 ["", "UpdateAllUnits", "src/UpdateAllUnits.cpp", "global_write", "g_hwnd"],
                 ["", "UpdateAllUnits", "src/UpdateAllUnits.cpp", "field_write", "Unit::hp"],
                 ["", "Draw", "src/gui/Draw.cpp", "field_write", "Unit::selected"],
                 ["", "UpdateAllUnits", "src/UpdateAllUnits.cpp", "field_write", "Game::tick"],
                 ["", "LoadMatch", "src/LoadMatch.cpp", "call", "Init"]]
        rows += [["0x401000", "Init", "src/Init.cpp", "function", ""]]
        rows.append(["", "Init", "src/Init.cpp", "global_write", "g_count"])
        return rows + list(extra)

    def build(self, scans=None, regions=(), extra_rows=()):
        rows = self.rows(extra_rows)
        graph = hazards.Graph(rows)
        scans = scans if scans is not None else [{"functions": [], "fields": {}, "static_types": {}}]
        return sc.rows_of(rows, graph, self.GLOBALS, scans, list(regions), set())

    def by(self, rows, kind, name):
        return next(r for r in rows if r["kind"] == kind and r["name"] == name)

    def test_one_global_row_for_every_row_of_globals_csv(self):
        names = [r["name"] for r in self.build() if r["kind"] == "global"]
        self.assertEqual(sorted(names), sorted(g["name"] for g in self.GLOBALS))

    def test_a_dat_target_joins_the_global_with_that_address(self):
        row = self.by(self.build(), "global", "Tmpl::_Nil")
        self.assertEqual(row["phases_write"], "1")
        self.assertEqual(row["proposed"], "sim")

    def test_a_global_a_phase_and_presentation_touch(self):
        row = self.by(self.build(), "global", "g_count")
        self.assertEqual((row["phases_write"], row["presentation"], row["load_write"]), ("1", "r", "yes"))
        self.assertEqual(row["proposed"], "sim")
        self.assertEqual(row["writers"], "2:Helper|Init")

    def test_a_platform_name_is_platform(self):
        self.assertEqual(self.by(self.build(), "global", "g_hwnd")["proposed"], "platform")

    def test_a_global_nothing_names_has_no_proposal(self):
        row = self.by(self.build(), "global", "g_untouched")
        self.assertEqual((row["proposed"], row["rule"]), ("", "none: no access found"))

    def test_a_target_that_joins_nothing_is_a_static_row(self):
        row = self.by(self.build(), "static", "s_view@src/draw.cpp")
        self.assertEqual((row["proposed"], row["presentation"]), ("view", "w"))

    def test_a_game_region_takes_the_accesses_that_overlap_it(self):
        scan = {"functions": [{"function": "UpdateAllUnits", "file": "src/a.cpp", "start": 1, "address": "",
                               "regions": [("region_write", "4-8")], "statics": [], "allocs": []}],
                "fields": {}, "static_types": {}}
        regions = [{"name": "tick", "type": "int", "start": 4, "end": 8, "bits": False},
                   {"name": "unknown_8", "type": "char[8]", "start": 8, "end": 16, "bits": False}]
        rows = self.build([scan], regions)
        self.assertEqual(self.by(rows, "game-region", "tick")["phases_write"], "1")
        unknown = self.by(rows, "game-region", "unknown_8")
        self.assertEqual((unknown["proposed"], unknown["rule"]), ("", "none: unknown range, no access found"))

    def test_a_region_of_structs_takes_the_fields_of_the_type_and_of_the_structs_in_it(self):
        scan = {"functions": [], "static_types": {},
                "fields": {"Unit": {"hp": "int", "selected": "int", "settings": "Settings"},
                           "Settings": {"speed": "int"}}}
        regions = [{"name": "units", "type": "Unit[10]", "start": 0, "end": 40, "bits": False},
                   {"name": "next", "type": "Unit *", "start": 40, "end": 44, "bits": False}]
        rows = self.build([scan], regions)
        units = self.by(rows, "game-region", "units")
        self.assertEqual((units["phases_write"], units["presentation"], units["proposed"]), ("1", "w", "mixed"))
        self.assertEqual(self.by(rows, "game-region", "next")["proposed"], "")

    def test_a_heap_site_in_game_logic_is_a_row_and_one_outside_is_not(self):
        sites = [{"function": "Init", "file": "src/Init.cpp", "start": 1, "address": "0x401000", "regions": [],
                  "statics": [], "allocs": [("new", "Unit", 3, ""), ("new", "Unit", 4, ""), ("malloc", "char", 5, "")]},
                 {"function": "Draw", "file": "src/Draw.cpp", "start": 1, "address": "", "regions": [],
                  "statics": [], "allocs": [("new", "Sprite", 3, "")]}]
        scan = {"functions": sites, "fields": {"Unit": {"hp": "int", "selected": "int"}}, "static_types": {}}
        rows = [r for r in self.build([scan]) if r["kind"] == "heap"]
        self.assertEqual([r["name"] for r in rows], ["Init: malloc char", "Init: new Unit", "Init: new Unit #2"])
        self.assertEqual(rows[1]["address"], "0x401000")
        self.assertEqual((rows[1]["load_write"], rows[1]["presentation"], rows[1]["proposed"]), ("yes", "w", "mixed"))
        self.assertEqual(rows[0]["proposed"], "sim")

    def test_a_heap_site_of_a_type_the_walk_uses_is_a_row_wherever_it_is(self):
        def site(function, file, what):
            return {"function": function, "file": file, "start": 1, "address": "", "regions": [], "statics": [],
                    "allocs": [("new", what, 3, "")]}
        scan = {"functions": [site("Draw", "src/Draw.cpp", "Unit"), site("Draw", "src/Draw.cpp", "Sprite"),
                              site("Draw", "src/debug/Draw.cpp", "Unit")],
                "fields": {}, "static_types": {}}
        rows = [r["name"] for r in self.build([scan]) if r["kind"] == "heap"]
        self.assertEqual(rows, ["Draw: new Unit"])

    def test_a_raw_buffer_stored_where_the_walk_looks_is_a_row_and_a_local_one_is_not(self):
        def site(function, file, stored):
            return {"function": function, "file": file, "start": 1, "address": "", "regions": [], "statics": [],
                    "allocs": [("operator new", "char", 3, stored)]}
        scan = {"functions": [site("Draw", "src/Draw.cpp", "Packets::buffer"), site("Show", "src/Show.cpp", ""),
                              site("Flip", "src/Flip.cpp", "Packets::other"),
                              site("Trace", "src/debug/Trace.cpp", "Packets::buffer"),
                              site("Hold", "src/Hold.cpp", "g_held")],
                "fields": {}, "static_types": {}}
        extra = [["", "UpdateAllUnits", "u.cpp", "field_read", "Packets::buffer"],
                 ["", "UpdateAllUnits", "u.cpp", "global_write", "g_held"]]
        rows = [r["name"] for r in self.build([scan], extra_rows=extra) if r["kind"] == "heap"]
        self.assertEqual(rows, ["Draw: operator new char", "Hold: operator new char"])

    def test_stored_target_names_a_field_or_a_global_and_not_a_local(self):
        def assign(lhs):
            call = {"kind": "CallExpr", "inner": []}
            cast = {"kind": "CStyleCastExpr", "type": {"qualType": "char *"}, "inner": [call]}
            return call, [{"kind": "BinaryOperator", "opcode": "=", "inner": [lhs, cast]}, cast]
        member = {"kind": "MemberExpr", "name": "buffer", "inner": [{"kind": "DeclRefExpr", "type": {"qualType": "Packets"}}]}
        ref = lambda i: {"kind": "DeclRefExpr", "referencedDecl": {"kind": "VarDecl", "id": i, "name": "v" + i}}
        call, parents = assign(member)
        self.assertEqual(sc.stored_target(call, parents, {}), "Packets::buffer")
        call, parents = assign(ref("1"))
        self.assertEqual(sc.stored_target(call, parents, {}), "v1")
        self.assertEqual(sc.stored_target(call, parents, {"1": ""}), "")

    def test_the_null_address_is_no_static_row(self):
        items = sc.Items([["", "F", "f.cpp", "global_read", "DAT_00000000"]], self.GLOBALS)
        self.assertIsNone(items.join("DAT_00000000"))
        self.assertEqual(dict(items.access), {})

    def test_a_global_compared_as_a_bound_is_not_an_escape(self):
        rows = [["", "F", "f.cpp", "global_escape", "g_count"], ["", "G", "g.cpp", "global_escape", "g_count"]]
        items = sc.Items(rows, self.GLOBALS, {("F", "g_count")})
        self.assertEqual(items.access[("global", "g_count")].reads, {"G"})

    def test_a_global_inside_another_has_only_the_accesses_that_name_it(self):
        globals_rows = self.GLOBALS + [{"name": "g_inner", "address": "0x500004", "type": "int", "defined": "in g_count+0x4"}]
        rows = self.rows() + [["", "UpdateAllUnits", "src/UpdateAllUnits.cpp", "global_write", "g_count"]]
        built = sc.rows_of(rows, hazards.Graph(rows), globals_rows, [{"functions": [], "fields": {},
                                                                      "static_types": {}}], [], set())
        inner = self.by(built, "global", "g_inner")
        self.assertEqual((inner["writers"], inner["readers"], inner["proposed"]), ("", "", ""))

    def test_a_function_static_is_a_row_with_its_function_address(self):
        scan = {"functions": [{"function": "Init", "file": "src/Init.cpp", "start": 1, "address": "0x401000",
                               "regions": [], "statics": [("n@Init", "int")], "allocs": []}],
                "fields": {}, "static_types": {}}
        row = self.by(self.build([scan]), "static", "n@Init")
        self.assertEqual(row["address"], "0x401000")

    def test_rows_are_sorted_by_kind_then_address_then_name(self):
        rows = self.build()
        keys = [sc.row_order(r) for r in rows]
        self.assertEqual(keys, sorted(keys))
        self.assertEqual([r["kind"] for r in rows][0], "global")

    def test_the_phase_walk_stops_at_presentation_code_and_the_presentation_walk_at_a_phase_root(self):
        rows = self.rows() + [["", "Helper", "src/Helper.cpp", "call", "Draw"],
                              ["", "Draw", "src/gui/Draw.cpp", "call", "Helper"],
                              ["", "Draw", "src/gui/Draw.cpp", "call", "UpdateAllUnits"],
                              ["", "Draw", "src/gui/Draw.cpp", "call", "Deeper"],
                              ["", "Deeper", "src/Deeper.cpp", "function", ""]]
        rows += [["", "Draw", "src/gui/Draw.cpp", "call", "Late"], ["", "Late", "src/Late.cpp", "function", ""],
                 ["", "UpdateAllUnits", "src/UpdateAllUnits.cpp", "call", "Late"]]
        walks = sc.Walks(rows, hazards.Graph(rows))
        self.assertEqual(walks.phases["Helper"], {"1"})
        self.assertNotIn("Draw", walks.phases)
        self.assertNotIn("UpdateAllUnits", walks.presentation)
        self.assertEqual(walks.phases["Late"], {"1"})
        self.assertLessEqual({"Draw", "Helper", "Deeper", "Late"}, walks.presentation)

    def test_step_gaf_sequence_is_sim_although_it_lives_under_graphics(self):
        rows = self.rows() + [["", "StepAllGafSequences", "src/network/net_stats.cpp", "call", "StepGafSequence"],
                              ["", "StepGafSequence", "src/graphics/gaf.cpp", "function", ""]]
        walks = sc.Walks(rows, hazards.Graph(rows))
        self.assertEqual(walks.phases["StepGafSequence"], {"6"})
        self.assertNotIn("StepGafSequence", walks.presentation)

    def test_a_weak_access_names_the_start_of_the_range_a_strong_one_the_whole(self):
        spans = {(4, 100, False): sc.Access(reads={"Read"}), (4, 100, True): sc.Access(reads={"Escape"})}
        near = {"start": 4, "end": 8}
        far = {"start": 8, "end": 12}
        self.assertEqual(sc.region_access(near, spans).reads, {"Read", "Escape"})
        self.assertEqual(sc.region_access(far, spans).reads, {"Read"})

    def test_a_missing_phase_root_fails(self):
        with self.assertRaises(SystemExit):
            sc.Walks(function_rows("UpdateAllUnits"), hazards.Graph(function_rows("UpdateAllUnits")))


class RegenerationTest(unittest.TestCase):
    def row(self, name, **kw):
        row = {c: "" for c in sc.COLUMNS}
        row.update(name=name, kind="global", rule="sim: x")
        row.update(kw)
        return row

    def test_class_and_note_are_kept_by_kind_and_name(self):
        old = [self.row("a", **{"class": "view", "note": "why"}), self.row("b", **{"class": "sim"})]
        new = [self.row("a", proposed="sim"), self.row("c")]
        merged = sc.merge_kept(new, old)
        self.assertEqual([(r["name"], r["class"], r["note"], r["proposed"]) for r in merged],
                         [("a", "view", "why", "sim"), ("c", "", "", "")])

    def test_differences_ignore_class_and_note_only(self):
        a, b = self.row("a"), self.row("a", **{"class": "sim", "note": "n"})
        self.assertEqual(sc.differences([a], [b]), [])
        self.assertNotEqual(sc.differences([a], [self.row("a", proposed="sim")]), [])

    def test_render_round_trips_through_read(self):
        rows = [self.row("a, b", **{"note": 'has "quotes", and a comma'})]
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "s.csv"
            path.write_text(sc.render(rows))
            self.assertEqual(sc.read_rows(path), rows)


class DataTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.rows = sc.read_rows()

    def test_every_global_of_globals_csv_has_a_row_with_its_address(self):
        have = {(r["name"], r["address"]) for r in self.rows if r["kind"] == "global"}
        missing = [g["name"] for g in sc.read_globals() if (g["name"], g["address"]) not in have]
        self.assertEqual(missing, [], "globals missing from data/state_classes.csv: run tools/stateclasses.py")

    def test_rows_have_valid_kinds_proposals_and_classes(self):
        for r in self.rows:
            self.assertIn(r["kind"], sc.KINDS, r["name"])
            self.assertIn(r["proposed"], ("",) + sc.CLASSES, r["name"])
            self.assertIn(r["class"], ("",) + sc.CLASSES, r["name"])

    def test_rows_are_sorted_and_names_unique_per_kind(self):
        keys = [sc.row_order(r) for r in self.rows]
        self.assertEqual(keys, sorted(keys))
        names = [(r["kind"], r["name"]) for r in self.rows]
        self.assertEqual(len(names), len(set(names)))

    def test_the_file_has_every_kind(self):
        self.assertEqual({r["kind"] for r in self.rows}, set(sc.KINDS))


if __name__ == "__main__":
    unittest.main()
