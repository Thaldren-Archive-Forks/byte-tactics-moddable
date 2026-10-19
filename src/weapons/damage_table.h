#ifndef DAMAGE_TABLE_H
#define DAMAGE_TABLE_H

#include <vector>

struct Entry_00432cf0;

#pragma pack(push, 1)

struct Table_00499cd0 {
    char less;
    std::vector<Entry_00432cf0> v;

    Entry_00432cf0* LowerBound(const char* key);
};

#pragma pack(pop)

#endif
