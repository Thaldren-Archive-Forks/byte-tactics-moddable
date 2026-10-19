#ifndef FLAGS_00495E90_37EBE_H
#define FLAGS_00495E90_37EBE_H

#pragma pack(push, 1)
struct Flags_00495e90_37ebe {
    unsigned short b0 : 1;
    unsigned short b1 : 1;
    unsigned short b2 : 1;
    unsigned short rest : 13;
};
#pragma pack(pop)

#if defined(__clang__) || !defined(_MSC_VER) || _MSC_VER >= 1600
static_assert(sizeof(Flags_00495e90_37ebe) == 2, "Flags_00495e90_37ebe");
#endif

#endif
