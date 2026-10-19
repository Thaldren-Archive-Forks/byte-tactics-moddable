// Decompiled by Opus, space-bunny-free, Sonnet 5.5, deepseek-v4.1-flash, GPT-6.1-sol, claude-opus-5-5, mimo-v2.6-pro, GPT-6, DeepSeek V4.1 Flash, deepseek-v4.1, Space Bunny Free, Haiku, Claude Opus 5.5, Claude Sonnet 5.5, claude-sonnet-5-5 and Sonnet. Names are provisional.
// The unit code: map cell and height helpers, the highest point along a line, the unit memory pool, creating
// a unit (InitUnitFromType, InitUnitScript, InitUnit, CreateUnit and the
// network CreateUnitFromPacket) and killing units (KillUnit, ApplyUnitDeath).

#include <stdio.h>
#include "../state/block.h"
// Included only for its declarations: the symbol counter of GetCellMeanHeight needs them.
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

#include "../map/mission.h"

class MissionConditions;
class CobScript;
class UnitMotion;
class UnitResources;
struct ObjectState_00485d40;
struct Object3do;                      // object definition, only passed on

#pragma pack(push, 1)
struct Cell {
    unsigned short unit;               // +0x0
    char unknown_2[0x4 - 0x2];
    unsigned char height;              // +0x4
    unsigned char high;                // +0x5
    unsigned char low;                 // +0x6
    char metal;
    unsigned short feature;            // +0x8
    char unknown_a[0xd - 0xa];
};

#include "../util/vec3.h"

struct Entry_486360 {
    char unknown_0[0xf4];
    unsigned short next;               // +0xf4
    char unknown_f6[0x100 - 0xf6];
};

#include "../network/player_info.h"

// Unused here: a real function declared to keep the file's symbol count (docs/c2-regalloc.md).
int StepTowards(int, int, int);

struct Unit;

#include "../network/player.h"

#include "name_004864b0.h"

#include "cob_types.h"

union TypeFlags {                      // the type's word at +0x241
    unsigned int all;
    struct {
        unsigned int movOrder : 2;     // bits 0-1
        unsigned int fireOrder : 2;    // bits 2-3
        unsigned int initCloaked : 1;
        unsigned int b5 : 1;
        unsigned int b6 : 1;
        unsigned int b7 : 1;           // bit 7
        unsigned int b8 : 1;
        unsigned int b9 : 1;           // bit 9
        unsigned int b10 : 1;
        unsigned int b11 : 1;
        unsigned int b12 : 1;
        unsigned int b13 : 1;
        unsigned int b14 : 1;
        unsigned int b15 : 1;
        unsigned int hi : 16;          // bits 16-31
    } bits;
    struct {
        unsigned int lo : 18;
        unsigned int bit18 : 1;
        unsigned int mid : 5;
        unsigned int bit24 : 1;
        unsigned int top : 7;
    };
};

// A view of UnitDef (units/unit_def.h): its bit-field flag words and ShortPair
// offset do not fit that header, so it stays a view of its own.
struct UnitType {                      // 0x249 bytes
    char unknown_0[0x20];
    char name[0x20];                   // +0x20
    char unknown_40[0x14a - 0x40];
    Point16 offset;
    char unknown_14e[0x15a - 0x14e];
    int limit;                         // +0x15a
    char unknown_15e[0x16e - 0x15e];
    union {
        int modelMaxY;                 // +0x16e
        struct {
            short unknown_16e;
            short x170;                // +0x170
        };
    };
    char unknown_172[0x18a - 0x172];
    float x18a;                        // +0x18a
    ScriptTable* data;               // +0x18e
    char unknown_192[0x1bc - 0x192];
    unsigned short corpse;             // +0x1bc
    char unknown_1be[0x1fa - 0x1be];
    union {
        unsigned int maxHealth;        // +0x1fa
        short hp;
    };
    char unknown_1fe[0x202 - 0x1fe];
    short x202;                        // +0x202
    char unknown_204[0x210 - 0x204];
    unsigned short buildangle;         // +0x210
    char unknown_212[0x22e - 0x212];
    unsigned char buildMenuPageCount;  // +0x22e
    unsigned char mobile;              // +0x22f
    char unknown_230[0x241 - 0x230];
    TypeFlags flags;                   // +0x241
    char unknown_245[0x249 - 0x245];
};

struct Fixed_00485070 {
    unsigned short frac;
    short whole;
};

struct Pos_00485070 {
    Fixed_00485070 x;                  // +0x0
    Fixed_00485070 y;                  // +0x4
    Fixed_00485070 z;                  // +0x8
};



struct FixedParts_004853b0 {
    unsigned int frac : 16;
    int whole : 16;
};

union Fixed_004853b0 {
    int value;
    FixedParts_004853b0 parts;
};

struct Vec3_004853b0 {
    Fixed_004853b0 x;
    Fixed_004853b0 y;
    Fixed_004853b0 z;
};

// This header's symbol ids replace those of the local aim callback view
// (docs/c2-regalloc.md).
#include "../weapons/unit_weapon_slot.h"

class SpatialBucket {
public:
    char unknown_0[6];
    void* head;
    void UnlinkUnit(void* node);
};





#include "../game/unit_resources.h"

// This header's symbol ids replace those of three declarations that were here
// to keep 0x485140 and 0x4854a0 matching (docs/c2-regalloc.md).
#include "../util/angles.h"

#include "unit.h"

#include "../game/game.h"
#include "../quirks/quirks.h"
#pragma pack(pop)

// GLOBAL: 0x511de8
extern Game* g_game;

struct Point16_00485010 {
    short x;
    short y;
};

static inline Cell* CellAt(int x, int y)
{
    if (x >= 0 && x < g_game->width && y >= 0 && y < g_game->height)
        return &g_game->cells[y * g_game->width + x];
    return 0;
}

// FUNCTION: 0x485010
int __stdcall GetCellHeight(Point16_00485010* p)
{
    Cell* cell = CellAt(p->x, p->y);
    if (cell)
        return cell->height;
    return 0;
}

// Returns the average of the two height bytes (+5, +6) of the map cell under a
// 16.16 fixed-point position, or -1 off the map.

