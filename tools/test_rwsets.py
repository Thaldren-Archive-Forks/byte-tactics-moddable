"""Tests for rwsets.py, on hand-written functions compiled with clang-cl.

    uv run tools/test_rwsets.py
"""

import shutil
import tempfile
import unittest
from pathlib import Path
from unittest import mock

import rwsets

SOURCE = """struct Game { int tick; int wind; short pad; int values[4]; };
struct Unit { int hp; Game* game; };
extern Game* g_game;
extern int g_count;
extern int g_other;
typedef int (*Handler)(int);
extern Handler g_handlers[4];
struct Table { Handler fire; };
struct Counter { static int count; int n; int Get() const { return n; } void Bump() { n++; } };
extern Counter g_counter;
template <class T> struct Vec { T* b; T* begin() { return b; } const T* begin() const { return b; } };
struct Holder { Vec<int> v; };
static int s_local;
int Helper(int);
void Fill(int* out);
void Look(const int* in);

// FUNCTION: 0x401000
int ReadGlobal() { return g_count; }

// FUNCTION: 0x401010
void WriteGlobal() { g_count = 1; }

// FUNCTION: 0x401020
void Compound() { g_count += 1; }

void Increment() { g_other++; }

// FUNCTION: 0x401030
void FieldThroughPointer(Unit* u) { u->hp = u->game->tick; g_game->values[2] = 3; }

// FUNCTION: 0x401040
int Indirect(Table* t, int x) { return g_handlers[x](x) + t->fire(x); }

// FUNCTION: 0x401050
void Calls() { int local = 0; Fill(&local); Look(&local); Fill(&g_count); Look(&g_other); Helper(g_count); }

void ConstCall() { g_counter.Get(); }

void NonConstCall() { g_counter.Bump(); }

int ConstObject(const Holder* h) { return *h->v.begin(); }

int MutableObject(Holder* h) { return *h->v.begin(); }

int ClassStatic() { return Counter::count; }

int FileStatic() { return s_local; }

void Empty() {}

// FUNCTION: 0x401060
void Escapes() { int* p = &g_count; Handler h = Helper; *p = 1; (void)h; }

struct Plain { ~Plain(); int a; };
struct Virt { virtual ~Virt(); };
struct Sub : Virt { int b; };
struct Trivial { int a; };
typedef Plain PlainT;
Plain MakePlain();
void UsePlain(const Plain&);

void DeleteClass(Plain* p, PlainT* q) { delete p; delete q; }

void DeleteVirtual(Virt* v) { delete v; }

void DeleteInherited(Sub* s) { delete s; }

void DeleteArray(Plain* p) { delete[] p; }

void Temporary() { UsePlain(MakePlain()); }

void LocalAtScopeExit() { Plain x; Plain many[2]; Plain* ref = &x; Plain& alias = x; (void)ref; (void)alias; }

void TrivialType(Trivial* t) { Trivial local; delete t; (void)local; }
"""


def rows_of(source):
    tmp = tempfile.TemporaryDirectory()
    root = Path(tmp.name)
    path = root / "src/a.cpp"
    path.parent.mkdir()
    path.write_text(source)
    try:
        rows = rwsets.analyse_source(path, root)
    finally:
        tmp.cleanup()
    result = {}
    for address, function, file, kind, target in rows:
        result.setdefault(function, set())
        if kind != "function":
            result[function].add((kind, target))
    return rows, result


