#ifndef VEC3_H
#define VEC3_H

struct Vec3 {
    int x;
    int y;
    int z;

    Vec3 operator+(const Vec3& o) const
    {
        Vec3 r;
        r.x = x + o.x;
        r.y = y + o.y;
        r.z = z + o.z;
        return r;
    }
    Vec3 operator-(const Vec3& o) const
    {
        Vec3 r;
        r.x = x - o.x;
        r.y = y - o.y;
        r.z = z - o.z;
        return r;
    }
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
    void Scale(int s)
    {
        __int64 p;
        p = (__int64)x * s;
        x = (int)(p >> 16);
        p = (__int64)y * s;
        y = (int)(p >> 16);
        p = (__int64)z * s;
        z = (int)(p >> 16);
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