static inline Cell* GetCellCachedWidth(int x, int y)
{
    if (x >= 0) {
        // Width read into its own local after the x >= 0 test: decides the register tie.
        int w = g_game->width;
        if (x < w && y >= 0 && y < g_game->height)
            return &g_game->cells[w * y + x];
    }
    return 0;
}

// FUNCTION: 0x485140
int __stdcall GetCellMeanHeight(Pos_00485070* p)
{
    // One statement for x and y: settles the order of the two height-byte loads.
    int x = p->x.whole / 16, y = p->z.whole / 16;
    Cell* cell = GetCellCachedWidth(x, y);
    if (cell)
        return (cell->low + cell->high) >> 1;
    return -1;
}

// Converts map cell (x, y) to a 16.16 fixed-point position (cell * 16 in x
// and z, the cell's height byte in y); leaves `out` unchanged off the map.
// The zeroing is an inlined memset (a zero register stored three times
// through a copy of the pointer).

// FUNCTION: 0x485330
void __stdcall GetCellPosition(int x, int y, Pos_00485070* out)
{
    Cell* cell = CellAt(x, y);
    if (cell) {
        memset(out, 0, sizeof(*out));
        out->x.whole = x << 4;
        out->y.whole = cell->height;
        out->z.whole = y << 4;
    }
}

// Clamps a 16.16 fixed-point position's x and z to the map
// ([0, mapWidthWorld) and [0, mapHeightWorld) in whole units).

// Built as a bitfield struct in a local: plain integer arithmetic folds (size - 1) << 16.
static inline Fixed_004853b0 MakeFixed(int i)
{
    Fixed_004853b0 f;
    f.parts.frac = 0;
    f.parts.whole = i;
    return f;
}

// FUNCTION: 0x4853b0
void __stdcall ClampPositionToMap(Vec3_004853b0* p)
{
    if (p->x.value < 0)
        p->x.value = 0;
    else if (p->x.value >= MakeFixed(g_game->baseX).value)
        p->x = MakeFixed(g_game->baseX - 1);
    if (p->z.value < 0)
        p->z.value = 0;
    else if (p->z.value >= MakeFixed(g_game->baseY).value)
        p->z = MakeFixed(g_game->baseY - 1);
}


// Copies one player's visibility bit into another player's bit on every cell.
// FUNCTION: 0x485420
void __stdcall ShareMapInfo(unsigned char from, unsigned char to)
{
    unsigned short fromBit = 1 << from;
    unsigned short toBit = 1 << to;
    int n = g_game->width * g_game->height / 4;
    for (int i = 0; i < n; i++) {
        unsigned short* p = &g_game->visibilityMask[i];
        if (*p & fromBit)
            *p |= toBit;
    }
}

// This translation unit stands in for <algorithm> with __stdcall
// instantiations, because the original was built with /Gz and its sort helpers
// are callee-clean (0x488810, 0x488920, 0x488960). The three helper templates
// carry their real global names (QuickSort/InsertShift/Partition) so
// the recursive and out-of-line calls at 0x485691/0x48574b/0x48576d/0x485778
// reference the names data/symbols.csv already has, while the body and
// one-level inline stay exactly as the std:: originals. The std headers are
// included for their __cdecl helper templates (copy_backward) and type traits.

#include <iterator>
#include "../../interfaces/seams.h"
#include <xutility>

#define _ALGORITHM_

template<class _RI, class _Ty, class _Pr> void __stdcall QuickSort(_RI _F, _RI _L, _Pr _P, _Ty *);
template<class _RI, class _Ty, class _Pr> _RI __stdcall Partition(_RI _F, _RI _L, _Ty _Piv, _Pr _P);
template<class _RI, class _Ty, class _Pr> void __stdcall InsertShift(_RI _L, _Ty _V, _Pr _P);

namespace std {
const int _CHUNK_SIZE = 7;
const int _SORT_MAX = 16;

template<class _Ty, class _Pr> inline _Ty __stdcall _Median(_Ty _X, _Ty _Y, _Ty _Z, _Pr _P)
    {if (_P(_X, _Y))
        return (_P(_Y, _Z) ? _Y : _P(_X, _Z) ? _Z : _X);
    else
        return (_P(_X, _Z) ? _X : _P(_Y, _Z) ? _Z : _Y); }

template<class _FI1, class _FI2> inline void __stdcall iter_swap(_FI1 _X, _FI2 _Y)
    {_Iter_swap(_X, _Y, _Val_type(_X)); }
template<class _FI1, class _FI2, class _Ty> inline void __stdcall _Iter_swap(_FI1 _X, _FI2 _Y, _Ty *)
    {_Ty _Tmp = *_X;
    *_X = *_Y, *_Y = _Tmp; }

template<class _RI, class _Pr> inline void __stdcall sort(_RI _F, _RI _L, _Pr _P)
    {_Sort_0(_F, _L, _P, _Val_type(_F)); }
template<class _RI, class _Ty, class _Pr> inline void __stdcall _Sort_0(_RI _F, _RI _L, _Pr _P, _Ty *)
    {if (_L - _F <= _SORT_MAX)
        _Insertion_sort(_F, _L, _P);
    else
        {QuickSort(_F, _L, _P, (_Ty *)0);
        _Insertion_sort(_F, _F + _SORT_MAX, _P);
        for (_F += _SORT_MAX; _F != _L; ++_F)
            InsertShift(_F, _Ty(*_F), _P); }}
template<class _RI, class _Pr> inline void __stdcall _Insertion_sort(_RI _F, _RI _L, _Pr _P)
    {_Insertion_sort_1(_F, _L, _P, _Val_type(_F)); }
template<class _RI, class _Ty, class _Pr> inline void __stdcall _Insertion_sort_1(_RI _F, _RI _L, _Pr _P, _Ty *)
    {if (_F != _L)
        for (_RI _M = _F; ++_M != _L; )
            {_Ty _V = *_M;
            if (!_P(_V, *_F))
                InsertShift(_M, _V, _P);
            else
                {copy_backward(_F, _M, _M + 1);
                *_F = _V; }}}
}

template<class _RI, class _Ty, class _Pr> void __stdcall InsertShift(_RI _L, _Ty _V, _Pr _P)
    {for (_RI _M = _L; _P(_V, *--_M); _L = _M)
        *_L = *_M;
    *_L = _V; }

