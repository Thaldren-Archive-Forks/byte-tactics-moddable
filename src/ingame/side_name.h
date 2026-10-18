#ifndef SIDE_NAME_H
#define SIDE_NAME_H

#pragma pack(push, 1)
struct SideName {
    char name[0x224];
    int field_224;
    char unknown_228[0x232 - 0x228];
};
#pragma pack(pop)

#if !defined(_MSC_VER) || _MSC_VER >= 1600
static_assert(sizeof(SideName) == 562, "SideName");
#endif

#endif
