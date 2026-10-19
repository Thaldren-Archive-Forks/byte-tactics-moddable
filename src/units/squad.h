#ifndef SQUAD_H
#define SQUAD_H

#include <vector>

class Squad {
public:
    union { int field_0; int owner; };
    union { int field_4; int index; };
    int field_8;
    int field_c;
    std::vector<int> items;
    Squad(Squad&);
    Squad(int, int);
};

#endif
