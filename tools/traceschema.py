"""Generate src/harness/trace_schema.h, the field tables the test harness hashes and dumps.

    uv run tools/traceschema.py          # rewrite the header; a second run leaves it unchanged
    uv run tools/traceschema.py --check  # exit 1 when the committed header is out of date

Layouts are read from include/ta_types.h (packed, so a struct's fields follow each other
with no padding), checked against the "+0x" offsets and the "0x.. bytes" size in its
comments, then flattened to leaf fields {name, offset, size, kind}. Hand-written
overrides (names, int fields that hold pointers, pointer targets, exclusions with
their reasons, structs ta_types.h has no single view of) are the data at the top.
"""

import argparse
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
TYPES = ROOT / "include/ta_types.h"
OUT = ROOT / "src/harness/trace_schema.h"

BASE_SIZES = {
    "char": 1, "signed char": 1, "unsigned char": 1, "short": 2, "unsigned short": 2,
    "int": 4, "unsigned int": 4, "unsigned": 4, "long": 4, "unsigned long": 4,
    "float": 4, "double": 8, "__int64": 8, "unsigned __int64": 8,
}

POINTER_TARGETS = {
    "Unit": "UNIT", "Player": "PLAYER", "UnitDef": "UNITDEF", "WeaponDef": "WEAPONDEF",
    "Def_0049d270": "WEAPONDEF", "UnitMotion": "WEAPONDEF", "Obj_00406f50": "PROJECTILE",
    "Feature": "FEATURE", "Object3do": "PIECE", "Object3D": "PIECE",
}

# type -> {offset: name}: names better than the ta_types.h ones, from the code that uses them.
NAMES = {
    "Unit": {0xA6: "unitDefIndex"},
    "Debris": {0x0: "obj"},
}

# type -> {offset: target}: fields ta_types.h types as int (or as a pointer to the wrong
# thing) that hold a pointer.
POINTERS = {
    "Unit": {0x0: "HEAP", 0x8: "HEAP", 0x24: "HEAP", 0x40: "HEAP", 0x82: "HEAP", 0x86: "HEAP"},
    "Player": {0x74: "HEAP"},
    "Projectile": {0x4E: "HEAP"},
    "Debris": {0x0: "PIECE"},
}

POINTER_NAMES = {}

# (type, field name or "+0xoffset", reason): fields left out of the hash and the dump.
EXCLUSIONS = []

# (type, offset) -> (mask, reason): scalar fields of which only the masked bits are state.
MASKS = {
    ("ExplodedPiece", 0x28): (0x3F, "bits 6 to 31 are never written; StartExplodePiece copies them from an uninitialised stack header"),
}

SYNTHETIC = {
    "ExplodedPiece": """
        Unit* unit;
        int index;
        int angVelZ;
        int angVelX;
        int angVelY;
        int vel_x;
        int vel_y;
        int vel_z;
        int ticks;
        int scale;
        unsigned int flags;
        void* rec;
    """,
    "PieceRec": """
        Object3do* desc;
        char unknown_4[12];
        short angle_10;
        short angle_12;
        short angle_14;
        Vec3 pos;
        void* verts;
        char unknown_26[2];
        unsigned short flags;
        void* link0;
        void* link1;
        void* link2;
    """,
    "Face": """
        int color;
        int nverts;
        int unknown_8;
        void* vindex;
        void* texFrame;
        unsigned char texKind;
        char unknown_15[3];
        void* texSrc;
        unsigned int flags;
    """,
    "SpotObject": """
        short prev;
        short next;
        void* state;
        Vec3 pos;
        Vec3 vel;
        Rot16 rot;
        unsigned short damage;
        short x;
        short z;
        unsigned short feature;
        unsigned char timer;
        unsigned char flags;
    """,
    "SpotAnim": """
        short prev;
        short next;
        Handle anim;
        Handle shadow;
        char unknown_1c[4];
        Rot16 rot;
        unsigned short damage;
        short x;
        short z;
        unsigned short feature;
        unsigned char timer;
        unsigned char flags;
    """,
}
SYNTHETIC_SIZES = {
    "ExplodedPiece": 0x30, "PieceRec": 0x36, "Face": 0x20, "SpotObject": 0x30, "SpotAnim": 0x30,
}

# (ta_types.h or synthetic type, table name)
TYPE_ORDER = [
    ("Unit", "Unit"),
    ("Obj_00406f50", "Projectile"),
    ("Player", "Player"),
    ("Debris_00420f30", "Debris"),
    ("Object3do", "Piece"),
    ("ExplodedPiece", "ExplodedPiece"),
    ("PieceRec", "PieceRec"),
    ("Face", "Face"),
    ("SpotObject", "SpotObject"),
    ("SpotAnim", "SpotAnim"),
]

