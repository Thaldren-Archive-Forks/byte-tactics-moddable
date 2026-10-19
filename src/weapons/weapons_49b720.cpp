// Decompiled by deepseek-v4.1-flash, finished by GPT-6.1-sol, finished by GPT-6, finished by deepseek-v4.1-flash, finished by deepseek-v4.1-flash, finished by deepseek-v4.1-flash, finished by claude-opus-5-5. Names are provisional.
//
// The projectile update loop. One if/else-if chain over the type's flag
// bits (selfProp guided, lineOfSight straight, ballistic timed drift, dropped
// drift, meteor tumbling), with
// a burst-fire branch for projectiles whose counter is not 0.
// Remove() is the inlined "deselect and mark dead" block that 0x49b090 also
// writes out.

// Needed: without <stdio.h> the b0 arm loads g_game first and gets a 5-byte mov.
#include <stdio.h>

#pragma pack(push, 1)

struct Vec3_0049b720 {
    int x;
    union {
        int y;
        struct {
            unsigned short lo;
            short hi;
        } yw;
    };
    int z;
    Vec3_0049b720& operator+=(const Vec3_0049b720& o)
    {
        x += o.x;
        y += o.y;
        z += o.z;
        return *this;
    }
};

#include "weapon_def.h"



#include "../units/unit.h"

union Word_0049b720 {
    int i;
    struct {
        unsigned short lo;
        short hi;
    } s;
};

struct ProjFlags_0049b720 {
    unsigned short b0 : 1;
    unsigned short dead : 1;
    unsigned short b2 : 2;
    unsigned short state : 2;
    unsigned short rest : 10;
};

struct Proj_0049b720 {
    WeaponDef* type;                   // +0x0
    Vec3_0049b720 pos;                 // +0x4
    Vec3_0049b720 start;               // +0x10
    Vec3_0049b720 vel;                 // +0x1c
    char unknown_28[0x34 - 0x28];
    short roll;                        // +0x34
    short heading;                     // +0x36
    short pitch;                       // +0x38
    unsigned int speed;                // +0x3a
    unsigned int range;                // +0x3e
    unsigned int f42;                  // +0x42
    unsigned int f46;                  // +0x46
    unsigned int f4a;                  // +0x4a
    int f4e;                           // +0x4e
    Unit* unit;                        // +0x52
    void* f56;                         // +0x56
    char unknown_5a[0x60 - 0x5a];
    unsigned short counter;            // +0x60
    unsigned short piece;              // +0x62
    short f64;                         // +0x64
    char unknown_66[0x69 - 0x66];
    ProjFlags_0049b720 flags;          // +0x69
};

struct Net_0049b720 {
    char unknown_0[0xd48];
    int noSeaLevelTrigger;
};

#include "../game/game.h"

#include "../map/cell.h"

#pragma pack(pop)

extern Game* g_game;

void __stdcall CompactProjectiles();
void __stdcall CheckProjectileCollision(WeaponDef* type, Proj_0049b720* p);
Vec3_0049b720* __stdcall GetProjectileAimPoint(Proj_0049b720* p);
int __stdcall TurnUnitTowardsPoint(Proj_0049b720* p, Vec3_0049b720* target);
void __stdcall DetonateProjectile(Proj_0049b720* p, void* unit);
void __stdcall GetWeaponPiecePosition(Unit* unit, Vec3_0049b720* out, unsigned char weapon, int piece);
int __stdcall PlaySoundAt(int sound, Vec3_0049b720* pos, int flag);
void __stdcall EmitWhiteSmoke(Vec3_0049b720* pos, short kind);
#include "../../interfaces/seams.h"
void __stdcall AddExplosionEffect(Vec3_0049b720* pos, void* src, int index, int flag);
int __stdcall RandomInt(int range);
int __cdecl FUN_004b70ef(short angle, int scale);
int __cdecl FUN_004b7123(short angle, int scale);

// A copy of AllocProjectile (matched in its own file), the function just before
// this one in the original source; defined here so that /Ob2 inlines it.
Proj_0049b720* AllocProjectile()
{
    Proj_0049b720* p = 0;
    if (g_game->projectileCount < 300) {
        p = &(*(Proj_0049b720 **)&g_game->projectiles)[g_game->projectileCount++];
        p->flags.dead = 0;
        p->f4e = 0;
    }
    return p;
}

// Gravity goes through this pointer store, not `p->vel.y -= ...`: it makes
// p->type get reloaded, so `type` is not propagated.
static inline void Fall(Vec3_0049b720* v)
{
    v->y -= g_game->gravity;
}

static inline void Remove(Proj_0049b720* p)
{
    if (p == (*(Proj_0049b720 **)&g_game->tracked)) {
        (*(Vec3_0049b720*)&g_game->trackedPos) = (*(Proj_0049b720 **)&g_game->tracked)->pos;
        (*(unsigned short*)&g_game->value_1434b) = p->type->holdTime;
        (*(Proj_0049b720 **)&g_game->tracked) = 0;
    }
    p->flags.dead = 1;
}

