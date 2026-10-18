#ifndef ANIM_MANAGER_00421F20_H
#define ANIM_MANAGER_00421F20_H

struct AnimEntry_00421f20;
struct FeatureUnit_00421f20;

#pragma pack(push, 1)
struct AnimManager_00421f20 {
    char unknown_0[16];
    AnimEntry_00421f20* anim;
    FeatureUnit_00421f20* featureUnit;
    int activeHead;
    int idleHead;
    int freeHead;
    char unknown_24[52];
    int nameCount;
    char unknown_5c[24];
    char (*names)[256];
};
#pragma pack(pop)

#if !defined(_MSC_VER) || _MSC_VER >= 1600
static_assert(sizeof(AnimManager_00421f20) == 120, "AnimManager_00421f20");
#endif

#endif
