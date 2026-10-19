#ifndef VEC3_H
#define VEC3_H

#include <math.h>

struct Vec3 {
    int x;
    int y;
    int z;

    void operator+=(const Vec3& v)
    {
        x += v.x;
        y += v.y;
        z += v.z;
    }
    void operator-=(const Vec3& v)
    {
        x -= v.x;
        y -= v.y;
        z -= v.z;
    }
    void Add(const Vec3* o)
    {
        x += o->x;
        y += o->y;
        z += o->z;
    }
    int NonZero() const
    {
        return x || z || y;
    }
    int Length() const
    {
        double fx = x;
        double fy = y;
        double fz = z;
        return (int)sqrt(fx * fx + fy * fy + fz * fz);
    }
    void Scale(int s)
    {
        x = (int)(((__int64)x * s) >> 16);
        y = (int)(((__int64)y * s) >> 16);
        z = (int)(((__int64)z * s) >> 16);
    }
    int Square() const
    {
        __int64 a = x, b = z;
        return (int)((a * a) >> 32) + (int)((b * b) >> 32);
    }
};

struct Vec3Words {
    unsigned short xFrac;
    short xWhole;
    unsigned short yFrac;
    short yWhole;
    unsigned short zFrac;
    short zWhole;
};

struct Point16 {
    short x;
    short y;
    int operator!=(const Point16& o) const
    {
        return x != o.x || y != o.y;
    }
};

#endif
