// Decompiled by Opus, deepseek-v4.1-flash, GPT-5.6-Terra, claude-sonnet,
// Space Bunny Free, GPT-6.1-sol, deepseek-v4.1, space-bunny-free,
// mimo-v2.6-pro, LongCat 2.5 Preview Free, Claude Opus 5.5, Haiku,
// Claude Sonnet 5.5 and GPT-6. Names are provisional.
// The weapons module (0x499a30 to 0x49e630), Cavedog's weapons.cpp: the
// projectile array, the weapon fire and hit packets, weapon damage and area
// damage, the ballistic launch-angle solver, the weapon reach tests, the
// projectile initialiser and the four firing routines, the turret aiming path
// with its angle helpers, the remote fire packet and its projectile scan, the
// weapon fire handlers, the unit's weapon slots and the loop that updates
// them, and the weapon name table. The module's parts joined in address
// order; 0x49a120 (a gap region) and 0x49b720 keep their own files
// (docs/split-modules.md). 0x49b090 and 0x49be60 come last, out of address
// order: their register plans follow their symbol ids (docs/c2-regalloc.md).

#include "../util/vec3.h"
#include "../state/block.h"

typedef Vec3 Vec3_0049e1a0;

union Fixed {
    int value;
    struct {
        unsigned short fraction;
        short whole;
    };
};

#include "../units/cob_script.h"

// Unused here: these headers and forward declarations take the symbol ids that keep
// WeaponCanReachPos (0x49aa80) and WeaponCanReachUnit (0x49abb0) matching (docs/c2-regalloc.md).
#include "../util/hapi_bank.h"
#include "../map/mission.h"
#include "../sound/sound.h"
struct BmpFileHeader;
struct BmpInfo;
struct BmpInfoHeader;
struct CalcedExplosion;
struct FrameTable;
struct Chunk;
class DamagedAllyCollector;
class AssaultTimer;
class EscortTimer;
class SquadScoutTimer;


#include "../network/player.h"
#include "../units/unit_def.h"
#include "unit_weapon_slot.h"

struct Gun_0049c9c0 {
    short angle;
    char unknown_2[2];
};

#pragma pack(push, 1)

struct Unit;

struct Type_0049d270 {
    char unknown_0[0x20];
    int param;                         // +0x20
};

struct Entry_0049d270 {                // 0x1c bytes
    char unknown_0[0xc];
    Type_0049d270* type;               // +0xc
    char unknown_10[0x16 - 0x10];
    short f_16;                        // +0x16
    short f_18;                        // +0x18
    char unknown_1a[0x1c - 0x1a];
};

struct Point_0049e1a0 {
    short x;
    short z;
};

struct Entry_0049e1a0 {        // 0x1c bytes
    Point_0049e1a0 point;      // +0x00
    char* name;                // +0x04
    int f_8;                   // +0x08
    WeaponDef* attached;       // +0x0c
    char unknown_10[0x14 - 0x10];
    unsigned short f_14; // +0x14, the slot's tick counter
    short f_16;          // +0x16
    short f_18;          // +0x18
    unsigned char f_1a;  // +0x1a
    unsigned char flags; // +0x1b
};

#include "../game/unit_resources.h"

struct Owner_0049dd60 {
    char unknown_0[0x20];
    int id;                            // +0x20
};

struct UnitType_0049d000 {
    char unknown_0[0x20];
    int range;                         // +0x20
};

#include "weapon_def.h"

union Fbb_0049d580 {
    unsigned short value;
    struct {
        unsigned short b0 : 1, b1 : 1, b2 : 1, b3 : 1;
        unsigned short b4 : 1, b5 : 1, b6 : 1, b7 : 1;
        unsigned short b8 : 1, b9 : 1, b10 : 1, b11 : 1;
        unsigned short b12 : 1, b13 : 1, b14 : 1, b15 : 1;
    } f;
};

union Fba_0049e1a0 {
    unsigned short w;
    unsigned char b[2];
};

// 499a30's fixed-point position view (Unit below holds it beside Vec3).
union Fixed_0049abb0 {
    int value;                                      // 16.16 fixed point
    struct {
        unsigned short fraction;
        short whole;
    } parts;
};

struct Vec3_0049abb0 {
    int x;
    Fixed_0049abb0 y;
    int z;
};

// 499a30's views are folded in under the union's other members: its def at
// +0xc is f_c (a WeaponDef), its weapons[3]
// are slots[3], its f16/f18/f1b are f_16/f_18/f_1b, its owner is playerIndex,
// its armour is killCount, its state is flags, its type is utype and its pos is
// pos_0049abb0.
// The unit, as the weapon code sees it. The views disagree about the bytes at
// +0x4 (49e1a0's three weapon entries, 49d270's one, 49d580's fire state and
// 49c740's and 49e070's weapon slots all overlap there), about +0x0 (49d000's
// type, 49d270's type and 49dd60's owner), about +0xbc (49e1a0's resources,
// 49e070's field_e4 and 49c920's and 49c9c0's f_dc and f_e6) and about the flag word (+0x110 in 49c740, 49d880 and 49d270, +0x111 in
// 49c920 and 49c9c0); the rest keep one name. Size 0x118, the unit array's
// stride.
#include "../units/unit.h"
#include "projectile.h"

struct Unit_0049c880;


// Unused here: real functions declared for their symbol ids, which keep
// 0x49a890 and 0x49aa80 matching, and make up the ids of the bit-field views
// that weapon_def.h no longer declares (docs/c2-regalloc.md).
void DrawDashedCircle(void*, int, int, int, int, int, int);
int HAPINET_createnewgame(void*, char*, int, int, int, int, int);
int __stdcall SquashPack(Chunk*, int*, char*, int, int, int);
int HAPINET_enumconnections(void*, void*, unsigned long, void*, unsigned long, void*);
int PickRandomReclaimableResourcesInRadius(int, int, int, int, int, int);
int CreateUnit(int, int, int, int, int, int);

struct MapFeature_0049b090;

// The game state, as the weapon code sees it. The time word keeps one name per
// view (now, frame, gameTick, teamColor); the two tracked-projectile pointers
// are one union, 499a30's selected and 49c740's tracked.
struct Net {
    char unknown_0[0xd48];
    int noSeaLevelTrigger;
};

#include "../game/game.h"

#pragma pack(pop)

#include <string.h>

#pragma pack(push, 1)

struct Unit;
struct Spot_0049a120;
struct FeatureDef_0049a120;
struct MapFeature_0049b090;
struct Cell;

#pragma pack(pop)

extern Game* g_game;


// Allocates and clears the 0x7d64-byte weapon array, then resets its count.
// FUNCTION: 0x499a30
void AllocWeaponArray(void)
{
    (*(void **)&g_game->projectiles) = SimAllocTagged("WEAPON ARRAY", 0x7d64);
    memset((void *)g_game->projectiles, 0, 0x7d64);
    g_game->projectileCount = 0;
}

void __cdecl GameFreeThunk(void* p);

// FUNCTION: 0x499a80
void FreeWeaponArray(void)
{
    GameFreeThunk((void *)g_game->projectiles);
    (*(void **)&g_game->projectiles) = 0;
}

#pragma pack(push, 1)

struct Vec3_00499ab0 {
    int x;
    int y;
    int z;
};

struct Kind_00499ab0 {
    char unknown_0[4];
    int f4;                          // +0x4, the player / "who"
};

struct Obj_00499ab0 {
    char unknown_0[0x96];
    Kind_00499ab0* kind;             // +0x96
    char unknown_9a[0xa8 - 0x9a];
    unsigned short team;             // +0xa8
};

// One view of the 0x24 byte weapon fire packet (type 0xd): the two partial
// senders at 0x499ba0 and 0x49df10, the four firing routines' views and the
// receiver at 0x49d270 were folded in. Where their own views were, an unused
// declaration keeps the file's symbol ids (docs/c2-regalloc.md).
struct WeaponFirePacket {
    unsigned char type;              // +0x0
    Vec3 origin;                     // +0x1
    Vec3 aimOrVel;                   // +0xd
    unsigned char weaponDefIndex;    // +0x19
    unsigned char flag : 1;          // +0x1a, bit 0
    unsigned char : 7;
    unsigned short fireHeading;      // +0x1b
    unsigned short firePitch;        // +0x1d
    unsigned short targetUnitId;     // +0x1f
    unsigned short ownerUnitId;      // +0x21
    unsigned char weaponSlotIndex;   // +0x23
};

#pragma pack(pop)

int __stdcall BroadcastPacket(int player, void* data, int size);

// Builds the 0x24 byte "unit status" network message (type 0xd) and hands it
// to BroadcastPacket. Only sent while g_game's bit 0 flag is set. Sibling of
// 0x499ba0, which sends the same type and size.
//
// The byte at packet +0x1a is a 1 bit bitfield that is only ever read and
// write back by the assignment at the end, so its upper seven bits are never
// initialised here; see the note in the bug list.
// FUNCTION: 0x499ab0
void __stdcall SendWeaponFirePacket(UnitWeaponSlot* unit, Obj_00499ab0* source,
                            Obj_00499ab0* target, Vec3_00499ab0* a,
                            Vec3_00499ab0* b)
{
    WeaponFirePacket packet;
    if (g_game->flags_2a44 & 1) {
        packet.type = 0xd;
        packet.origin = *(Vec3*)a;
        packet.aimOrVel = *(Vec3*)b;
        packet.weaponDefIndex = unit->weapon->index;
        packet.weaponSlotIndex = (unit->flags >> 2) & 3;
        packet.ownerUnitId = !source ? 0 : source->team;
        packet.targetUnitId = !target ? 0 : target->team;
        packet.fireHeading = unit->fireHeading;
        packet.firePitch = unit->firePitch;
        packet.flag = unit->weapon->flags.value >> 30;
        BroadcastPacket(source->kind->f4, &packet, 0x24);
    }
}

struct Vec3_00499ba0 {
    int x;
    int y;
    int z;
};

#pragma pack(push, 1)
// Unused here: the symbol ids this declaration takes keep the allocation (docs/c2-regalloc.md).
void Unused_00499ba0(int a, int b, int c, int d, int e, int f);
#pragma pack(pop)

int __cdecl GetLocalDpid();

// FUNCTION: 0x499ba0
void __stdcall BroadcastWeaponFire(char param_1, Vec3_00499ba0* a, Vec3_00499ba0* b)
{
    WeaponFirePacket packet;
    if (g_game->flags_2a44 & 1) {
        packet.type = 0xd;
        packet.origin = *(Vec3*)a;
        packet.aimOrVel = *(Vec3*)b;
        packet.weaponDefIndex = param_1;
        BroadcastPacket(GetLocalDpid(), &packet, 0x24);
    }
}

#pragma pack(push, 2)
struct Object_00499c10 {
    char unknown_0[0x66];
    short heading;                     // +0x66
    char unknown_68[0x9a - 0x68];
    CobScript* anims;                  // +0x9a
};
#pragma pack(pop)

struct Source_00499c10 {
    char unknown_0[0x16];
    short heading;                     // +0x16
};

#include "../../interfaces/seams.h"

// Fixed-point trig helpers written in assembly.
int __cdecl FUN_004b70ef(short angle, int scale);
int __cdecl FUN_004b7123(short angle, int scale);

// FUNCTION: 0x499c10
void __stdcall RockUnit(Object_00499c10* obj, Source_00499c10* src)
{
    short angle = src->heading - obj->heading;
    int a = -FUN_004b70ef(angle, 800);
    int b = -FUN_004b7123(angle, 800);
    g_seams.units->StartScriptWithArgs(obj->anims, "RockUnit", 0, 0, 2, b, a, 0, 0);
}

#include "../ai/squad_manager.h"

#include "damage_table.h"

int __stdcall ApplyWeaponDamage(Projectile* weapon, Unit* target, float scale);

// FUNCTION: 0x499c70
void __stdcall ApplyWeaponHit(Projectile* weapon, Unit* target)
{
    unsigned short damage = ApplyWeaponDamage(weapon, target, 1.0f);
    Unit* attacker = weapon->attacker;
    if (attacker) {
        unsigned short a = 0;
        unsigned short b = 0;
        if (weapon->owner != target->playerIndex)
            a = damage;
        else
            b = damage;
        g_seams.ai->MarkOwnerNetDirtyFromDamageSplit(attacker->player->ai, 0, weapon, a, b);
    }
}

extern "C" int __cdecl _strcmpi(const char* str1, const char* str2);

struct Pair_00499cd0 {
    char* name;                     // +0x0
    int value;                      // +0x4
};

struct NameLess_00499cd0 {
    bool operator()(const char* a, const char* b) const
    {
        return _strcmpi(a, b) < 0;
    }
};

#pragma pack(push, 1)
struct Flags_00499cd0 {
    unsigned short bits0_6 : 7;
    unsigned short flag7 : 1;       // bit 7, 0x80
    unsigned short flag8 : 1;       // bit 8, 0x100
    unsigned short rest : 7;
};
#pragma pack(pop)

short __cdecl FUN_004b715a(int x, int z);
void __stdcall DamageUnit(Unit* source, Unit* target,
                            int amount, int type, unsigned short extra);

