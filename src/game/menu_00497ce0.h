#ifndef MENU_00497CE0_H
#define MENU_00497CE0_H

#pragma pack(push, 1)
struct Menu_00497ce0 {
    int field_51d;
    char unknown_8[0x18 - 0x8];
    void* data;
};
#pragma pack(pop)

#if !defined(_MSC_VER) || _MSC_VER >= 1600
static_assert(sizeof(Menu_00497ce0) == 24, "Menu_00497ce0");
#endif

#endif