// Stays in its own file: the b0 arm's register tie follows this file's symbol
// ids, which the joined weapons.cpp moves.
// FUNCTION: 0x49b720
void UpdateProjectiles()
{
    int n = g_game->projectileCount;
    for (int i = 0; i < n; i++) {
        Proj_0049b720* p = &(*(Proj_0049b720 **)&g_game->projectiles)[i];
        // type is read before oldY: orders the two spill stores.
        WeaponDef* type = p->type;
        int oldY = p->pos.yw.hi;

        if (p->counter != 0) {
            if ((*(unsigned int*)&g_game->ticks) >= p->f42 + type->burstRate) {
                if (type->burstRate >= 5 || (p->counter & 1)) {
                    unsigned char w;
                    for (w = 0; w < 3; w++) {
                        if (p->unit->weapons[w].weapon == type)
                            break;
                    }
                    GetWeaponPiecePosition(p->unit, &p->pos, w, p->piece);
                }
                p->counter--;
                p->f42 += type->burstRate;
                Proj_0049b720* q = AllocProjectile();
                if (q) {
                    *q = *p;
                    q->f42 = (*(unsigned int*)&g_game->ticks);
                    if (type->flags.b.soundTrigger)
                        PlaySoundAt(type->sound, &p->pos, 0);
                    if (type->lifetime != 0)
                        q->f46 = (*(unsigned int*)&g_game->ticks) + type->lifetime;
                    else
                        q->f46 = (p->range + 0x100000) / p->speed + (*(unsigned int*)&g_game->ticks);
                    if (type->lifeRand != 0)
                        q->f46 += RandomInt(type->lifeRand) - (type->lifeRand >> 1);
                    q->counter = 0;
                    if (type->spread != 0) {
                        short ang = RandomInt(type->spread) + (short)(p->heading - (type->spread >> 1));
                        int t = FUN_004b7123(p->pitch, type->speed);
                        p->vel.x = -FUN_004b70ef(ang, t);
                        p->vel.z = -FUN_004b7123(ang, t);
                    }
                }
                if (p->counter == 0)
                    Remove(p);
            }
            continue;
        }

        if (type->flags.b.propeller)
            p->f64 += 0x400;

        if (type->flags.b.selfProp) {
            if (p->f46 > (*(unsigned int*)&g_game->ticks)) {
                if (type->flags.b.waterWeapon && p->pos.yw.hi >= g_game->seaLevel) {
                    Fall(&p->vel);
                    p->pitch = 0;
                } else {
                    int seek = 0;
                    if (p->speed < type->speed) {
                        p->speed += type->acceleration;
                        if (p->speed > type->speed)
                            p->speed = type->speed;
                    }
                    if (type->flags.b.twoPhase) {
                        if (p->flags.state > 0)
                            seek = 1;
                    } else if (type->flags.b.guidance) {
                        seek = 1;
                    }
                    if (seek) {
                        Vec3_0049b720* target = GetProjectileAimPoint(p);
                        if (TurnUnitTowardsPoint(p, target) == 0)
                            DetonateProjectile(p, 0);
                    }
                    p->vel.y = FUN_004b70ef(p->pitch, p->speed);
                    int t = FUN_004b7123(p->pitch, p->speed);
                    p->vel.x = -FUN_004b70ef(p->heading, t);
                    p->vel.z = -FUN_004b7123(p->heading, t);
                }
            } else if (type->flags.b.burnBlow) {
                DetonateProjectile(p, 0);
            } else {
                Fall(&p->vel);
                if (type->flags.b.twoPhase) {
                    if (p->flags.state == 0) {
                        p->f46 = (*(unsigned int*)&g_game->ticks) + p->type->flightTime;
                        p->flags.state++;
                        if (!p->type->flags.b.tracks) {
                            p->f56 = 0;
                            p->f4e = 0;
                        }
                    }
                }
            }
            p->pos += p->vel;
            CheckProjectileCollision(type, p);
        } else if (type->flags.b.lineOfSight) {
            if (p->f46 > (*(unsigned int*)&g_game->ticks)) {
                p->pos += p->vel;
                if (type->flags.b.beamWeapon) {
                    if (p->flags.b0)
                        p->start += p->vel;
                    else if ((*(unsigned int*)&g_game->ticks) > p->f42 + type->duration)
                        p->flags.b0 = 1;
                }
                CheckProjectileCollision(type, p);
            } else {
                Remove(p);
            }
        } else if (type->flags.b.ballistic) {
            if (type->lifetime != 0) {
                if (p->f46 > (*(unsigned int*)&g_game->ticks)) {
                    p->pos += p->vel;
                    p->pos += (*(Vec3_0049b720*)&g_game->wind);
                    p->vel.y -= g_game->gravity;
                    CheckProjectileCollision(type, p);
                } else if (type->flags.b.burnBlow) {
                    DetonateProjectile(p, 0);
                } else {
                    EmitWhiteSmoke(&p->pos, 9);
                    Remove(p);
                }
            } else {
                p->pos += p->vel;
                p->pos += (*(Vec3_0049b720*)&g_game->wind);
                p->vel.y -= g_game->gravity;
                CheckProjectileCollision(type, p);
            }
        } else if (type->flags.b.dropped) {
            p->pos += p->vel;
            p->pos += (*(Vec3_0049b720*)&g_game->wind);
            p->vel.y -= g_game->gravity;
            CheckProjectileCollision(type, p);
        } else if (type->flags.b.meteor) {
            p->pos += p->vel;
            p->roll += ((short*)&p->vel.x)[1] << 8;
            p->pitch += ((short*)&p->vel.z)[1] << 8;
            CheckProjectileCollision(type, p);
        }

        if (!p->flags.dead) {
            if (type->flags.b.smokeTrail && p->f46 > (*(unsigned int*)&g_game->ticks) && p->f4a < (*(unsigned int*)&g_game->ticks)) {
                EmitWhiteSmoke(&p->pos, 9);
                p->f4a += type->smokeRate;
            }
            if (oldY > g_game->seaLevel && p->pos.yw.hi <= g_game->seaLevel) {
                Cell* cell = g_seams.los_radar->GetMapCellAtPosition((Vec3*)&p->pos);
                if (cell && cell->high < g_game->seaLevel && (*(Net_0049b720 **)&g_game->net)->noSeaLevelTrigger == 0)
                    AddExplosionEffect(&p->pos, type->splash, 0, 1);
            }
        }
    }
    CompactProjectiles();
}