static inline int* Find_00499cd0(const Table_00499cd0* table, char* name)
{
    NameLess_00499cd0 less;
    Pair_00499cd0* first = (Pair_00499cd0*)table->v.begin();
    Pair_00499cd0* last = (Pair_00499cd0*)table->v.end();
    while (first != last) {
        Pair_00499cd0* mid = first + (last - first) / 2;
        if (less(mid->name, name))
            first = mid + 1;
        else
            last = mid;
    }
    if (first == (Pair_00499cd0*)table->v.end() || less(name, first->name))
        return 0;
    return &first->value;
}

// Damage a weapon does to a target. The weapon's def holds a base damage
// (+0xd4) and a name-keyed damage table (+0x64); the target's unit type name
// is looked up in it (the same sorted (name, value) array and lower_bound as
// 0x4c4630). The result is scaled, boosted by the attacker's armour
// (6% per point, capped at 5 points), doubled or halved by two global flags,
// and handed to DamageUnit together with the angle from the weapon to the
// target minus the target's heading.
// FUNCTION: 0x499cd0
int __stdcall ApplyWeaponDamage(Projectile* weapon, Unit* target,
                           float scale)
{
    WeaponDef* def = weapon->type;
    int damage = def->damage;
    Table_00499cd0* table = def->table;
    if (table) {
        int* p = Find_00499cd0(table, target->def->unitname);
        if (p)
            damage = *p;
    }
    damage = (int)(damage * scale);
    short angle = FUN_004b715a(weapon->position.x - target->pos.x,
                               weapon->position.z - target->pos.z) - target->heading;
    Unit* attacker = weapon->attacker;
    if (attacker) {
        int armour = attacker->killCount / 5;
        if (armour > 5)
            armour = 5;
        damage = (armour * 6 + 100) * damage / 100;
    }
    Flags_00499cd0* flags = (Flags_00499cd0*)((char*)g_game + 0x37f2f);
    if (flags->flag7)
        damage *= 2;
    if (flags->flag8)
        damage /= 2;
    bool paralyzer = weapon->type->flags.b.paralyzer;
    int type = paralyzer ? 2 : 1;
    DamageUnit(attacker, target, damage, type, angle);
    return damage;
}


// FUNCTION: 0x499e50
void __stdcall UntrackProjectile(Projectile* projectile)
{
    if (projectile == g_game->tracked) {
        g_game->trackedPos = g_game->tracked->position;
        g_game->value_1434b = projectile->type->holdTime;
        (*(void **)&g_game->tracked) = 0;
    }
    projectile->flags |= 2;
}

void __stdcall AccumulateScreenShake(int a, int b, int c);void __stdcall AddExplosionEffect(Vec3* position, void* value, int a, int b);
void __stdcall EmitWhiteSmoke(Vec3* position, int value);
// The sound id stays unsigned int: gives the original's zero extension.
void __stdcall PlaySoundAt(unsigned int sound, Vec3* position, int value);
void __stdcall ApplyAreaDamage(Projectile* projectile, Vec3* position);

// FUNCTION: 0x499eb0
void __stdcall DetonateProjectile(Projectile* projectile, Unit* unit)
{
    int hostile = 0;
    WeaponDef* type = projectile->type;
    Vec3* position = &projectile->position;
    unsigned char* value = (unsigned char*)g_seams.los_radar->GetMapCellAtPosition((Vec3*)position);
    if (value != 0)
        hostile = value[5] < g_game->seaLevel;
    if (!type->flags.b.noExplode) {
        if (projectile == g_game->tracked) {
            g_game->trackedPos = g_game->tracked->position;
            g_game->value_1434b = projectile->type->holdTime;
            (*(void **)&g_game->tracked) = 0;
        }
        projectile->flags = projectile->flags | 2;
    }
    if (((Net *)g_game->net)->noSeaLevelTrigger && hostile && !unit) {
        if (projectile == g_game->tracked) {
            g_game->trackedPos = g_game->tracked->position;
            g_game->value_1434b = projectile->type->holdTime;
            (*(void **)&g_game->tracked) = 0;
        }
        projectile->flags = projectile->flags | 2;
        return;
    }
    AccumulateScreenShake(type->shakeMagnitude, type->shakeMagnitude, type->shakeDuration);
    if (hostile && !unit) {
        PlaySoundAt(type->soundWater, position, 0);
        AddExplosionEffect(position, type->splash, 0, hostile);
    } else {
        PlaySoundAt(type->soundHit, position, 0);
        if (type->flags.b.endSmoke)
            EmitWhiteSmoke(position, 9);
        else
            AddExplosionEffect(position, type->explosionSeq, 0, hostile);
    }
    unsigned int player = projectile->owner;
    Player* record = &g_game->players[player];
    if (!record->active || record->type != 3) {
        if (type->areaOfEffect <= 0x10 && unit) {
            int damage = ApplyWeaponDamage(projectile, unit, 1.0f);
            Unit* source = projectile->attacker;
            if (source) {
                int a = 0;
                int b = 0;
                if (projectile->owner != unit->playerIndex)
                    a = damage;
                else
                    b = damage;
                g_seams.ai->MarkOwnerNetDirtyFromDamageSplit(source->player->ai, 0, projectile, a & 0xffff, b & 0xffff);
                return;
            }
        } else {
            ApplyAreaDamage(projectile, position);
        }
    }
}

// Builds a zeroed 0x6b-byte parameter block (owner, position, kind 10) and
// hands it to ApplyAreaDamage (an ebp-framed __stdcall routine in a gap).

struct Vec3_0049a0c0 {
    int x;
    int y;
    int z;
};

#pragma pack(push, 1)
struct Params_0049a0c0 {
    void* owner;                       // +0x0
    Vec3_0049a0c0 pos;                 // +0x4
    char unknown_10[0x42];
    int ownerUnit;                     // +0x52
    char unknown_56[0x10];
    char kind;                         // +0x66
    char unknown_67[4];
};
#pragma pack(pop)

void __stdcall ApplyAreaDamage(Params_0049a0c0* params, Vec3_0049a0c0* pos);

// FUNCTION: 0x49a0c0
void __stdcall ApplyAreaDamageAt(void* owner, Vec3_0049a0c0* pos)
{
    Params_0049a0c0 p;
    memset(&p, 0, sizeof(p));
    p.ownerUnit = 0;
    p.owner = owner;
    p.kind = 10;
    p.pos = *pos;
    ApplyAreaDamage(&p, pos);
}

// 0x49a120's views. The function itself is a gap region (one of
// data/functions.csv's 29 gap rows) and stays in weapons_49a120.cpp; its
// declarations stay here for the symbol ids the functions after it match at.
#include <math.h>

// A 16.16 fixed-point value and its whole part.
union Fixed_0049a120 {
    int raw;
    struct {
        unsigned short frac;
        short whole;
    } part;
};

struct FixedVec3 {
    Fixed_0049a120 x;
    Fixed_0049a120 y;
    Fixed_0049a120 z;
};

struct CellPos_0049a120 {
    short x;
    short z;
};

#pragma pack(push, 1)
// Unused here: real functions declared for their symbol ids, which keep
// 0x49a890 and 0x49aa80 matching (docs/c2-regalloc.md).
int CloakOnOrder(char*, int, int);
int GetUnitCount(int, unsigned int);

struct Cell {
    unsigned short unit;               // +0x0
    unsigned short unit2;              // +0x2
    unsigned char height;              // +0x4
    unsigned char high;                // +0x5, highest floor
    unsigned char low;                 // +0x6, lowest floor
    unsigned char metal;               // +0x7
    unsigned short feature;            // +0x8
    union {
        unsigned short spot;           // +0xa
        struct {
            unsigned char offsetZ;     // +0xa
            unsigned char offsetX;     // +0xb
        } origin;
    };
    unsigned char flags;               // +0xc
};

struct Spot_0049a120 {
    char unknown_0[8];
    Vec3 pos;
    char unknown_14[0x30 - 0x14];
};

struct FeatureDef_0049a120 {
    char unknown_0[0x100];
};

// Unused here: the symbol ids this declaration takes keep the allocation (docs/c2-regalloc.md).
void Unused_0049a120(int a, int b, int c, int d, int e, int f);
#pragma pack(pop)

// <vector> comes after the game's own declarations: the symbol ids it gives
// cell, feature and g_game decide the spot and feature address arithmetic of
// the reach tests. The 0x49a120 gap file keeps the lean windows.h its own
// function was matched with.
#include <vector>

// What one explosion has already hit, so nothing takes damage twice.
struct Hits_0049a120 {
    int numUnits;
    int numFeatures;
    Unit* units[20];
    Cell* features[64];

    // Each returns 0 if the thing was hit already, else records it (while
    // there is room) and returns 1.
    int AddUnit(Unit* unit)
    {
        for (int k = 0; k < numUnits; k++) {
            if (units[k] == unit)
                return 0;
        }
        if (numUnits < 20) {
            units[numUnits] = unit;
            numUnits++;
        }
        return 1;
    }
    int AddFeature(Cell* cell)
    {
        for (int k = 0; k < numFeatures; k++) {
            if (features[k] == cell)
                return 0;
        }
        if (numFeatures < 64) {
            features[numFeatures] = cell;
            numFeatures++;
        }
        return 1;
    }
};

int __stdcall ApplyWeaponDamage(Projectile* weapon, Unit* target, float scale);
void __stdcall DetonateProjectile(Projectile* weapon, Unit* unit);
int __stdcall VectorLength(Vec3* v);
Vec3 __stdcall GetFootprintCentre(CellPos_0049a120* cell, FeatureDef_0049a120* def);
void __stdcall DamageFeature(Cell* cell, int x, int z, WeaponDef* def);
int __stdcall BroadcastPacket(int id, void* data, int size);

static inline int Length(Vec3* v)
{
    double x = v->x;
    double y = v->y;
    double z = v->z;
    return (int)sqrt(x * x + y * y + z * z);
}

static inline Unit* UnitFromId(unsigned short id)
{
    if (id == 0)
        return 0;
    return &g_game->units[id];
}

static inline Vec3 Sub(const Vec3& a, const Vec3& b)
{
    Vec3 r;
    r.x = a.x - b.x;
    r.y = a.y - b.y;
    r.z = a.z - b.z;
    return r;
}

// <time.h> and <malloc.h> are here for their symbol ids: they decide
// SolveLaunchAngle's x87 schedule and the reach tests' register plans.
#include <time.h>
#include <malloc.h>

// The length of an integer vector, truncated to an int.

struct Vec3i_0049a850 {
    int x;                             // +0x0
    int y;                             // +0x4
    int z;                             // +0x8
};

// FUNCTION: 0x49a850
int __stdcall VectorLength(Vec3i_0049a850* v)
{
    double x = v->x;
    double y = v->y;
    double z = v->z;
    return (int)sqrt(x * x + y * y + z * z);
}

// Ballistic launch-angle solver: the two roots
// of the trajectory quadratic, each turned into a launch angle with
// acos(sqrt(root) / speed), pi/2 when a root is not positive.
#include <stdio.h>

// PI stays this literal and `use / PI` a separate statement from
// `use * 32768.0`: the constant pool depends on it.
#define PI 3.14159265358979

extern "C" double __cdecl _hypot(double x, double y);

// FUNCTION: 0x49a890
short __stdcall SolveLaunchAngle(int x, int height, int z, int speed, float angle)
{
    int g = g_game->gravity;
    int gg = g * g;
    // The redundant parentheses stay, with these exact counts: they steer the
    // x87 schedule. disc is expanded as (s2 + 2*gh) * s2 + h2 * gg.
    double distance = (((_hypot(x, z))));
    double d = distance * distance;
    double gh = (double)g * (double)height;
    double h2 = (((((double)height * (double)height))));
    double s2 = (double)speed * (double)speed;
    double sum = h2 + d;
    double disc = (((((s2 + 2*gh) * s2)) + h2 * gg)) * (d * d) - (((d * d * gg)) * sum);
    if (disc < 0.0)
        return 0x8000;
    disc = sqrt(disc);
    d = (s2 + gh) * d;
    double high = (disc + d) / (2*sum);
    double low = (d - disc) / (2*sum);
    double highAngle;
    double lowAngle;
    if (high > 0.0)
        highAngle = acos(sqrt(high) / (double)speed);
    else
        highAngle = PI / 2;
    if (low > 0.0)
        lowAngle = acos(sqrt(low) / (double)speed);
    else
        lowAngle = PI / 2;
    double use;
    if (angle < highAngle && highAngle <= PI / 4)
        use = highAngle;
    else if (lowAngle > angle && lowAngle <= PI / 4)
        use = lowAngle;
    else
        return 0x8000;
    use = use * 32768.0;
    return (short)(use / PI);
}

