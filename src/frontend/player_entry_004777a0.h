#ifndef PLAYER_ENTRY_004777A0_H
#define PLAYER_ENTRY_004777A0_H

struct PlayerInfo;

#pragma pack(push, 1)
struct PlayerEntry_004777a0 {
    PlayerInfo* info;
    char unknown_4[0x14b - 4];
};
#pragma pack(pop)

#if !defined(_MSC_VER) || _MSC_VER >= 1600
static_assert(sizeof(PlayerEntry_004777a0) == 331, "PlayerEntry_004777a0");
#endif

#endif
