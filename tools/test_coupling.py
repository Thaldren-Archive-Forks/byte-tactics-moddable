"""Tests for coupling.py, on hand-written rwsets rows and on the committed data files.

    uv run tools/test_coupling.py
"""

import csv
import unittest
from types import SimpleNamespace

import coupling as cp
import hazards
import stateclasses as sc


def fn(name, file, address="", calls=(), reads=(), writes=(), fields_read=(), fields_written=()):
    """The rwsets rows of one function."""
    rows = [[address, name, file, "function", ""]]
    rows += [[address, name, file, "call", c] for c in calls]
    rows += [[address, name, file, "global_read", g] for g in reads]
    rows += [[address, name, file, "global_write", g] for g in writes]
    rows += [[address, name, file, "field_read", f] for f in fields_read]
    rows += [[address, name, file, "field_write", f] for f in fields_written]
    return rows


def subsystem_of(name, file, reached=None):
    return cp.rule_of(name, file, reached or {})[0]


class RuleTest(unittest.TestCase):
    def test_folder_decides_platform_and_harness(self):
        self.assertEqual(cp.rule_of("Hash", "src/util/math.cpp", {}), ("platform", "folder:src/util/"))
        self.assertEqual(subsystem_of("DumpFields", "src/harness/harness.cpp"), "harness")
        self.assertEqual(subsystem_of("DumpFields", "src/debug/debug_lib.cpp", {"DumpFields": {"UpdateAllUnits"}}),
                         "platform")

    def test_file_beats_name_and_phase(self):
        reached = {"AddOrder": {"UpdateWind"}}
        self.assertEqual(cp.rule_of("AddOrder", "src/orders/order_list.cpp", reached)[0], "orders")
        self.assertEqual(cp.rule_of("RetargetWeapons", "src/ai/ai_player.cpp", {})[0], "ai")
        self.assertEqual(cp.rule_of("Pathfinder::StartSearch", "src/ai/pathfind.cpp", {})[0], "pathfinding")

    def test_phase_decides_in_a_folder_of_several_subsystems(self):
        reached = {"Helper": {"UpdateFeatures"}}
        self.assertEqual(cp.rule_of("Helper", "src/game/main_loop.cpp", reached), ("features", "phase:UpdateFeatures"))
        self.assertEqual(cp.rule_of("Helper", "src/game/main_loop.cpp", {})[0], "game")

    def test_two_phases_decide_nothing(self):
        reached = {"Helper": {"UpdateFeatures", "UpdateMeteors"}}
        self.assertEqual(cp.rule_of("Helper", "src/game/main_loop.cpp", reached)[0], "game")

    def test_the_packet_phase_dispatches_and_decides_nothing(self):
        reached = {"Helper": {"HandleNetPackets"}, "Both": {"HandleNetPackets", "UpdatePlayers"}}
        self.assertEqual(cp.rule_of("Helper", "src/game/main_loop.cpp", reached)[0], "game")
        self.assertEqual(cp.rule_of("Both", "src/game/main_loop.cpp", reached)[0], "players")

    def test_phase_beats_the_presentation_folder(self):
        reached = {"StepGafSequence": {"StepAllGafSequences"}}
        self.assertEqual(cp.rule_of("StepGafSequence", "src/graphics/gaf.cpp", reached),
                         ("game", "phase:StepAllGafSequences"))

    def test_a_reached_function_is_never_presentation(self):
        reached = {"Paint": {"UpdateExplosions", "UpdateWind"}, "DrawThing": {"UpdateWind", "UpdateMeteors"}}
        for name in reached:
            for file in ("src/graphics/gaf.cpp", "src/ingame/control_panel.cpp", "src/sound/sound.cpp",
                         "src/gui/gui.cpp", "src/frontend/multi.cpp"):
                self.assertNotEqual(subsystem_of(name, file, reached), "presentation", (name, file))

    def test_presentation_folders_and_drawing(self):
        self.assertEqual(subsystem_of("OpenMenu", "src/frontend/multi.cpp"), "presentation")
        self.assertEqual(subsystem_of("OpenMenu", "src/gui/gui.cpp"), "presentation")
        self.assertEqual(subsystem_of("Item::DrawFrame", "src/map/features.cpp"), "presentation")
        self.assertEqual(subsystem_of("DrawMapTiles", "src/map/line_of_sight.cpp"), "presentation")
        self.assertEqual(subsystem_of("DrawMapTiles", "src/map/line_of_sight.cpp", {"DrawMapTiles": {"UpdateWind"}}),
                         "los_radar")

    def test_names_decide_where_the_file_does_not(self):
        self.assertEqual(subsystem_of("SetWeaponTargetUnit", "src/units/unit_targets.cpp"), "targeting")
        self.assertEqual(subsystem_of("GetPassMapCellValue", "src/units/movement_class.cpp"), "map")
        self.assertEqual(subsystem_of("SpendMetal", "src/game/main_loop.cpp"), "economy")
        self.assertEqual(subsystem_of("Helper", "src/units/unit.cpp"), "units")
        self.assertEqual(subsystem_of("Helper", "src/game/main.cpp"), "game")

    def test_a_presentation_name_does_not_claim_a_reached_function(self):
        self.assertEqual(subsystem_of("CMemoryCache::Alloc", "src/units/unit_motion.cpp"), "presentation")
        self.assertEqual(subsystem_of("CMemoryCache::Alloc", "src/units/unit_motion.cpp",
                                      {"CMemoryCache::Alloc": {"UpdateAllUnits"}}), "units")

    def test_library_instantiations_are_platform(self):
        self.assertEqual(subsystem_of("_Construct", "src/ai/pathfind.cpp"), "platform")

    def test_override_is_kept_and_rule_still_shows(self):
        rows = fn("Helper", "src/ingame/control_panel.cpp", "0x401000") + fn("Other", "src/util/x.cpp", "0x401010")
        out = cp.build_subsystems(rows, {}, {("Helper", "src/ingame/control_panel.cpp"): "units"})
        self.assertEqual([(r["function"], r["subsystem"], r["rule"], r["override"]) for r in out],
                         [("Helper", "units", "presentation", "units"), ("Other", "platform", "folder:src/util/", "")])
        text = cp.render(cp.SUBSYSTEM_COLUMNS, out)
        self.assertIn("Helper,src/ingame/control_panel.cpp,units,presentation,units\n", text)

    def test_one_row_per_function_and_file(self):
        rows = fn("Add", "src/ai/a.cpp", "0x402000") + fn("Add", "src/util/b.cpp", "0x401000") \
            + [["0x401000", "Add", "src/util/b.cpp", "call", "X"]]
        out = cp.build_subsystems(rows, {}, {})
        self.assertEqual([(r["function"], r["file"]) for r in out], [("Add", "src/util/b.cpp"), ("Add", "src/ai/a.cpp")])

    def test_stale_override_is_found(self):
        out = [{"function": "A", "file": "f"}]
        self.assertEqual(cp.stale_overrides({("A", "f"): "ai", ("B", "f"): "ai"}, out), [("B", "f")])