// Unused here: real functions declared for their symbol ids, which keep
// 0x49aa80 matching (docs/c2-regalloc.md).
void RegisterUnitOrders();
void RegisterGroundOrders();
void EnableAICommands();
void RegisterAICommands();
void ResetAIPlayers();
void RegisterVtolOrders();
void StepAllGafSequences();
void ResetNetStats();
void InitCommands();
void RefreshSelectionOrders();
int RIReport(int, int, int, int, int, int, int, int, int, int);
void EnumPlayersCallback(int, int, int, int, int);
int CheckDirectXVersion(int, int, int, int, int);
void EmitThrustParticles(int, int, int, int, short);
int AimCobStub(int, int, int, int);
void DispatchOrdersPanelPageFlags();
void ResetCameraState();
void FindLocalCommander();
void ClearCameraFollowState();
void ClampCameraPosition();
void ClampCameraTarget();
void UpdateScreenShake();
void UpdateCameraFollow();
void BeginMouseScroll();
void EndMouseScroll();
void UpdateMouseScroll();
void UpdateEdgeScroll();

// 0x49aa80's file included <windows.h> and <stdio.h> here for the operand
// order in the height check; both are already included above.
#pragma pack(push, 1)

union Fixed_0049aa80 {
    int value;
    struct { unsigned short fraction; short whole; } parts;
};

struct Vec3_0049aa80 {
    int x;                             // +0x0 (16.16 fixed point)
    Fixed_0049aa80 y;                  // +0x4
    int z;                             // +0x8
};

#pragma pack(pop)

short __stdcall SolveLaunchAngle(int dx, int dy, int dz, int a, int b);

// The subtraction of two points is a real by-value operator taking both
// operands by value and returning the point by value, and the line-of-fire test
// goes through a by-value helper, both exactly as in the matching sibling
// 0x49abb0 (whose MATCH this idiom produced). Those two boundaries are what put
// a2 in esi and a3 in edi for the whole function and keep the weapon def in
// ebx, which is the first block's register allocation (see the pass note at the
// top of the file).
inline Vec3_0049aa80 operator-(Vec3_0049aa80 a, Vec3_0049aa80 b)
{
    Vec3_0049aa80 r;
    r.x = b.x - a.x;
    r.y.value = b.y.value - a.y.value;
    r.z = b.z - a.z;
    return r;
}

static inline short LineOfFire_0049aa80(Vec3_0049aa80 to, Vec3_0049aa80 from, int s, int f)
{
    Vec3_0049aa80 d = from - to;
    return SolveLaunchAngle(d.x, d.y.value, d.z, s, f);
}

// FUNCTION: 0x49aa80
int __stdcall WeaponCanReachPos(Unit* a1, Vec3_0049aa80* a2, Vec3_0049aa80* a3, int a4)
{
    WeaponDef* wdef = a1->weapons[a4 & 0xff].weapon;

    // z difference first, as named int locals: the original's order.
    int dz = a3->z - a2->z;
    int dx = a3->x - a2->x;
    if ((int)(((__int64)dx * dx) >> 32) + (int)(((__int64)dz * dz) >> 32) > wdef->range * wdef->range)
        return 0;

    if (wdef->flags.b.waterWeapon)
        return 1;

    if (a2->y.parts.whole + a1->def->field_170 <= g_game->seaLevel)
        return 0;

    if (wdef->flags.b.ballistic) {
        if (LineOfFire_0049aa80(*a2, *a3, wdef->speed, wdef->field_c8) == (short)0x8000)
            return 0;
    }
    return 1;
}

// Both includes change the register choice in the sea-level tests.
#include <stdlib.h>

#pragma pack(push, 1)

#pragma pack(pop)

// The vector from point `a` to point `b`. Both operands and the result are by
// value, which is what makes the line-of-fire block below compile the way the
// original does: MSVC builds the two 12-byte argument copies as one setup unit
// instead of turning the first aggregate into a live pointer, so `unit2` stays in
// ebx across the call and the second distance tail needs no reload. Note the
// operands are named in the order the callers pass them (shooter, target) while
// the result is target - shooter, because the copy order, not the arithmetic, is
// what the register allocation is sensitive to here.
inline Vec3_0049abb0 operator-(Vec3_0049abb0 a, Vec3_0049abb0 b)
{
    Vec3_0049abb0 r;
    r.x = b.x - a.x;
    r.y.value = b.y.value - a.y.value;
    r.z = b.z - a.z;
    return r;
}

// Can the shooter hit the target? Both positions are taken by value, which is
// what puts the 12-byte copies in the frame while the units themselves are read
// where they are. The parameters are named for the order the caller passes them
// in, and the subtraction gives the target relative to the shooter.
static inline short LineOfFire_0049abb0(Vec3_0049abb0 to, Vec3_0049abb0 from, int s, int f)
{
    Vec3_0049abb0 d = from - to;
    return SolveLaunchAngle(d.x, d.y.value, d.z, s, f);
}

static inline int Dist2_0049abb0(Vec3_0049abb0* b, Vec3_0049abb0* a)
{
    int dz = a->z - b->z;
    int dx = a->x - b->x;
    return (int)(((__int64)dx * dx) >> 32) + (int)(((__int64)dz * dz) >> 32);
}

// FUNCTION: 0x49abb0
int __stdcall WeaponCanReachUnit(Unit* unit1, Unit* unit2, unsigned char weapon)
{
    WeaponDef* w = unit1->weapons[weapon].weapon;

    if (w->flags.b.waterWeapon) {
        if (!(unit2->def->flags1 & 0x80000) && unit2->yWhole > g_game->seaLevel)
            return 0;
        // Written (height >> 1) + whole: sets the add's operand order.
        if ((unit2->def->flags1 & 0x1000) && (unit2->def->field_170 >> 1) + unit2->yWhole > g_game->seaLevel)
            return 0;
        return Dist2_0049abb0((Vec3_0049abb0*)&unit1->pos, (Vec3_0049abb0*)&unit2->pos) <= w->range * w->range;
    }

    if (unit1->yWhole + unit1->def->field_170 <= g_game->seaLevel)
        return 0;
    if (unit2->yWhole + unit2->def->field_170 <= g_game->seaLevel)
        return 0;
    if (w->flags.b.toAirWeapon && (unit2->flags & 3) != 2)
        return 0;
    // Nested ifs, not one && condition: the flag test codegen differs.
    if (w->flags.b.ballistic) {
        if (LineOfFire_0049abb0(*(Vec3_0049abb0*)&unit1->pos, *(Vec3_0049abb0*)&unit2->pos, w->speed, w->field_c8) == (short)0x8000)
            return 0;
    }
    return Dist2_0049abb0((Vec3_0049abb0*)&unit1->pos, (Vec3_0049abb0*)&unit2->pos) <= w->range * w->range;
}

// FUNCTION: 0x49adf0
int __stdcall GetWeaponRange(int param1, unsigned int param2)
{
    unsigned char idx = (unsigned char)param2;
    int offset = idx * 7;
    int* intermediate = (int*)((char*)param1 + offset * 4 + 0x10);
    int result = *(int*)((char*)*intermediate + 0xdc);
    return result;
}

// Compacts the projectile array: entries whose flags bit 1 is set are dropped
// and the remaining ones are moved down. Each projectile's own old index is
// written to compactIndex first, so the pointers held at interceptedProjectile
// (relinked to the moved targets in the second pass) can be resolved by
// searching for that index.
// FUNCTION: 0x49ae20
void CompactProjectiles()
{
    Projectile* projectiles = g_game->projectiles;
    int count = g_game->projectileCount;
    int n = 0;
    Projectile* dest = 0;
    int i = 0;
    short a[300];
    short b[300];

    for (i = 0; i < g_game->projectileCount; i++) {
        Projectile* p = &projectiles[i];
        bool dead = p->flagBits.dead;
        p->compactIndex = (short)i;
        if (dead) {
            if (!dest) {
                dest = p;
            }
            count--;
        } else {
            if (dest) {
                if (g_game->tracked == p) {
                    g_game->tracked = dest;
                }
                if (p->interceptedProjectile) {
                    a[n] = (short)(dest - projectiles);
                    b[n] = (short)(p->interceptedProjectile - projectiles);
                    n++;
                }
                *dest = *p;
                dest++;
            }
        }
    }

    if (n != 0) {
        int found = 0;
        for (int j = 0; j < count; j++) {
            for (int k = 0; k < n; k++) {
                if (projectiles[j].compactIndex == b[k]) {
                    projectiles[a[k]].interceptedProjectile = &projectiles[j];
                    if (++found == n) {
                        goto done;
                    }
                }
            }
        }
    }
done:
    g_game->projectileCount = count;
}

typedef Vec3 Vec3_0049af90;

#pragma pack(push, 1)
// One view of the 0xe byte projectile detonate packet (type 0xe): the sender
// at 0x49a120, a view of which is in the gap file, holds the other.
struct ProjectileDetonatePacket {
    unsigned char type;                // +0x0
    Vec3_0049af90 aim;                 // +0x1
    unsigned char weaponIndex;         // +0xd
};
#pragma pack(pop)

static inline int SamePos(const Vec3_0049af90& a, const Vec3_0049af90& b)
{
    return a.x == b.x && a.z == b.z && a.y == b.y;
}

// Finds the projectile a network packet refers to (by position and type) and
// removes it. The position compare is an inlined helper taking references;
// written inline, MSVC keeps one induction pointer instead of two.
// FUNCTION: 0x49af90
void __stdcall ApplyProjectileHitPacket(int unused, ProjectileDetonatePacket* p)
{
    Projectile* proj = g_game->projectiles;
    for (int i = 0; i < g_game->projectileCount; i++, proj++) {
        if (SamePos(proj->target, p->aim) && proj->type->index == p->weaponIndex) {
            DetonateProjectile(proj, 0);
            return;
        }
    }
}

// Fires one of a unit's two weapons (by type) from the unit's position,
// building the projectile on the stack; see 0x49af90 for the projectile.
// FUNCTION: 0x49b000
void __stdcall DetonateUnitWeapon(Unit* unit, int second)
{
    // The self-destruct and explosion weapons of the unit's definition.
    WeaponDef* weapon = second ? (WeaponDef*)unit->def->selfdestructas
                               : (WeaponDef*)unit->def->explodeas;
    if (weapon) {
        Projectile proj;
        proj.type = weapon;
        proj.position = unit->pos;
        proj.start = unit->pos;
        proj.targetUnit = 0;
        proj.attacker = 0;
        proj.owner = unit->playerIndex;
        DetonateProjectile(&proj, 0);
    }
}

#define max(a, b) (((a) > (b)) ? (a) : (b))

static Fixed Length(const Vec3& v)
{
    double fx = v.x;
    double fy = v.y;
    double fz = v.z;
    Fixed r;
    r.value = (int)sqrt(fx * fx + fy * fy + fz * fz);
    return r;
}

// FUNCTION: 0x49b3e0
Vec3* __stdcall GetProjectileAimPoint(Projectile* p)
{
    unsigned char flag = (unsigned char)p->type->flags.b.cruise;
    if (flag) {
        Fixed dist = Length(p->position - p->target);
        if (dist.whole > 0x400) {
            p->start = p->target;
            ((Vec3Words*)&p->start)->yWhole = 0x2bc;
            return &p->start;
        }
    } else {
        if (p->interceptedProjectile != 0)
            return &p->interceptedProjectile->position;
        Unit* q = p->targetUnitPtr;
        if (q != 0 && (q->flags & 0x10000000) != 0)
            return &q->pos;
    }
    if (flag) {
        p->start = p->target;
        p->start.y = max(g_seams.units->GetGroundHeight((Vec3*)&p->target), g_game->seaLevel) << 16;
        return &p->start;
    }
    return &p->target;
}

// Aim a unit's two turret angles at a target point. The heading (+0x36) is
// atan2(dx, dz); the pitch (+0x38) comes from the vertical difference against
// the horizontal distance. Each angle is then moved towards the wanted one by
// at most the unit type's turn rate (+0xE8), and if the wanted angle is more
// than 27000 (about 148 degrees) away while the type's burnBlow flag (bit 23 of
// +0x111) is set, the function gives up and returns 0. Otherwise it returns 1.
// FUNCTION: 0x49b520
int __stdcall TurnUnitTowardsPoint(Projectile* p, Vec3* target)
{
    WeaponDef* type = p->type;
    // Declared dx, dy, dz in this order: it decides the stack slot assignment.
    int dx = p->position.x - target->x;
    Fixed dy;
    dy.value = p->position.y - target->y;
    int dz = p->position.z - target->z;
    short a1 = FUN_004b715a(dx, dz);
    int dist = (int)_hypot((double)dx, (double)dz);
    short a2 = FUN_004b715a(-dy.whole, (short)(dist >> 16));

    short diff1 = a1 - (short)p->angles.heading;
    short adiff1 = abs(diff1);
    if (adiff1 > 27000 && type->flags.b.burnBlow)
        return 0;
    if (adiff1 < type->turnRate)
        p->angles.heading = a1;
    else if (diff1 < 0)
        p->angles.heading = p->angles.heading - type->turnRate;
    else
        p->angles.heading = p->angles.heading + type->turnRate;

    short diff2 = a2 - p->angles.pitch;
    short adiff2 = abs(diff2);
    if (adiff2 > 27000 && type->flags.b.burnBlow)
        return 0;
    if (adiff2 < type->turnRate)
        p->angles.pitch = a2;
    else if (diff2 < 0)
        p->angles.pitch = p->angles.pitch - type->turnRate;
    else
        p->angles.pitch = p->angles.pitch + type->turnRate;
    return 1;
}

