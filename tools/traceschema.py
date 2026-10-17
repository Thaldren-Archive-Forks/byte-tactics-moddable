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
TRACE_FORMAT = 3
TYPES = ROOT / "include/ta_types.h"
OUT = ROOT / "src/harness/trace_schema.h"

BASE_SIZES: dict[str, int] = {
    "char": 1, "signed char": 1, "unsigned char": 1, "short": 2, "unsigned short": 2,
    "int": 4, "unsigned int": 4, "unsigned": 4, "long": 4, "unsigned long": 4,
    "float": 4, "double": 8, "__int64": 8, "unsigned __int64": 8,
}

POINTER_TARGETS: dict[str, str] = {
    "Unit": "UNIT", "Player": "PLAYER", "UnitDef": "UNITDEF", "WeaponDef": "WEAPONDEF",
    "Def_0049d270": "WEAPONDEF", "UnitMotion": "WEAPONDEF", "Obj_00406f50": "PROJECTILE",
    "Feature": "FEATURE", "Object3do": "PIECE", "Object3D": "PIECE",
    "Rec": "REC", "Node": "NODE", "MovementClass": "MOVECLASS",
    "OrderRef": "ORDER", "OrderFxRef": "ORDERFX", "LinkRef": "LINK",
}

NAMES: dict[str, dict[int, str]] = {
    "Unit": {0xA6: "unitDefIndex"},
    "Debris": {0x0: "obj"},
}

POINTERS: dict[str, dict[int, str]] = {
    "Unit": {0x0: "HEAP", 0x8: "HEAP", 0x24: "HEAP", 0x40: "HEAP", 0x82: "REC", 0x86: "UNIT"},
    "Player": {0x74: "HEAP"},
    "Projectile": {0x4E: "HEAP"},
    "Debris": {0x0: "PIECE"},
}

POINTER_NAMES: dict[tuple[str, int], str] = {}

