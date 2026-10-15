#include "bitexact.h"

static __declspec(thread) bx_rand_state game_stream = {1};

int __cdecl bx_game_rand(void)
{
    return bx_rand(&game_stream);
}

void __cdecl bx_game_srand(unsigned int seed)
{
    bx_srand(&game_stream, seed);
}

unsigned __cdecl bx_game_holdrand(void)
{
    return game_stream.holdrand;
}
