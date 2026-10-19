#ifndef GROUP_H
#define GROUP_H

#include <vector>

struct Player;
struct Unit;
struct Vec3;

struct Group {
    Player* player;
    int id;
    char unknown_8[0x10 - 0x8];
    std::vector<Unit*> units;

    void Send(unsigned char mode, int remove, Unit* target, Vec3* pos, int flags, int extra);
};

#endif
