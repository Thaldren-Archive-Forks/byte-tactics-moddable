#ifndef UNIT_TYPE_SET_H
#define UNIT_TYPE_SET_H

#include <string.h>

class UnitTypeSet {
public:
    int bits[16];

    UnitTypeSet() { memset(this, 0, sizeof(UnitTypeSet)); }
    void AddTypeOrCategory(char* text, int* out);
};

#endif
