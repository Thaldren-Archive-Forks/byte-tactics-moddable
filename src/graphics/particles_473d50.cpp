// Decompiled by deepseek-v4.1-flash, finished by space-bunny-free. Names are provisional.
//
// Slot 4 of NanoParticles (vtable 0x4fd5b8, family in 0x471cc0.cpp; the rest
// of the class is in nano_particles.cpp, which uses the real <vector>): makes
// room in the std::vector<NanoParticle> at +0xc for five particles a beat,
// then per particle picks a random point in the box at +0x28 around the
// centre at +0x1c and one in the box at +0x40 around the point at +0x34,
// appends a record holding both points, the step that divides the segment
// between them (so a particle moves about 4 units a tick), an expiry and a
// per-particle kind, and finally pushes the clock at +0x8 one tick ahead.
// The expiry is the high half of (length << 16) / 0x40000, so it is 2^16 /
// step ticks away; a zero step drops the particle.
#include <math.h>
#include <stddef.h>
#include <stdlib.h>
#include <vector>

void* __cdecl operator new(size_t n);
void __cdecl operator delete(void* p);

extern char* g_game;                   // 0x511de8, frame at +0x38a47

struct Vec3_00473d50 {
    int x;
    int y;
    int z;

    Vec3_00473d50 operator-(const Vec3_00473d50& o) const
    {
        Vec3_00473d50 r;
        r.x = x - o.x;
        r.y = y - o.y;
        r.z = z - o.z;
        return r;
    }
    int Length() const
    {
        float fx = x;
        float fy = y;
        float fz = z;
        return (int)sqrt(fx * fx + fy * fy + fz * fz);
    }
};

// 16.16 fixed point seen as the short above the short below.
union Fix_00473d50 {
    int whole;
    short half[2];
};

struct NanoParticle {
    Vec3_00473d50 pos;                 // +0x00
    Vec3_00473d50 tgt;                 // +0x0c
    Vec3_00473d50 vel;                 // +0x18
    int unused;                        // +0x24
    int flags;                         // +0x28
    int endTime;                       // +0x2c
};

class NanoParticles {
public:
    char unknown_0[4];                          // +0x00
    int deadline;                               // +0x04
    int time;                                   // +0x08
    std::vector<NanoParticle> records;
    Vec3_00473d50 center;                       // +0x1c
    Vec3_00473d50 radius;                       // +0x28
    Vec3_00473d50 target;                       // +0x34
    Vec3_00473d50 spread;                       // +0x40

    void Emit();
};

int HarnessViewRand();

// FUNCTION: 0x473d50
void NanoParticles::Emit()
{
    int grow = deadline - *(int*)(g_game + 0x38a47) + 1;
    if (grow > 0)
        records.reserve(records.size() + grow * 5);

    int i = 0;
    for (int n = 5; n != 0; n--) {
        NanoParticle e;
        e.pos.x = (int)(((__int64)HarnessViewRand() * radius.x) / 0x8000) + center.x;
        e.pos.y = (int)(((__int64)HarnessViewRand() * radius.y) / 0x8000) + center.y;
        e.pos.z = (int)(((__int64)HarnessViewRand() * radius.z) / 0x8000) + center.z;
        e.tgt.x = (int)(((__int64)HarnessViewRand() * spread.x) / 0x8000) + target.x;
        e.tgt.y = (int)(((__int64)HarnessViewRand() * spread.y) / 0x8000) + target.y;
        e.tgt.z = (int)(((__int64)HarnessViewRand() * spread.z) / 0x8000) + target.z;

        e.vel = e.tgt - e.pos;
        int len = e.vel.Length();
        Fix_00473d50 step;
        step.whole = (int)(((__int64)len << 16) / 0x40000);
        short s = step.half[1];
        if (s != 0) {
            // Divided straight into e.vel, no named Vec3: fixes the vel store order.
            e.vel.x = e.vel.x / s;
            e.vel.y = e.vel.y / s;
            e.vel.z = e.vel.z / s;
            e.endTime = *(int*)(g_game + 0x38a47) + s;
            e.unused = 0x100;
            e.flags = i % 7 + 0xa1;
            // push_back, not insert(end(), e): the insert position is copied out of ecx.
            records.push_back(e);
        }
        i++;
    }
    time = *(int*)(g_game + 0x38a47) + 1;
}
