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

#include "../util/vec3.h"
typedef Vec3 Vec3_0049b720;

#include "weapon_def.h"
#include "projectile.h"



#include "../units/unit.h"

union Word_0049b720 {
    int i;
    struct {
        unsigned short lo;
        short hi;
    } s;
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
void __stdcall CheckProjectileCollision(WeaponDef* type, Projectile* p);
Vec3_0049b720* __stdcall GetProjectileAimPoint(Projectile* p);
int __stdcall TurnUnitTowardsPoint(Projectile* p, Vec3_0049b720* target);
void __stdcall DetonateProjectile(Projectile* p, void* unit);
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
static Projectile* AllocProjectile()
{
    Projectile* p = 0;
    if (g_game->projectileCount < 300) {
        p = &g_game->projectiles[g_game->projectileCount++];
        p->flagBits.dead = 0;
        p->targetUnit = 0;
    }
    return p;
}

// Gravity goes through this pointer store, not `p->vel.y -= ...`: it makes
// p->type get reloaded, so `type` is not propagated.
static inline void Fall(Vec3_0049b720* v)
{
    v->y -= g_game->gravity;
}

static inline void Remove(Projectile* p)
{
    if (p == g_game->tracked) {
        g_game->trackedPos = g_game->tracked->position;
        (*(unsigned short*)&g_game->value_1434b) = p->type->holdTime;
        g_game->tracked = 0;
    }
    p->flagBits.dead = 1;
}

// Stays in its own file: the b0 arm's register tie follows this file's symbol
// ids, which the joined weapons.cpp moves.
// FUNCTION: 0x49b720
void UpdateProjectiles()
{
    int n = g_game->projectileCount;
    for (int i = 0; i < n; i++) {
        Projectile* p = &g_game->projectiles[i];
        // type is read before oldY: orders the two spill stores.
        WeaponDef* type = p->type;
        int oldY = p->positionWords.yWhole;

        if (p->active != 0) {
            if ((unsigned int)g_game->ticks >= p->spawnTick + type->burstRate) {
                if (type->burstRate >= 5 || (p->active & 1)) {
                    unsigned char w;
                    for (w = 0; w < 3; w++) {
                        if (p->attacker->weapons[w].weapon == type)
                            break;
                    }
                    GetWeaponPiecePosition(p->attacker, &p->position, w, p->piece);
                }
                p->active--;
                p->spawnTick += type->burstRate;
                Projectile* q = AllocProjectile();
                if (q) {
                    *q = *p;
                    q->spawnTick = (unsigned int)g_game->ticks;
                    if (type->flags.b.soundTrigger)
                        PlaySoundAt(type->sound, &p->position, 0);
                    if (type->lifetime != 0)
                        q->time = (unsigned int)g_game->ticks + type->lifetime;
                    else
                        q->time = (p->range + 0x100000) / (unsigned int)p->speed + (unsigned int)g_game->ticks;
                    if (type->lifeRand != 0)
                        q->time += RandomInt(type->lifeRand) - (type->lifeRand >> 1);
                    q->active = 0;
                    if (type->spread != 0) {
                        short ang = RandomInt(type->spread) + (short)(p->angles.heading - (type->spread >> 1));
                        int t = FUN_004b7123(p->angles.pitch, type->speed);
                        p->direction.x = -FUN_004b70ef(ang, t);
                        p->direction.z = -FUN_004b7123(ang, t);
                    }
                }
                if (p->active == 0)
                    Remove(p);
            }
            continue;
        }

        if (type->flags.b.propeller)
            p->propellerSpin += 0x400;

        if (type->flags.b.selfProp) {
            if (p->time > (unsigned int)g_game->ticks) {
                if (type->flags.b.waterWeapon && p->positionWords.yWhole >= g_game->seaLevel) {
                    Fall(&p->direction);
                    p->angles.pitch = 0;
                } else {
                    int seek = 0;
                    if ((unsigned int)p->speed < type->speed) {
                        p->speed += type->acceleration;
                        if ((unsigned int)p->speed > type->speed)
                            p->speed = type->speed;
                    }
                    if (type->flags.b.twoPhase) {
                        if (p->flagBits.state > 0)
                            seek = 1;
                    } else if (type->flags.b.guidance) {
                        seek = 1;
                    }
                    if (seek) {
                        Vec3_0049b720* target = GetProjectileAimPoint(p);
                        if (TurnUnitTowardsPoint(p, target) == 0)
                            DetonateProjectile(p, 0);
                    }
                    p->direction.y = FUN_004b70ef(p->angles.pitch, p->speed);
                    int t = FUN_004b7123(p->angles.pitch, p->speed);
                    p->direction.x = -FUN_004b70ef(p->angles.heading, t);
                    p->direction.z = -FUN_004b7123(p->angles.heading, t);
                }
            } else if (type->flags.b.burnBlow) {
                DetonateProjectile(p, 0);
            } else {
                Fall(&p->direction);
                if (type->flags.b.twoPhase) {
                    if (p->flagBits.state == 0) {
                        p->time = (unsigned int)g_game->ticks + p->type->flightTime;
                        p->flagBits.state++;
                        if (!p->type->flags.b.tracks) {
                            p->interceptedProjectile = 0;
                            p->targetUnit = 0;
                        }
                    }
                }
            }
            p->position += p->direction;
            CheckProjectileCollision(type, p);
        } else if (type->flags.b.lineOfSight) {
            if (p->time > (unsigned int)g_game->ticks) {
                p->position += p->direction;
                if (type->flags.b.beamWeapon) {
                    if (p->flagBits.b0)
                        p->start += p->direction;
                    else if ((unsigned int)g_game->ticks > p->spawnTick + type->duration)
                        p->flagBits.b0 = 1;
                }
                CheckProjectileCollision(type, p);
            } else {
                Remove(p);
            }
        } else if (type->flags.b.ballistic) {
            if (type->lifetime != 0) {
                if (p->time > (unsigned int)g_game->ticks) {
                    p->position += p->direction;
                    p->position += (*(Vec3_0049b720*)&g_game->wind);
                    p->direction.y -= g_game->gravity;
                    CheckProjectileCollision(type, p);
                } else if (type->flags.b.burnBlow) {
                    DetonateProjectile(p, 0);
                } else {
                    EmitWhiteSmoke(&p->position, 9);
                    Remove(p);
                }
            } else {
                p->position += p->direction;
                p->position += (*(Vec3_0049b720*)&g_game->wind);
                p->direction.y -= g_game->gravity;
                CheckProjectileCollision(type, p);
            }
        } else if (type->flags.b.dropped) {
            p->position += p->direction;
            p->position += (*(Vec3_0049b720*)&g_game->wind);
            p->direction.y -= g_game->gravity;
            CheckProjectileCollision(type, p);
        } else if (type->flags.b.meteor) {
            p->position += p->direction;
            p->angles.bank += ((short*)&p->direction.x)[1] << 8;
            p->angles.pitch += ((short*)&p->direction.z)[1] << 8;
            CheckProjectileCollision(type, p);
        }

        if (!p->flagBits.dead) {
            if (type->flags.b.smokeTrail && p->time > (unsigned int)g_game->ticks && p->nextSmokeTick < (unsigned int)g_game->ticks) {
                EmitWhiteSmoke(&p->position, 9);
                p->nextSmokeTick += type->smokeRate;
            }
            if (oldY > g_game->seaLevel && p->positionWords.yWhole <= g_game->seaLevel) {
                Cell* cell = g_seams.los_radar->GetMapCellAtPosition((Vec3*)&p->position);
                if (cell && cell->high < g_game->seaLevel && ((Net_0049b720 *)g_game->net)->noSeaLevelTrigger == 0)
                    AddExplosionEffect(&p->position, type->splash, 0, 1);
            }
        }
    }
    CompactProjectiles();
}