def subsystems(*entries):
    return [{"function": f, "file": file, "subsystem": s} for f, file, s in entries]


class CallTest(unittest.TestCase):
    def counts(self, rows, entries, edges=()):
        of, names = cp.subsystem_maps(subsystems(*entries))
        return dict(cp.call_counts(rows, list(edges), of, names))

    def test_calls_across_subsystems_aggregate_per_target(self):
        rows = fn("A", "a.cpp", calls=["T"]) + fn("B", "a.cpp", calls=["T", "S"]) + fn("T", "t.cpp") + fn("S", "t.cpp")
        got = self.counts(rows, [("A", "a.cpp", "units"), ("B", "a.cpp", "units"), ("T", "t.cpp", "map"),
                                 ("S", "t.cpp", "map")])
        self.assertEqual(got, {("units", "map", "call", "T"): 2, ("units", "map", "call", "S"): 1})

    def test_a_call_inside_a_subsystem_and_out_of_the_source_is_no_row(self):
        rows = fn("A", "a.cpp", calls=["B", "memset"]) + fn("B", "b.cpp")
        self.assertEqual(self.counts(rows, [("A", "a.cpp", "units"), ("B", "b.cpp", "units")]), {})

    def test_an_overloaded_callee_stays_home_when_it_can(self):
        rows = fn("A", "a.cpp", calls=["Add"]) + fn("Add", "a2.cpp") + fn("Add", "m.cpp")
        entries = [("A", "a.cpp", "units"), ("Add", "a2.cpp", "units"), ("Add", "m.cpp", "map")]
        self.assertEqual(self.counts(rows, entries), {})
        entries[0] = ("A", "a.cpp", "orders")
        self.assertEqual(self.counts(rows, entries), {("orders", "map", "call", "Add"): 1})

    def test_resolved_indirect_calls_count_as_indirect(self):
        rows = fn("A", "a.cpp") + fn("T", "t.cpp")
        got = self.counts(rows, [("A", "a.cpp", "units"), ("T", "t.cpp", "weapons")], edges=[("A", "T")])
        self.assertEqual(got, {("units", "weapons", "indirect", "T"): 1})


