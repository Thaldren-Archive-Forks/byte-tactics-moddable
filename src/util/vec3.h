#ifndef VEC3_H
#define VEC3_H

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
    int SquareXZ() const
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
