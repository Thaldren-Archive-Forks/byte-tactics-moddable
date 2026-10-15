#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../bitexact.h"
#include "vectors.h"

#ifdef BX_REPLAY
#define ORACLE_INT(call) ((int)vec_get())
#define ORACLE_ULONG(call) ((unsigned long)vec_get())
#define ORACLE_BYTES(call, data, size) vec_get_bytes(data, size)
#define ORACLE_ONLY(call)
#else
void __cdecl orig_random_shuffle_int(int *first, int *last);
#define ORACLE_INT(call) ((int)vec_put((bx_u32)(call)))
#define ORACLE_ULONG(call) ((unsigned long)vec_put((bx_u32)(call)))
#define ORACLE_BYTES(call, data, size) \
    do { \
        call; \
        vec_put_bytes(data, size); \
    } while (0)
#define ORACLE_ONLY(call) call
#endif

typedef struct {
    const char *name;
    unsigned long cases;
    unsigned long mismatches;
} stat_t;

static stat_t st_rand = {"bx_rand"};
static stat_t st_shuffle = {"bx_random_shuffle_int"};
static stat_t st_load = {"load_thread_sequence"};
static stat_t st_qsort = {"bx_qsort"};
static stat_t *all_stats[] = {&st_rand, &st_shuffle, &st_load, &st_qsort};

static unsigned int rng_state = 0x2545f491u;

static unsigned int rnd32(void)
{
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 17;
    rng_state ^= rng_state << 5;
    return rng_state;
}

static void fail(stat_t *s, const char *fmt, unsigned long a, unsigned long b, unsigned long c)
{
    s->mismatches++;
    if (s->mismatches <= 5) {
        printf("  %s mismatch: ", s->name);
        printf(fmt, a, b, c);
        printf("\n");
    }
}

static unsigned int seed_at(int i)
{
    static const unsigned int fixed[] = {0u, 1u, 0xffffffffu, 0x7fffu, 0x8000u, 0x80000000u, 0x7fffffffu, 12345u};
    if (i < (int)(sizeof(fixed) / sizeof(fixed[0])))
        return fixed[i];
    return rnd32();
}

static void test_rand(void)
{
    int i, j;
    for (i = 0; i < 1500; i++) {
        unsigned int seed = seed_at(i);
        bx_rand_state s;
        ORACLE_ONLY(srand(seed));
        bx_srand(&s, seed);
        st_rand.cases++;
        for (j = 0; j < 10000; j++) {
            int want = ORACLE_INT(rand());
            int got = bx_rand(&s);
            if (want != got) {
                fail(&st_rand, "seed 0x%lx draw %lu want %lu", seed, j, want);
                break;
            }
        }
    }
}

static int same_state(const bx_rand_state *s)
{
    bx_rand_state t = *s;
    int k;
    for (k = 0; k < 4; k++)
        if (ORACLE_INT(rand()) != bx_rand(&t))
            return 0;
    return 1;
}

static int check_shuffle(int n, unsigned int seed, int *a, int *b)
{
    bx_rand_state s;
    int i;
    for (i = 0; i < n; i++)
        a[i] = b[i] = i * 3 + 1;
    ORACLE_ONLY(srand(seed));
    bx_srand(&s, seed);
    ORACLE_BYTES(orig_random_shuffle_int(a, a + n), a, n * sizeof(int));
    bx_random_shuffle_int(&s, b, b + n);
    if (n > 0 && memcmp(a, b, n * sizeof(int)) != 0)
        return 0;
    return same_state(&s);
}

static void test_shuffle(void)
{
    static const int big[] = {1000, 32767, 32768, 32769, 40000, 100000};
    int *a = (int *)malloc(100000 * sizeof(int));
    int *b = (int *)malloc(100000 * sizeof(int));
    int n, i;
    for (n = 0; n <= 64; n++)
        for (i = 0; i < 100; i++) {
            unsigned int seed = seed_at(i);
            st_shuffle.cases++;
            if (!check_shuffle(n, seed, a, b))
                fail(&st_shuffle, "size %lu seed 0x%lx (%lu)", n, seed, 0);
        }
    for (n = 0; n < (int)(sizeof(big) / sizeof(big[0])); n++)
        for (i = 0; i < 20; i++) {
            unsigned int seed = seed_at(i);
            st_shuffle.cases++;
            if (!check_shuffle(big[n], seed, a, b))
                fail(&st_shuffle, "size %lu seed 0x%lx (%lu)", big[n], seed, 0);
        }
    free(a);
    free(b);
}

#ifndef BX_REPLAY
static void orig_explosion_draws(void)
{
    static const int sizes[] = {
        64, 60, 56, 52, 48, 44, 40, 36, 32, 28, 24, 20,
        128, 121, 114, 107, 100, 93, 86, 79, 72, 65, 58, 51, 44, 37, 30,
        200, 189, 178, 167, 156, 145, 134, 123, 112, 101, 90, 79, 68, 57, 46};
    int i, x, y;
    for (i = 0; i < (int)(sizeof(sizes) / sizeof(sizes[0])); i++)
        for (y = 0; y < sizes[i]; y++)
            for (x = 0; x < sizes[i]; x++)
                rand();
}
#endif