template<class _RI, class _Ty, class _Pr> _RI __stdcall Partition(_RI _F, _RI _L, _Ty _Piv, _Pr _P)
    {for (; ; ++_F)
        {for (; _P(*_F, _Piv); ++_F)
            ;
        for (; _P(_Piv, *--_L); )
            ;
        if (_L <= _F)
            return (_F);
        std::iter_swap(_F, _L); }}

template<class _RI, class _Ty, class _Pr> void __stdcall QuickSort(_RI _F, _RI _L, _Pr _P, _Ty *)
    {for (; std::_SORT_MAX < _L - _F; )
        {_RI _M = Partition(_F, _L, std::_Median(_Ty(*_F),
            _Ty(*(_F + (_L - _F) / 2)), _Ty(*(_L - 1)), _P), _P);
        if (_L - _M <= _M - _F)
            QuickSort(_M, _L, _P, std::_Val_type(_F)), _L = _M;
        else
            QuickSort(_F, _M, _P, std::_Val_type(_F)), _F = _M; }}

#include <algorithm>

void* __cdecl GameAllocIgnoreTag(const char* name, unsigned int size);
int __stdcall ComparePlayers(Player* a, Player* b);

// FUNCTION: 0x4854a0
void __stdcall AllocateUnitMemory(void)
{
    g_game->focusUnitId = 0;
    g_game->autoFollowFlags_14373 &= 0xfffffffd;
    g_game->unitsPerPlayer = g_game->field_37ee6;
    g_game->poolCount = (unsigned short)(g_game->field_37ee6 * 10 + 1);

    unsigned char* pool = (*(unsigned char **)&g_game->units) = (unsigned char*)SimAllocTagged("UNIT MEMORY", g_game->poolCount * 0x118);
    memset(pool, 0, g_game->poolCount * 0x118);

    unsigned int ten = g_game->field_37ee6 * 10;
    (*(void **)&g_game->list_1435f) = GameAllocIgnoreTag("HOT UNITS", ten * 2);
    g_game->hotRadar = GameAllocIgnoreTag("HOT RADAR UNITS", ten * 10);
    (*(unsigned char **)&g_game->unitsEnd) = (unsigned char *)g_game->units + g_game->poolCount * 0x118 - 0x118;

    unsigned short n;
    for (n = 0; n < g_game->poolCount; n++) {
        *(unsigned short*)(pool + n * 0x118 + 0xa8) = n;
        *(UnitType**)(pool + n * 0x118 + 0x92) = (UnitType *)g_game->defs;
    }

    Player* v[10];
    int k;
    for (k = 0; k < 10; k++)
        v[k] = (Player*)((unsigned char*)g_game->players + k * 0x14b);

    std::sort(v, v + 10, ComparePlayers);

    pool[0xff] = 0xff;
    *(unsigned int*)(pool + 0x96) = 0;
    int i;
    for (i = 0; i < 10; i++) {
        Player* item = v[i];
        int c = g_game->field_37ee6 * i + 1;
        *(unsigned char**)((char*)item + 0x67) = pool + c * 0x118;
        *(unsigned char**)((char*)item + 0x6b) =
            *(unsigned char**)((char*)item + 0x67) + g_game->field_37ee6 * 0x118 - 0x118;
        *(unsigned short*)((char*)item + 0x6f) =
            *(unsigned short*)(*(unsigned char**)((char*)item + 0x67) + 0xa8);
        *(unsigned short*)((char*)item + 0x71) =
            *(unsigned short*)(*(unsigned char**)((char*)item + 0x6b) + 0xa8);
        // Item fields read back directly, no `slot` local: keeps the edx/esi roles.
        for (unsigned char* q = *(unsigned char**)((char*)item + 0x67);
             q <= *(unsigned char**)((char*)item + 0x6b); q += 0x118) {
            *(void**)(q + 0x96) = item;
            q[0xff] = *(unsigned char*)((char*)item + 0x146);
            *(unsigned int*)(q + 0xac) = 0xffffffffu;
        }
    }
}

// Ordering of two items: by the key at +4 when the mode object reports 3,
// otherwise by address.
// FUNCTION: 0x485940
int __stdcall ComparePlayers(Player* a, Player* b)
{
    if (g_seams.map->GetGameType(g_game->net) == 3)
        return a->id < b->id;
    return a < b;
}

void __stdcall KillUnit(Unit* unit, int param_2);
void __cdecl GameFreeThunk(void* p);

// FUNCTION: 0x485980
void FreeUnitMemory(void)
{
    Unit* u = g_game->units;
    Unit* end = g_game->unitsEnd;
    if (u != 0) {
        for (; u <= end; u = (Unit*)((char*)u + 0x118)) {
            if (u->unitDefIndex != 0)
                KillUnit(u, 8);
        }
    }
    if (g_game->hotRadar != 0)
        GameFreeThunk(g_game->hotRadar);
    g_game->hotRadar = 0;
    if ((void *)g_game->list_1435f != 0)
        GameFreeThunk((void *)g_game->list_1435f);
    (*(void **)&g_game->list_1435f) = 0;
    if (g_game->units != 0)
        GameFreeThunk(g_game->units);
    g_game->units = 0;
}


int __stdcall RandomInt(int range);

