#ifndef CLOCK_FLAGS_0042F9A0_H
#define CLOCK_FLAGS_0042F9A0_H

#pragma pack(push, 1)
struct ClockFlags_0042f9a0 {
    unsigned short unused0 : 1;
    unsigned short bit1 : 1;
    unsigned short bit2 : 1;
    unsigned short bit3 : 1;
    unsigned short bit4 : 1;
    unsigned short unused5 : 1;
    unsigned short clock : 1;
};
#pragma pack(pop)

#if defined(__clang__) || !defined(_MSC_VER) || _MSC_VER >= 1600
static_assert(sizeof(ClockFlags_0042f9a0) == 2, "ClockFlags_0042f9a0");
#endif

#endif
