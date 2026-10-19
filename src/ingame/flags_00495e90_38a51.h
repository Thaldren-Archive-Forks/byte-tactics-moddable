#ifndef FLAGS_00495E90_38A51_H
#define FLAGS_00495E90_38A51_H

#pragma pack(push, 1)
struct Flags_00495e90_38a51 {
    unsigned short b0 : 1;
    unsigned short rest : 15;
};
#pragma pack(pop)

#if defined(__clang__) || !defined(_MSC_VER) || _MSC_VER >= 1600
static_assert(sizeof(Flags_00495e90_38a51) == 2, "Flags_00495e90_38a51");
#endif

#endif
