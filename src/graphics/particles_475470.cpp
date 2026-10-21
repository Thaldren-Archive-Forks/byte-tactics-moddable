// Decompiled by Opus, Haiku, Sonnet, DeepSeek V4.1 Flash and Space Bunny Free. Names are provisional.
// Stays in its own file: merged with the module's second part its fog-culled
// loop walks from the wrong field (particles.cpp).
// SmokeParticles (vtable 0x4fd618, 0x38 bytes), derived from ParticleSystem
// (the family is listed in particles.cpp): smoke that drifts with the wind.
#include "win_types.h"
#include <stddef.h>
#include <stdlib.h>
#include <vector>
// Only for its symbol ids: the fog-culled draw (slot 2) matches only in a
// window of the symbol count.
#include <io.h>

#include "../util/vec3.h"

struct Position_00475470 {             // 16.16 fixed point; only high words read
    short xFrac;
    short x;                           // +0x2
    short yFrac;
    short y;                           // +0x6
    short zFrac;
    short z;                           // +0xa
};

#pragma pack(push, 1)
#include "../network/player.h"

#include "../game/game.h"
#pragma pack(pop)

extern Game* g_game;

int __stdcall GetGafFrameCount(void* ptr);
void* __stdcall GetGafFrame(void* a, int b);
void __stdcall DrawFrameBlended(void* dest, void* src, int x, int y);

static inline int IsExplored(Player* map, Position_00475470* pos)
{
    int tx = pos->x >> 5;
    int ty = (pos->z - (pos->y >> 1)) >> 5;
    if ((unsigned int)tx < (unsigned int)map->exploredWidth && (unsigned int)ty < (unsigned int)map->exploredHeight &&
        map->explored[(unsigned int)map->exploredWidth * ty + tx])
        return 1;
    return 0;
}

// The body of the matched 0x408090.
static inline int IsSeen(Player* map, Position_00475470* pos)
{
    int tx = pos->x >> 5;
    int ty = (pos->z - (pos->y >> 1)) >> 5;
    if (!((unsigned int)tx < (unsigned int)map->exploredWidth && (unsigned int)ty < (unsigned int)map->exploredHeight)) {
        return 0;
    }
    // Pointer local declared after the Contains test: the extra register
    // makes the width load materialise.
    Player* b = map;
    return (g_game->visibilityMask[(unsigned int)b->exploredWidth * ty + tx] &
            (1 << g_game->playerIndex)) != 0;
}

static inline int IsVisible(Player* map, Position_00475470* pos)
{
    if ((g_game->mapFlags & 2) == 2)
        return IsExplored(map, pos);
    return IsSeen(map, pos);
}

// The 32-byte record; its unculled draw method is 0x475040.
// One particle, 0x20 bytes.
struct SmokeParticle {
    void* data;                        // +0x00, the animation
    union {
        Vec3 pos;
        Position_00475470 posw;
    };
    int limit;                         // +0x10, the rounds it lives
    int count;                         // +0x14, the rounds so far (the frame)
    int period;                        // +0x18
    int timer;                         // +0x1c

    void Step();
    void DrawParticle(int dest, short px, short py);
    int IsExpired(int unused);
    void DrawIfVisible(void* dest, short px, short py)
    {
        short sx = posw.x - px + 0x80;
        short sy = posw.z - (posw.y >> 1) - py + 0x20;
        if (IsVisible(&g_game->players[g_game->playerIndex], &posw))
            DrawFrameBlended(dest, GetGafFrame(data, count), sx, sy);
    }
};

typedef std::vector<SmokeParticle> Vec_00474cd0;

// The particle vector, to call its out-of-line insert under this name.
class Class_00476210 {
public:
    void insert(Vec_00474cd0::iterator p, unsigned int m,
                const SmokeParticle& x);
};

#include "particle_system.h"

class SmokeParticles : public ParticleSystem {
public:
    int time;                                           // +0x8, the next emit tick
    Vec_00474cd0 records;                               // +0xc (_First +0x10)
    int emitPeriod;                                     // +0x1c, the emit period
    int holdPeriod;                                     // +0x20
    int maxFrame;                                       // +0x24, the frame count - 1
    int altAnimation;                                   // +0x28, the other animation
    Vec3 pos;

    SmokeParticles();
    virtual void Update();                              // slot 1, 0x475340
    virtual void Render(int);                           // slot 2, 0x475470
    virtual int IsFinished();                           // slot 3, 0x474f80
    virtual void Emit();                                // slot 4, 0x474df0
    virtual int IsEmitDue();                            // slot 5, 0x475440
    virtual void Init(Vec3* pos, int limit, int a, int b, int c,
                              int alt);                 // slot 6, 0x474d50
};

// FUNCTION: 0x475470
void SmokeParticles::Render(int dest)
{
    for (std::vector<SmokeParticle>::iterator it = records.begin(); it != records.end();
         ++it) {
        it->DrawIfVisible((void*)dest, g_game->scrollX, g_game->scrollY);
    }
}
