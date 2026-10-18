#ifndef BITS_00426E80_H
#define BITS_00426E80_H

#pragma pack(push, 1)
struct Bits_00426e80 {
    unsigned short b0 : 1;
    unsigned short b1 : 1;
    unsigned short b2 : 1;
    unsigned short b3 : 1;
    unsigned short b4 : 1;
    unsigned short : 11;
};
#pragma pack(pop)

#if !defined(_MSC_VER) || _MSC_VER >= 1600
static_assert(sizeof(Bits_00426e80) == 2, "Bits_00426e80");
#endif

#endif