static void test_load(void)
{
    int i, n;
    for (i = 0; i < 300; i++) {
        unsigned int seed = seed_at(i);
        for (n = 3; n <= 10; n++) {
            int a[10], b[10], k;
            bx_rand_state s;
            for (k = 0; k < 10; k++)
                a[k] = b[k] = k;
            ORACLE_ONLY(srand(seed));
            ORACLE_ONLY(orig_explosion_draws());
            ORACLE_BYTES(orig_random_shuffle_int(a, a + n), a, sizeof(a));
            bx_srand(&s, seed);
            bx_init_explosions_rand(&s);
            bx_random_shuffle_int(&s, b, b + n);
            st_load.cases++;
            if (memcmp(a, b, sizeof(a)) != 0 || !same_state(&s))
                fail(&st_load, "seed 0x%lx n %lu (%lu)", seed, n, 0);
        }
    }
}

static unsigned long cmp_calls, cmp_hash;
static const unsigned char *cmp_base;
static int cmp_mode;

static int cmp_element(const void *pa, const void *pb)
{
    unsigned long oa = (unsigned long)((const unsigned char *)pa - cmp_base);
    unsigned long ob = (unsigned long)((const unsigned char *)pb - cmp_base);
    int ka = *(const unsigned char *)pa, kb = *(const unsigned char *)pb;
    cmp_calls++;
    cmp_hash = (cmp_hash ^ oa) * 16777619u;
    cmp_hash = (cmp_hash ^ ob) * 16777619u;
    return cmp_mode ? kb - ka : ka - kb;
}

static void fill(unsigned char *p, int num, int width, int range, int shape)
{
    int i, j;
    for (i = 0; i < num; i++) {
        unsigned int key = rnd32() % range;
        if (shape == 1)
            key = (unsigned int)(i * range / (num ? num : 1));
        else if (shape == 2)
            key = (unsigned int)((num - 1 - i) * range / (num ? num : 1));
        p[i * width] = (unsigned char)key;
        for (j = 1; j < width; j++)
            p[i * width + j] = (unsigned char)(i * 7 + j * 13 + (i >> 8));
    }
}

static void check_qsort(int num, int width, int range, int shape, int mode)
{
    static unsigned char a[2000 * 0x62], b[2000 * 0x62];
    unsigned long calls_a, hash_a;
    fill(a, num, width, range, shape);
    memcpy(b, a, (size_t)num * width);
    cmp_mode = mode;
    cmp_calls = 0;
    cmp_hash = 2166136261u;
    cmp_base = a;
    ORACLE_BYTES(qsort(a, num, width, cmp_element), a, (size_t)num * width);
    calls_a = ORACLE_ULONG(cmp_calls);
    hash_a = ORACLE_ULONG(cmp_hash);
    cmp_calls = 0;
    cmp_hash = 2166136261u;
    cmp_base = b;
    bx_qsort(b, num, width, cmp_element);
    st_qsort.cases++;
    if (memcmp(a, b, (size_t)num * width) != 0)
        fail(&st_qsort, "order: num %lu width %lu range %lu", num, width, range);
    else if (calls_a != cmp_calls || hash_a != cmp_hash)
        fail(&st_qsort, "comparator calls: num %lu width %lu range %lu", num, width, range);
}

static void test_qsort(void)
{
    static const int ranges[] = {1, 2, 3, 5, 16, 256};
    int num, i;
    for (num = 0; num <= 70; num++)
        for (i = 0; i < 60; i++)
            check_qsort(num, 1 + (int)(rnd32() % 0x62), ranges[i % 6], (int)(rnd32() % 3), (int)(rnd32() & 1));
    for (i = 0; i < 400; i++) {
        int width = (i & 3) == 0 ? 1 + (int)(rnd32() % 0x62) : (i & 3) == 1 ? 4 : (int)(1 + rnd32() % 8);
        check_qsort(71 + (int)(rnd32() % 1930), width, ranges[rnd32() % 6], (int)(rnd32() % 3), (int)(rnd32() & 1));
    }
    for (i = 0; i < 12; i++)
        check_qsort(2000, i == 0 ? 1 : i == 1 ? 0x62 : 1 + (int)(rnd32() % 0x62), ranges[i % 6], i % 3, i & 1);
    for (i = 1; i <= 0x62; i++)
        check_qsort(500, i, 3, 0, 0);
}

int main(int argc, char **argv)
{
    int i, bad = 0;
    if (argc > 1 && !vec_open(argv[1]))
        return 2;
    test_rand();
    test_shuffle();
    test_load();
    test_qsort();
    if (argc > 1)
        bad |= vec_close();
    printf("info: InitExplosions draws %lu times\n", (unsigned long)bx_init_explosions_draws());
    for (i = 0; i < (int)(sizeof(all_stats) / sizeof(all_stats[0])); i++) {
        printf("%s: %lu cases, %lu mismatches\n", all_stats[i]->name, all_stats[i]->cases, all_stats[i]->mismatches);
        if (all_stats[i]->mismatches)
            bad = 1;
    }
    return bad;
}