// FUNCTION: 0x49b680
void __stdcall ComputeVelocityFromAngles(Projectile* obj)
{
    obj->direction.y = FUN_004b70ef(obj->angles.pitch, obj->speed);
    int r = FUN_004b7123(obj->angles.pitch, obj->speed);
    obj->direction.x = -FUN_004b70ef((short)obj->angles.heading, r);
    obj->direction.z = -FUN_004b7123((short)obj->angles.heading, r);
}

// Takes the next free entry of the 300-entry projectile array (see
// 0x499a30), resets two of its fields and returns it, or 0 when full.
// FUNCTION: 0x49b6e0
Projectile* AllocProjectile()
{
    Projectile* p = 0;
    if (g_game->projectileCount < 300) {
        p = &g_game->projectiles[g_game->projectileCount++];
        p->flags &= ~2;
        p->targetUnit = 0;
    }
    return p;
}


extern Game* g_game;

int __stdcall QueryWeaponPiece(Unit* unit, unsigned char weapon);
void __stdcall PlaySoundAt(int sound, Vec3* pos, int param_3);

// FUNCTION: 0x49c740
void __stdcall InitProjectile(Projectile* proj, WeaponDef* shot, Vec3* pos,
                           Vec3* aim, int field_5, Unit* unit)
{
    unsigned char i;
    proj->type = shot;
    proj->position = *pos;
    proj->start = *pos;
    if (aim)
        proj->target = *aim;
    proj->spawnTick = field_5;
    proj->flags &= ~1;
    proj->active = 0;
    unsigned short f = proj->flags;
    proj->nextSmokeTick = g_game->ticks;
    proj->interceptedProjectile = 0;
    proj->targetUnit = 0;
    proj->flags = f & ~0x30;
    if (unit) {
        proj->owner = unit->playerIndex;
        proj->attacker = unit;
        if ((unit->flags & 0x20000000) && (*(Unit **)&g_game->value_142f3) == unit)
            g_game->tracked = proj;
        for (i = 0; i < 3; i++)
            if (unit->weapons[i].weapon == shot)
                break;
        proj->piece = (short)QueryWeaponPiece(unit, i);
        unit->workTime = g_game->ticks + 0x258;
    } else {
        proj->owner = 0xa;
        proj->attacker = 0;
    }
    PlaySoundAt(shot->sound, pos, 0);
}

void CompactProjectiles();

// Same body as the matched 0x499e50, which the original inlined here.
static inline void Untrack_0049c880(Projectile* proj)
{
    if (proj == g_game->tracked) {
        g_game->trackedPos = g_game->tracked->position;
        g_game->value_1434b = proj->type->holdTime;
        g_game->tracked = 0;
    }
    proj->flags |= 2;
}

// FUNCTION: 0x49c880
void __stdcall RemoveUnitProjectiles(Unit_0049c880* owner)
{
    Projectile* proj = g_game->projectiles;
    for (int i = 0; i < g_game->projectileCount; i++, proj++) {
        if (proj->active != 0 && proj->attacker == (Unit*)owner) {
            Untrack_0049c880(proj);
            CompactProjectiles();
        }
    }
}

// FUNCTION: 0x49c920
void __stdcall ComputeProjectileTime(Projectile* obj)
{
    WeaponDef* type = obj->type;
    if ((unsigned int)type->speed != 0 && !(type->flags.value & 0x8000000)) {
        obj->time = (type->range << 16) / (unsigned int)type->speed + (unsigned int)g_game->ticks;
        return;
    }
    obj->time = (unsigned int)g_game->ticks + type->lifetime;
}

// FUNCTION: 0x49c980
void __stdcall CopyWeaponVelocityParam(Projectile* obj, WeaponDef* src)
{
    if (src->startSpeed) {
        obj->speed = src->startSpeed;
        return;
    }
    if (src->acceleration == 0) {
        obj->speed = src->speed;
        return;
    }
    obj->speed = 0;
}

#include <math.h>

short __cdecl FUN_004b715a(int x, int z);
int __cdecl FUN_004b70ef(short angle, int scale);
int __cdecl FUN_004b7123(short angle, int scale);
void __stdcall EmitWeaponSmoke(Vec3* p, short index);

extern char* g_fireScriptNames[4];

struct Fire_0049c9c0 {
    char unknown_0[0xc];
    WeaponDef* shot;                   // +0xc
    char unknown_10[0x1b - 0x10];
    unsigned char f_1b;                // +0x1b
};

void __stdcall InitProjectile(Projectile* proj, WeaponDef* shot,
                           Vec3* pos, Vec3* aim, int frame,
                           Unit* unit);

// FUNCTION: 0x49c9c0
int __stdcall FireLineOfSightProjectile(Fire_0049c9c0* fire, Unit* unit,
                           Vec3* p3, Vec3* p4, int param_5)
{
    Projectile* proj = 0;
    int i = g_game->projectileCount;
    if (i < 0x12c) {
        proj = g_game->projectiles + i;
        g_game->projectileCount = i + 1;
        proj->flags &= ~2;
        proj->targetUnit = 0;
    }
    if (!proj)
        return 0;

    InitProjectile(proj, fire->shot, p3, p4, g_game->ticks, unit);

    int dx = p3->x - p4->x;
    // 4-byte union: only the high half is read, as a 16-bit load from a frame slot.
    union { int value; short halves[2]; } dy;
    dy.value = p3->y - p4->y;
    int dz = p3->z - p4->z;
    short a1 = FUN_004b715a(dx, dz);
    // Unsigned: the >> 16 below is a logical shift.
    unsigned int dist = (int)_hypot(dx, dz);
    proj->range = (int)dist;
    short a2 = FUN_004b715a(-(int)dy.halves[1], (short)(dist >> 16));
    proj->angles.heading = a1;
    proj->angles.pitch = a2;

    // The same helper as 0x49c980, inlined here.
    if (fire->shot->startSpeed) {
        proj->speed = fire->shot->startSpeed;
    } else if (!fire->shot->acceleration) {
        proj->speed = fire->shot->speed;
    } else {
        proj->speed = 0;
    }

    proj->direction.y = FUN_004b70ef(a2, proj->speed);
    int t = FUN_004b7123(a2, proj->speed);
    proj->direction.x = -FUN_004b70ef(a1, t);
    proj->direction.z = -FUN_004b7123(a1, t);

    // The same helper as 0x49c920, inlined here.
    WeaponDef* u = proj->type;
    if ((unsigned int)u->speed != 0 && !(u->flags.value & 0x8000000))
        proj->time = (u->range << 16) / (unsigned int)u->speed + g_game->ticks;
    else
        proj->time = g_game->ticks + u->lifetime;
    proj->targetUnit = param_5;

    proj->active = fire->shot->burst;
    g_seams.units->StartScript((CobScript*)unit->script, g_fireScriptNames[(fire->f_1b >> 2) & 3], 0, 0);
    short angle = unit->weapons[(fire->f_1b >> 2) & 3].fireHeading - unit->heading;
    int a = -FUN_004b70ef(angle, 800);
    int b = -FUN_004b7123(angle, 800);
    g_seams.units->StartScriptWithArgs((CobScript*)unit->script, "RockUnit", 0, 0, 2, b, a, 0, 0);

    if (fire->shot->flags.b.startSmoke)
        EmitWeaponSmoke(p3, 9);
    return 1;
}

#include <string.h>

#pragma pack(push, 1)
struct Shot_0049cc20 {
    char unknown_0[0x8];
    int aimCobDone;                    // +0x8, the slot's aimCob.nAimCobDone
    WeaponDef* def;                    // +0xc, the weapon that fired
    char unknown_10[0x1b - 0x10];
    unsigned char flags;               // +0x1b
};

void __stdcall InitProjectile(Projectile* proj, WeaponDef* shot, Vec3* pos,
                            Vec3* aim, int field_5, Unit* unit);

// FUNCTION: 0x49cc20
int __stdcall FireVLaunchProjectile(Shot_0049cc20* shot, Unit* unit, Vec3* pos,
                           Vec3* aim, int param_5, int param_6)
{
    Projectile* proj = 0;
    if (g_game->projectileCount < 300) {
        proj = &g_game->projectiles[g_game->projectileCount++];
        proj->flags &= ~2;
        proj->targetUnit = 0;
    }
    if (!proj)
        return 0;

    InitProjectile(proj, shot->def, pos, aim, g_game->ticks, unit);
    proj->angles.heading = 0;
    proj->angles.pitch = 0x4000;
    shot->aimCobDone = 0;
    if (shot->def->startSpeed) {
        proj->speed = shot->def->startSpeed;
    } else if (shot->def->acceleration == 0) {
        proj->speed = shot->def->speed;
    } else {
        proj->speed = 0;
    }
    memset(&proj->direction, 0, 12);
    WeaponDef* u = proj->type;
    if (u->speed != 0 && !u->flags.b.noAutoRange) {
        proj->time = (u->range << 16) / (unsigned int)u->speed + g_game->ticks;
    } else {
        proj->time = g_game->ticks + u->lifetime;
    }
    proj->targetUnit = param_5;
    proj->interceptedProjectile = (Projectile*)param_6;
    proj->active = shot->def->burst;
    g_seams.units->StartScript((CobScript*)unit->script, g_fireScriptNames[(shot->flags >> 2) & 3], 0, 0);
    short angle = unit->weapons[(shot->flags >> 2) & 3].fireHeading - unit->heading;
    int a = -FUN_004b70ef(angle, 800);
    int b = -FUN_004b7123(angle, 800);
    g_seams.units->StartScriptWithArgs((CobScript*)unit->script, "RockUnit", 0, 0, 2, b, a, 0, 0);
    if (shot->def->flags.b.startSmoke)
        EmitWeaponSmoke(pos, 9);
    return 1;
}

#pragma pack(push, 1)
struct Shot_0049cde0 {
    char unknown_0[0xc];
    WeaponDef* def;                    // +0xc, the weapon that fired
    unsigned int muzzleAimFromDeltaZ;  // +0x10
    char unknown_14[0x16 - 0x14];
    short heading;                     // +0x16
    short pitch;                       // +0x18
    char unknown_1a[0x1b - 0x1a];
    unsigned char flags;               // +0x1b
};

#pragma pack(pop)

void __stdcall InitProjectile(Projectile* proj, WeaponDef* shot, Vec3* pos,
                            Vec3* aim, int field_5, Unit* unit);

// FUNCTION: 0x49cde0
int __stdcall FireBallisticProjectile(Shot_0049cde0* shot, Unit* unit, Vec3* pos,
                           Vec3* aim, int param_5)
{
    Projectile* proj = 0;
    if (g_game->projectileCount < 300) {
        proj = &g_game->projectiles[g_game->projectileCount++];
        proj->flags &= ~2;
        proj->targetUnit = 0;
    }
    if (proj) {
        InitProjectile(proj, shot->def, pos, 0, g_game->ticks, unit);
        proj->angles.heading = shot->heading;
        proj->angles.pitch = shot->pitch;
        // The whole product is its own statement: inside the assignment the
        // division and multiply sink after the call.
        int q = (shot->muzzleAimFromDeltaZ / shot->def->speed) * g_game->gravity;
        proj->direction.y = FUN_004b70ef(shot->pitch, shot->def->speed) - q;
        int scale = FUN_004b7123(shot->pitch, shot->def->speed);
        proj->direction.x = -FUN_004b70ef(shot->heading, scale);
        proj->direction.z = -FUN_004b7123(shot->heading, scale);
        if (shot->def->flags.b.burnBlow) {
            proj->time = (int)_hypot((double)(pos->x - aim->x), (double)(pos->z - aim->z)) / scale
                       + g_game->ticks;
        } else {
            proj->time = g_game->ticks + shot->def->lifetime;
        }
        // active before targetUnit: the reverse of the natural order is the original's.
        proj->active = shot->def->burst;
        proj->targetUnit = param_5;
        g_seams.units->StartScript((CobScript*)unit->script, g_fireScriptNames[(shot->flags >> 2) & 3], 0, 0);
        short angle = *(int*)&unit->weapons[(shot->flags >> 2) & 3].fireHeading - unit->heading;
        int a = -FUN_004b70ef(angle, 800);
        int b = -FUN_004b7123(angle, 800);
        g_seams.units->StartScriptWithArgs((CobScript*)unit->script, "RockUnit", 0, 0, 2, b, a, 0, 0);
        if (shot->def->flags.b.startSmoke)
            EmitWeaponSmoke(pos, 9);
        return 1;
    }
    return 0;
}

#pragma pack(push, 1)
struct Shot_0049d000 {
    char unknown_0[0xc];
    void* weapon;                     // +0xc
};

#pragma pack(pop)

static inline void Setup_0049d000(Projectile* proj, Unit* u)
{
    proj->angles.heading = u->heading;
    proj->speed = 0;
    proj->direction.y = 0;
}

void __stdcall InitProjectile(Projectile*, void*, void*, int, int, Unit*);

