#include "bitexact.h"

void bx_srand(bx_rand_state *s, bx_u32 seed)
{
    s->holdrand = seed;
}

int bx_rand(bx_rand_state *s)
{
    s->holdrand = s->holdrand * 214013u + 2531011u;
    return (int)((s->holdrand >> 16) & 0x7fff);
}

void bx_random_shuffle_int(bx_rand_state *s, int *first, int *last)
{
    int *x = first;
    int d = 1;

    if (first == last)
        return;
    for (; ++x != last; ++d) {
        unsigned long rm = 0x7fff;
        unsigned long rn = (unsigned long)bx_rand(s) & 0x7fff;
        int t;

        for (; rm < (unsigned long)d && rm != ~0UL; rm = rm << 15 | 0x7fff)
            rn = rn << 15 | 0x7fff;
        t = *x;
        *x = first[(int)(rn % (unsigned long)d)];
        first[(int)(rn % (unsigned long)d)] = t;
    }
}

bx_u32 bx_explosion_frame_draws(int size)
{
    return size > 0 ? (bx_u32)size * (bx_u32)size : 0;
}

static bx_u32 explosion_draws(int count, int start, int end)
{
    bx_u32 total = 0;
    int radius = start;
    int step = (end - start) / count;
    int i;

    for (i = 0; i < count; i++) {
        total += bx_explosion_frame_draws(radius);
        radius += step;
    }
    return total;
}

bx_u32 bx_init_explosions_draws(void)
{
    return explosion_draws(24 / 2, 64, 8) + explosion_draws(30 / 2, 128, 16) + explosion_draws(30 / 2, 200, 32);
}

void bx_init_explosions_rand(bx_rand_state *s)
{
    bx_u32 n = bx_init_explosions_draws();

    while (n--)
        bx_rand(s);
}
