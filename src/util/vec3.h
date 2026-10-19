// Vec3, a 3D point: three ints (Thaldren's 0xc-byte Vec3, x/y/z), each also the
// 16.16 fixed-point words of a position (xFrac and xWhole), and Point16, a map
// cell or footprint: two 16-bit coordinates, x and y. The one declaration of
// each for the files that share them. No constructors: a user constructor
// changes the inlining of MoveTowards (PR #6278).
#ifndef VEC3_H
#define VEC3_H

#include <math.h>

struct Vec3 {
    union {
        int x;
        struct {
            unsigned short xFrac;
            short xWhole;
        };
    };
    union {
        int y;
        struct {
            unsigned short yFrac;
            short yWhole;
        };
    };
    union {
        int z;
        struct {
            unsigned short zFrac;
            short zWhole;
        };
    };

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
    Vec3 operator+(const Vec3& v) const
    {
        Vec3 r;
        r.x = x + v.x;
        r.y = y + v.y;
        r.z = z + v.z;
        return r;
    }
    Vec3 operator-(const Vec3& v) const
    {
        Vec3 r;
        r.x = x - v.x;
        r.y = y - v.y;
        r.z = z - v.z;
        return r;
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

struct Point16 {
    short x;
    short y;
    int operator!=(const Point16& o) const
    {
        return x != o.x || y != o.y;
    }
};

#endif
