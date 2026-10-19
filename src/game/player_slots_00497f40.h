#ifndef PLAYER_SLOTS_00497F40_H
#define PLAYER_SLOTS_00497F40_H

#pragma pack(push, 1)
struct PlayerSlots_00497f40 {
    int inGame[10];
    int unknown_28;
    int flag40[10];
    char unknown_54[56];
};
#pragma pack(pop)

#if defined(__clang__) || !defined(_MSC_VER) || _MSC_VER >= 1600
static_assert(sizeof(PlayerSlots_00497f40) == 140, "PlayerSlots_00497f40");
#endif

#endif
