#ifndef GROUP_H
#define GROUP_H

#include "../state/sim_allocator.h"

struct Player;
struct Unit;
struct Vec3;

struct Group {
    Player* player;
    int id;
    char unknown_8[0x10 - 0x8];
    SimVector<Unit*> units;

    void Send(unsigned char mode, int remove, Unit* target, Vec3* pos, int flags, int extra);
};

static_assert(__builtin_offsetof(Group, units) == 0x10, "Group::units");
static_assert(sizeof(Group) == 0x20, "Group size");

#endif
