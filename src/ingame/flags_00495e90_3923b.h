#ifndef FLAGS_00495E90_3923B_H
#define FLAGS_00495E90_3923B_H

#pragma pack(push, 1)
union Flags_00495e90_3923b {
    unsigned short raw;
    struct {
        unsigned short b0 : 1;
        unsigned short b1 : 1;
        unsigned short rest : 14;
    };
};
#pragma pack(pop)

#if !defined(_MSC_VER) || _MSC_VER >= 1600
static_assert(sizeof(Flags_00495e90_3923b) == 2, "Flags_00495e90_3923b");
#endif

#endif