@unittest.skipUnless(shutil.which("clang-cl"), "clang-cl is not installed")
class RwsetsTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.rows, cls.by_function = rows_of(SOURCE)

    def test_read(self):
        self.assertEqual(self.by_function["ReadGlobal"], {("global_read", "g_count")})

    def test_write(self):
        self.assertEqual(self.by_function["WriteGlobal"], {("global_write", "g_count")})

    def test_compound_assignment_reads_and_writes(self):
        self.assertEqual(self.by_function["Compound"], {("global_read", "g_count"), ("global_write", "g_count")})
        self.assertEqual(self.by_function["Increment"], {("global_read", "g_other"), ("global_write", "g_other")})

    def test_field_through_pointer(self):
        self.assertEqual(self.by_function["FieldThroughPointer"], {
            ("field_write", "Unit::hp"), ("field_read", "Unit::game"), ("field_read", "Game::tick"),
            ("global_read", "g_game"), ("field_write", "Game::values")})

    def test_indirect_call(self):
        self.assertEqual(self.by_function["Indirect"], {
            ("indirect_call", "indirect via int (*)(int) [g_handlers]"),
            ("indirect_call", "indirect via int (*)(int) [Table::fire]"),
            ("global_read", "g_handlers"), ("field_read", "Table::fire")})

    def test_call_arguments(self):
        self.assertEqual(self.by_function["Calls"], {
            ("call", "Fill"), ("call", "Look"), ("call", "Helper"),
            ("global_maybe_written", "g_count"), ("global_read", "g_other"), ("global_read", "g_count")})

    def test_const_method_on_a_global_reads_it(self):
        self.assertEqual(self.by_function["ConstCall"], {("call", "Counter::Get"), ("global_read", "g_counter")})
        self.assertEqual(self.by_function["NonConstCall"], {
            ("call", "Counter::Bump"), ("global_maybe_written", "g_counter")})

    def test_method_call_on_a_const_object_reads_it(self):
        self.assertIn(("field_read", "Holder::v"), self.by_function["ConstObject"])
        self.assertNotIn(("field_maybe_written", "Holder::v"), self.by_function["ConstObject"])
        self.assertIn(("field_maybe_written", "Holder::v"), self.by_function["MutableObject"])

    def test_class_static_is_a_global_of_the_class(self):
        self.assertEqual(self.by_function["ClassStatic"], {("global_read", "Counter::count")})

    def test_file_static_carries_the_repo_path(self):
        self.assertEqual(self.by_function["FileStatic"], {("global_read", "s_local@src/a.cpp")})

    def test_escapes_and_function_references(self):
        self.assertEqual(self.by_function["Escapes"], {
            ("global_escape", "g_count"), ("fn_ref", "Helper"), ("deref_write", "*int")})

    def test_delete_calls_the_destructor_and_operator_delete(self):
        self.assertEqual(self.by_function["DeleteClass"], {
            ("call", "operator delete"), ("call", "Plain::~Plain")})

    def test_delete_of_a_virtual_destructor_is_also_an_indirect_call(self):
        self.assertEqual(self.by_function["DeleteVirtual"], {
            ("call", "operator delete"), ("call", "Virt::~Virt"), ("indirect_call", "virtual Virt::~Virt")})
        self.assertEqual(self.by_function["DeleteInherited"], {
            ("call", "operator delete"), ("call", "Sub::~Sub"), ("indirect_call", "virtual Sub::~Sub")})

    def test_delete_array_is_a_direct_destructor_call(self):
        self.assertEqual(self.by_function["DeleteArray"], {
            ("call", "operator delete"), ("call", "Plain::~Plain")})

    def test_temporary_calls_the_destructor(self):
        self.assertEqual(self.by_function["Temporary"], {("call", "MakePlain"), ("call", "UsePlain"),
                                                         ("call", "Plain::~Plain")})

    def test_local_calls_the_destructor_at_scope_exit(self):
        self.assertEqual(self.by_function["LocalAtScopeExit"], {("call", "Plain::~Plain")})

    def test_trivial_destructor_has_no_row(self):
        self.assertEqual(self.by_function["TrivialType"], {("call", "operator delete")})

    def test_every_function_has_a_definition_row(self):
        defined = {row[1] for row in self.rows if row[3] == "function" and row[4] == ""}
        self.assertEqual(defined, set(self.by_function))
        self.assertEqual(self.by_function["Empty"], set())

    def test_rows_carry_the_marker_address_and_are_sorted(self):
        addresses = {row[1]: row[0] for row in self.rows}
        self.assertEqual(addresses["ReadGlobal"], "0x401000")
        self.assertEqual(addresses["Increment"], "")
        self.assertEqual(self.rows, sorted(self.rows, key=lambda r: (r[0] == "", int(r[0] or "0", 16), *r[1:])))