// Sets up a unit from its unit type: flags, offsets, screen position, weapon
// targets and the squad. The screen Y projection uses the passed position Z.
// FUNCTION: 0x485a40
void __stdcall InitUnitFromType(Unit* unit, Vec3 pos, int param_5)
{
    unit->def = (UnitDef*)&((UnitType *)g_game->defs)[(unsigned short)unit->unitDefIndex];
    unit->alive = 1;
    unit->immobile = (((UnitType*)unit->def)->mobile == 0);
    unit->killPending = 0;
    unit->footprint = ((UnitType*)unit->def)->offset;
    unit->armed = ((UnitType*)unit->def)->flags.bits.hi;
    unit->zBufferBit0 = ((UnitType*)unit->def)->flags.bits.b7;
    unit->cargoSelectable = ((UnitType*)unit->def)->flags.bits.b9;

    if (param_5) {
        unit->buildLeft = 0;
        unit->health = ((UnitType*)unit->def)->hp;
    } else {
        unit->buildLeft = 1.0f;
        unit->unfinishedInitZero = 0;
        unit->health = 0;
    }

    unit->bit0 = 0;
    unit->bit1 = 0;
    unit->bit2 = 0;
    unit->bit3 = 0;
    unit->mode = 1;
    unit->unknown_110_2 = 0;
    unit->selected = 0;
    unit->selectable = 1;
    unit->unknown_110_10 = 0;
    unit->cloakOn = 0;
    unit->moved = 1;
    unit->carriedNoPiece = 0;
    unit->healthPercent = 0;
    unit->prevHealthPercent = 0;
    unit->activateFlags = 0;
    unit->workTime = 0;
    unit->angles.pitch = 0;
    unit->pos = pos;

    Point16 off = unit->footprint;
    Point16 screen;
    screen.x = (short)((pos.x - off.x * 0x80000 + 0x80000) >> 20);
    screen.y = (short)((pos.z - off.y * 0x80000 + 0x80000) >> 20);
    unit->cell = screen;

    unit->angles.heading = (short)(RandomInt(((UnitType*)unit->def)->buildangle)
                                    + (0x8000 - ((UnitType*)unit->def)->buildangle / 2));
    unit->angles.bank = 0;
    unit->losCacheCellX = 0;
    unit->losCacheCellZ = 0;
    unit->recentlyDamagedTimer = 0;
    unit->postTransferHoldoff = 0;
    unit->ownerIsLocal = (unit->player->index == g_game->playerIndex);
    unit->unknown_110_8 = 0;

    for (int i = 0; i < 3; i++) {
        g_seams.targeting->ResetWeaponTarget((Unit*)unit, i);
        g_seams.weapons->ReleaseWeapons((Unit*)unit, 0, i);
    }

    unit->netDirtyFlags = 0;
    unit->killCount = 0;
    unit->attacker = 0;
    unit->lastAttackerSlot = 0xa;
    unit->moveOrder = ((UnitType*)unit->def)->flags.bits.movOrder;
    unit->fireOrder = ((UnitType*)unit->def)->flags.bits.fireOrder;
    unit->cloakOn = ((UnitType*)unit->def)->flags.bits.initCloaked;
    unit->f26_27 = 0;
    if (((UnitType*)unit->def)->buildMenuPageCount > 1) {
        unit->f22_23 = 3;
        unit->f24_25 = 0;
    } else {
        unit->f22_23 = 0;
        unit->f24_25 = 0;
    }

    unit->losSightFrameIdx = 0;
    g_seams.economy->Reset(&unit->resourceSlot, unit->playerIndex);
    unit->transportPiece = 0xff;
    unit->hoverBobPhase = (short)RandomInt(0x10000);
    g_seams.ai->SetUnitSquad(unit, 0);
}

// Builds the piece-tree "Object State" block of a placed object, called from
// the three places that finish placing a unit. The block comes from
// CreatePlayerObjectState, which also reorders the entries to match the build list, when
// the definition reached through +0x92 has object data at +0x18e; a 0x544-byte
// variable block is allocated for the object first and told about that data.
// Without the data the plain CreateObjectState builds the block and the owner
// pointer at +0xc is filled in by hand. Both arms store the block at +0x9e and
// clear its flag at +0x10.
//
// Both calls take the definition object (g_game->unitModels[id]) as their
// argument.


struct ObjectState_00485d40 {
    char unknown_0[8];
    int dirty;                         // +0x8
    void* unit;                        // +0xc
    int drawFrame;                     // +0x10
};

#include "cob_script.h"

// Unused here: this header and these forward declarations take the symbol ids that keep
// AllocateUnitMemory (0x4854a0) and KillUnit (0x4864b0) matching (docs/c2-regalloc.md).
#include "../sound/sound.h"
struct BmpFileHeader;
struct BmpInfo;
struct BmpInfoHeader;


#include "unit_script.h"

void* __cdecl operator new(size_t size);
ObjectState_00485d40* __stdcall CreatePlayerObjectState(Object3do* obj, ScriptTable* data, int player);
ObjectState_00485d40* __stdcall CreateObjectState(Object3do* obj);

// FUNCTION: 0x485d40
void __stdcall InitUnitScript(Unit* self)
{
    Object3do* obj = ((Object3do **)g_game->types)[self->unitDefIndex];
    if (((UnitType*)self->def)->data) {
        self->script = (Class_004b0a70*)new (kSim) UnitScript;
        ((CobScript*)self->script)->SetCob(((UnitType*)self->def)->data);
        self->state = (SpotState*)CreatePlayerObjectState(obj, ((UnitType*)self->def)->data, (int)self);
        ((UnitScript*)self->script)->SetObjectState((int)self->state);
        ((CobScript*)self->script)->StartScript("Create", 0, 1);
    } else {
        self->script = 0;
        self->state = (SpotState*)CreateObjectState(obj);
        ((ObjectState_00485d40*)self->state)->unit = self;
    }
    // Shared tail after the if/else: reloads the block in both exits.
    ((ObjectState_00485d40*)self->state)->drawFrame = 0;
}

// The compiler-generated scalar deleting destructor of UnitScript, the
// only class derived from CobScript (src/units/cob.cpp). Its
// vtable at 0x4fd698 has the base's 21 slots, all overridden; slot 20 holds
// this function. The derived class has no destructor of its own, so the
// implicit one only calls the base destructor.
//
// The overrides live at 0x480770-0x481470. Slots 7-19 here carry the base
// names, with their addresses (from 0x4fd698) noted beside them. All 20 are
// defined as members of UnitScript (src/units/unit_script.cpp).
//
// InitUnitScript builds the object (`new` of 0x544 bytes, then this class's
// vtable).
// The global below exists only to emit the vtable and this COMDAT, as in
// src/game/data_files_42a870.cpp.

// FUNCTION: 0x485e30 ??_GUnitScript@@UAEPAXI@Z
static UnitScript* s_object;
// A namespace-scope `new` would construct the object during CRT init, and the
// base constructor reads a global that is not set until later.
UnitScript* emit_00485e30() { return new UnitScript; }

#include "../orders/unit_motion.h"