// FUNCTION: 0x49d000
int __stdcall SpawnProjectileFromUnitMotion(Shot_0049d000* shot, Unit* unit, Vec3* pos)
{
    Projectile* proj = 0;
    if (g_game->projectileCount < 300) {
        proj = &g_game->projectiles[g_game->projectileCount++];
        proj->flags &= ~2;
        proj->targetUnit = 0;
    }
    if (proj) {
        InitProjectile(proj, shot->weapon, pos, 0, g_game->ticks, unit);
        Setup_0049d000(proj, unit);
        proj->direction.x = -FUN_004b70ef(proj->angles.heading, ((UnitType_0049d000*)unit->motion)->range);
        proj->direction.z = -FUN_004b7123(proj->angles.heading, ((UnitType_0049d000*)unit->motion)->range);
        return 1;
    }
    return 0;
}

struct Object_0049d0c0 {
    char unknown_0[0xc];
    WeaponDef* info;                   // +0xc
};

int __stdcall FireLineOfSightProjectile(Object_0049d0c0* obj, int a, int b, int c, int d);
int __stdcall FireBallisticProjectile(Object_0049d0c0* obj, int a, int b, int c, int d);

// The last two arguments are passed to both callees in swapped order.
// FUNCTION: 0x49d0c0
int __stdcall FireWeaponByFlags(Object_0049d0c0* obj, int a, int b, int d, int c)
{
    int result = 0;
    if (obj->info->flags.b.lineOfSight || obj->info->flags.b.selfProp)
        result = FireLineOfSightProjectile(obj, a, b, c, d);
    else if (obj->info->flags.b.ballistic)
        result = FireBallisticProjectile(obj, a, b, c, d);
    return result;
}

#pragma pack(push, 1)
struct Entry_0049d120 {              // 0x1c bytes
    char unknown_0[0xc];
    WeaponDef* def;                  // +0xc
    char unknown_10[0x1a - 0x10];
    unsigned char active;            // +0x1a
    char unknown_1b;
};

struct Table_0049d120 {
    char unknown_0[4];
    // Only three entries are declared: the index is masked to a byte, but the
    // fields below have to land on 0x6a and 0xff, which fixes the array size.
    Entry_0049d120 entries[3];       // +0x4
    char unknown_58[0x6a - 0x58];
    Vec3 pos;                        // +0x6a
    char unknown_76[0xff - 0x76];
    char player;                     // +0xff
};


#pragma pack(pop)

// FUNCTION: 0x49d120
Projectile* __stdcall FindTargetableProjectile(Table_0049d120* table, int index)
{
    Entry_0049d120* entry = &table->entries[index & 0xff];
    // Flag read before the radius, shift inside its initialiser: else the flag
    // load hoists into the prologue.
    if (!entry->active)
        return 0;
    int radius = entry->def->coverage << 16;
    // projs is read before projCount: keeps g_game in ecx.
    Projectile* proj = g_game->projectiles;
    int count = g_game->projectileCount;
    for (int i = 0; i < count; i++, proj++) {
        if ((char)proj->owner != table->player) {
            // Nested ifs, not one && chain: else the bitfield test folds into a mem test.
            if (proj->type->flags.b.targetable) {
                if ((unsigned)(table->pos.x - proj->target.x + radius) <= 2u * radius) {
                    if ((unsigned)(table->pos.z - proj->target.z + radius) <= 2u * radius) {
                        int j;
                        for (j = 0; j < g_game->projectileCount; j++)
                            if (g_game->projectiles[j].interceptedProjectile == proj)
                                break;
                        if (j == g_game->projectileCount)
                            return proj;
                    }
                }
            }
        }
    }
    return 0;
}

#pragma pack(push, 1)
struct Event_0049d1e0 {
    char unknown_0[0xd];
    Vec3 pos;                          // +0xd
    char unknown_19;
    unsigned char flag : 1;            // +0x1a
    char unknown_1b[0x1f - 0x1b];
    short ownerId;                     // +0x1f
};
#pragma pack(pop)

static inline int SamePos(Vec3& a, Vec3& b)
{
    return a.x == b.x && a.z == b.z && a.y == b.y;
}

// FUNCTION: 0x49d1e0
Projectile* __stdcall FindRemoteProjectile(Event_0049d1e0* ev)
{
    if (!ev->flag)
        return 0;
    char me = (char)g_game->localPlayer;
    Projectile* proj = g_game->projectiles;
    for (int i = 0; i < g_game->projectileCount; i++, proj++) {
        if ((char)proj->owner != me && SamePos(proj->target, ev->pos) && proj->attacker->id == ev->ownerId)
            return proj;
    }
    return 0;
}

#pragma pack(push, 1)
// Unused here: the symbol ids this declaration takes keep the allocation (docs/c2-regalloc.md).
void Unused_0049d270(int a, int b, int c, int d, int e, int f);
#pragma pack(pop)

static inline int SamePos_0049d270(Vec3& a, Vec3& b)
{
    return a.x == b.x && a.z == b.z && a.y == b.y;
}

// The projectile scan of 0x49d1e0, defined here unannotated so that /Ob2
// inlines it into ApplyWeaponFirePacket as it did in the original. It has no
// callers in the exe. Written with the positive `if (ev->flag) { ... } return 0;`
// test, which also matches 0x49d1e0 on its own; the `if (!ev->flag) return 0;`
// spelling above compiles to the same standalone bytes but, inlined here,
// gives the cursor a register and spills `unit`.
Projectile* __stdcall FindRemoteProjectile_0049d270(WeaponFirePacket* ev)
{
    if (ev->flag) {
        char me = (char)g_game->localPlayer;
        Projectile* proj = g_game->projectiles;
        for (int i = 0; i < g_game->projectileCount; i++, proj++) {
            if ((char)proj->owner != me && SamePos_0049d270(proj->target, ev->aimOrVel) && proj->attacker->id == ev->targetUnitId)
                return proj;
        }
    }
    return 0;
}

void __stdcall InitProjectile(void*, void*, void*, int, int, void*);
void __stdcall FireLineOfSightProjectile(void*, void*, void*, void*, void*);
void __stdcall FireVLaunchProjectile(void*, void*, void*, void*, void*, void*);
void __stdcall FireBallisticProjectile(void*, void*, void*, void*, void*);

// FUNCTION: 0x49d270
void __stdcall ApplyWeaponFirePacket(int arg1, WeaponFirePacket* ev)
{
    unsigned char defIndex = ev->weaponDefIndex;
    WeaponDef* def = &g_game->weapons[defIndex];
    if (def->flags.b.meteor) {
        Vec3* pos = &ev->aimOrVel;
        void* arg = (void*)((char*)ev + 1);
        Projectile* proj = 0;
        if (g_game->projectileCount < 300) {
            proj = &g_game->projectiles[g_game->projectileCount++];
            proj->flags &= 0xfffd;
            proj->targetUnit = 0;
        }
        if (!proj)
            return;
        InitProjectile(proj, def, arg, 0, g_game->ticks, 0);
        proj->direction = *pos;
        return;
    }
    Unit* unit = ev->ownerUnitId == 0 ? 0 : &g_game->units[ev->ownerUnitId];
    if (!unit)
        return;
    if (!unit->alive)
        return;
    Entry_0049d270* entry = (Entry_0049d270*)&unit->weapons[ev->weaponSlotIndex];
    entry->f_18 = ev->firePitch;
    entry->f_16 = ev->fireHeading;
    unsigned short targetId = ev->targetUnitId;
    Unit* target = targetId == 0 ? 0 : &g_game->units[targetId];
    Projectile* found = FindRemoteProjectile_0049d270(ev);
    if (def->flags.b.ballistic) {
        FireBallisticProjectile(entry, unit, (void*)((char*)ev + 1), &ev->aimOrVel, target);
        return;
    }
    if (def->flags.b.vLaunch) {
        FireVLaunchProjectile(entry, unit, (void*)((char*)ev + 1), &ev->aimOrVel, target, found);
        return;
    }
    if (def->flags.b.lineOfSight || def->flags.b.selfProp) {
        FireLineOfSightProjectile(entry, unit, (void*)((char*)ev + 1), &ev->aimOrVel, target);
        return;
    }
    if (def->flags.b.dropped) {
        Projectile* proj = 0;
        if (g_game->projectileCount < 300) {
            proj = &g_game->projectiles[g_game->projectileCount++];
            proj->flags &= 0xfffd;
            proj->targetUnit = 0;
        }
        if (!proj)
            return;
        InitProjectile(proj, entry->type, (void*)((char*)ev + 1), 0, g_game->ticks, unit);
        proj->angles.heading = unit->heading;
        proj->speed = 0;
        proj->direction.y = 0;
        proj->direction.x = -FUN_004b70ef(proj->angles.heading, ((Type_0049d270*)unit->motion)->param);
        proj->direction.z = -FUN_004b7123(proj->angles.heading, ((Type_0049d270*)unit->motion)->param);
        return;
    }
}

#pragma pack(push, 1)
// Unused here: the symbol ids this declaration takes keep the allocation (docs/c2-regalloc.md).
void Unused_0049d580(int a, int b, int c, int d, int e, int f);
#pragma pack(pop)

void __stdcall GetWeaponPiecePosition(Unit* obj, Vec3* out, unsigned char weapon, int piece);
short __stdcall SolveLaunchAngle(int dx, int dy, int dz, int speed, float pitch);
// The weapon parameter must stay an unsigned char.
int __stdcall CalcAimAngles(Unit* unit, WeaponDef* target, short* out_heading,
                           short* out_pitch, unsigned char weapon, Vec3* point);
int __stdcall AimWithinTolerance(Unit* unit, Unit* aim, short angle1, short angle2);
int __stdcall FireLineOfSightProjectile(Unit* fire, Unit* unit, Vec3* p3,
                           Vec3* point, Unit* target);
int __stdcall FireBallisticProjectile(Unit* shot, Unit* unit, Vec3* pos,
                           Vec3* aim, Unit* target);
int __stdcall RandomInt(int range);
int __stdcall BroadcastPacket(int player, void* data, int size);

// FUNCTION: 0x49d580
int __stdcall FireTurretWeapon(Unit* fire, UnitWeaponSlot* unit,
                           Unit* target, Vec3* point)
{
    if ((unit->flags & 1) && unit->aimCob.nAimCobDone) {
        WeaponDef* def = unit->weapon;
        short heading;
        short pitch;
        int ok;
        if (def->flags.b.ballistic) {
            Vec3 p;
            g_seams.units->GetAimFromPosition(fire, &p, unit->flags >> 2 & 3);
            int dx = p.x - point->x;
            int dy = p.y - point->y;
            int dz = p.z - point->z;
            heading = FUN_004b715a(dx, dz) - (unsigned short)fire->heading;
            pitch = SolveLaunchAngle(dx, dy, dz, def->speed, def->pitch);
            ok = (unsigned short)pitch != 0x8000;
        } else if (def->flags.b.lineOfSight) {
            ok = CalcAimAngles(fire, def, (short*)&heading, (short*)&pitch,
                              unit->flags >> 2 & 3, point);
        } else {
            ok = 0;
        }
        if (!ok) {
            unit->flags &= 0xfe;
            ((Fbb_0049d580*)&fire->netDirtyFlags)->f.b12 = 1;
            return 0;
        }
        if (!AimWithinTolerance(fire, (Unit*)unit, heading, pitch)) {
            unit->flags &= 0xfe;
            return 0;
        }
        Vec3 gunpos;
        GetWeaponPiecePosition(fire, &gunpos, unit->flags >> 2 & 3, -1);
        unit->fireHeading += (unsigned short)fire->heading;
        short spread = unit->weapon->accuracy - (short)((fire->health << 11) / fire->def->maxHealth) + 0x800;
        int parts = fire->killCount / 12;
        if (parts > 1)
            spread = (unsigned short)spread / parts;
        if (spread) {
            // Keep `range >> 1` inline, no `half` local.
            unsigned short range = spread;
            unit->fireHeading += (short)(RandomInt(range) - (range >> 1));
            unit->firePitch += (short)(RandomInt(range) - (range >> 1));
        }
        int fired = 0;
        if (unit->weapon->flags.b.lineOfSight || unit->weapon->flags.b.selfProp)
            fired = FireLineOfSightProjectile((Unit*)unit, fire, &gunpos, point, target);
        else if (unit->weapon->flags.b.ballistic)
            fired = FireBallisticProjectile((Unit*)unit, fire, &gunpos, point, target);
        if (!fired)
            return 0;
        unit->aimCob.nAimCobDone = 0;
        unit->flags &= 0xfe;
        if (g_game->netGameFlags & 1) {
            WeaponFirePacket msg;
            msg.type = 0xd;
            msg.origin = gunpos;
            msg.aimOrVel = *point;
            msg.weaponDefIndex = unit->weapon->index;
            msg.weaponSlotIndex = unit->flags >> 2 & 3;
            if (fire == 0)
                msg.ownerUnitId = 0;
            else
                msg.ownerUnitId = fire->id;
            msg.targetUnitId = target ? target->id : 0;
            msg.fireHeading = unit->fireHeading;
            msg.firePitch = unit->firePitch;
            msg.flag = unit->weapon->flags.value >> 30;
            BroadcastPacket(fire->player->id, &msg, 0x24);
        }
        return 1;
    }
    return 0;
}

