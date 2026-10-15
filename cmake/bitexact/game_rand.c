#include "bitexact.h"

extern void *__cdecl _getptd(void);

static bx_rand_state *game_stream(void)
{
    return (bx_rand_state *)((char *)_getptd() + 0x14);
}

int __cdecl bx_game_rand(void)
{
    return bx_rand(game_stream());
}

void __cdecl bx_game_srand(unsigned int seed)
{
    bx_srand(game_stream(), seed);
}
