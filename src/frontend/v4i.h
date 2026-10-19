#ifndef V4I_H
#define V4I_H

#pragma pack(push, 1)
struct V4i {
    unsigned long data1;
    unsigned long data2;
    unsigned long data3;
    unsigned long data4;
};
#pragma pack(pop)

#if defined(__clang__) || !defined(_MSC_VER) || _MSC_VER >= 1600
static_assert(sizeof(V4i) == 16, "V4i");
#endif

#endif