EXCLUSIONS: list[tuple[str, str, str]] = [
    ("Eye", "screen", "points at the record's own screenPos"),
    ("Eye", "flagPtr", "points at the record's own flagB"),
    ("Node", "penalty", "NodeData(x, y, g, f, steps) leaves it unset for the start node; the visit hashes it when g is not 0"),
    ("Pathfinder", "capacity", "an allocation size, not behaviour: nothing reads it but the growth of the pool"),
    ("Pathfinder", "found_x", "written only when a search reaches its goal, read right after"),
    ("Pathfinder", "found_y", "written only when a search reaches its goal, read right after"),
    ("Order", "vptr", "the vtable, the same for every order"),
    ("Order", "linkVptr", "the vtable of the embedded PathOrderAttach"),
    ("Order", "cached", "written only by DrawWeaponCoverage (markers.cpp:563) and read only there (markers.cpp:557)"),
    ("Order", "next", "the chain: the visit walks it, so the order of the records carries it"),
    ("ApproachRadius", "vptr", "the class is hashed as a tag"),
    ("RingApproach", "vptr", "the class is hashed as a tag"),
    ("PointMarker", "vptr", "the class is hashed as a tag"),
    ("PathOrder", "vptr", "the class is hashed as a tag"),
    ("PathOrder", "refVptr", "the vtable of the embedded PathOrderAttach"),
    ("AirManeuverOrder", "vptr", "the class is hashed as a tag"),
    ("AirManeuverOrder", "saveOnly22", "set to 0 by the constructor and read only by SerializeToSave"),
    ("AirManeuverOrder", "value26", "set only by the loading constructors and read only by SerializeToSave"),
    ("UnitMotion", "obj", "the goal object: the visit hashes its class and fields"),
    ("UnitMotion", "lastMoveTick", "written by UpdatePosition (order_list.cpp:2001) and read by nothing"),
    ("PatrolGoal", "vptr", "the class is hashed as a tag"),
    ("PatrolGoal", "points", "only the first count points are live: the visit hashes those"),
    ("AiSearchGoal", "vptr", "the class is hashed as a tag"),
    ("AiSearchGoal", "points", "only the first count points are live: the visit hashes those"),
    ("PackedGoal", "vptr", "the class is hashed as a tag"),
    ("PackedGoal", "field26", "padding byte no constructor or method writes or reads"),
    ("ScriptState", "vptr", "the vtable of UnitScript"),
    ("ScriptState", "statics", "the visit hashes the staticCount ints it points at"),
    ("ScriptState", "pieces", "the visit hashes the pieceCount records it points at"),
    ("ScriptState", "channels", "the visit hashes the channels that are not idle"),
    ("ScriptState", "state", "the visit hashes the piece transforms it points at"),
    ("ScriptChannel", "stack", "only the words up to sp are live: the visit hashes those"),
    ("PieceTransform", "object", "the model piece, fixed when the state is built"),
    ("PieceTransform", "pos", "the world position UpdateObjectState derives from the transforms"),
    ("PieceTransform", "sfxOffset", "points at constant model data"),
    ("PieceTransform", "value", "a cache cleared when the piece changes"),
    ("PieceTransform", "rest", "bytes 0x2a to 0x35, which nothing in the script or the sim writes"),
    ("PlayerAI", "visible", "the visit hashes the size and the unit IDs it holds"),
    ("PlayerAI", "known", "the visit hashes the size and the unit IDs it holds"),
    ("PlayerAI", "factories", "the visit hashes the size and the unit IDs it holds"),
    ("PlayerAI", "cells", "the visit hashes the size and the elements it holds, never the begin and end pointers"),
    ("PlayerAI", "vec_65", "the visit hashes the size and the elements it holds"),
    ("PlayerAI", "counts", "the visit hashes the size and the elements it holds"),
    ("PlayerAI", "vec_8d", "the visit hashes the size and the elements it holds"),
    ("PlayerAI", "weights", "the visit hashes the size and the elements it holds"),
    ("PlayerAI", "vec_ad", "the visit hashes the size and the elements it holds"),
    ("PlayerAI", "vec_bd", "the visit hashes the size and the elements it holds"),
    ("PlayerAI", "values", "the visit hashes the size and the elements it holds"),
    ("PlayerAI", "locked", "the visit hashes the size and the elements it holds"),
    ("SquadManager", "timers", "the visit hashes the class and the fields of each timer"),
    ("SquadTimer", "vptr", "the class is hashed as a tag"),
    ("SquadTimer", "manager", "points back at the SquadManager that owns the timer"),
    ("SquadTimer", "group", "points into the Player's own groups, which the Player table hashes"),
    ("AssaultTimer", "vptr", "the class is hashed as a tag"),
    ("AssaultTimer", "manager", "points back at the SquadManager that owns the timer"),
    ("AssaultTimer", "group", "points into the Player's own groups, which the Player table hashes"),
    ("EscortTimer", "vptr", "the class is hashed as a tag"),
    ("EscortTimer", "manager", "points back at the SquadManager that owns the timer"),
    ("EscortTimer", "group", "points into the Player's own groups, which the Player table hashes"),
    ("SpatialTimer", "vptr", "the class is hashed as a tag"),
    ("SpatialTimer", "manager", "points back at the SquadManager that owns the timer"),
    ("SpatialTimer", "group", "points into the Player's own groups, which the Player table hashes"),
    ("Squad", "items", "the visit hashes the size and the unit IDs it holds, never the begin and end pointers"),
    ("MissionConditions", "victory", "the visit walks the condition objects"),
    ("MissionConditions", "defeat", "the visit walks the condition objects"),
    ("MissionCondition", "vptr", "the class is hashed as a tag"),
    ("ConditionNumUnits", "head", "the MissionCondition table hashes these bytes"),
    ("ConditionNumLeft", "head", "the MissionCondition table hashes these bytes"),
    ("MissionData", "text", "campaign, map names, descriptions and file names: nothing in a phase reads them"),
    ("MissionData", "minWindSpeed", "Game.windSpeedMin is the traced copy"),
    ("MissionData", "maxWindSpeed", "Game.windSpeedMax is the traced copy"),
    ("MissionData", "units", "the visit hashes the unit records and their names"),
    ("MissionData", "rules", "the visit hashes the rule records"),
    ("MissionData", "features", "the visit hashes the feature records and their names"),
    ("MissionData", "tail", "the memory and player count strings of the lobby: nothing in a phase reads them"),
    ("WeaponDefData", "description", "text shown to the player"),
    ("WeaponDefData", "fire", "SetWeaponFireHandler derives it from the flags"),
    ("WeaponDefData", "table", "the visit hashes the damage entries it points at"),
    ("WeaponDefData", "model", "render asset pointer"),
    ("WeaponDefData", "explosionSeq", "render asset pointer"),
    ("WeaponDefData", "splash", "render asset pointer"),
    ("WeaponDefData", "modelName", "name of the render model"),
    ("WeaponDefData", "shakeMagnitude", "screen shake, read only by drawing"),
    ("WeaponDefData", "shakeDuration", "screen shake, read only by drawing"),
    ("WeaponDefData", "sound", "sound index, read only by sound playback"),
    ("WeaponDefData", "soundHit", "sound index, read only by sound playback"),
    ("WeaponDefData", "soundWater", "sound index, read only by sound playback"),
    ("WeaponDefData", "pad100", "unnamed bytes no code reads or writes"),
    ("WeaponDefData", "shotKind", "how the shot is drawn"),
    ("WeaponDefData", "colour", "palette index of the line"),
    ("WeaponDefData", "colour2", "palette index of the second line"),
    ("WeaponDefData", "pad10f", "unnamed bytes no code reads or writes"),
    ("UnitDefData", "description", "text shown to the player"),
    ("UnitDefData", "data", "the loaded COB script: constant, read from the files"),
    ("UnitDefData", "soundcategory", "sound selection, read only by sound playback"),
    ("UnitDefData", "yardmap", "the visit hashes the footprint it points at"),
    ("UnitDefData", "ids", "the visit hashes the build list it points at"),
    ("UnitDefData", "weaponCategories", "the visit hashes the masks they point at"),
    ("UnitDefData", "categories", "the visit hashes the mask it points at"),
]