// Creates the unit's 0x2f-byte object (constructor 0x43dc00) and copies a
// value from the unit type.
// FUNCTION: 0x485e50
void __stdcall CreateUnitMotion(Unit* unit)
{
    unit->motion = (int)new (kSim) UnitMotion(unit);
    unit->angles.heading = ((UnitType*)unit->def)->buildangle;
}

extern void* g_weaponAimCobVtable[];


// Initialises a unit: finds its unit type, gives the aim callback of each of
// the three weapon slots (at +8, +0x24 and +0x40) its vtable, stores the type
// id at +0xa6, then hands the unit to the position/flags setter, the object
// builder and two more methods. When the unit type's +0x22f is 1 the unit
// also gets a new UnitMotion and the type's +0x210 copied to +0x66, the tail
// of the matched CreateUnitMotion inlined.
// The three vtable stores are guarded by `if (unit)` but the +0xa6 store right
// after them, and the new expression at the end, are not: a null unit pointer
// writes to 0xa6, then to 0, and reads 0x92. Kept as the original has it.
// FUNCTION: 0x485e90
void __stdcall InitUnit(int unitType, Vec3 pos, int param_5, Unit* unit)
{
    UnitType* type = &((UnitType *)g_game->defs)[(unsigned short)unitType];
    if (unit) {
        for (int i = 0; i < 3; i++)
            unit->weapons[i].aimCob.ppVtbl = g_weaponAimCobVtable;
    }
    unit->unitDefIndex = (short)unitType;
    InitUnitFromType(unit, pos, param_5);
    InitUnitScript(unit);
    g_seams.weapons->InitUnitWeaponSlots(unit);
    g_seams.economy->UpdateMetalExtraction((Unit*)unit);
    if (type->mobile == 1) {
        unit->motion = (int)new (kSim) UnitMotion(unit);
        unit->angles.heading = ((UnitType*)unit->def)->buildangle;
    }
}

// Creates a unit of a given type for a player: refuses when the type is not
// buildable or the player already has its limit of that type, takes the
// requested unit slot (or the first free one) from the player's unit list,
// initialises it (InitUnit inlined) and registers it.

#include "../game/mission_conditions.h"

// Unused here: the symbol id this declaration takes keeps 0x4854a0 matching
// after the header moved the file's symbol count (docs/c2-regalloc.md).

void __stdcall UpdateUnitHeight(Unit* unit);
void __stdcall AddUnitToMap(Unit* unit);
void __stdcall SendNewUnit(Unit* unit);
void __stdcall BroadcastBuilderLink(Unit* a, Unit* b);

static inline void __stdcall InitUnit_00485e90(unsigned short unitType, Vec3 pos,
                                               int param_5, Unit* unit)
{
    UnitType* type = &((UnitType *)g_game->defs)[unitType];
    if (unit) {
        for (int i = 0; i < 3; i++)
            unit->weapons[i].aimCob.ppVtbl = g_weaponAimCobVtable;
    }
    unit->unitDefIndex = unitType;
    InitUnitFromType(unit, pos, param_5);
    InitUnitScript(unit);
    g_seams.weapons->InitUnitWeaponSlots(unit);
    g_seams.economy->UpdateMetalExtraction((Unit*)unit);
    if (type->mobile == 1) {
        unit->motion = (int)new (kSim) UnitMotion(unit);
        unit->angles.heading = ((UnitType*)unit->def)->buildangle;
    }
}

// FUNCTION: 0x485f50
Unit* __stdcall CreateUnit(unsigned char player, unsigned short typeId, Vec3 pos,
                                      int param_5, int mode, unsigned short id)
{
    int off = player * 0x14b;
    Player* pl = (Player*)((char*)g_game + off + 0x1b63);
    if (typeId == 0)
        return 0;
    UnitType* type = &((UnitType *)g_game->defs)[typeId];
    if (!(type->flags.all & 0x800000))
        return 0;
    if (type->limit != -1) {
        int count = 0;
        for (Unit* u = pl->unitsBegin; u <= pl->unitsEnd; u++) {
            if (u->unitDefIndex == typeId)
                count++;
        }
        if (count >= type->limit)
            return 0;
    }
    Unit* unit;
    for (unit = pl->unitsBegin; unit <= pl->unitsEnd; unit++) {
        if (id != 0) {
            unit = &g_game->units[id];
            if (unit < pl->unitsBegin || unit > pl->unitsEnd)
                return 0;
        }
        if (unit->unitDefIndex == 0)
            goto found;
        if (id != 0)
            return 0;
    }
    return 0;
found:
    InitUnit_00485e90(typeId, pos, param_5, unit);
    unit->mode = mode;
    UpdateUnitHeight(unit);
    g_seams.map->AddUnitToMap(unit);
    SendNewUnit(unit);
    if (param_5) {
        if (type->mobile == 0)
            BroadcastBuilderLink(unit, unit);
        if (((UnitType*)unit->def)->flags.bit18)
            ((Unit*)unit)->SetStateBits(1, 1);
        if (((UnitType*)unit->def)->flags.bit24) {
            unit->lastDamageType = 7;
            unit->killPending = 1;
        }
    }
    g_seams.los_radar->RevealNewUnit(unit);
    // Array subscript, not a byte-offset cast: it fixes the SIB operand order.
    g_game->players[player].unitCount++;
    g_game->players[player].unitsCreated++;
    g_seams.players->NotifyUnitCreated(g_game->list_391ed, 0, unit);
    return unit;
}

// Creates a unit from a spawn record (the 0x11-byte record the build and
// placement code fills in: player, unit type, unit id, position). It takes the
// unit slot for the record's id out of the unit array, refuses to go on when
// that player has no unit list, clears the slot when it is still in use,
// initialises the unit (the matched InitUnit, inlined), then registers it
// with UpdateUnitHeight, AddUnitToMap, RevealNewUnit and the list manager and bumps
// the player's counters at +0x144 and +0x140.

#pragma pack(push, 1)
struct Spawn_004861d0 {                  // 0x11 bytes
    unsigned char player;                // +0x0
    unsigned short type;                 // +0x1
    unsigned short id;                   // +0x3
    Vec3 pos;
};
#pragma pack(pop)