WIND_FIELDS = [
    ("windCounter", "unsigned int"), ("windSpeedDenominator", "int"), ("windX", "int"),
    ("windY", "int"), ("windZ", "int"), ("windDirection", "unsigned short"),
    ("windSpeed", "int"), ("windStrength", "float"), ("windEnabled", "int"),
    ("windSpeedMin", "int"), ("windSpeedMax", "int"),
]

# (name, global, size, pointer target)
METEOR_GLOBALS = [
    ("nextStrikeTime", "g_meteorNextStrikeTime", 4, None),
    ("weaponName", "g_meteorWeaponName", 32, None),
    ("spawnMagnitude", "g_meteorSpawnMagnitude", 4, None),
    ("spawnInterval", "g_meteorSpawnInterval", 4, None),
    ("active", "g_meteorActive", 4, None),
    ("strikeEndTime", "g_meteorStrikeEndTime", 4, None),
    ("origin", "g_meteorOrigin", 4, None),
    ("showerDuration", "g_meteorShowerDuration", 4, None),
    ("weapon", "g_meteorWeapon", 4, "WEAPONDEF"),
    ("enabled", "g_meteorsEnabled", 4, None),
    ("nextHitTime", "g_meteorNextHitTime", 4, None),
    ("target", "g_meteorTarget", 4, None),
    ("scheduleGap", "g_meteorScheduleGap", 4, None),
]
METEOR_DECLS = {
    "g_meteorWeaponName": "char g_meteorWeaponName[32]",
    "g_meteorOrigin": "Point16 g_meteorOrigin",
    "g_meteorTarget": "Point16 g_meteorTarget",
    "g_meteorWeapon": "WeaponDef* g_meteorWeapon",
}


class Schema:
    def __init__(self):
        self.text = TYPES.read_text(encoding="latin-1")
        self.cache = {}
        self.errors = []

    def struct_source(self, name):
        m = re.search(r"^(?:struct|class|union) %s \{[^\n]*\n(.*?)^\};" % re.escape(name),
                      self.text, re.S | re.M)
        if not m:
            raise KeyError(name)
        head = re.search(r"^(?:struct|class|union) %s \{\s*//\s*0x([0-9a-f]+) bytes" % re.escape(name),
                         self.text, re.M)
        return m.group(1), int(head.group(1), 16) if head else None

    def size_of(self, ctype):
        ctype = " ".join(ctype.split())
        if "*" in ctype:
            return 4
        if ctype in BASE_SIZES:
            return BASE_SIZES[ctype]
        return self.layout(ctype)[1]

    def layout(self, name):
        if name in self.cache:
            return self.cache[name]
        if name in SYNTHETIC:
            body, declared = SYNTHETIC[name], SYNTHETIC_SIZES[name]
        else:
            body, declared = self.struct_source(name)
        result = self.parse(name, body, declared)
        self.cache[name] = result
        return result

    def parse(self, name, body, declared):
        leaves = []
        pos = 0
        group = None
        for raw in body.split("\n"):
            line = raw.strip()
            if not line or line.startswith("//") or line in ("public:", "private:", "protected:"):
                continue
            comment = re.search(r"//\s*\+0x([0-9a-f]+)", line)
            explicit = int(comment.group(1), 16) if comment else None
            line = line.split("//")[0].strip()
            if not line.endswith(";") or "(" in line:
                continue
            line = line[:-1].strip()
            if line.startswith("union {") and line.endswith("}"):
                members = [m.strip() for m in line[len("union {"):-1].split(";") if m.strip()]
                decls = [self.decl(m) for m in members]
                decl = dict(decls[0])
                decl["size"] = max(d["size"] for d in decls)
                decl["union"] = True
                group = None
                pos = self.place(name, leaves, pos, explicit, decl)
                continue
            d = self.decl(line)
            if d["bits"] is not None:
                size = d["size"]
                if group and group["size"] == size and group["used"] + d["bits"] <= 8 * size:
                    group["used"] += d["bits"]
                    if group["name"] == "" and d["name"]:
                        group["name"] = d["name"]
                    continue
                if explicit is not None and explicit != pos:
                    self.errors.append("%s: %s at %#x, comment says %#x" % (name, d["name"], pos, explicit))
                    pos = explicit
                group = {"size": size, "used": d["bits"], "name": d["name"]}
                leaves.append({"name": "", "offset": pos, "size": size, "kind": "scalar",
                               "target": None, "group": group})
                pos += size
                continue
            group = None
            pos = self.place(name, leaves, pos, explicit, d)
        for leaf in leaves:
            if "group" in leaf:
                leaf["name"] = "" if leaf["group"]["name"].startswith("unknown") else leaf["group"]["name"]
                del leaf["group"]
        if declared is not None and pos != declared:
            self.errors.append("%s: layout is %#x bytes, header says %#x" % (name, pos, declared))
        return leaves, declared if declared is not None else pos

    def decl(self, text):
        m = re.match(r"^(.*?)\s*:\s*(\d+)$", text)
        if m:
            left = " ".join(m.group(1).split())
            name = ""
            if left not in BASE_SIZES:
                left, name = left.rsplit(" ", 1)
            return {"name": name, "ctype": left, "ptr": False, "dims": [], "bits": int(m.group(2)),
                    "size": self.size_of(left)}
        m = re.match(r"^(.*?)(\w+)((?:\[[^\]]+\])*)$", text)
        ctype = " ".join(m.group(1).replace("*", " * ").split())
        dims = [int(x, 0) for x in re.findall(r"\[([^\]]+)\]", m.group(3))]
        ptr = "*" in ctype
        elem = 4 if ptr else self.size_of(ctype)
        count = 1
        for n in dims:
            count *= n
        return {"name": m.group(2), "ctype": ctype.replace(" * ", "*").replace(" *", "*"), "ptr": ptr,
                "dims": dims, "bits": None, "size": elem * count}

    def place(self, tname, leaves, pos, explicit, d):
        if explicit is not None and explicit != pos:
            self.errors.append("%s: %s at %#x, comment says %#x" % (tname, d["name"], pos, explicit))
            pos = explicit
        name = "" if d["name"].startswith("unknown") else d["name"]
        ctype = d["ctype"]
        base = ctype.rstrip("*").strip()
        count = 1
        for x in d["dims"]:
            count *= x
        if d.get("union"):
            leaves.append({"name": name, "offset": pos, "size": d["size"],
                           "kind": "scalar" if d["size"] in (1, 2, 4, 8) else "bytes", "target": None})
        elif d["ptr"]:
            target = POINTER_TARGETS.get(base, "HEAP")
            for i in range(count):
                label = name + ("[%d]" % i if d["dims"] else "")
                leaves.append({"name": label, "offset": pos + 4 * i, "size": 4, "kind": "pointer",
                               "target": target})
        elif ctype in BASE_SIZES:
            size = d["size"]
            kind = "scalar" if not d["dims"] and size in (1, 2, 4, 8) else "bytes"
            leaves.append({"name": name, "offset": pos, "size": size, "kind": kind, "target": None})
        else:
            sub, subsize = self.layout(ctype)
            for i in range(count):
                label = name + ("[%d]" % i if d["dims"] else "")
                for leaf in sub:
                    leaves.append(dict(leaf, name=(label + "." + leaf["name"]) if leaf["name"] else "",
                                       offset=pos + i * subsize + leaf["offset"]))
        return pos + d["size"]


