#include "game_rand.h"
#include "block.h"
#include "../../lib/bitexact/bitexact.h"

static bx_rand_state* Stream(RandStream stream)
{
    return (bx_rand_state*)SimRandStream(stream);
}

int __cdecl GameRand(RandStream stream)
{
    return bx_rand(Stream(stream));
}

void __cdecl GameSrand(RandStream stream, unsigned int seed)
{
    bx_srand(Stream(stream), seed);
}

unsigned int __cdecl GameHoldrand(RandStream stream)
{
    return Stream(stream)->holdrand;
}

int GameShuffleDraw::operator()(int d)
{
    unsigned long rm = 0x7fff;
    unsigned long rn = GameRand(stream) & 0x7fff;
    for (; rm < (unsigned long)d && rm != ~0UL; rm = rm << 15 | 0x7fff)
        rn = rn << 15 | 0x7fff;
    return (int)(rn % (unsigned long)d);
}
