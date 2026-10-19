// Decompiled by Opus. Names are provisional.
// Stays in its own file: merged with the module's second part, the fog arm's
// cell address picks the other SIB base (particles.cpp).
// The nano spark: moved by Step, drawn as one pixel by DrawParticle when the
// local player can see it, and dropped once IsExpired.

struct Vec3_004739b0 {
    int x, y, z;
    Vec3_004739b0& operator+=(const Vec3_004739b0& o)
    {
        x += o.x;
        y += o.y;
        z += o.z;
        return *this;
    }
};

// The integer halves of the 16.16 position.
struct Pos_004739b0 {
    char unknown_0[0x2];
    short x;                           // +0x2
    char unknown_4[0x2];
    short height;                      // +0x6
    char unknown_8[0x2];
    short y;                           // +0xa
};

#pragma pack(push, 1)

#include "rect.h"

#include "../network/player.h"

#include "../game/game.h"

extern Game* g_game;

static inline int Identity_00473a00(int v) { return v; }

// The second arm, inlined. The two player parameters are not a typo: the
// Contains test through `p` and the width in the index through `q` are what
// stop the index from folding into the imul.
static inline int IsSeen_00473a00(Player* p, Player* q, int col, int row)
{
    if (!((unsigned int)col < (unsigned int)p->exploredWidth && (unsigned int)row < (unsigned int)p->exploredHeight))
        return 0;
    return (g_game->visibilityMask[(unsigned int)q->exploredWidth * row + col] &
            (1 << g_game->playerIndex)) != 0;
}

// One nano spark (the element of NanoParticles' vector), 0x30 bytes.
class NanoParticle {
public:
    union {
        Vec3_004739b0 pos;             // +0x0
        Pos_004739b0 posw;
    };
    char unknown_c[0xc];
    Vec3_004739b0 vel;                 // +0x18
    char unknown_24[4];
    int flags;                         // +0x28, low 4 bits: frame; the colour
    int endTime;                       // +0x2c, the tick it expires

    void Step();
    void DrawParticle(int dest, short x, short y);
    int IsExpired(int value);
};
#pragma pack(pop)

void __stdcall FillRectangle(void* surface, Rect* rect, int color);

// FUNCTION: 0x473a00
void NanoParticle::DrawParticle(int dest, short x, short y)
{
    Rect r;
    r.left = (short)(posw.x - x) + 0x80;
    r.top = (short)(posw.y - y) - (posw.height >> 1) + 0x20;
    r.right = r.left + 1;
    r.bottom = r.top + 1;

    // Two locals with the same value. The original reads the map width twice
    // per arm, once for the bounds test and once for the index, and a single
    // local makes MSVC 5 fold one of the two away. This spelling keeps both
    // loads and the prologue's register choice.
    Player* p = &g_game->players_1b63[g_game->playerIndex];
    Player* q = &g_game->players_1b63[g_game->playerIndex];
    int visible;
    if ((g_game->mapFlags & 2) == 2) {
        int col = posw.x >> 5;
        int row = (posw.y - (posw.height >> 1)) >> 5;
        visible = (unsigned int)col < (unsigned int)p->exploredWidth && (unsigned int)row < (unsigned int)p->exploredHeight &&
                  p->explored[(unsigned int)p->exploredWidth * row + col] != 0;
    } else {
        int col = posw.x >> 5;
        int row = (posw.y - (posw.height >> 1)) >> 5;
        visible = Identity_00473a00(IsSeen_00473a00(p, q, col, row));
    }

    if (visible)
        FillRectangle((void*)dest, &r, flags);
}

