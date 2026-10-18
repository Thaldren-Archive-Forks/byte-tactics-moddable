#ifndef FLAGS_00495E90_37F06_H
#define FLAGS_00495E90_37F06_H

#pragma pack(push, 1)
struct Flags_00495e90_37f06 {
    unsigned short b0 : 1;
    unsigned short b1_6 : 6;
    unsigned short b7 : 1;
    unsigned short b8 : 1;
    unsigned short rest : 7;
};
#pragma pack(pop)

#if !defined(_MSC_VER) || _MSC_VER >= 1600
static_assert(sizeof(Flags_00495e90_37f06) == 2, "Flags_00495e90_37f06");
#endif

#endif