// SUSPECTED ORIGINAL BUG (kept from an earlier pass, still worth reporting):
// the ballistic path (weapon flag bit 1) stores its heading into argument one's
// home slot, the shooter's pointer, and that slot is what the call to AimWithinTolerance
// at 0x49d681 then reads as its angle1 (`mov edx, dword ptr [esp + 0x58]`). The edi
// register still holds the real shooter pointer, so the first angle checked is the
// low 16 bits of a pointer. Worse, the third argument, the target unit, is read back
// at 0x49d745 from that same clobbered slot, so on this path the `target->f_a8` at
// 0x49d812 and the target handed to FireLineOfSightProjectile at 0x49d77d can both be a pointer's
// low half. `test al, 1` at 0x49d58e means the path is only taken when f_1b bit 0 is
// set, so this is the laser (flags bit 1) aiming path.

#include <stdlib.h>


struct Aim_0049d880 {
    char unknown_0[0xc];
    WeaponDef* type;                   // +0xc
    char unknown_10[0x16 - 0x10];
    short fireHeading;                 // +0x16
    short firePitch;                   // +0x18
};

// True when both of the aim object's angles are within tolerance of the
// corresponding unit angles. With no tolerance in the type, a moved unit
// (flags 0xc) gets a generous one, a still unit a tighter one.
// FUNCTION: 0x49d880
int __stdcall AimWithinTolerance(Unit* unit, Aim_0049d880* aim, short angle1, short angle2)
{
    unsigned short a = aim->type->tolerance;
    int x;
    int y;
    if (a == 0) {
        if (unit->flags & 0xc) {
            x = 2000;
            y = x;
        } else {
            x = 150;
            y = x;
        }
    } else {
        x = a;
        // if/else re-reading the field, not a ternary: it swaps the two tolerances.
        if (aim->type->pitchTolerance)
            y = aim->type->pitchTolerance;
        else
            y = a;
    }
    if (abs((short)(aim->fireHeading - angle1)) <= x
        && abs((short)(aim->firePitch - angle2)) <= y)
        return 1;
    return 0;
}

// FUNCTION: 0x49d910
int __stdcall CalcAimAngles(Unit* unit, Unit* target, short* out_heading, short* out_pitch, int weapon, Vec3* point)
{
    Vec3 p;
    g_seams.units->GetAimFromPosition(unit, &p, weapon);
    int dx = p.x - point->x;
    // Read through the 16.16 union and initialised before the heading: loads only the high word.
    Fixed dy;
    dy.value = p.y - point->y;
    int dz = p.z - point->z;
    *out_heading = (short)(FUN_004b715a(dx, dz) - unit->heading);
    *out_pitch = (short)FUN_004b715a(-dy.whole, (short)((int)_hypot((double)dx, (double)dz) >> 16));
    return 1;
}

#pragma pack(push, 1)
struct Aim_0049d9c0 {
    char unknown_0[0xc];
    WeaponDef* type;                  // +0xc
    char unknown_10[0x16 - 0x10];
    short fireHeading;                // +0x16
    short firePitch;                  // +0x18
    char unknown_1a;
    unsigned char weapon;             // +0x1b
};

// Unused here: the symbol ids this declaration takes keep the allocation (docs/c2-regalloc.md).
void Unused_0049d9c0(int a, int b, int c, int d, int e, int f);
#pragma pack(pop)

int __stdcall AimWithinTolerance(Unit* unit, Aim_0049d9c0* aim, short angle1, short angle2);
int __stdcall FireLineOfSightProjectile(Aim_0049d9c0* aim, Unit* unit, Vec3* aimPos,
                           Vec3* point, Unit* target);

// FUNCTION: 0x49d9c0
int __stdcall FireLineOfSightWeapon(Unit* unit, Aim_0049d9c0* aim,
                           Unit* target, Vec3* point)
{
    Vec3 p;
    GetWeaponPiecePosition(unit, &p, aim->weapon >> 2 & 3, -1);
    int dx = p.x - point->x;
    Fixed dy;
    dy.value = p.y - point->y;
    int dz = p.z - point->z;
    aim->fireHeading = (short)FUN_004b715a(dx, dz);
    aim->firePitch = (short)FUN_004b715a(-dy.whole,
                                       (short)((int)_hypot((double)dx, (double)dz) >> 16));
    if (AimWithinTolerance(unit, aim, unit->heading, unit->pitch)) {
        if (FireLineOfSightProjectile(aim, unit, &p, point, target)) {
            if (g_game->netGameFlags & 1) {
                WeaponFirePacket msg;
                msg.type = 0xd;
                msg.origin = unit->pos;
                msg.aimOrVel = *point;
                msg.weaponDefIndex = aim->type->index;
                msg.weaponSlotIndex = aim->weapon >> 2 & 3;
                // if/else testing the unit, not a ternary: the ternary flips the branches.
                if (unit == 0)
                    msg.ownerUnitId = 0;
                else
                    msg.ownerUnitId = unit->id;
                msg.targetUnitId = target ? target->id : 0;
                msg.fireHeading = aim->fireHeading;
                msg.firePitch = aim->firePitch;
                msg.flag = aim->type->flags.value >> 30;
                BroadcastPacket(unit->player->id, &msg, 0x24);
            }
            return 1;
        }
    }
    return 0;
}

#pragma pack(push, 1)
struct Shot_0049db70 {
    char unknown_0[8];
    int piece;
    WeaponDef* def;
    char unknown_10[6];
    short heading;
    short pitch;
    char unknown_1a;
    unsigned char weapon;
};

struct Kind_0049db70 {
    char unknown_0[4];
    int player;
};

struct Object_0049db70 {
    char unknown_0[0x96];
    Kind_0049db70* kind;
    char unknown_9a[0xa8 - 0x9a];
    short team;
};

// Unused here: the symbol ids this declaration takes keep the allocation (docs/c2-regalloc.md).
void Unused_0049db70(int a, int b, int c, int d, int e, int f);
#pragma pack(pop)

void __stdcall GetWeaponPiecePosition(Object_0049db70* obj, Vec3* out, unsigned char weapon, int piece);
// The weapon index must stay an unsigned char: it makes the shift use byte registers.
int* __stdcall FindTargetableProjectile(Object_0049db70* obj, unsigned char weapon);
int __stdcall FireVLaunchProjectile(Shot_0049db70* shot, Object_0049db70* source, Vec3* pos,
                           Vec3* aim, Object_0049db70* target, int* param_6);
double __cdecl _hypot(double x, double y);
long __cdecl _ftol();

// FUNCTION: 0x49db70
int __stdcall FireVLaunchWeapon(Object_0049db70* source, Shot_0049db70* shot,
                           Object_0049db70* target, Vec3* aim)
{
    Vec3 pos;
    if (shot->piece != 0) {
        GetWeaponPiecePosition(source, &pos, (shot->weapon >> 2) & 3, -1);
        int dx = pos.x - aim->x;
        // dy must be a 4-byte union read by its high half, not a plain int.
        union { int value; short halves[2]; } dy;
        dy.value = pos.y - aim->y;
        int dz = pos.z - aim->z;
        shot->heading = FUN_004b715a(dx, dz);
        int dist = (int)_hypot((double)dx, (double)dz);
        shot->pitch = FUN_004b715a(-(int)dy.halves[1], (short)(dist >> 16));
        // Uninitialised here and zeroed only in the else arm.
        int* p;
        if (shot->def->flags.b.interceptor) {
            p = FindTargetableProjectile(source, (shot->weapon >> 2) & 3);
            if (!p)
                return 0;
        } else {
            p = 0;
        }
        if (FireVLaunchProjectile(shot, source, &pos, aim, target, p)) {
            if (g_game->flags_2a44 & 1) {
                WeaponFirePacket packet;
                packet.type = 0xd;
                packet.origin = pos;
                packet.aimOrVel = *aim;
                packet.weaponDefIndex = shot->def->index;
                packet.weaponSlotIndex = (shot->weapon >> 2) & 3;
                // Keep the `== 0 ? 0 : team` ternaries: they put the zero store on the fall-through.
                packet.ownerUnitId = source == 0 ? 0 : source->team;
                packet.targetUnitId = target == 0 ? 0 : target->team;
                packet.fireHeading = shot->heading;
                packet.firePitch = shot->pitch;
                packet.flag = shot->def->flags.b.interceptor;
                BroadcastPacket(source->kind->player, &packet, 0x24);
            }
            shot->piece = 0;
            shot->weapon &= 0xfe;
            return 1;
        }
    }
    return 0;
}

#pragma pack(push, 1)
struct Aim_0049dd60 {
    char unknown_0[0xc];
    WeaponDef* type;
    char unknown_10[0x16 - 0x10];
    short fireHeading;
    short firePitch;
    char unknown_1a;
    unsigned char weapon;
};

// Unused here: the symbol ids this declaration takes keep the allocation (docs/c2-regalloc.md).
void Unused_0049dd60(int a, int b, int c, int d, int e, int f);
#pragma pack(pop)

void __stdcall InitProjectile(Projectile*, WeaponDef*, Vec3*, int, int, Unit*);

// FUNCTION: 0x49dd60
int __stdcall FireDroppedWeapon(Unit* unit, Aim_0049dd60* aim,
                           Unit* target, Vec3* point)
{
    Vec3 p;
    GetWeaponPiecePosition(unit, &p, aim->weapon >> 2 & 3, -1);
    Projectile* projectile = 0;
    if (g_game->projectileCount < 300) {
        projectile = &g_game->projectiles[g_game->projectileCount++];
        projectile->flags &= ~2;
        projectile->targetUnit = 0;
    }
    if (projectile != 0) {
        InitProjectile(projectile, aim->type, &p, 0, g_game->ticks, unit);
        projectile->angles.heading = unit->heading;
        projectile->speed = 0;
        projectile->direction.y = 0;
        projectile->direction.x = -FUN_004b70ef(projectile->angles.heading, ((Owner_0049dd60*)unit->motion)->id);
        projectile->direction.z = -FUN_004b7123(projectile->angles.heading, ((Owner_0049dd60*)unit->motion)->id);
        if (g_game->netGameFlags & 1) {
            WeaponFirePacket packet;
            packet.type = 0xd;
            packet.origin = p;
            packet.aimOrVel = *point;
            packet.weaponDefIndex = aim->type->index;
            packet.weaponSlotIndex = aim->weapon >> 2 & 3;
            if (unit == 0)
                packet.ownerUnitId = 0;
            else
                packet.ownerUnitId = unit->id;
            if (target == 0)
                packet.targetUnitId = 0;
            else
                packet.targetUnitId = target->id;
            packet.fireHeading = aim->fireHeading;
            packet.firePitch = aim->firePitch;
            packet.flag = aim->type->flags.value >> 30;
            BroadcastPacket(unit->player->id, &packet, 0x24);
        }
        return 1;
    }
    return 0;
}

// Unused here: the symbol ids this declaration takes keep the allocation (docs/c2-regalloc.md).
void Unused_0049df10(int a, int b, int c, int d, int e, int f);
#pragma pack(pop)

int __cdecl GetLocalDpid();
void __stdcall InitProjectile(Projectile*, void*, void*, int, int, void*);

// FUNCTION: 0x49df10
int __stdcall SpawnProjectile(WeaponDef* weapon, Vec3* a, Vec3* b, int flag)
{
    Projectile* proj = 0;
    if (g_game->projectileCount < 300) {
        proj = &g_game->projectiles[g_game->projectileCount++];
        proj->flags &= ~2;
        proj->targetUnit = 0;
    }
    if (!proj)
        return 0;
    InitProjectile(proj, weapon, a, 0, g_game->ticks, 0);
    proj->direction = *b;
    if (flag) {
        // Read into a local first: only then does MSVC hoist the byte load
        // above the g_game->netGameFlagsWord test, into the low byte of the register
        // that held `weapon`, which is what leaves al free for the `lea` and
        // pushes the +0xd store and the type store in their original places.
        unsigned char team = weapon->index;
        if (g_game->flags_2a44 & 1) {
            WeaponFirePacket packet;
            packet.type = 0xd;
            packet.origin = *a;
            packet.aimOrVel = *b;
            packet.weaponDefIndex = team;
            BroadcastPacket(GetLocalDpid(), &packet, 0x24);
        }
    }
    return 1;
}

typedef int (__stdcall* WeaponFireFn)(void* unit, void* aim, void* target, void* point);

int __stdcall FireTurretWeapon(void* unit, void* aim, void* target, void* point);
int __stdcall FireVLaunchWeapon(void* unit, void* aim, void* target, void* point);
int __stdcall FireDroppedWeapon(void* unit, void* aim, void* target, void* point);
int __stdcall FireLineOfSightWeapon(void* unit, void* aim, void* target, void* point);

// Picks the routine for the object's flags.
// FUNCTION: 0x49e010
void __stdcall SetWeaponFireHandler(WeaponDef* weapon)
{
    if (weapon->flags.b.turret) {
        weapon->fire = (WeaponFireFn)FireTurretWeapon;
        return;
    }
    if (weapon->flags.b.vLaunch) {
        weapon->fire = (WeaponFireFn)FireVLaunchWeapon;
        return;
    }
    if (weapon->flags.b.lineOfSight || weapon->flags.b.selfProp)
        weapon->fire = (WeaponFireFn)FireLineOfSightWeapon;
    else if (weapon->flags.b.dropped)
        weapon->fire = (WeaponFireFn)FireDroppedWeapon;
}

