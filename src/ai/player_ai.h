// PlayerAI: the per-player AI state (g_playerAI[player], built by CreatePlayerAI
// at 0x40b320): the owner and its index, the visible, known and factory unit
// lists, the base centre, the placement cells, and the per-unit-type rating,
// count and weight tables. The one declaration of the class, for ai_player.cpp
// and the files that hold its other methods; the types behind the pointers
// stay private to their own files.
//
// Declare Point16 (src/util/vec3.h, or a view of it) and include <vector>
// before this header: the members hold both by value.
#ifndef PLAYER_AI_H
#define PLAYER_AI_H

#include "../state/sim_allocator.h"

struct Player;
struct Unit;
struct UnitDef;
struct Vec3;

// A map cell and its sort key, the element of the player AI's cell vector.
struct Elem_0040cc40 {
    Point16 pos;                       // +0x0
    float key;                         // +0x4
    Elem_0040cc40() {}
    // FUNCTION: 0x40a5b0
    Elem_0040cc40(const Elem_0040cc40& o) : pos(o.pos), key(o.key) {}
    // The value is taken as a float parameter: gives the original's fld/fstp copy.
    Elem_0040cc40(short x, short y, float k) { pos.x = x; pos.y = y; key = k; }
    bool operator<(const Elem_0040cc40& o) const { return key < o.key; }
};

struct Elem_0040cfb0 {
    char a;
    char b;
    char c;
};

struct Elem_0040d4f0 {
    unsigned char value;
};

struct Elem_0040d550 {
    int unknown_0;
};

struct Pos_00409160 {
    short x, y;
    int unknown_4;
    Pos_00409160(short ax, short ay) : unknown_4(0) { x = ax; y = ay; }
};

struct Vec3_00409160 {
    int x, y, z;
    Vec3_00409160(int ax, int ay, int az) : x(ax), y(ay), z(az) {}
    Vec3_00409160() { *this = Vec3_00409160(0, 0, 0); }
};

struct UnitList_00409160 {
    SimVector<Unit*> units;
    void Clear() { units.clear(); }
};

struct Group_00409160 {
    UnitList_00409160 list;
    void Clear() { list.Clear(); }
};

class PlayerAI {
public:
    Player* owner;                     // +0x00
    unsigned char index;               // +0x04
    // Wrapper depth decides which vector constructors inline: visible and
    // known are one level deep, factories two.
    UnitList_00409160 visible;         // +0x05
    UnitList_00409160 known;           // +0x15
    Group_00409160 factories;          // +0x25
    Vec3_00409160 centre;              // +0x35
    Vec3_00409160 pos_41;              // +0x41
    SimVector<Elem_0040cc40> cells;  // +0x4d
    Pos_00409160 center;               // +0x5d
    SimVector<Elem_0040cfb0> vec_65; // +0x65
    int builders;                      // +0x75
    int hasSpecial;                    // +0x79
    SimVector<short> counts;         // +0x7d
    SimVector<unsigned char> vec_8d; // +0x8d
    SimVector<unsigned char> weights; // +0x9d
    SimVector<Elem_0040d4f0> vec_ad; // +0xad
    SimVector<Elem_0040d550> vec_bd; // +0xbd
    SimVector<Elem_0040d550> values; // +0xcd
    SimVector<Elem_0040d550> locked; // +0xdd
    unsigned int lastTick;             // +0xed
    Point16 spacing0;                  // +0xf1
    Point16 offset0;                   // +0xf5
    int margin0;                       // +0xf9
    Point16 spacing1;                  // +0xfd
    Point16 offset1;                   // +0x101
    int margin1;                       // +0x105
    int searchRadius;                  // +0x109

    PlayerAI(unsigned char player);
    void InitUnitTables();
    void InitPlacementGrid();
    void ComputeBaseWeights();
    bool FindCellNearFeatures(UnitDef* type, Vec3* pos, SimVector<Elem_0040cc40>* list, int range, Point16* out);
    bool FindRandomPlacementCell(UnitDef* type, Vec3* pos, int range, Point16* out);
    void BuildFeatureCells();
    void RefreshUnitLists();
    void UpdateEveryThirtyTicks();
};

#define PLAYER_AI_OFFSET(field, off) \
    static_assert(__builtin_offsetof(PlayerAI, field) == off, "PlayerAI::" #field)
PLAYER_AI_OFFSET(visible, 0x05);
PLAYER_AI_OFFSET(known, 0x15);
PLAYER_AI_OFFSET(factories, 0x25);
PLAYER_AI_OFFSET(cells, 0x4d);
PLAYER_AI_OFFSET(vec_65, 0x65);
PLAYER_AI_OFFSET(counts, 0x7d);
PLAYER_AI_OFFSET(vec_8d, 0x8d);
PLAYER_AI_OFFSET(weights, 0x9d);
PLAYER_AI_OFFSET(vec_ad, 0xad);
PLAYER_AI_OFFSET(vec_bd, 0xbd);
PLAYER_AI_OFFSET(values, 0xcd);
PLAYER_AI_OFFSET(locked, 0xdd);
#undef PLAYER_AI_OFFSET
static_assert(sizeof(PlayerAI) == 0x10d, "PlayerAI size");

#endif