def retype_pointer(schema, key, leaves, offset, target):
    """Make bytes offset..offset+4 one pointer leaf; leaves it overlaps are cut to the bytes outside it."""
    end = offset + 4
    out = []
    for leaf in leaves:
        lo, hi = leaf["offset"], leaf["offset"] + leaf["size"]
        if hi <= offset or lo >= end:
            out.append(leaf)
            continue
        if leaf["kind"] == "pointer":
            schema.errors.append("%s: pointer override at %#x cuts the pointer at %#x" % (key, offset, lo))
        if lo < offset:
            out.append({"name": "", "offset": lo, "size": offset - lo, "kind": "bytes", "target": None})
        if hi > end:
            out.append({"name": "", "offset": end, "size": hi - end, "kind": "bytes", "target": None})
    out.append({"name": POINTER_NAMES.get((key, offset), ""), "offset": offset, "size": 4, "kind": "pointer",
                "target": target})
    out.sort(key=lambda leaf: leaf["offset"])
    return out


def finish(schema, ctype, key):
    leaves, size = schema.layout(ctype)
    names = NAMES.get(key, {})
    excluded = {(t, f) for t, f, _ in EXCLUSIONS if t == key}
    leaves = [dict(leaf) for leaf in leaves]
    for offset, target in sorted(POINTERS.get(key, {}).items()):
        leaves = retype_pointer(schema, key, leaves, offset, target)
    out = []
    for leaf in leaves:
        if leaf["offset"] in names and "." not in leaf["name"]:
            leaf["name"] = names[leaf["offset"]]
        mask = MASKS.get((key, leaf["offset"]))
        if mask:
            leaf["mask"] = mask[0]
        label = leaf["name"] or "+0x%x" % leaf["offset"]
        if (key, label) in excluded or (key, "+0x%x" % leaf["offset"]) in excluded:
            continue
        out.append(leaf)
    return out, size


def c_string(text):
    return '"%s"' % text