class SharedTest(unittest.TestCase):
    ENTRIES = [("W", "w.cpp", "weapons"), ("W2", "w.cpp", "weapons"), ("U", "u.cpp", "units"),
               ("P", "p.cpp", "presentation")]

    def shared(self, rows, classes, entries=None):
        of, _ = cp.subsystem_maps(subsystems(*(entries or self.ENTRIES)))
        by_name, by_function = cp.item_accesses(rows, sc.Items(rows, []))
        classes = {k: v for k, v in classes.items()}
        counts, wrote, read = cp.shared_counts(by_function, classes, of)
        return dict(counts), wrote, read

    def test_a_write_and_a_read_in_two_subsystems(self):
        rows = fn("W", "w.cpp", writes=["g_hp"]) + fn("U", "u.cpp", reads=["g_hp"]) + fn("P", "p.cpp", reads=["g_hp"])
        got, wrote, read = self.shared(rows, {("static", "g_hp"): "sim"})
        self.assertEqual(got, {("weapons", "units", "shared_write", "g_hp"): 1,
                               ("weapons", "presentation", "shared_write", "g_hp"): 1})
        self.assertEqual(wrote["weapons"], {"g_hp"})
        self.assertEqual(read["units"], {"g_hp"})

    def test_two_writers_make_a_row_each_way(self):
        rows = fn("W", "w.cpp", writes=["g_hp"]) + fn("U", "u.cpp", writes=["g_hp"])
        got, _, _ = self.shared(rows, {("static", "g_hp"): "mixed"})
        self.assertEqual(got, {("weapons", "units", "shared_write", "g_hp"): 1,
                               ("units", "weapons", "shared_write", "g_hp"): 1})

    def test_count_is_the_functions_of_the_other_subsystem(self):
        rows = fn("W", "w.cpp", writes=["g_hp"]) + fn("U", "u.cpp", reads=["g_hp"]) + fn("W2", "w.cpp", reads=["g_hp"])
        got, _, _ = self.shared(rows, {("static", "g_hp"): "sim"},
                                self.ENTRIES + [("U2", "u.cpp", "units")])
        self.assertEqual(got, {("weapons", "units", "shared_write", "g_hp"): 1})

    def test_one_subsystem_or_reads_only_are_no_row(self):
        rows = fn("W", "w.cpp", writes=["g_hp"]) + fn("W2", "w.cpp", reads=["g_hp"]) \
            + fn("U", "u.cpp", reads=["g_tab"]) + fn("P", "p.cpp", reads=["g_tab"])
        got, _, _ = self.shared(rows, {("static", "g_hp"): "sim", ("static", "g_tab"): "sim"})
        self.assertEqual(got, {})

    def test_view_and_platform_items_are_no_row(self):
        rows = fn("W", "w.cpp", writes=["g_cam"]) + fn("U", "u.cpp", reads=["g_cam"])
        for cls in ("view", "platform", ""):
            got, _, _ = self.shared(rows, {("static", "g_cam"): cls})
            self.assertEqual(got, {}, cls)

    def test_a_pointer_passed_on_is_a_read_not_a_write(self):
        rows = fn("W", "w.cpp") + [["", "W", "w.cpp", "global_maybe_written", "g_hp"]] + fn("U", "u.cpp", reads=["g_hp"])
        got, wrote, _ = self.shared(rows, {("static", "g_hp"): "sim"})
        self.assertEqual(got, {})
        self.assertEqual(wrote["weapons"], set())

    def test_a_global_joins_a_row_by_name_and_by_address(self):
        globals_rows = [{"name": "g_named", "address": "0x500000"}]
        rows = fn("W", "w.cpp", writes=["DAT_00500000"]) + fn("U", "u.cpp", reads=["g_named"])
        _, by_function = cp.item_accesses(rows, sc.Items(rows, globals_rows))
        self.assertEqual(list(by_function), [("global", "g_named")])
        self.assertEqual(sorted(by_function[("global", "g_named")].values()), [False, True])

    def test_fields_take_the_class_of_their_phases_and_anonymous_types_are_left_out(self):
        rows = fn("W", "w.cpp", fields_written=["Unit::hp", "<anon>::x"]) \
            + fn("U", "u.cpp", fields_read=["Unit::hp", "<anon>::x"]) \
            + fn("P", "p.cpp", fields_written=["Cam::x"]) + fn("U", "u.cpp", fields_read=["Cam::x"])
        walks = SimpleNamespace(phases={"W": {"2"}}, load=set(), presentation={"P"}, sim={"W"},
                                files={}, rank=lambda f: (f,))
        by_name, by_function = cp.item_accesses(rows, sc.Items(rows, []))
        classes = cp.item_classes(by_name, [], walks)
        self.assertEqual(classes[("field", "Unit::hp")], "sim")
        self.assertEqual(classes[("field", "Cam::x")], "view")
        self.assertNotIn(("field", "<anon>::x"), by_name)
        of, _ = cp.subsystem_maps(subsystems(*self.ENTRIES))
        counts, _, _ = cp.shared_counts(by_function, classes, of)
        self.assertEqual(dict(counts), {("weapons", "units", "shared_write", "Unit::hp"): 1})

    def test_result_rows_are_sorted_with_string_counts(self):
        result = cp.Result({("b", "a", "call", "X"): 2, ("a", "b", "call", "Y"): 1}, {}, {})
        self.assertEqual([(r["from"], r["count"]) for r in result.rows], [("a", "1"), ("b", "2")])


