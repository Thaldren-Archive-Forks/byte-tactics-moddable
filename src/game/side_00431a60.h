#ifndef SIDE_00431A60_H
#define SIDE_00431A60_H

#ifndef TA_TYPES_H
#include "../graphics/rect.h"
#endif

#pragma pack(push, 1)
struct Side_00431a60 {
    char name[30];
    char nameprefix[4];
    char commander[32];
    Rect logo;
    Rect energyBar;
    Rect energyNum;
    Rect metalBar;
    Rect metalNum;
    Rect totalUnits;
    Rect totalTime;
    Rect energyMax;
    Rect metalMax;
    Rect energy0;
    Rect metal0;
    Rect energyProduced;
    Rect energyConsumed;
    Rect metalProduced;
    Rect metalConsumed;
    Rect logo2;
    Rect unitName;
    Rect damageBar;
    Rect unitEnergyMake;
    Rect unitEnergyUse;
    Rect unitMetalMake;
    Rect unitMetalUse;
    Rect missionText;
    Rect unitName2;
    Rect damageBar2;
    Rect nameBlock;
    Rect description;
    Rect reload[3];
    int energyColor;
    int metalColor;
    int sideNumber;
    void* font;
};
#pragma pack(pop)

#if !defined(_MSC_VER) || _MSC_VER >= 1600
static_assert(sizeof(Side_00431a60) == 562, "Side_00431a60");
#endif

#endif