def plan_of(fields):
    plan = []
    for f in fields:
        if f["kind"] == "pointer" or f.get("mask"):
            plan.append(f)
        elif plan and plan[-1]["kind"] == "bytes" and plan[-1]["offset"] + plan[-1]["size"] == f["offset"]:
            plan[-1] = dict(plan[-1], size=plan[-1]["size"] + f["size"])
        else:
            plan.append({"name": "", "offset": f["offset"], "size": f["size"], "kind": "bytes", "target": None})
    return plan


def emit_table(lines, cname, entries, with_name):
    lines.append("static const TraceField %s[] = {" % cname)
    for e in entries:
        kind = {"scalar": "TF_SCALAR", "bytes": "TF_BYTES", "pointer": "TF_POINTER"}[e["kind"]]
        target = "TT_" + e["target"] if e["target"] else "TT_NONE"
        name = c_string(e["name"]) if with_name else '""'
        lines.append("    { %s, 0x%x, %d, %s, %s, 0x%x }," % (name, e["offset"], e["size"], kind, target,
                                                         e.get("mask", 0)))
    lines.append("};")
    lines.append("")


def generate():
    schema = Schema()
    lines = [
        "// Generated by tools/traceschema.py from include/ta_types.h. Do not edit.",
        "#ifndef TRACE_SCHEMA_H",
        "#define TRACE_SCHEMA_H",
        "",
        "enum TraceKind { TF_SCALAR, TF_BYTES, TF_POINTER };",
        "enum TraceTarget { TT_NONE, TT_HEAP, TT_UNIT, TT_PLAYER, TT_UNITDEF, TT_WEAPONDEF, TT_PROJECTILE,",
        "                   TT_FEATURE, TT_PIECE };",
        "",
        "struct TraceField {",
        "    const char* name;",
        "    unsigned offset;",
        "    unsigned size;",
        "    unsigned char kind;",
        "    unsigned char target;",
        "    unsigned mask;",
        "};",
        "",
        "struct TraceGlobal {",
        "    const char* name;",
        "    const void* address;",
        "    unsigned size;",
        "    unsigned char kind;",
        "    unsigned char target;",
        "};",
        "",
        "struct TraceTable {",
        "    const TraceField* fields;",
        "    unsigned fieldCount;",
        "    const TraceField* plan;",
        "    unsigned planCount;",
        "    unsigned size;",
        "};",
        "",
    ]
    tables = []
    for ctype, key in TYPE_ORDER:
        fields, size = finish(schema, ctype, key)
        plan = plan_of(fields)
        emit_table(lines, "k%sFields" % key, fields, True)
        emit_table(lines, "k%sPlan" % key, plan, False)
        tables.append((key, len(fields), len(plan), size))
    for key, nf, npl, size in tables:
        lines.append("static const TraceTable k%sTable = { k%sFields, %d, k%sPlan, %d, 0x%x };"
                     % (key, key, nf, key, npl, size))
    lines.append("")

    game_text = schema.struct_source("Game")[0]
    wind = []
    for name, ctype in WIND_FIELDS:
        m = re.search(r"\b%s;\s*//\s*\+0x([0-9a-f]+)" % name, game_text)
        if not m:
            schema.errors.append("Game has no field %s" % name)
            continue
        wind.append({"name": name, "offset": int(m.group(1), 16), "size": schema.size_of(ctype),
                     "kind": "scalar", "target": None})
    emit_table(lines, "kWindFields", wind, True)
    lines.append("static const unsigned kWindFieldCount = %d;" % len(wind))
    lines.append("")

    for _, symbol, size, _ in METEOR_GLOBALS:
        lines.append("extern %s;" % METEOR_DECLS.get(symbol, "int %s" % symbol))
    lines.append("")
    lines.append("static const TraceGlobal kMeteorGlobals[] = {")
    for name, symbol, size, target in METEOR_GLOBALS:
        kind = "TF_POINTER" if target else ("TF_BYTES" if size > 8 else "TF_SCALAR")
        lines.append("    { %s, &%s, %d, %s, %s }," % (c_string(name), symbol, size, kind,
                                                      "TT_" + target if target else "TT_NONE"))
    lines.append("};")
    lines.append("static const unsigned kMeteorGlobalCount = %d;" % len(METEOR_GLOBALS))
    lines.append("")
    lines.append("#endif")
    return "\n".join(lines) + "\n", schema.errors


def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    text, errors = generate()
    for e in errors:
        print("layout: " + e, file=sys.stderr)
    if args.check:
        sys.exit(0 if OUT.exists() and OUT.read_text() == text else 1)
    OUT.parent.mkdir(parents=True, exist_ok=True)
    if not OUT.exists() or OUT.read_text() != text:
        OUT.write_text(text, newline="\n")
        print("wrote %s" % OUT.relative_to(ROOT))
    else:
        print("unchanged %s" % OUT.relative_to(ROOT))


if __name__ == "__main__":
    main()
