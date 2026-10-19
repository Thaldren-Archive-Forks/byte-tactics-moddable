#ifndef SLOT_0041E420_H
#define SLOT_0041E420_H

#pragma pack(push, 1)
struct Slot_0041e420 {
    unsigned char active;
    char unknown_1[29];
    int stats[7];
};
#pragma pack(pop)

#if defined(__clang__) || !defined(_MSC_VER) || _MSC_VER >= 1600
static_assert(sizeof(Slot_0041e420) == 58, "Slot_0041e420");
#endif

#endif