PATHFINDER_SEARCH_FIELDS = {
    "start_x", "start_y", "goalX", "goalY", "probe", "range", "path", "target", "owner", "turnCost", "stepCost",
}

SPLITS: dict[str, list[tuple[str, set[str]]]] = {
    "Pathfinder": [("PathfinderSearch", PATHFINDER_SEARCH_FIELDS)],
    "PathOrder": [("PathOrderHeading", {"heading"}), ("PathOrderPiece", {"piece"}),
                  ("PathOrderFollow", {"followOffset"}), ("PathOrderPosY", {"pos.y"})],
    "AirManeuverOrder": [("AirManeuverOrderHeading", {"heading"})],
    "Order": [("OrderPosY", {"pos.y"})],
    "PackedGoal": [("LiteGoalState", {"netDirty"})],
    "MissionData": [("MissionStartRest", {"startMetalRest", "startEnergyRest"})],
    "ScriptChannel": [("ScriptChannelDelay", {"delay"}), ("ScriptChannelWait", {"waiting"}),
                      ("ScriptChannelPiece", {"piece", "axis"})],
}

HEAP_TYPES: list[tuple[str, str]] = [
    ("Order", "Order"),
    ("ApproachRadius", "ApproachRadius"),
    ("RingApproach", "RingApproach"),
    ("PointMarker", "PointMarker"),
    ("PathOrder", "PathOrder"),
    ("AirManeuverOrder", "AirManeuverOrder"),
    ("UnitMotion", "UnitMotion"),
    ("PatrolGoal", "PatrolGoal"),
    ("AiSearchGoal", "AiSearchGoal"),
    ("PackedGoal", "PackedGoal"),
    ("ScriptState", "ScriptState"),
    ("ScriptChannel", "ScriptChannel"),
    ("ScriptPiece", "ScriptPiece"),
    ("PieceTransform", "PieceTransform"),
    ("PlayerAI", "PlayerAI"),
    ("SquadManager", "SquadManager"),
    ("SquadTimer", "SquadTimer"),
    ("AssaultTimer", "AssaultTimer"),
    ("EscortTimer", "EscortTimer"),
    ("SpatialTimer", "SpatialTimer"),
    ("Squad", "Squad"),
    ("UnitResources", "UnitResources"),
    ("MissionConditions", "MissionConditions"),
    ("MissionCondition", "MissionCondition"),
    ("ConditionNumUnits", "ConditionNumUnits"),
    ("ConditionNumLeft", "ConditionNumLeft"),
    ("MissionData", "MissionData"),
    ("WeaponDefData", "WeaponDefData"),
    ("UnitDefData", "UnitDefData"),
]

MASKS: dict[tuple[str, int], tuple[int, str]] = {
    ("ExplodedPiece", 0x28): (0x3F, "bits 6 to 31 are never written; StartExplodePiece copies them from an uninitialised stack header"),
    ("Order", 0x42): (0xFFDFFFFF, "bit 0x200000 is set by DrawWeaponCoverage (markers.cpp:564) and read only there (markers.cpp:556)"),
    ("UnitMotion", 0x2E): (0x07, "bits 3 to 7 are never written; the constructor sets mode and flag (order_list.cpp:2096)"),
    ("AiSearchGoal", 0x64): (0x0B, "bits 4 to 7 are never written; bit 2 is unset by the constructor and read only after SerializeNetUnitState writes it"),
    ("PackedGoal", 0x27): (0x07, "bits 3 to 7 are never written"),
    ("PieceTransform", 0x28): (0x07, "bits 3 to 15 are never written; SaveScriptState saves visible, cached and shaded"),
}