// FUNCTION: 0x4861d0
Unit* __stdcall CreateUnitFromPacket(unsigned char player, Spawn_004861d0* spawn)
{
    Player* pl = &g_game->players[player];
    Unit* unit;
    if (spawn->id == 0) {
        unit = 0;
    } else {
        unit = &g_game->units[spawn->id];
    }
    if (pl->unitsBegin == 0) {
        return 0;
    }
    if (unit->unitDefIndex != 0) {
        KillUnit(unit, 0);
    }
    InitUnit_00485e90(spawn->type, spawn->pos, 0, unit);
    UpdateUnitHeight(unit);
    AddUnitToMap(unit);
    g_seams.los_radar->RevealNewUnit(unit);
    // Indexed again from g_game, not through pl: keeps the player index live early.
    g_game->players[player].unitCount++;
    g_game->players[player].unitsCreated++;
    g_game->list_391ed->NotifyUnitCreated(unit);
    return unit;
}

struct Result_486360 {
    char unknown_0[0x18];
    int velY;                        // +0x18
    int velZ;                        // +0x1c
};

// GetGroundHeight (units_485070.cpp) only matches in a file of its own: its symbol
// ids have to stay below a low limit.
int __stdcall GetGroundHeight(Pos_00485070* pos);
// Also declared for the symbol count that AllocateUnitMemory needs.
int __stdcall FindHighestPointOnLine(Vec3 a, Vec3 b);
void __stdcall EmitSmoke(Pos_00485070* pos, int a, int b, int c);

// FUNCTION: 0x486360
void __stdcall CreateUnitCorpse(Unit* unit, int depth, int flag)
{
    unsigned short id = ((UnitType*)unit->def)->corpse;
    for (; depth > 1; depth--) {
        if (id >= 0xfffb) {
            return;
        }
        id = ((Entry_486360 *)g_game->features)[id].next;
    }
    if (id < 0xfffb) {
        void* target = g_seams.los_radar->GetMapCell(unit->cell.x, unit->cell.y);
        if (target != 0) {
            Pos_00485070* pos = (Pos_00485070*)&unit->pos;
            if (GetGroundHeight(pos) <= g_game->seaLevel) {
                Result_486360* r = (Result_486360*)g_seams.features->PlaceFeature((Cell*)target, id, (Vec3*)pos, (Rot16*)&unit->angles, unit->playerIndex);
                if (r != 0) {
                    if (!(((UnitType*)unit->def)->flags.all & 0x1000000)) {
                        r->velY = -11468;
                        r->velZ = 0;
                    }
                    flag = 0;
                }
            } else {
                g_seams.features->PlaceFeature((Cell*)target, id, (Vec3*)pos, (Rot16*)&unit->angles, unit->playerIndex);
            }
            if (flag) {
                EmitSmoke(pos, 0xf, 900, 9);
            }
        }
    }
}

// FUNCTION: 0x486460
int __stdcall IsUnitCommander(Unit* unit)
{
    return _strcmpi(g_game->names_37f5f[unit->player->info->side].name, ((UnitType*)unit->def)->name) == 0;
}

// A live unit that changed its type (the linked unit name at g_game+0x37f5f
// matches the unit type's name) recomputes a "Killed" kill count and sends the
// 0xb byte killed packet (type 0xc) it builds here to its own player; the
// handler ApplyUnitDeath takes the same packet. param_2 is the kill type
// (the packet's high nibble at +0xa): 7 means a spy / non-kill path, 4/5/9 or a positive
// health skip the recount, otherwise the kills are a percentage of the
// type's maxHealth.

#pragma pack(push, 1)
struct UnitKilledPacket {
    unsigned char type;                // +0x0
    unsigned short unitId;             // +0x1
    int attackerPlayerNetId;           // +0x3, the player net id of the last attacker slot
    unsigned short attackerUnitId;     // +0x7, 0 for none
    signed char cobKilledSeverity;     // +0x9, the Killed script's first result
    // 4-bit bitfields, not a plain char: they give the XOR read-modify-writes.
    unsigned char wreckageLevel : 4;   // +0xa
    unsigned char killType : 4;        // +0xa
};
#pragma pack(pop)

int __stdcall GetSlotDpid(unsigned char index);
int __stdcall BroadcastPacket(int id, unsigned char* packet, int size);
void __stdcall ApplyUnitDeath(UnitKilledPacket* cmd, int param);
void __stdcall PopUntilNamedLayout(int param);
void __stdcall KillPlayerUnits(unsigned char player);

// FUNCTION: 0x4864b0
void __stdcall KillUnit(Unit* unit, int param_2)
{
    if ((unit->flags & 0x10000000) != 0) {
        int same = _strcmpi(g_game->names_37f5f[unit->player->info->side].name,
                            ((UnitType*)unit->def)->name) == 0;
        if (same) {
            unit->player->flags &= 0xfffe;
        }
        int amount;
        int flag;
        if (param_2 == 7) {
            flag = 1;
            amount = 0;
        } else if (param_2 == 4 || param_2 == 5 || param_2 == 9 || unit->health > 0) {
            amount = 0;
            flag = 0;
        } else {
            amount = ((int)(unit->health * -100 / ((UnitType*)unit->def)->maxHealth) + unit->prevHealthPercent) / 2;
            if (amount < 1)
                amount = 1;
            if (amount > 100)
                amount = 100;
            ((CobScript*)unit->script)->QueryScript("Killed", &amount, &flag, 0, 0);
        }
        if (unit->buildLeft != 0.0f) {
            flag = 0;
        }
        UnitKilledPacket cmd;
        // unitId before the severity: sets the register roles.
        cmd.unitId = unit->id;
        cmd.cobKilledSeverity = amount;
        cmd.type = 0xc;
        cmd.wreckageLevel = flag;
        cmd.killType = param_2;
        cmd.attackerPlayerNetId = GetSlotDpid(unit->lastAttackerSlot);
        if (unit->attacker == 0)
            cmd.attackerUnitId = 0;
        else
            cmd.attackerUnitId = unit->attacker->id;
        if (unit->player->active != 0 &&
            (unit->player->type == 1 || unit->player->type == 2)) {
            BroadcastPacket(unit->player->id, (unsigned char*)&cmd, 0xb);
        }
        ApplyUnitDeath(&cmd, 1);
        if (same && g_game->commanderDeath != 0 && unit->player->active != 0 &&
            (unit->player->type == 1 || unit->player->type == 2)) {
            PopUntilNamedLayout(1);
            g_seams.players->KillPlayerUnits(unit->playerIndex);
        }
    }
}

