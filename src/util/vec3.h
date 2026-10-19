// Vec3, a 3D point: three ints (Thaldren's 0xc-byte Vec3, x/y/z), and Point16, a
// map cell or footprint: two 16-bit coordinates, x and y. The one declaration of
// each for the files that share them. No constructors: a user constructor
// changes the inlining of MoveTowards (PR #6278).
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

// The 16.16 words of a Vec3 holding a position: the fraction and whole half
// of each coordinate. Read a Vec3 through it with a cast.
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