// The loop counter and the reload-time maximum share one 8-byte local; the
// counter sits at +0x0 and the maximum at +0x4.
struct Frame_0049e070 {
    unsigned char i;                  // +0x0
    int maxTime;                      // +0x4
};

// The weapon parameters must stay unsigned char: the loop counter slot is passed unwidened.
// FUNCTION: 0x49e070
void __stdcall InitUnitWeaponSlots(Unit* unit)
{
    Frame_0049e070 frame = {0, 0};
    for (frame.i = 0; frame.i < 3; frame.i++) {
        UnitWeaponSlot* s = &unit->weapons[frame.i];
        s->reloadTimer = 0;
        s->flags = (s->flags & 0xf2) | ((frame.i & 3) << 2);
        s->weapon = unit->def->weapons[frame.i];
        // Read index inline with no named temporary; stockpile is zeroed after this line.
        s->flags = (s->flags & 0xfd) | (((unit->def->weapons[frame.i]->index != 0) & 1 | 8) * 2);
        s->stockpile = 0;
        Vec3 a;
        GetWeaponPiecePosition(unit, &a, frame.i, -1);
        Vec3 b;
        g_seams.units->GetAimFromPosition(unit, &b, frame.i);
        s->muzzleAimFromDeltaZ = (a.z - b.z) * 1.25;
        if (s->weapon->reloadTime > frame.maxTime)
            frame.maxTime = s->weapon->reloadTime;
    }
    g_seams.units->StartScriptWithArgs((CobScript*)unit->script, "SetMaxReloadTime", 0, 0, 1, frame.maxTime * 1000 / 30, 0, 0, 0);
}

// The three aim script names. The original's array has three elements, with
// the string "AimTertiary" right after it, so a weapon index of 3 would read
// string bytes as a pointer.
extern char* g_aimScriptNames[3];

void __stdcall SubtractVec3(Vec3* a, Vec3* b, int* dx, int* dy, int* dz);
int __stdcall CalcAimAngles(Unit* unit, WeaponDef* target,
                           unsigned short* out_heading, unsigned short* out_pitch,
                           unsigned char weapon, Vec3* point);
int __stdcall WeaponCanReachPos(Unit* unit, Vec3* a2, Vec3* a3,
                           unsigned char a4);
int __stdcall SendScriptCallByName(Unit* obj, char* name, char field_5, int field_6, int field_a,
                           unsigned short field_e, unsigned short field_12);
void __stdcall UpdateBuildMenuIfFocusUnit(Unit* unit);

// FUNCTION: 0x49e1a0
void __stdcall UpdateUnitWeapons(Unit* unit) {
    unsigned short heading;
    unsigned char i;
    int dz;
    int dx;
    int dy;
    Vec3 pos;
    Vec3 aim;

    for (i = 0; i < 3; i++) {
        Entry_0049e1a0* e = (Entry_0049e1a0*)&unit->weapons[i];
        unsigned char fl = e->flags;
        WeaponDef* attached = e->attached;
        if (!(fl & 2))
            continue;
        if (e->f_14 > 0)
            e->f_14--;
        if (!g_seams.targeting->GetWeaponTargetPos(unit, &pos, i)) {
            e->flags &= 0xfe;
            continue;
        }
        if (attached->fire == 0)
            continue;
        if (attached->flags.b.turret) {
            if (!(e->flags & 1)) {
                WeaponDef* t = e->attached;
                unsigned short angle;
                int ok;
                if (t->flags.b.ballistic) {
                    g_seams.units->GetAimFromPosition(unit, &aim, (unsigned char)((e->flags >> 2) & 3));
                    SubtractVec3(&aim, &pos, &dx, &dy, &dz);
                    heading = (unsigned short)(FUN_004b715a(dx, dz) - unit->heading);
                    angle = SolveLaunchAngle(dx, dy, dz, t->speed, t->pitch);
                    ok = (angle != 0x8000);
                } else if (t->flags.b.lineOfSight) {
                    ok = CalcAimAngles(unit, t, &heading, &angle, (unsigned char)(e->flags >> 2 & 3),
                                      &pos);
                } else {
                    ok = 0;
                }
                if (ok) {
                    e->f_18 = angle;
                    e->f_16 = heading;
                    e->f_8 = 0;
                    // Name read afresh for each call, no cached local: lets the two tails merge.
                    g_seams.units->StartScriptWithArgs((CobScript*)unit->script, g_aimScriptNames[(e->flags >> 2) & 3], (Callback*)&e->name, 0, 2,
                                               heading, angle, 0, 0);
                    SendScriptCallByName(unit, g_aimScriptNames[(e->flags >> 2) & 3], 2, heading, angle, 0, 0);
                    e->flags |= 1;
                }
            }
        } else {
            // Via a bool local: testing the bitfield in place shifts the register rotation.
            bool armed = attached->flags.b.vLaunch;
            if (armed && (!attached->flags.b.stockpile || e->f_1a) && !(e->flags & 1)) {
                e->f_8 = 0;
                g_seams.units->StartScriptWithArgs((CobScript*)unit->script, g_aimScriptNames[(e->flags >> 2) & 3], (Callback*)&e->name, 0, 2, 0, 0,
                                           0, 0);
                SendScriptCallByName(unit, g_aimScriptNames[(e->flags >> 2) & 3], 2, 0, 0, 0, 0);
                e->flags |= 1;
            }
        }
        if (e->f_14 != 0)
            continue;
        if (WeaponCanReachPos(unit, &unit->pos, &pos, i)) {
            int can = 0;
            if (attached->flags.b.stockpile) {
                if (e->f_1a)
                    can = 1;
            } else {
                if (unit->resourceSlot.player->energy >= attached->energyPerShot &&
                    unit->resourceSlot.player->metal >= attached->metalPerShot)
                    can = 1;
            }
            if (can == 0)
                continue;
            Unit* fired = g_seams.targeting->GetWeaponTargetUnit(unit, i);
            if (((WeaponFireFn)attached->fire)(unit, &e->point, fired, &pos) == 0)
                continue;
            if (attached->flags.b.stockpile) {
                e->f_1a--;
                UpdateBuildMenuIfFocusUnit(unit);
            } else {
                int n = unit->killCount / 5;
                if (n > 5)
                    n = 5;
                int q = unit->health * 20 / unit->def->maxHealth;
                int pct = 100 - n * 6;
                e->f_14 = (short)((120 - q) * (pct * attached->reloadTime / 100) / 100);
            }
            int m = (attached->flags.b.commandFire) ? 0x800 : 0x400;
            *(unsigned short*)&unit->netDirtyFlags |= m;
            if (!attached->flags.b.stockpile)
                g_seams.economy->SpendEnergyAndMetal(&unit->resourceSlot, attached->energyPerShot, attached->metalPerShot);
        } else {
            ((unsigned char*)&unit->netDirtyFlags)[1] |= 0x10;
        }
    }
}

// FUNCTION: 0x49e570
void __stdcall SubtractVec3(int* a, int* b, int* dx, int* dy, int* dz)
{
    *dx = a[0] - b[0];
    *dy = a[1] - b[1];
    *dz = a[2] - b[2];
}

extern "C" int __cdecl _strcmpi(const char* str1, const char* str2);

// FUNCTION: 0x49e5b0
char* __stdcall FindWeaponByName(char* name)
{
    if (name == 0 || strlen(name) == 0)
        return 0;
    for (int i = 0; i < 0x100; i++) {
        char* entry = g_game->weapons[i].name;
        if (_strcmpi(entry, name) == 0)
            return entry;
    }
    return 0;
}

// Unused here: real functions declared for their symbol ids, which keep
// 0x49b090 and 0x49be60 matching (docs/c2-regalloc.md).
void DrawTriangleContours(void*, int, int, int, int, int, int);
void DrawRangeCircle(void*, void*, void*, int, int, const char*, int);
void AddOrder(int, int, void*, void*, void*, int, int);
void IssueOrCancelOrder(unsigned char, int, void*, int, int*, int, int);
int SendScriptCall(void*, short, char, int, int, int, int);
void AddTextGadget(void*, char*, char*, int, short, int, int);
int HAPINET_updategameinfo(void*, char*, char*, int, int, int, int);
int IsFootprintVisible(void*, int, int, int, int, short);
void DrawProgressBar(void*, int, int, void*, void*, int);
void IssueOrderToSelection(void*, unsigned char, int, int*, int, int);
void WriteScreenshot(char*, char*, int, int, int, int);
void DrawTextClipped(void*, char*, int, int, int, int);
int OpenInputDialog(void*, char*, char*, char*, char*, int);
int ScanDirectory(char*, char*, char*, int, int, int);
double DotProduct(float, float, float, float, float, float);
void DrawFrameScaledBlended(void*, void*, int, int, double, double);
void DrawFrameScaled(void*, void*, int, int, double, double);
void* DrawLitLine(void*, int, int, int, int, int);
void* DrawBlendedLine(void*, int, int, int, int, int);
void OrderSquad(void*, int, unsigned char, int, void*, int, int, int);
int AddBlinkWord(void*, const char*, int, int, int, int, float, float);
void CenterCameraOnRadarClick();

// 0x49b090 and 0x49be60 sit here, after the module's other functions, and
// 0x49be60 after the TdfFile global below.
#pragma pack(push, 1)

// The feature records are an array of these, indexed as `features[f * 256]`.
struct MapFeature_0049b090 {
    char unknown_0[0xfa];
    unsigned char height;             // +0xfa
    char unknown_fb[0x100 - 0xfb];
};

#pragma pack(pop)

// The projectile collision test. It looks up the map cell holding the
// projectile; with none it drops the camera follow, sets the dead flag and
// returns. Otherwise it kills a projectile that reaches the one it is chasing,
// sets the projectile's floor radius, tests the cell's two units against the
// owner and their height window, resolves the map feature (including the
// 0xfffe "read the neighbour cell" case), and applies the flag rules, the sea
// level ceiling and the net game check before the final kill.
// The 0xfffe reload path re-tests the feature id only against 0xfffb and skips
// the feature count check, so an id read from the neighbouring cell indexes
// g_game->features unchecked (an original bug).
// FUNCTION: 0x49b090
void __stdcall CheckProjectileCollision(WeaponDef* type, Projectile* proj)
{
    // No `Game* g = g_game` local: g_game is read at each use to stay in edi.
    // Named pos local, used again after the lookup: gives the original's prologue.
    Vec3* pos = &proj->position;
    Cell* cell = g_seams.los_radar->GetMapCellAtPosition(pos);

    if (!cell) {
        if (proj == g_game->tracked) {
            g_game->trackedPos = g_game->tracked->position;
            g_game->value_1434b = proj->type->holdTime;
            g_game->tracked = 0;
        }
        proj->flagBits.dead = 1;
        return;
    }
    if (proj->interceptedProjectile) {
        int dx = pos->x - proj->interceptedProjectile->position.x;
        int dy = pos->y - proj->interceptedProjectile->position.y;
        int dz = pos->z - proj->interceptedProjectile->position.z;
        int r = proj->type->areaOfEffect;
        int d = (int)(((__int64)dx * dx) >> 32) + (int)(((__int64)dy * dy) >> 32)
            + (int)(((__int64)dz * dz) >> 32);
        if (d < r * r)
            DetonateProjectile(proj, 0);
    }
    proj->radius = (cell->high + cell->low) / 2;
    if (cell->unit) {
        Unit* u = &g_game->units[cell->unit];
        if (u->playerIndex != proj->owner && proj->position.y < u->def->modelMaxY + u->pos.y) {
            DetonateProjectile(proj, u);
            return;
        }
    }
    if (cell->unit2) {
        Unit* u = &g_game->units[cell->unit2];
        if (u->playerIndex != proj->owner) {
            if (proj->position.y >= u->def->modelMinY + u->pos.y
                && proj->position.y <= u->def->modelMaxY + u->pos.y) {
                DetonateProjectile(proj, u);
                return;
            }
        }
    }
    // Mask tests (here bit 14, unitsOnly, and bit 16, waterWeapon below), not the
    // bit-fields: the bit-field form changes the code.
    if (type->flags.value & 0x4000)
        return;
    {
        short cx = proj->positionWords.xWhole / 16;
        short cz = proj->positionWords.zWhole / 16;
        unsigned short f = cell->feature;
        MapFeature_0049b090* mf;
        // The `!(f == 0xfffe)` chain shape sets the block order.
        if (f < 0xfffb) {
            if (f < g_game->featureCount)
                mf = (MapFeature_0049b090 *)g_game->features + f;
            else
                mf = 0;
        } else if (!(f == 0xfffe)) {
            mf = 0;
        } else {
            // This index spelling and `unsigned short f2` merge the feature tails.
            int n = cell->origin.offsetX + g_game->width * cell->origin.offsetZ;
            unsigned short f2 = (cell - n)->feature;
            if (f2 >= 0xfffb) {
                mf = 0;
            } else {
                mf = (MapFeature_0049b090 *)g_game->features + f2;
            }
        }
        if (mf) {
            // Positive direction (sum > hi, else mf = 0): sets the compare operand order.
            if (mf->height + cell->low > proj->positionWords.yWhole) {
                if (proj->cellX == cx && proj->cellZ == cz) {
                    mf = 0;
                } else {
                    proj->cellX = cx;
                    proj->cellZ = cz;
                }
            } else {
                mf = 0;
            }
        }
        if (mf) {
            DetonateProjectile(proj, 0);
            return;
        }
    }
    if (cell->low > proj->positionWords.yWhole) {
        if (type->flags.b.groundBounce) {
            proj->direction.y = -(proj->direction.y >> 2);
            return;
        }
    } else if (type->flags.value & 0x10000) {
        return;
    } else if (proj->positionWords.yWhole >= g_game->seaLevel) {
        return;
    } else if (((Net *)g_game->net)->noSeaLevelTrigger) {
        return;
    }
    DetonateProjectile(proj, 0);
}

