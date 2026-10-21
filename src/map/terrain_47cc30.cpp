// Decompiled by deepseek-v4.1, finished by space-bunny-free, finished by GPT-6.1-sol, finished by deepseek-v4.1-flash, finished by mimo-v2.6-pro, finished by opus. Names are provisional.
#include "win_types.h"

#pragma pack(push, 1)

struct Obj_0047cc30;

struct Owner_0047cc30 {
    char unknown_0[6];
    Obj_0047cc30* first;                // +0x6
};

struct Unit_0047cc30 {
    char unknown_0[0x14e];
    unsigned char* mask;                // +0x14e
};

#include "../network/player.h"

struct UnitRec_0047cc30 {               // 0x118 bytes
    char unknown_0[0x92];
    void* def;                          // +0x92
    Player* player;                     // +0x96
    char unknown_9a[0x110 - 0x9a];
    unsigned int flags;                 // +0x110
    char unknown_114[0x118 - 0x114];
};

#include "cell.h"

#include "../util/vec3.h"

struct Position_0047cc30 {
    int x;
    int y;
    int z;
};

union Flags_0047cc30 {
    struct {
        unsigned int unknown_0 : 27;
        unsigned int flag27 : 1;
        unsigned int unknown_1 : 4;
    } bits;
    int all;
};

struct Obj_0047cc30 {
    unsigned char* motion;              // +0x0
    char unknown_4[0x26 - 0x4];
    int field_26;                       // +0x26
    char unknown_2a[0x6a - 0x2a];
    Position_0047cc30 position;         // +0x6a
    Point16 pos;                 // +0x76
    char unknown_7a[4];
    Point16 footprint;           // +0x7e
    Owner_0047cc30* spatialBucket;      // +0x82
    int carrier;                        // +0x86
    char unknown_8a[4];
    Obj_0047cc30* cargoNext;            // +0x8e
    Unit_0047cc30* def;                 // +0x92
    char unknown_96[0xa8 - 0x96];
    unsigned short id;                  // +0xa8
    char unknown_aa[0x10f - 0xaa];
    unsigned char bit0 : 1;             // +0x10f
    unsigned char bit1 : 1;
    unsigned char bit2 : 1;
    Flags_0047cc30 flags;               // +0x110
};

#include "../game/game.h"
#pragma pack(pop)

extern Game* g_game;

#include "../../interfaces/seams.h"
void __stdcall RefreshAllPassMaps(Point16 pos, Point16 size);

static void SetOwner_0047cc30(Obj_0047cc30* obj, Owner_0047cc30* nw)
{
    if (nw != obj->spatialBucket) {
        if (obj->carrier == 0) {
            Owner_0047cc30* old = obj->spatialBucket;
            if (old != 0) {
                Obj_0047cc30** pp = &old->first;
                while (*pp != obj)
                    pp = &(*pp)->cargoNext;
                *pp = obj->cargoNext;
                obj->cargoNext = 0;
            }
            obj->cargoNext = nw->first;
            nw->first = obj;
        }
        obj->spatialBucket = nw;
    }
}

// Stays in a file of its own: in the merged file the symbol count moves the
// owner index multiply onto the other operand.
// FUNCTION: 0x47cc30
void __stdcall AddUnitToMap(Obj_0047cc30* obj)
{
    Point16 size = obj->footprint;
    if (obj->motion != 0)
        *(int*)(obj->motion + 0x26) = g_game->ticks;
    {
        // px and py are short locals, py declared after px's test; the sums
        // stay on the left of >=.
        short px = obj->pos.x;
        if (px < 0)
            goto remove;
        short py = obj->pos.y;
        if (py < 0 || px + size.x >= g_game->width || py + size.y >= g_game->height)
            goto remove;
    }
    {
        Position_0047cc30 p = obj->position;
        SetOwner_0047cc30(obj,
            &((Owner_0047cc30 *)g_game->cells_1429f)[(p.x >> 23) + (p.z >> 23) * (int)g_game->width_142a3]);
    }
    {
        Cell* cell = &g_game->cells[g_game->width * obj->pos.y + obj->pos.x];
        unsigned int f = obj->flags.all;
        int index = 0;

        if (f & 0x20000000) {
            for (int y = size.y; y > 0; y--) {
                for (int x = size.x; x > 0; x--) {
                    unsigned char m = obj->def->mask[index++];
                    if (m & (obj->bit2 ? 2 : 4)) {
                        unsigned short id = cell->unit;
                        if (id != 0) {
                            UnitRec_0047cc30* rec = &((UnitRec_0047cc30 *)g_game->units)[id];
                            if (rec->player->active == 0) {
                                goto a_bad;
                            } else if (rec->player->type != 3) {
                                goto a_bad;
                            }
                            rec->flags |= 0x8000000;
                            obj->flags.all |= 0x4000000;
                            goto a_write;
                        a_bad:
                            rec->flags |= 0x4000000;
                            obj->flags.all |= 0x8000000;
                            goto a_next;
                        a_write: ;
                        }
                        cell->unit = obj->id;
                    }
                a_next:
                    if (m & 1)
                        cell->flags |= 2;
                    cell++;
                }
                cell += g_game->width - size.x;
            }
            Point16 grown;
            grown.x = size.x + 2;
            grown.y = size.y + 2;
            Point16 pad;
            pad.x = obj->pos.x - 1;
            pad.y = obj->pos.y - 1;
            g_seams.los_radar->UpdateCellHeightRange(pad, grown);
            RefreshAllPassMaps(obj->pos, obj->footprint);
            return;
        }
        if ((f & 3) == 1) {
            for (int y = size.y; y > 0; y--) {
                for (int x = size.x; x > 0; x--) {
                    unsigned short id = cell->unit;
                    if (id != 0) {
                        UnitRec_0047cc30* rec = &((UnitRec_0047cc30 *)g_game->units)[id];
                        if (rec->player->active == 0) {
                            goto b_bad;
                        } else if (rec->player->type != 3) {
                            goto b_bad;
                        }
                        rec->flags |= 0x8000000;
                        obj->flags.all |= 0x4000000;
                        goto b_write;
                    b_bad:
                        rec->flags |= 0x4000000;
                        obj->flags.all |= 0x8000000;
                        goto b_next;
                    b_write: ;
                    }
                    cell->unit = obj->id;
                b_next:
                    cell++;
                }
                cell += g_game->width - size.x;
            }
            return;
        }
        if ((f & 3) == 2) {
            for (int y = size.y; y > 0; y--) {
                for (int x = size.x; x > 0; x--) {
                    unsigned short id = cell->unit2;
                    if (id != 0) {
                        UnitRec_0047cc30* rec = &((UnitRec_0047cc30 *)g_game->units)[id];
                        if (rec->player->active == 0) {
                            goto c_bad;
                        } else if (rec->player->type != 3) {
                            goto c_bad;
                        }
                        rec->flags |= 0x8000000;
                        obj->flags.all |= 0x4000000;
                        goto c_write;
                    c_bad:
                        rec->flags |= 0x4000000;
                        obj->flags.all |= 0x8000000;
                        goto c_next;
                    c_write: ;
                    }
                    cell->unit2 = obj->id;
                c_next:
                    cell++;
                }
                cell += g_game->width - size.x;
            }
        }
    }
    return;

remove:
    SetOwner_0047cc30(obj, (*(Owner_0047cc30 **)&g_game->overflowBucket));
}