// Handles the "unit died" record that 0x4864b0 builds: credits the kill, updates the
// kill leaderboard ("%s has taken the lead with %d kills"), then tears the unit down.

extern char g_killedScriptName[];
extern char g_takenLeadFormat[];

unsigned char __stdcall FindSlotByDpid(int id);
void __stdcall RemoveSpeechOfUnit(void* unit);
void __stdcall AttachUnitToPiece(void* unit, void* builder, int a, int c);
void __stdcall AddCdActivitySample(int flag);
char* __stdcall Translate(char* text);
void __stdcall AddMessage(char* text, int a, int b, int c);
void __stdcall FlashScorePanelKillLoss(int a, int b);
void __stdcall ClearUnitRefs(void* unit);
void __stdcall FreeObjectState(void* state);
void __cdecl operator delete(void* p);
void __stdcall AnnouncePlayerLeft(int id);
void __stdcall AnnounceForcesDestroyed(void* player);

// FUNCTION: 0x4866d0
void __stdcall ApplyUnitDeath(UnitKilledPacket* cmd, int local)
{
    Unit* unit;
    if (cmd->unitId == 0)
        unit = 0;
    else
        unit = &g_game->units[cmd->unitId];
    if (g_quirks.ignoreNullDeathRecord && unit == 0)
        return;
    if ((unit->flags & 0x10000000) == 0)
        return;

    if (unit->player->index == g_game->playerIndex)
        g_seams.los_radar->AddEyeball((Vec3*)&unit->pos, ((UnitType*)unit->def)->x202, ((UnitType*)unit->def)->x170, 60);
    Unit* parent;
    if (cmd->attackerUnitId == 0)
        parent = 0;
    else
        parent = &g_game->units[cmd->attackerUnitId];
    unit->attacker = parent;
    unit->lastAttackerSlot = FindSlotByDpid(cmd->attackerPlayerNetId);
    g_seams.players->NotifyUnitDied(g_game->list_391ed, 0, unit);
    g_seams.orders->DeleteOrders(unit, 1);
    RemoveSpeechOfUnit(unit);
    g_seams.ai->SetUnitSquad(unit, -1);
    g_seams.weapons->RemoveUnitProjectiles(unit);
    if (unit->carrier != 0)
        AttachUnitToPiece(unit, 0, -1, 1);
    while (unit->cargo != 0) {
        unsigned char depth = cmd->killType != 3 ? 6 : 3;
        g_seams.weapons->DamageUnit(unit->attacker, unit->cargo, 30000, depth, 0);
        AttachUnitToPiece(unit->cargo, 0, -1, 1);
    }
    g_seams.map->ClearFootprintAndUnlink(unit);
    if ((g_game->mapFlags & 2) == 2)
        g_seams.los_radar->RemoveUnitLineOfSight(unit);
    if (local == 0 && cmd->cobKilledSeverity > 0)
        ((CobScript*)unit->script)->StartScriptWithArgs(g_killedScriptName, 0, 1, 1, cmd->cobKilledSeverity, 0, 0, 0);

    int credited = 0;
    switch (cmd->killType) {
    case 5:
        if (unit->lastAttackerSlot == 10 || unit->lastAttackerSlot == unit->playerIndex)
            break;
    case 1:
    case 6:
        if (unit->player != 0) {
            unit->player->losses++;
            if (unit->lastAttackerSlot != 10 && unit->buildLeft == 0.0f && unit->playerIndex != unit->lastAttackerSlot)
                g_game->players[unit->lastAttackerSlot].kills++;
            int same = _strcmpi(g_game->names_37f5f[unit->player->info->side].name,
                                ((UnitType*)unit->def)->name) == 0;
            if (same) {
                if (unit->lastAttackerSlot != 10)
                    g_game->players[unit->lastAttackerSlot].commanderKills++;
                unit->player->commanderLosses++;
            }
            if (unit->attacker != 0 && unit->buildLeft == 0.0f && unit->playerIndex != unit->lastAttackerSlot)
                unit->attacker->killCount++;
            if (unit->lastAttackerSlot == g_game->localPlayer)
                AddCdActivitySample(5);
            credited = 1;
        }
        break;
    case 3:
        if (unit->player != 0 && g_game->players[g_game->localPlayer].shareVision[unit->player->index] == 0) {
            unit->player->losses++;
            int same = _strcmpi(g_game->names_37f5f[unit->player->info->side].name,
                                ((UnitType*)unit->def)->name) == 0;
            if (same)
                unit->player->commanderLosses++;
            credited = 1;
        }
        break;
    }

    if (credited && unit->lastAttackerSlot != 10) {
        Player* rec = &g_game->players[unit->lastAttackerSlot];
        if (rec->active != 0 && (rec->type == 1 || rec->type == 2 || rec->type == 3)
            && rec->index != 10
            && (g_seams.map->GetGameType(g_game->net) == 3 || g_seams.map->GetGameType(g_game->net) == 2)
            && rec->rank > 0) {
            int rank = rec->rank;
            int best = rank;
            int mine = g_game->commanderDeath == 2 ? rec->commanderKills : rec->kills;
            int i = 10;
            Player* p = g_game->players;
            // do/while over a Player pointer: otherwise the frame grows.
            do {
                if (p->type != 0) {
                    bool hid = p->info->bit6;
                    if (!hid) {
                        // Two compare arms: the compiler merges their setg.
                        int ahead;
                        if (g_game->commanderDeath == 2)
                            ahead = mine > p->commanderKills;
                        else
                            ahead = mine > p->kills;
                        if (ahead) {
                            if (p->rank < best)
                                best = p->rank;
                        }
                    }
                }
                p++;
                i--;
            } while (i != 0);
            if (best < rank) {
                i = 10;
                p = g_game->players;
                do {
                    if (p->rank >= best && p->rank < rec->rank)
                        p->rank = p->rank + 1;
                    p++;
                    i--;
                } while (i != 0);
                rec->rank = best;
                if (best == 0) {
                    char text[100];
                    sprintf(text, Translate(g_takenLeadFormat), rec->name,
                            g_game->commanderDeath == 2 ? rec->commanderKills : rec->kills);
                    AddMessage(text, 2, 0, 10);
                }
            }
        }
        if (g_game->bits_37f06_1.b7)
            FlashScorePanelKillLoss(unit->lastAttackerSlot, unit->player->index);
    }

    if (cmd->killType == 5 && unit->attacker != 0) {
        // Field pointer, not a Unit* copy; and the copy into f is needed too.
        Unit** par = &unit->attacker;
        float health = 1.0f - unit->buildLeft;
        float f = health;
        f *= ((UnitType*)unit->def)->x18a;
        if ((*par)->resourceSlot.player->active != 0 && (*par)->resourceSlot.player->type == 2) {
            switch (g_game->difficulty) {
            case 0:
                (*par)->resourceSlot.metalMake = (*par)->resourceSlot.metalMake - f * -0.5;
                break;
            case 1:
                (*par)->resourceSlot.metalMake = (*par)->resourceSlot.metalMake - f * -0.7;
                break;
            default:
                (*par)->resourceSlot.metalMake += f;
            }
        } else {
            (*par)->resourceSlot.metalMake += f;
        }
    }
    if (cmd->cobKilledSeverity > 0 && unit->buildLeft == 0.0f)
        g_seams.weapons->DetonateUnitWeapon(unit, cmd->killType == 3);
    if (cmd->wreckageLevel > 0) {
        // Local flag: without it the script's virtual delete uses another register.
        int flag = cmd->killType != 7;
        CreateUnitCorpse(unit, cmd->wreckageLevel, flag);
    }

    ClearUnitRefs(unit);
    if (unit->script != 0) {
        delete (CobScript*)unit->script;
        unit->script = 0;
    }
    if (unit->state != 0) {
        FreeObjectState(unit->state);
        unit->state = 0;
    }
    UnitMotion* head = (UnitMotion*)unit->motion;
    if (head != 0) {
        g_seams.orders->DestroyMotionObject(head);
        operator delete(head);
        unit->motion = 0;
    }
    unit->unitDefIndex = 0;
    // Keep this order: adjacent clears would fold into one `and`.
    unit->flags &= ~0x10000000;
    unit->def = (UnitDef*)(UnitType *)g_game->defs;
    unit->flags &= ~0x30;
    unit->player->unitCount--;
    if (unit->player->unitCount == 0) {
        if (g_seams.map->GetGameType(g_game->net) == 3)
            AnnouncePlayerLeft(unit->player->id);
        if (g_seams.map->GetGameType(g_game->net) == 2)
            AnnounceForcesDestroyed(unit->player);
    }
}