#include "../util/tdf.h"

// A global object of the 12-byte class constructed by 0x4c2ea0 and destroyed
// by 0x4c2eb0. The compiler generates its initialiser (0x49e610) and the
// destructor it registers with atexit (0x49e630).
// FUNCTION: 0x49e610 _$E5
// FUNCTION: 0x49e630 _$E3
TdfFile g_tdfGlobalParser;

// 0x49be60 sits after the TdfFile global: its palette and scroll register plan
// follows the symbol ids.
#include "../util/angles.h"

#pragma pack(push, 1)
struct Sprite_0049be60 {
    char unknown_0[0x30];
    void* child;                       // +0x30
};

#pragma pack(pop)

void* __stdcall GetGafFrame(void* gaf, int frame);
void __stdcall DrawFrame(void* dest, void* src, int x, int y);
void __stdcall DrawFrameBlended(void* dest, void* src, int x, int y);
void __stdcall DrawModel3doProjected(void* dest, Vec3* pos, void* sprite, void* rect);
int __stdcall PointInRect(void* region, int x, int y);
void __stdcall DrawLens(void* dest, void* src, int x, int y);
void __stdcall DrawLine(void* dest, int x1, int y1, int x2, int y2, unsigned int color);
int __stdcall GetGafFrameCount(void* gaf);
int __stdcall IsPointVisible(Player* pi, Vec3* pos);
int HarnessViewRand();

// The projectile render pass: it walks the projectile array and draws every
// live projectile (counter 0) that the local player can see, by the shot kind
// (0 to 7). With map-flags bit 2 set the visibility test reads the player's
// explored-cell grid, otherwise it calls IsPointVisible.
//  - Kind 0's two line arms each make both DrawLine calls (colour2, then
//    colour1).
//  - Kinds 1, 3 and 6 pass a 3-short angle struct (the projectile's +0x34);
//    kind 3 passes its own `rot3`, which is never written (docs/bugs.md).
//  - Kind 7's jitter is 64-bit, `(rand() * 11) / 0x8000 - 5`, added to the
//    high shorts of pt.
// FUNCTION: 0x49be60
void __stdcall DrawProjectiles(void* surface)
{
    WeaponDef* type;
    int fr;
    Vec3 sp;
    Vec3 pt;
    Vec3 prev;
    Vec3 d;
    __int64 n64;
    int time = g_game->ticks;
    void* frame0 = GetGafFrame(g_game->gaf_1480f, 0);
    int index = 0;
    int visible;
    if (g_game->projectileCount <= 0)
        return;
    int offset = 0;
    while (1) {
        Projectile* p = (Projectile*)((char*)g_game->projectiles + offset);
        if (p->active == 0) {
            unsigned char player = (char)g_game->playerIndex;
            Player* pb = &g_game->players_1b63[player];
            Vec3* pos = &p->position;
            if ((g_game->mapFlags & 2) == 2) {
                int col = (int)*(short*)((char*)pos + 2) >> 5;
                int row = ((int)*(short*)((char*)pos + 10)
                           - ((int)*(short*)((char*)pos + 6) >> 1)) >> 5;
                if ((unsigned int)col < (unsigned int)pb->exploredWidth && (unsigned int)row < (unsigned int)pb->exploredHeight
                    && pb->explored[(unsigned int)pb->exploredWidth * row + col])
                    visible = 1;
                else
                    visible = 0;
            } else {
                visible = IsPointVisible(pb, pos);
            }
            if (visible) {
                type = p->type;
                if (type->shotKind == 0) {
                    unsigned int color1 = g_game->colors_dcb[type->colour];
                    unsigned int color2 = g_game->colors_dcb[type->colour2];
                    int x1;
                    int y1;
                    int x2;
                    int y2;
                    x1 = (int)*(short*)((char*)pos + 2) - (short)g_game->scrollX + 0x80;
                    y1 = ((int)*(short*)((char*)pos + 10)
                              - ((int)*(short*)((char*)pos + 6) >> 1))
                             - (short)g_game->scrollY + 0x20;
                    x2 = (int)*(short*)((char*)&p->start + 2) - (short)g_game->scrollX + 0x80;
                    y2 = ((int)*(short*)((char*)&p->start + 10)
                              - ((int)*(short*)((char*)&p->start + 6) >> 1))
                             - (short)g_game->scrollY + 0x20;
                    if (type->colour2 != 0) {
                        // Both calls stay in each arm: one shared colour1 call swaps the colour slots.
                        if (abs(x1 - x2) > abs(y1 - y2)) {
                            if (x1 > x2) {
                                int t = x1; x1 = x2; x2 = t;
                                t = y1; y1 = y2; y2 = t;
                            }
                            DrawLine(surface, x1, y1 - 1, x2, y2 - 1, color2);
                            DrawLine(surface, x1, y1, x2, y2, color1);
                        } else {
                            if (y1 > y2) {
                                int t = x1; x1 = x2; x2 = t;
                                t = y1; y1 = y2; y2 = t;
                            }
                            DrawLine(surface, x1 - 1, y1, x2 + 1, y2, color2);
                            DrawLine(surface, x1, y1, x2, y2, color1);
                        }
                    } else {
                        DrawLine(surface, x1, y1, x2, y2, color1);
                    }
                } else if (type->shotKind == 1) {
                    sp.x = pos->x - (g_game->scrollX << 16);
                    sp.y = p->position.y;
                    sp.z = p->position.z - (g_game->scrollY << 16);
                    int sx = 0x80 + (int)*(short*)((char*)&sp + 2);
                    int sy = (int)*(short*)((char*)&sp + 10) - ((unsigned short)p->groundHeightAvg >> 1) + 0x20;
                    DrawFrameBlended(surface, frame0, sx, sy);
                    Angles16 rot = p->angles;
                    rot.heading += 0x8000;
                    rot.pitch += 0x8000;
                    DrawModel3doProjected(surface, &sp, type->model, &rot);
                    Sprite_0049be60* s = (Sprite_0049be60*)type->model;
                    if (0 != s->child && p->time > time) {
                        if (type->flags.b.propeller) {
                            rot.bank = p->propellerSpin;
                            DrawModel3doProjected(surface, &sp, s->child, &rot);
                        } else {
                            DrawModel3doProjected(surface, &sp, s->child, &rot);
                        }
                    }
                } else if (type->shotKind == 2) {
                    int sx = (int)*(short*)((char*)pos + 2) - (short)g_game->scrollX + 0x80;
                    int sy = ((int)*(short*)((char*)pos + 10)
                              - ((int)*(short*)((char*)pos + 6) >> 1))
                             - (short)g_game->scrollY + 0x20;
                    if (PointInRect(g_game->region_37e27, sx, sy) == 0)
                        return;
                    DrawLens(surface, g_game->explosionLensFrame, sx, sy);
                } else if (type->shotKind == 3) {
                    sp.x = pos->x - (g_game->scrollX << 16);
                    sp.y = p->position.y;
                    sp.z = p->position.z - (g_game->scrollY << 16);
                    int sx = (int)*(short*)((char*)&sp + 2) + 0x80;
                    int sy = (int)*(short*)((char*)&sp + 10) - ((unsigned short)p->groundHeightAvg >> 1) + 0x20;
                    DrawFrameBlended(surface, frame0, sx, sy);
                    Angles16 rot3;
                    DrawModel3doProjected(surface, &sp, type->model, &rot3);
                } else if (type->shotKind == 4) {
                    if (type->colour < 0xff) {
                        void* gaf = 0;
                        sp.x = pos->x - (g_game->scrollX << 16);
                        sp.y = p->position.y;
                        sp.z = p->position.z - (g_game->scrollY << 16);
                        DrawFrameBlended(surface, frame0,
                                     (int)*(short*)((char*)&sp + 2) + 0x80,
                                     (int)*(short*)((char*)&sp + 10) - ((unsigned short)p->groundHeightAvg >> 1) + 0x20);
                        int sy = ((int)*(short*)((char*)&sp + 10)
                                  - ((int)*(short*)((char*)&sp + 6) >> 1)) + 0x20;
                        int sx = (int)*(short*)((char*)&sp + 2) + 0x80;
                        switch (type->colour) {
                        case 0: gaf = g_game->gaf_147bb; break;
                        case 1: gaf = g_game->gaf_147bf; break;
                        case 2: gaf = g_game->gaf_147c3; break;
                        case 3: gaf = g_game->gaf_147c7; break;
                        case 4: gaf = g_game->gaf_147cb; break;
                        }
                        if (gaf) {
                            int n = *(unsigned short*)gaf;
                            DrawFrame(surface, GetGafFrame(gaf, (time - p->spawnTick) % n), sx, sy);
                        }
                    }
                } else if (type->shotKind == 5) {
                    int sx = (int)*(short*)((char*)pos + 2) - (short)g_game->scrollX + 0x80;
                    int sy = ((int)*(short*)((char*)pos + 10)
                              - ((int)*(short*)((char*)pos + 6) >> 1))
                             - (short)g_game->scrollY + 0x20;
                    void* gaf = g_game->unknown_147f3;
                    int n = GetGafFrameCount(gaf);
                    fr = n - ((p->time - time) * n) / (int)type->lifetime;
                    if (fr >= 0 && fr < n) {
                        DrawFrameBlended(surface, GetGafFrame(gaf, fr), sx, sy);
                    }
                } else if (type->shotKind == 6) {
                    sp.x = pos->x - (g_game->scrollX << 16);
                    sp.y = p->position.y;
                    sp.z = p->position.z - (g_game->scrollY << 16);
                    int sy = (int)*(short*)((char*)&sp + 10) - ((unsigned short)p->groundHeightAvg >> 1) + 0x20;
                    int sx = (int)*(short*)((char*)&sp + 2) + 0x80;
                    DrawFrameBlended(surface, frame0, sx, sy);
                    DrawModel3doProjected(surface, &sp, type->model, &p->angles);
                } else if (type->shotKind == 7) {
                    unsigned int color = g_game->colors_dcb[type->colour];
                    Vec3* start = &p->start;
                    d.x = pos->x - start->x;
                    d.y = pos->y - start->y;
                    d.z = pos->z - start->z;
                    union { int i; short s[2]; } nSeg;
                    nSeg.i = (int)(((__int64)((int)sqrt(d.x * (double)d.x + (double)d.y * d.y + (double)d.z * d.z)) << 16) / 0x50000);
                    if (0 != nSeg.i) {
                        n64 = nSeg.i;
                        d.x = (int)(((__int64)d.x << 16) / n64);
                        d.y = (int)(((__int64)d.y << 16) / n64);
                        d.z = (int)(((__int64)d.z << 16) / n64);
                        for (int outer = 0; outer < 2; outer++) {
                            pt = *start;
                            sp = *start;
                            {
                                short n = nSeg.s[1];
                                if (n > 0) {
                                    int i = n;
                                    do {
                                        // The unused z stays: it gives pt.z the extra reference
                                        // that sets the register assignment.
                                        int z = pt.z;  // unused; needed for the match
                                        prev = pt;
                                        sp.x += d.x;
                                        sp.y += d.y;
                                        sp.z += d.z;
                                        pt = sp;
                                        *(short*)((char*)&pt + 2) +=
                                            (short)((int)(((__int64)HarnessViewRand() * 11) / 0x8000) - 5);
                                        *(short*)((char*)&pt + 6) +=
                                            (short)((int)(((__int64)HarnessViewRand() * 11) / 0x8000) - 5);
                                        *(short*)((char*)&pt + 10) +=
                                            (short)((int)(((__int64)HarnessViewRand() * 11) / 0x8000) - 5);
                                        DrawLine(surface,
                                            (int)*(short*)(2 + (char*)&prev) - (short)g_game->scrollX + 0x80,
                                            ((int)*(short*)(10 + (char*)&prev)
                                             - ((int)*(short*)((char*)&prev + 6) >> 1))
                                            - (short)g_game->scrollY + 0x20,
                                            (int)*(short*)((char*)&pt + 2) - (short)g_game->scrollX + 0x80,
                                            0x20 + (((int)*(short*)((char*)&pt + 10)
                                             - ((int)*(short*)(6 + (char*)&pt) >> 1))
                                            - (short)g_game->scrollY),
                                            color);
                                        i = i - 1;
                                    } while (i != 0);
                                }
                            }
                        }
                    }
                }
            }
        }
        index = index + 1;
        offset += 0x6b;
        if (index >= g_game->projectileCount)
            break;
    }
}
