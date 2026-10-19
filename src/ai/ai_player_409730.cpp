// Decompiled by Claude Opus 5.5, finished by deepseek-v4.1-flash, edited by deepseek-v4.1, finished by space-bunny-free, finished by deepseek-v4.1-flash, finished by deepseek-v4.1-flash, finished by deepseek-v4.1-flash, finished by mimo-v2.6-pro, finished by Space Bunny Free, finished by DeepSeek V4.1 Flash, finished by GPT-6, finished by Claude Opus 5.5. Names are provisional.
// A method of PlayerAI, whose other methods are in ai_player.cpp.
#include <stdio.h>
#include <minmax.h>
#include <vector>
#include "../units/unit.h"

#pragma pack(push, 1)
struct Weapon_00409730 {
    char unknown_0[0xd4];
    unsigned short damage;             // +0xd4
    char unknown_d6[0xdc - 0xd6];
    int range;                         // +0xdc
    char unknown_e0[0x10a - 0xe0];
    char index;                        // +0x10a
};

#include "../units/unit_def.h"

#include "../game/game.h"

#include "player_ai.h"
#pragma pack(pop)

extern Game* g_game;

#include "../../interfaces/seams.h"

// 0x409520 and 0x4095d0 (matched in their own files): no callers in the
// exe, both inlined into the loop below.
#define MIN(a, b) (((a) > (b)) ? (b) : (a))

int __stdcall RateWeapons_409730(UnitDef* p)
{
    int result = 1;
    if (p->c04)
        result = 0xb;
    Weapon_00409730** pp = (Weapon_00409730**)p->weapons;
    for (int i = 3; i != 0; i--) {
        Weapon_00409730* s = *pp;
        if (s->index != 0)
            result = result + s->damage / 40 + s->range / 100 + 5;
        pp++;
    }
    if (MIN(result, 100) < -100)
        return -100;
    return MIN(result, 100);
}

int __stdcall RateUnitType_409730(UnitDef* p)
{
    int result = 1;
    if (p->extractsMetal != 0.0f)
        result = 0xb;
    if (p->makesMetal != 0)
        result += 10;
    if (g_seams.economy->GetEnergyUse(p) < 0.0f)
        result += 10;
#ifdef __clang__
    result = (int)((int)((double)result - (double)p->metalCost * -0.01f) - (double)p->energyCost * -0.002f);
#else
    result = (int)((int)(result - p->metalCost * -0.01f) - p->energyCost * -0.002f);
#endif
    result += (signed char)RateWeapons_409730(p);
    if (MIN(result, 100) < -100)
        return -100;
    return MIN(result, 100);
}

// FUNCTION: 0x409730
void PlayerAI::ComputeBaseWeights()
{
    vec_8d.resize(g_game->count_1438f, 0);
    {
        Elem_0040cfb0 e;
        e.a = 0;
        e.b = 0;
        e.c = 0;
        vec_65.resize(g_game->count_1438f, e);
    }
    for (int i = 1; i < g_game->count_1438f; i++) {
        UnitDef* def = &g_game->defs[(unsigned short)i];
        vec_8d[i] = RateUnitType_409730(def);

        int a = 1;
        Elem_0040cfb0* e = &vec_65[i];
        int n = (short)counts[i];
        if (def->c04)
            a = 21;
        if (def->canbuild && n < 3)
            a += 30;
        if (g_seams.economy->GetEnergyUse(def) < 0.0f)
            a += 50;
        // Stays a method: the first resize's erase needs its inline budget.
        if (def->HasField1ce())
            a += 50;
        if (def->makesMetal)
            a += 25;
        unsigned int flags = def->flags1;
        if ((flags >> 11) & 1)
            a += 40;
        if (def->sonardistance)
            a += 15;
        if (def->radardistance)
            a += 5;
        int x = (int)(a + min(max(def->energymake, 0.0f), 30.0f));
        if (n == 0)
            x *= 4;
        if (n == 1)
            x *= 2;
        if (def->minwaterdepth >= 0)
            x *= 3;
        if (owner->unitCount > (unsigned short)(g_game->unitsPerPlayer / 2)) {
            x += (char)vec_8d[i] / 2;
        }
        if (def->c08)
            x = 0;
        if ((flags >> 24) & 1)
            x = 0;
        if (def->windgenerator != 0.0f && g_game->windSpeedMax < g_game->windSpeedDenominator / 2)
            x = 0;
        x = min(x, 100);
        e->a = x;
        e->c = (char)max(0.0f, min(100.0f, def->energyCost * -0.0025f - g_seams.economy->GetEnergyUse(def) * 5.0f));
        // The e->b bonus is a plain conditional: RateWeapons only inlines then.
#ifdef __clang__
        e->b = (char)max(0.0f, min(100.0f, (double)(float)((double)def->metalCost * -0.02f + (def->makesMetal ? 25 : 0)) + (def->extractsMetal != 0.0f ? 100 : 0)));
#else
        e->b = (char)max(0.0f, min(100.0f, (float)(def->metalCost * -0.02f) + (def->makesMetal ? 25 : 0) + (def->extractsMetal != 0.0f ? 100 : 0)));
#endif
    }
}



