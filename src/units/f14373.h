#ifndef F14373_H
#define F14373_H

#pragma pack(push, 1)
union F14373 {
    struct {
        unsigned int b0 : 1;
        unsigned int b1 : 1;
        unsigned int rest : 30;
    } bits;
    unsigned int all;
};
#pragma pack(pop)

#if defined(__clang__) || !defined(_MSC_VER) || _MSC_VER >= 1600
static_assert(sizeof(F14373) == 4, "F14373");
#endif

#endif