SIM_COMMON = """struct Counter { int n; int Get() const { return n; } void Bump() { n++; } };
typedef int (*Handler)(int);
void Fill(int* out);
void FillU(unsigned* out);
void Look(const int* in);
void Put(char* out);
void Peek(const char* in);
"""

PLAIN_DECLS = """extern int g_count;
extern int g_arr[4];
extern Counter g_counter;
extern unsigned g_viewed[4];
extern unsigned g_scalar;
extern int g_inner;
extern Handler g_handlers[4];
static int g_st;
extern char g_sep[];
extern char g_buf[];
"""

SIM_DECLS = """struct SimGlobals {
    int g_count;
    int g_arr[4];
    Counter g_counter;
    int g_viewed[4];
    int g_scalar;
    char g_container[16];
    Handler g_handlers[4];
    int g_st_a;
    char g_buf[8];
};
extern char g_sep[];
SimGlobals& Sim();
static SimGlobals* g_simGlobals;
SimGlobals& Sim() { return *g_simGlobals; }
#define g_count (Sim().g_count)
#define g_arr (Sim().g_arr)
#define g_counter (Sim().g_counter)
#define g_handlers (Sim().g_handlers)
#define g_st (Sim().g_st_a)
#define g_buf (Sim().g_buf)
typedef unsigned SimView_g_viewed[4];
#define g_viewed (*reinterpret_cast<SimView_g_viewed*>(&Sim().g_viewed))
#define g_scalar (*reinterpret_cast<unsigned*>(&Sim().g_scalar))
#define g_inner (*reinterpret_cast<int*>(reinterpret_cast<char*>(&Sim().g_container) + 8))
"""

SIM_BODIES = """
int Read() { return g_count; }
void Write() { g_count = 1; }
void Compound() { g_count += 2; g_scalar += 2; g_inner++; }
void ArrayWrite(int i) { g_arr[i] = 3; g_viewed[i] = 4; }
int ArrayRead(int i) { return g_arr[i] + g_viewed[i]; }
void Escape() { int* p = &g_count; unsigned* q = &g_scalar; int* r = &g_inner; *p = 1; *q = 2; *r = 3; }
void Args() { Fill(&g_count); Fill(g_arr); Look(&g_count); FillU(&g_scalar); FillU(g_viewed); Fill(&g_inner); }
void Views() { g_scalar = 5; g_viewed[1] = g_scalar; }
void Interior() { g_inner = 7; Look(&g_inner); }
int Method() { g_counter.Bump(); return g_counter.Get() + g_counter.n; }
int Indirect(int x) { return g_handlers[x](x); }
void PickWrite(int f) { Put(f ? g_sep : g_buf); }
void PickPeek(int f) { Peek(f ? g_sep : g_buf); }
int Static() { g_st = 1; return g_st; }
const char* Pick(int f) { return f ? g_sep : g_buf; }
void Mixed() { Counter local; local.Bump(); Sim(); }
"""

SIM_MAP_TEXT = (
    "field,name,container,offset\n"
    "g_st_a,g_st@src/a.cpp,,\n"
    "g_inner,g_inner,g_container,0x8\n"
)


