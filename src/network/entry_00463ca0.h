#ifndef ENTRY_00463CA0_H
#define ENTRY_00463CA0_H

#pragma pack(push, 1)
struct Entry_00463ca0 {
    char text[64];
    unsigned int time;
    unsigned short field_44;
    char field_46;
    unsigned char flags;
};
#pragma pack(pop)

#if !defined(_MSC_VER) || _MSC_VER >= 1600
static_assert(sizeof(Entry_00463ca0) == 72, "Entry_00463ca0");
#endif

#endif
