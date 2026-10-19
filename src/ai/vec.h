#ifndef VEC_H
#define VEC_H

struct Vec {
    int x;
    int y;
    int z;
    Vec() {}
    Vec(int a, int b, int c) : x(a), y(b), z(c) {}
};

#endif