SYNTHETIC: dict[str, str] = {
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
SYNTHETIC.update({
    "Rec": """
        unsigned char a;
        unsigned char b;
        int flags;
        Unit* first;
    """,
    "Eye": """
        Player* player;
        void* screen;
        short x;
        unsigned char flagA;
        char flagB;
        void* flagPtr;
        Vec3 pos;
        unsigned int expires;
        int screenPos;
    """,
    "Node": """
        int index;
        short pos_x;
        short pos_y;
        int g;
        int f;
        short penalty;
        short steps;
    """,
    "MovementClass": """
        void* name;
        short footprintX;
        short footprintZ;
        short maxWaterDepth;
        short minWaterDepth;
        unsigned char maxSlope;
        unsigned char badSlope;
        unsigned char maxWaterSlope;
        unsigned char badWaterSlope;
        unsigned int width;
        unsigned int height;
        void* cells;
        unsigned int lastTick;
    """,
    "Pathfinder": """
        void* pool;
        void* items;
        int freeHead;
        int used;
        int capacity;
        int count;
        int topPopped;
        void* cells;
        unsigned int width;
        unsigned int height;
        int cellCount;
        void* dirty;
        short start_x;
        short start_y;
        short found_x;
        short found_y;
        int goalX;
        int goalY;
        int probe;
        int range;
        int stepsPerTick;
        int steps;
        int costScale;
        int baseScale;
        Unit* object;
        void* path;
        void* target;
        MovementClass* owner;
        unsigned char turnCost[8];
        unsigned char stepCost[8];
        unsigned char player;
        Unit* cursor[10];
        int budget[10];
    """,
})
SYNTHETIC.update({
    "Order": """
        char vptr[4];                 // +0x0
        unsigned char kind;           // +0x4
        unsigned char state;          // +0x5
        unsigned int flags6;          // +0x6
        unsigned int wakeFrame;       // +0xa
        Unit* unit;                   // +0xe
        char linkVptr[4];             // +0x12
        Unit* linkOwner;              // +0x16
        LinkRef* linkNext;            // +0x1a
        OrderRef* linkValue;          // +0x1e
        Vec3 pos;                     // +0x22
        Point16 start;                // +0x2e
        Point16 cached;               // +0x32
        int arg0;                     // +0x36
        int arg1;                     // +0x3a
        int arg2;                     // +0x3e
        unsigned int flags;           // +0x42
        unsigned int created;         // +0x46
        OrderRef* next;               // +0x4a
        unsigned int subFlags;        // +0x4e
        OrderFxRef* attached;         // +0x52
    """,
    "ApproachRadius": """
        char vptr[4];                 // +0x0
        OrderRef* source;             // +0x4
        Point16 pos;                  // +0x8
        int radius;                   // +0xc
        int radiusSq;                 // +0x10
    """,
    "RingApproach": """
        char vptr[4];                 // +0x0
        OrderRef* source;             // +0x4
        Point16 pos;                  // +0x8
        int radius2;                  // +0xc
        int radius1;                  // +0x10
        int radius2Sq;                // +0x14
        int radius1Sq;                // +0x18
    """,
    "PointMarker": """
        char vptr[4];                 // +0x0
        OrderRef* source;             // +0x4
        int x1;                       // +0x8
        int x2;                       // +0xc
        int y1;                       // +0x10
        int y2;                       // +0x14
    """,
    "PathOrder": """
        char vptr[4];                 // +0x0
        OrderRef* source;             // +0x4
        unsigned short flags;         // +0x8
        short approachRadius;         // +0xa
        short altitude;               // +0xc
        short heading;                // +0xe
        short piece;                  // +0x10
        Unit* unit;                   // +0x12
        char refVptr[4];              // +0x16
        Unit* refOwner;               // +0x1a
        LinkRef* refNext;             // +0x1e
        OrderRef* refValue;           // +0x22
        Vec3 pos;                     // +0x26
        int followOffset;             // +0x32
    """,
    "AirManeuverOrder": """
        char vptr[4];                 // +0x0
        OrderRef* source;             // +0x4
        unsigned short flags;         // +0x8
        Vec3 target;                  // +0xa
        Vec3 other;                   // +0x16
        short saveOnly22;             // +0x22
        unsigned short heading;       // +0x24
        unsigned short value26;       // +0x26
        Unit* self;                   // +0x28
    """,
    "UnitMotion": """
        char* obj;                    // +0x0
        MovementClass* movementClass; // +0x4
        Vec3 velocity;                // +0x8
        Vec3 p2;                      // +0x14
        int speed;                    // +0x20
        short turn;                   // +0x24
        int pathLockStamp;            // +0x26
        int lastMoveTick;             // +0x2a
        unsigned char flags;          // +0x2e
    """,
    "PatrolGoal": """
        char vptr[4];                 // +0x0
        OrderFxRef* target;           // +0x4
        Unit* owner;                  // +0x8
        char points[12];              // +0xc
        int count;                    // +0x18
    """,
    "AiSearchGoal": """
        char vptr[4];                 // +0x0
        OrderFxRef* target;           // +0x4
        Unit* owner;                  // +0x8
        char points[80];              // +0xc
        int count;                    // +0x5c
        unsigned int repathClaimTick; // +0x60
        unsigned char flags;          // +0x64
    """,
    "PackedGoal": """
        char vptr[4];                 // +0x0
        OrderFxRef* target;           // +0x4
        Unit* owner;                  // +0x8
        Vec3 pos;                     // +0xc
        Vec3 vel;                     // +0x18
        short heading;                // +0x24
        char field26;                 // +0x26
        unsigned char netDirty;       // +0x27
    """,
    "ScriptState": """
        char vptr[4];                 // +0x0
        int scale;                    // +0x4
        char* table;                  // +0x8
        int checksum;                 // +0xc
        char* statics;                // +0x10
        char* pieces;                 // +0x14
        int changed;                  // +0x18
        char channels[0x520];         // +0x1c
        int activeCount;              // +0x53c
        char* state;                  // +0x540
    """,
    "ScriptChannel": """
        unsigned int state;           // +0x0
        int pc;                       // +0x4
        int sp;                       // +0x8
        int delay;                    // +0xc
        int piece;                    // +0x10
        int axis;                     // +0x14
        int waiting;                  // +0x18
        int signal;                   // +0x1c
        char* callback;               // +0x20
        char stack[0x80];             // +0x24
    """,
    "ScriptPiece": """
        int active;                   // +0x0
        int move[3];                  // +0x4
        int moveSpeed[3];             // +0x10
        int turn[3];                  // +0x1c
        int turnSpeed[3];             // +0x28
        int spin[3];                  // +0x34
        int acceleration[3];          // +0x40
    """,
    "PieceTransform": """
        int object;                   // +0x0
        int translation[3];           // +0x4
        unsigned short rotation[3];   // +0x10
        Vec3 pos;                     // +0x16
        int* sfxOffset;               // +0x22
        short value;                  // +0x26
        unsigned short flags;         // +0x28
        char rest[12];                // +0x2a
    """,
})
SYNTHETIC.update({
    "PlayerAI": """
        Player* owner;                // +0x0
        unsigned char index;          // +0x4
        char visible[16];             // +0x5
        char known[16];               // +0x15
        char factories[16];           // +0x25
        Vec3 centre;                  // +0x35
        Vec3 pos_41;                  // +0x41
        char cells[16];               // +0x4d
        Point16 center;               // +0x5d
        int centerPad;                // +0x61
        char vec_65[16];              // +0x65
        int builders;                 // +0x75
        int hasSpecial;               // +0x79
        char counts[16];              // +0x7d
        char vec_8d[16];              // +0x8d
        char weights[16];             // +0x9d
        char vec_ad[16];              // +0xad
        char vec_bd[16];              // +0xbd
        char values[16];              // +0xcd
        char locked[16];              // +0xdd
        unsigned int lastTick;        // +0xed
        Point16 spacing0;             // +0xf1
        Point16 offset0;              // +0xf5
        int margin0;                  // +0xf9
        Point16 spacing1;             // +0xfd
        Point16 offset1;              // +0x101
        int margin1;                  // +0x105
        int searchRadius;             // +0x109
    """,
    "SquadManager": """
        Player* player;               // +0x0
        unsigned char index;          // +0x4
        int countdown;                // +0x5
        int unused_9;                 // +0x9
        int nextAction;               // +0xd
        char timers[0x28];            // +0x11
        Unit* cursor;                 // +0x39
    """,
    "SquadTimer": """
        char vptr[4];                 // +0x0
        char manager[4];              // +0x4
        char group[4];                // +0x8
        int next;                     // +0xc
        unsigned int player;          // +0x10
    """,
    "AssaultTimer": """
        char vptr[4];                 // +0x0
        char manager[4];              // +0x4
        char group[4];                // +0x8
        int next;                     // +0xc
        unsigned int player;          // +0x10
        int minimum;                  // +0x14
        int maximum;                  // +0x18
        int limit;                    // +0x1c
        int kind;                     // +0x20
        int attacking;                // +0x24
    """,
    "EscortTimer": """
        char vptr[4];                 // +0x0
        char manager[4];              // +0x4
        char group[4];                // +0x8
        int next;                     // +0xc
        unsigned int player;          // +0x10
        int other;                    // +0x14
    """,
    "SpatialTimer": """
        char vptr[4];                 // +0x0
        char manager[4];              // +0x4
        char group[4];                // +0x8
        int next;                     // +0xc
        unsigned int player;          // +0x10
        Vec3 best;                    // +0x14
        Vec3 probe;                   // +0x20
        Vec3 step;                    // +0x2c
        int bestRating;               // +0x38
    """,
    "Squad": """
        Player* owner;                // +0x0
        int index;                    // +0x4
        int field_8;                  // +0x8
        int field_c;                  // +0xc
        char items[16];               // +0x10
    """,
    "UnitResources": """
        float energyMake;             // +0x0
        float energyUse;              // +0x4
        float energyUsePaid;          // +0x8
        float energyStall;            // +0xc
        float energyMakePrev;         // +0x10
        float energyUsePrev;          // +0x14
        float metalMake;              // +0x18
        float metalUse;               // +0x1c
        float metalUsePaid;           // +0x20
        float metalStall;             // +0x24
        float metalMakePrev;          // +0x28
        float metalUsePrev;           // +0x2c
        Player* player;               // +0x30
    """,
    "MissionConditions": """
        char victory[0x40];           // +0x0
        int victoryCount;             // +0x40
        char defeat[0x40];            // +0x44
        int defeatCount;              // +0x84
        int active;                   // +0x88
    """,
    "MissionCondition": """
        char vptr[4];                 // +0x0
        int satisfied;                // +0x4
        int celebrated;               // +0x8
    """,
    "ConditionNumUnits": """
        char head[0x10];              // +0x0
        int numUnits;                 // +0x10
    """,
    "ConditionNumLeft": """
        char head[0x2c];              // +0x0
        int numLeftToKill;            // +0x2c
    """,
    "MissionData": """
        int type;                     // +0x0
        char text[0xd28];             // +0x4
        int multi;                    // +0xd2c
        int threshold;                // +0xd30
        int minWindSpeed;             // +0xd34
        int maxWindSpeed;             // +0xd38
        int gravity;                  // +0xd3c
        float tidalStrength;          // +0xd40
        int lavaWorld;                // +0xd44
        int noSeaLevelTrigger;        // +0xd48
        int waterDoesDamage;          // +0xd4c
        int waterDamage;              // +0xd50
        float killMul;                // +0xd54
        float timeMul;                // +0xd58
        float startMetal[2];          // +0xd5c
        char startMetalRest[32];      // +0xd64
        float startEnergy[2];         // +0xd84
        char startEnergyRest[32];     // +0xd8c
        char* units;                  // +0xdac
        int unitCount;                // +0xdb0
        char* rules;                  // +0xdb4
        int ruleCount;                // +0xdb8
        char* features;               // +0xdbc
        int featureCount;             // +0xdc0
        char tail[0x100];             // +0xdc4
    """,
    "WeaponDefData": """
        char name[0x20];              // +0x0
        char description[0x40];       // +0x20
        char* fire;                   // +0x60
        char* table;                  // +0x64
        int speed;                    // +0x68
        int startSpeed;               // +0x6c
        int acceleration;             // +0x70
        char* model;                  // +0x74
        char* explosionSeq;           // +0x78
        char* splash;                 // +0x7c
        char modelName[0x40];         // +0x80
        float energyPerShot;          // +0xc0
        float metalPerShot;           // +0xc4
        float pitch;                  // +0xc8
        int shakeMagnitude;           // +0xcc
        int shakeDuration;            // +0xd0
        unsigned short damage;        // +0xd4
        unsigned short areaOfEffect;  // +0xd6
        float edgeDamage;             // +0xd8
        int range;                    // +0xdc
        int coverage;                 // +0xe0
        unsigned short reloadTime;    // +0xe4
        unsigned short lifetime;      // +0xe6
        unsigned short turnRate;      // +0xe8
        unsigned short burst;         // +0xea
        unsigned short burstRate;     // +0xec
        unsigned short spread;        // +0xee
        unsigned short duration;      // +0xf0
        unsigned short lifeRand;      // +0xf2
        unsigned short sound;         // +0xf4
        unsigned short soundHit;      // +0xf6
        unsigned short soundWater;    // +0xf8
        unsigned short smokeRate;     // +0xfa
        unsigned short flightTime;    // +0xfc
        unsigned short holdTime;      // +0xfe
        char pad100[4];               // +0x100
        short accuracy;               // +0x104
        unsigned short tolerance;     // +0x106
        unsigned short pitchTolerance; // +0x108
        unsigned char index;          // +0x10a
        unsigned char fireStarter;    // +0x10b
        unsigned char shotKind;       // +0x10c
        unsigned char colour;         // +0x10d
        unsigned char colour2;        // +0x10e
        char pad10f[2];               // +0x10f
        unsigned int flags;           // +0x111
    """,
    "UnitDefData": """
        char name[0x20];              // +0x0
        char unitname[0x20];          // +0x20
        char description[0x40];       // +0x40
        char objectname[0x20];        // +0x80
        char side[0x1e];              // +0xa0
        char aiWeight[0x40];          // +0xbe
        char aiLimit[0x40];           // +0xfe
        unsigned int fbiChecksum;     // +0x13e
        int scriptChecksum;           // +0x142
        int weaponHashXor;            // +0x146
        short footprintx;             // +0x14a
        short footprintz;             // +0x14c
        char* yardmap;                // +0x14e
        int count;                    // +0x152
        char* ids;                    // +0x156
        int maxThisUnit;              // +0x15a
        int modelBox[9];              // +0x15e
        int radius;                   // +0x182
        float energyCost;             // +0x186
        float metalCost;              // +0x18a
        char* data;                   // +0x18e
        int maxvelocity;              // +0x192
        int maxslopevelocity;         // +0x196
        int brakerate;                // +0x19a
        int acceleration;             // +0x19e
        int bankscale;                // +0x1a2
        int pitchscale;               // +0x1a6
        int damagemodifier;           // +0x1aa
        int moverate1;                // +0x1ae
        int moverate2;                // +0x1b2
        MovementClass* movementclass; // +0x1b6
        short turnrate;               // +0x1ba
        short corpse;                 // +0x1bc
        short maxwaterdepth;          // +0x1be
        short minwaterdepth;          // +0x1c0
        float energymake;             // +0x1c2
        float energyuse;              // +0x1c6
        float metalmake;              // +0x1ca
        float extractsMetal;          // +0x1ce
        float windgenerator;          // +0x1d2
        float tidalgenerator;         // +0x1d6
        float cloakcost;              // +0x1da
        float cloakcostmoving;        // +0x1de
        float energystorage;          // +0x1e2
        float metalstorage;           // +0x1e6
        int buildtime;                // +0x1ea
        WeaponDef* weapons[3];        // +0x1ee
        unsigned int maxHealth;       // +0x1fa
        unsigned short buildRate;     // +0x1fe
        short healtime;               // +0x200
        short range;                  // +0x202
        short radardistance;          // +0x204
        short sonardistance;          // +0x206
        short mincloakdistance;       // +0x208
        short radardistancejam;       // +0x20a
        short sonardistancejam;       // +0x20c
        short soundcategory;          // +0x20e
        short buildangle;             // +0x210
        unsigned short buildRange;    // +0x212
        short maneuverleashlength;    // +0x214
        unsigned short attackrunlength; // +0x216
        short kamikazedistance;       // +0x218
        short sortbias;               // +0x21a
        short altitude;               // +0x21c
        unsigned short id;            // +0x21e
        WeaponDef* explodeas;         // +0x220
        WeaponDef* selfdestructas;    // +0x224
        unsigned char maxslope;       // +0x228
        unsigned char maxwaterslope;  // +0x229
        unsigned char capacity;       // +0x22a
        char transportcapacity;       // +0x22b
        unsigned char draft;          // +0x22c
        unsigned char makesMetal;     // +0x22d
        unsigned char buildMenuPageCount; // +0x22e
        unsigned char mobile;         // +0x22f
        char defaultmissiontype;      // +0x230
        UnitTypeSet* weaponCategories[3]; // +0x231
        UnitTypeSet* categories;      // +0x23d
        unsigned int flags1;          // +0x241
        unsigned int flags2;          // +0x245
    """,
})
SYNTHETIC_SIZES: dict[str, int] = {
    "PlayerAI": 0x10D, "SquadManager": 0x3D, "SquadTimer": 0x14, "AssaultTimer": 0x28, "EscortTimer": 0x18,
    "SpatialTimer": 0x3C, "Squad": 0x20, "UnitResources": 0x34, "MissionConditions": 0x8C,
    "MissionCondition": 0xC, "ConditionNumUnits": 0x14, "ConditionNumLeft": 0x30, "MissionData": 0xEC4,
    "WeaponDefData": 0x115, "UnitDefData": 0x249,
    "Order": 0x56, "ApproachRadius": 0x14, "RingApproach": 0x1C, "PointMarker": 0x18, "PathOrder": 0x36,
    "AirManeuverOrder": 0x2C, "UnitMotion": 0x2F, "PatrolGoal": 0x1C, "AiSearchGoal": 0x65, "PackedGoal": 0x28,
    "ScriptState": 0x544, "ScriptChannel": 0xA4, "ScriptPiece": 0x4C, "PieceTransform": 0x36,
    "ExplodedPiece": 0x30, "PieceRec": 0x36, "Face": 0x20, "SpotObject": 0x30, "SpotAnim": 0x30,
    "Rec": 0xA, "Eye": 0x24, "Node": 0x14, "MovementClass": 0x20, "Pathfinder": 0xC9,
}

TYPE_ORDER: list[tuple[str, str]] = [
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
    ("Rec", "Rec"),
    ("Eye", "Eye"),
    ("Node", "Node"),
    ("MovementClass", "MoveClass"),
]

GAME_SCALARS: list[tuple[str, int, int, str | None, int]] = [
    ("numPlayers", 0x2A3C, 2, None, 0),
    ("localPlayer", 0x2A42, 1, None, 0),
    ("playerIndex", 0x2A43, 1, None, 0),
    ("netGameFlags", 0x2A44, 2, None, 0),
    ("spotFeatureUnit", 0x1420F, 4, "UNIT", 0),
    ("spotUsedHead", 0x14213, 4, None, 0),
    ("spotRestHead", 0x14217, 4, None, 0),
    ("spotFreeHead", 0x1421B, 4, None, 0),
    ("spotField_1421f", 0x1421F, 4, "HEAP", 0),
    ("scanIndex", 0x14257, 4, None, 0),
    ("mapFlags", 0x14281, 2, None, 0xFFF7),
    ("endGameCountdown", 0x39239, 2, None, 0),
    ("endGameFlags", 0x3923B, 2, None, 0),
    ("uiOptionFlags", 0x37F2F, 2, None, 0x788),
]

SCALAR_GLOBALS: list[tuple[str, str, int, str | None]] = [
    ("cdPathMismatch", "g_cdPathMismatch", 4, None),
    ("cdMismatchDefeatTick", "g_cdMismatchDefeatTick", 4, None),
    ("economyPacketTick", "g_economyPacketTick", 4, None),
]

PATH_GLOBALS: list[tuple[str, str, int, str | None]] = [
    ("playerTickLoad", "g_playerTickLoad", 40, None),
    ("playerBudgetCap", "g_playerBudgetCap", 40, None),
    ("budgetRefreshCounter", "g_budgetRefreshCounter", 4, None),
]

WIND_FIELDS: list[tuple[str, str]] = [
    ("windCounter", "unsigned int"), ("windSpeedDenominator", "int"), ("windX", "int"),
    ("windY", "int"), ("windZ", "int"), ("windDirection", "unsigned short"),
    ("windSpeed", "int"), ("windStrength", "float"), ("windEnabled", "int"),
    ("windSpeedMin", "int"), ("windSpeedMax", "int"),
]

METEOR_GLOBALS: list[tuple[str, str, int, str | None]] = [
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
GLOBAL_DECLS: dict[str, str] = {
    "g_cdMismatchDefeatTick": "unsigned int g_cdMismatchDefeatTick",
    "g_playerTickLoad": "int g_playerTickLoad[10]",
    "g_playerBudgetCap": "int g_playerBudgetCap[10]",
}
METEOR_DECLS: dict[str, str] = {
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
        if (key, label) in excluded or (key, base_name(leaf)) in excluded or (key, "+0x%x" % leaf["offset"]) in excluded:
            continue
        out.append(leaf)
    return out, size


def c_string(text):
    return '"%s"' % text


def base_name(leaf):
    return leaf["name"].split(".")[0].split("[")[0]


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
        "#define TRACE_FORMAT %d" % TRACE_FORMAT,
        "",
        "enum TraceKind { TF_SCALAR, TF_BYTES, TF_POINTER };",
        "enum TraceTarget { TT_NONE, TT_HEAP, TT_UNIT, TT_PLAYER, TT_UNITDEF, TT_WEAPONDEF, TT_PROJECTILE,",
        "                   TT_FEATURE, TT_PIECE, TT_REC, TT_NODE, TT_MOVECLASS, TT_ORDER, TT_ORDERFX, TT_LINK };",
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
    for ctype, key in [("Pathfinder", "Pathfinder")] + HEAP_TYPES:
        fields, size = finish(schema, ctype, key)
        parts = SPLITS.get(key, [])
        taken = set().union(*[names for _, names in parts]) if parts else set()
        pieces = [(key, [f for f in fields if base_name(f) not in taken and f["name"] not in taken])]
        pieces += [(part, [f for f in fields if base_name(f) in names or f["name"] in names]) for part, names in parts]
        for part_key, part in pieces:
            plan = plan_of(part)
            emit_table(lines, "k%sFields" % part_key, part, True)
            emit_table(lines, "k%sPlan" % part_key, plan, False)
            tables.append((part_key, len(part), len(plan), size))
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

    scalars = []
    game_text = schema.struct_source("Game")[0]
    for name, offset, size, target, mask in GAME_SCALARS:
        has_field = re.search(r"//\s*\+0x%x\b" % offset, game_text) or re.search(r"\bunknown_%x\[" % offset, game_text)
        if not has_field and offset not in (0x14217,):
            schema.errors.append("Game has no field at %#x for %s" % (offset, name))
        entry = {"name": name, "offset": offset, "size": size, "kind": "pointer" if target else "scalar",
                 "target": target}
        if mask:
            entry["mask"] = mask
        scalars.append(entry)
    emit_table(lines, "kGameScalarFields", scalars, True)
    lines.append("static const unsigned kGameScalarFieldCount = %d;" % len(scalars))
    lines.append("")

    declared = set()
    for group in (METEOR_GLOBALS, SCALAR_GLOBALS, PATH_GLOBALS):
        for _, symbol, size, _ in group:
            if symbol in declared:
                continue
            declared.add(symbol)
            decl = METEOR_DECLS.get(symbol) or GLOBAL_DECLS.get(symbol) or "int %s" % symbol
            lines.append("extern %s;" % decl)
    lines.append("")
    for cname, group in (("kMeteorGlobals", METEOR_GLOBALS), ("kScalarGlobals", SCALAR_GLOBALS),
                         ("kPathGlobals", PATH_GLOBALS)):
        lines.append("static const TraceGlobal %s[] = {" % cname)
        for name, symbol, size, target in group:
            kind = "TF_POINTER" if target else ("TF_BYTES" if size > 8 else "TF_SCALAR")
            lines.append("    { %s, &%s, %d, %s, %s }," % (c_string(name), symbol, size, kind,
                                                          "TT_" + target if target else "TT_NONE"))
        lines.append("};")
        lines.append("static const unsigned %sCount = %d;" % (cname, len(group)))
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