// FUNCTION: 0x486e80
void __stdcall KillUnitsOfType(short id)
{
    Unit* u = g_game->units;
    Unit* end = g_game->unitsEnd;
    if (id != 0 && u != 0) {
        for (; u <= end; u = (Unit*)((char*)u + 0x118)) {
            if ((short)u->unitDefIndex == id) {
                KillUnit(u, 8);
            }
        }
    }
}

// FUNCTION: 0x486ed0
void KillAllUnits(void)
{
    Unit* u = g_game->units;
    Unit* end = g_game->unitsEnd;
    if (u != 0) {
        for (; u <= end; u = (Unit*)((char*)u + 0x118)) {
            if (u->unitDefIndex != 0) {
                KillUnit(u, 8);
            }
        }
    }
}

// Walks one player slot's unit list and either damages every unit whose owner
// is a human or computer player (DamageUnit) or fires its second weapon and
// flags it (DetonateUnitWeapon / KillUnit).

// FUNCTION: 0x486f10
void __stdcall KillPlayerUnits(unsigned char player)
{
    Player* p = &g_game->players[player];
    if (p != 0) {
        if (p->unitCount != 0) {
            // Indexed again from g_game, not through p: keeps the index base clean.
            Unit* u = g_game->players[player].unitsBegin;
            Unit* last = g_game->players[player].unitsEnd;
            if (u != 0) {
                for (; u <= last; u++) {
                    unsigned int flags = u->flags;
                    if (flags & 0x10000000) {
                        if (!(flags & 0x4000)) {
                            Player* d = u->player;
                            if (d->active != 0 &&
                                (d->type == 1 || d->type == 2)) {
                                g_seams.weapons->DamageUnit(u, u, 0x7530, 3, 0);
                            } else {
                                g_seams.weapons->DetonateUnitWeapon(u, 1);
                                u->flags |= 0x4000;
                                g_seams.units->KillUnit(u, 3);
                            }
                        }
                    }
                }
            }
        }
    }
}

// Unused here: real functions declared to keep the file's symbol count (docs/c2-regalloc.md);
// SaveUnits stands in for the ids the weapon slot header moved.
void __stdcall NopRetC_B(int, int, int);

struct Vec3_004851c0 {
    int x;
    int y;
    int z;
};

// FUNCTION: 0x4851c0
int __stdcall FindHighestPointOnLine(Vec3_004851c0 a, Vec3_004851c0 b)
{
    // dz, then dx, as named locals written back into b before the copy; y stays in place on b.
    int dz = b.z - a.z;
    b.z = dz;
    b.y -= a.y;
    int dx = b.x - a.x;
    b.x = dx;
    Vec3_004851c0 d = b;
    // n computed in each arm with one `n++` after: the two tails merge.
    int n;
    if (abs(dx) < abs(dz))
        n = abs(dz) / 0x100000;
    else
        n = abs(dx) / 0x100000;
    n++;
    d.x /= n;
    d.z /= n;
    short best = 0;
    for (int i = 0; i <= n; i++) {
        Cell* c = CellAt(a.x / 0x100000, a.z / 0x100000);
        if (c) {
            short v = ((unsigned char*)(Entry_486360 *)g_game->features)[c->feature * 256 + 0xfa] + c->height;
            if (best < v) best = v;
            if (c->unit) {
                Unit* o = &g_game->units[c->unit];
                short w = (((UnitType*)o->def)->modelMaxY + o->pos.y) >> 16;
                if (best < w) best = w;
            }
        }
        a.x += d.x; a.y += d.y; a.z += d.z;
    }
    return best;
}
