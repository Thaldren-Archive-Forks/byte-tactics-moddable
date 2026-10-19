#ifndef ZERO11_004917D0_H
#define ZERO11_004917D0_H

#pragma pack(push, 1)
struct Zero11_004917d0 {
    int a;
    int b;
    short c;
    char d;
    void Clear()
    {
        a = 0;
        b = 0;
        c = 0;
        d = 0;
    }
};
#pragma pack(pop)

#if defined(__clang__) || !defined(_MSC_VER) || _MSC_VER >= 1600
static_assert(sizeof(Zero11_004917d0) == 11, "Zero11_004917d0");
#endif

#endif
