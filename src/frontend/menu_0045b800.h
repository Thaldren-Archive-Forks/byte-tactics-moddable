#ifndef MENU_0045B800_H
#define MENU_0045B800_H

struct Holder_0045b800;

#pragma pack(push, 1)
struct Menu_0045b800 {
    char unknown_0[0x18];
    Holder_0045b800* holder;
};
#pragma pack(pop)

#if defined(__clang__) || !defined(_MSC_VER) || _MSC_VER >= 1600
static_assert(sizeof(Menu_0045b800) == 28, "Menu_0045b800");
#endif

#endif
