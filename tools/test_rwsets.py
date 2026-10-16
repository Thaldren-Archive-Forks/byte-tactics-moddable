"""Tests for rwsets.py, on hand-written functions compiled with clang-cl.

    uv run tools/test_rwsets.py
"""

import shutil
import tempfile
import unittest
from pathlib import Path

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

int ClassStatic() { return Counter::count; }

int FileStatic() { return s_local; }

void Empty() {}

// FUNCTION: 0x401060
void Escapes() { int* p = &g_count; Handler h = Helper; *p = 1; (void)h; }
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

    def test_class_static_is_a_global_of_the_class(self):
        self.assertEqual(self.by_function["ClassStatic"], {("global_read", "Counter::count")})

    def test_file_static_carries_the_repo_path(self):
        self.assertEqual(self.by_function["FileStatic"], {("global_read", "s_local@src/a.cpp")})

    def test_escapes_and_function_references(self):
        self.assertEqual(self.by_function["Escapes"], {
            ("global_escape", "g_count"), ("fn_ref", "Helper"), ("deref_write", "*int")})

    def test_every_function_has_a_definition_row(self):
        defined = {row[1] for row in self.rows if row[3] == "function" and row[4] == ""}
        self.assertEqual(defined, set(self.by_function))
        self.assertEqual(self.by_function["Empty"], set())

    def test_rows_carry_the_marker_address_and_are_sorted(self):
        addresses = {row[1]: row[0] for row in self.rows}
        self.assertEqual(addresses["ReadGlobal"], "0x401000")
        self.assertEqual(addresses["Increment"], "")
        self.assertEqual(self.rows, sorted(self.rows, key=lambda r: (r[0] == "", int(r[0] or "0", 16), *r[1:])))


if __name__ == "__main__":
    unittest.main()