@unittest.skipUnless(shutil.which("clang-cl"), "clang-cl is not installed")
class SimGlobalsTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "sim_globals.csv"
            path.write_text(SIM_MAP_TEXT)
            cls.patch = mock.patch.object(rwsets, "SIM", rwsets.load_sim_map(path))
        cls.patch.start()
        _, cls.plain = rows_of(SIM_COMMON + PLAIN_DECLS + SIM_BODIES.replace("Sim();", ""))
        _, cls.sim = rows_of(SIM_COMMON + SIM_DECLS + SIM_BODIES)

    @classmethod
    def tearDownClass(cls):
        cls.patch.stop()

    def same(self, name):
        self.assertEqual(self.sim[name], self.plain[name])
        return self.sim[name]

    def test_read(self):
        self.assertEqual(self.same("Read"), {("global_read", "g_count")})

    def test_write(self):
        self.assertEqual(self.same("Write"), {("global_write", "g_count")})

    def test_compound_assignment(self):
        self.assertIn(("global_write", "g_count"), self.same("Compound"))

    def test_array_element(self):
        self.same("ArrayWrite")
        self.same("ArrayRead")

    def test_escape(self):
        self.assertIn(("global_escape", "g_count"), self.same("Escape"))

    def test_non_const_pointer_argument(self):
        rows = self.same("Args")
        self.assertIn(("global_maybe_written", "g_count"), rows)

    def test_per_file_view(self):
        rows = self.same("Views")
        self.assertIn(("global_write", "g_scalar"), rows)
        self.assertIn(("global_read", "g_scalar"), rows)

    def test_interior_offset(self):
        rows = self.same("Interior")
        self.assertEqual(rows, {("global_write", "g_inner"), ("global_read", "g_inner"), ("call", "Look")})

    def test_methods_and_indirect_calls(self):
        self.same("Method")
        self.assertIn(("indirect_call", "indirect via int (*)(int) [g_handlers]"), self.same("Indirect"))

    def test_static_is_named_by_its_row(self):
        self.assertEqual(self.same("Static"), {("global_write", "g_st@src/a.cpp"),
                                               ("global_read", "g_st@src/a.cpp")})

    def test_arrays_chosen_by_a_conditional_stay_reads(self):
        self.assertEqual(self.same("Pick"), {("global_read", "g_sep"), ("global_read", "g_buf")})

    def test_arrays_chosen_by_a_conditional_take_the_mode_of_the_context(self):
        self.assertEqual(self.sim["PickWrite"], {("global_maybe_written", "g_sep"), ("global_maybe_written", "g_buf"),
                                                 ("call", "Put")})
        self.assertEqual(self.sim["PickPeek"], {("global_read", "g_sep"), ("global_read", "g_buf"), ("call", "Peek")})

    def test_sim_calls_and_the_pointer_add_no_rows(self):
        rows = self.sim["Mixed"]
        self.assertEqual(rows, self.plain["Mixed"])
        self.assertFalse([r for r in rows if "Sim" in r[1] or "g_simGlobals" in r[1]])
        self.assertFalse([r for fn in self.sim.values() for r in fn if "g_simGlobals" in r[1]])


class SimMapTest(unittest.TestCase):
    TEXT = (
        "field,name,container,offset\n"
        "g_dup,g_dup,,\n"
        "g_dup,g_dup@src/a.cpp,,\n"
        "g_bare,g_bare,,\n"
        "g_bare,g_bare,,\n"
        "g_only,g_only@src/b.cpp,,\n"
        "g_in,g_in,g_dup,0x8\n"
    )

    def load(self, text):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "sim_globals.csv"
            path.write_text(text)
            return rwsets.load_sim_map(path)

    def test_static_row_is_used_only_in_its_own_file(self):
        sim = self.load(self.TEXT)
        sim.current_file = "src/a.cpp"
        self.assertEqual(sim.name("g_dup"), "g_dup@src/a.cpp")
        sim.current_file = "src/other.cpp"
        self.assertEqual(sim.name("g_dup"), "g_dup")

    def test_file_column_names_the_file_of_a_bare_static_row(self):
        sim = self.load("field,name,container,offset,file\ng_t,T::g_t,,,\ng_t,g_t,,,src/m.cpp\n")
        sim.current_file = "src/m.cpp"
        self.assertEqual(sim.name("g_t"), "g_t")
        sim.current_file = "src/u.cpp"
        self.assertEqual(sim.name("g_t"), "T::g_t")

    def test_lone_static_and_unlisted_fields(self):
        sim = self.load(self.TEXT)
        sim.current_file = "src/a.cpp"
        self.assertEqual(sim.name("g_only"), "g_only@src/b.cpp")
        self.assertEqual(sim.name("g_unlisted"), "g_unlisted")

    def test_every_name_maps_to_its_field_and_interiors_hang_on_the_field(self):
        sim = self.load(self.TEXT)
        self.assertEqual(sim.names["g_dup@src/a.cpp"], "g_dup")
        self.assertEqual(sim.names["g_dup"], "g_dup")
        self.assertEqual(sim.interiors[("g_dup", 8)], "g_in")


if __name__ == "__main__":
    unittest.main()
