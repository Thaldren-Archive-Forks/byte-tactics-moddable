#ifndef MENU_H
#define MENU_H

#pragma pack(push, 1)
struct Menu {
    char unknown_0[0x40];
};
#pragma pack(pop)

#if !defined(_MSC_VER) || _MSC_VER >= 1600
static_assert(sizeof(Menu) == 64, "Menu");
#endif

#endif
