#ifndef STATE_GAME_RAND_H
#define STATE_GAME_RAND_H

#include "../../interfaces/rand_stream.h"

// LIBCMT rand on the named stream: advances it and returns the next value, 0 to 0x7fff.
int __cdecl GameRand(RandStream stream);

// LIBCMT srand on the named stream: restarts it from seed.
void __cdecl GameSrand(RandStream stream, unsigned int seed);

// The current state of the named stream.
unsigned int __cdecl GameHoldrand(RandStream stream);

// The draw of std::random_shuffle(first, last), as MSVC 5's STL makes it, taken from one stream; pass it as the third argument.
class GameShuffleDraw {
public:
    explicit GameShuffleDraw(RandStream s) : stream(s) {}
    // Draws once from the stream and returns a number in [0, d), with the multi-draw widening of the STL for d above 0x7fff.
    int operator()(int d);

private:
    RandStream stream;
};

#endif