def call_row(f, t, kind="call", target="X", count="1"):
    return {"from": f, "to": t, "kind": kind, "target": target, "count": count}


class SeamTest(unittest.TestCase):
    def test_a_sim_to_sim_call_and_a_game_to_sim_call_are_edges(self):
        rows = [call_row("units", "weapons", target="Fire"), call_row("game", "map", target="Plot")]
        self.assertEqual(cp.seam_edges(rows), {("units", "weapons", "Fire"), ("game", "map", "Plot")})

    def test_other_ends_and_kinds_are_not_edges(self):
        rows = [call_row("units", "presentation"), call_row("units", "network"), call_row("units", "harness"),
                call_row("units", "platform"), call_row("units", "seams"), call_row("seams", "units"),
                call_row("presentation", "units"), call_row("network", "units"), call_row("platform", "map"),
                call_row("units", "game"), call_row("units", "weapons", kind="indirect"),
                call_row("units", "weapons", kind="shared_write")]
        self.assertEqual(cp.seam_edges(rows), set())

    def test_an_edge_fails_whatever_its_count(self):
        for count in ("1", "7"):
            lines, _ = cp.check_seams([call_row("units", "weapons", target="Fire", count=count)])
            self.assertEqual(len(lines), 2)
            self.assertIn("units -> weapons: Fire", lines[0])
            self.assertIn("interfaces/seams.h", lines[1])

    def test_every_edge_is_listed(self):
        lines, _ = cp.check_seams([call_row("units", "weapons", target="Fire"), call_row("game", "map", target="Plot")])
        self.assertEqual(len(lines), 3)

    def test_no_edge_passes(self):
        lines, summary = cp.check_seams([call_row("units", "platform"), call_row("units", "weapons", kind="indirect")])
        self.assertEqual(lines, [])
        self.assertIn("0 direct calls", summary)

    def test_the_committed_coupling_has_no_direct_call(self):
        lines, _ = cp.check_seams(cp.read_csv(cp.COUPLING_CSV))
        self.assertEqual(lines, [])

    def test_src_seams_is_its_own_subsystem(self):
        self.assertEqual(cp.rule_of("InstallNativeSeams", "src/seams/install.cpp", {}),
                         ("seams", "folder:src/seams/"))


class DataTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.rows, cls.graph = hazards.load_graph()
        cls.walks = sc.Walks(cls.rows, cls.graph)
        cls.walks.graph = cls.graph
        cls.subsystems = cp.read_csv(cp.SUBSYSTEMS_CSV)
        cls.coupling = cp.read_csv(cp.COUPLING_CSV)

    def test_every_function_of_rwsets_has_a_subsystem(self):
        have = {(r["function"], r["file"]) for r in self.subsystems}
        want = {(r[1], r[2]) for r in self.rows if r[3] == "function"}
        self.assertEqual(sorted(want - have), [])
        self.assertEqual(sorted(have - want), [])
        self.assertEqual(len(self.subsystems), len(have))

    def test_subsystems_are_known_and_overrides_are_what_the_row_says(self):
        for r in self.subsystems:
            self.assertIn(r["subsystem"], cp.SUBSYSTEMS, r["function"])
            self.assertEqual(r["subsystem"], r["override"] or r["subsystem"])
            if r["override"]:
                self.assertIn(r["override"], cp.SUBSYSTEMS, r["function"])

    def test_no_function_a_phase_walk_reaches_is_presentation(self):
        reached = cp.phase_roots(self.walks)
        bad = [r["function"] for r in self.subsystems
               if r["subsystem"] == "presentation" and r["function"] in reached and not r["override"]]
        self.assertEqual(bad, [])

    def test_no_phase_root_is_called_from_presentation(self):
        shown = [r["target"] for r in self.coupling
                 if r["from"] == "presentation" and r["kind"] in ("call", "indirect")
                 and r["target"] in hazards.ROOTS]
        self.assertEqual(shown, [])

    def test_the_phase_roots_are_in_their_subsystems(self):
        want = dict(cp.PHASE_SUBSYSTEMS, HandleNetPackets="network")
        got = {}
        for r in self.subsystems:
            if r["function"] in want:
                got.setdefault(r["function"], set()).add(r["subsystem"])
        self.assertEqual(got, {name: {subsystem} for name, subsystem in want.items()})

    def test_the_committed_subsystems_are_what_the_rules_give(self):
        overrides = cp.read_overrides()
        rebuilt = cp.build_subsystems(self.rows, cp.phase_roots(self.walks), overrides)
        self.assertEqual(rebuilt, self.subsystems)

    def test_coupling_rows_are_sorted_and_between_two_subsystems(self):
        keys = [(r["from"], r["to"], r["kind"], r["target"]) for r in self.coupling]
        self.assertEqual(keys, sorted(keys))
        self.assertEqual(len(keys), len(set(keys)))
        for r in self.coupling:
            self.assertNotEqual(r["from"], r["to"])
            self.assertIn(r["from"], cp.SUBSYSTEMS)
            self.assertIn(r["to"], cp.SUBSYSTEMS)
            self.assertIn(r["kind"], ("call", "indirect", "shared_write"))
            self.assertGreaterEqual(int(r["count"]), 1)

    def test_file_has_the_columns(self):
        with cp.SUBSYSTEMS_CSV.open(newline="") as f:
            self.assertEqual(next(csv.reader(f)), cp.SUBSYSTEM_COLUMNS)
        with cp.COUPLING_CSV.open(newline="") as f:
            self.assertEqual(next(csv.reader(f)), cp.COUPLING_COLUMNS)


if __name__ == "__main__":
    unittest.main()
