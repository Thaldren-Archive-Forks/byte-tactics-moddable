#include "bitexact.h"

extern void *__cdecl _getptd(void);

static bx_rand_state g_game_rand;

int __cdecl bx_game_rand(void)
{
    int value = bx_rand(&g_game_rand);
    *(bx_u32 *)((char *)_getptd() + 0x14) = g_game_rand.holdrand;
    return value;
}

void __cdecl bx_game_srand(unsigned int seed)
{
    bx_srand(&g_game_rand, seed);
    *(bx_u32 *)((char *)_getptd() + 0x14) = g_game_rand.holdrand;
}
