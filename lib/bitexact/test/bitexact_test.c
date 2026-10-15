#include <float.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../bitexact.h"
#include "../bx_tables.h"
#include "vectors.h"

#define INT_MIN_ (-2147483647 - 1)
#define INT_MAX_ 2147483647
#define RANDOM_CASES 1000000

extern double g_angle16ToRad;
int g_rotationMatrix[9];

#ifdef BX_REPLAY
#define ORACLE_INT(call) ((int)vec_get())
#define ORACLE_WORDS(call, words, n) \
    do { \
        int oracle_i; \
        for (oracle_i = 0; oracle_i < (n); oracle_i++) \
            (words)[oracle_i] = (int)vec_get(); \
    } while (0)
#else
extern int __cdecl FixedSine(unsigned short angle);
#define ORACLE_INT(call) ((int)vec_put((bx_u32)(call)))
#define ORACLE_WORDS(call, words, n) \
    do { \
        int oracle_i; \
        call; \
        for (oracle_i = 0; oracle_i < (n); oracle_i++) \
            vec_put((bx_u32)(words)[oracle_i]); \
    } while (0)
#endif
extern int run_math_tests(void);

typedef int (__cdecl *fn_angle)(unsigned short);
typedef int (__cdecl *fn_mul_hi)(int, int);
typedef int (__cdecl *fn_scaled)(short, int);
typedef int (__cdecl *fn_atan)(int, int);
typedef void (__cdecl *fn_rotate)(short, int *);
typedef void (__cdecl *fn_build)(const void *);
typedef void (__cdecl *fn_apply)(const int *, int *);
typedef int (__cdecl *fn_div3)(int, int, int);

static fn_angle orig_cos;
static fn_mul_hi orig_mul_hi;
static fn_scaled orig_sin_scaled, orig_cos_scaled;
static fn_atan orig_atan;
static fn_rotate orig_rotate;
static fn_build orig_build;
static fn_apply orig_apply;
static fn_div3 orig_div3;

typedef struct {
    const char *name;
    unsigned long cases;
    unsigned long mismatches;
    unsigned long skipped;
} stat_t;

static stat_t st_sine = {"bx_FixedSine"};
static stat_t st_cos = {"bx_fixed_cos"};
static stat_t st_mulhi = {"bx_mul_hi32"};
static stat_t st_sin_scaled = {"bx_FUN_004b70ef"};
static stat_t st_cos_scaled = {"bx_FUN_004b7123"};
static stat_t st_atan = {"bx_FUN_004b715a"};
static stat_t st_fsincos = {"fsincos_table"};
static stat_t st_rotate = {"bx_FUN_004b7173"};
static stat_t st_build = {"bx_build_rotation_matrix"};
static stat_t st_apply = {"bx_apply_rotation_matrix"};
static stat_t st_div3 = {"bx_FUN_004b7381"};
static stat_t *all_stats[] = {&st_sine, &st_cos, &st_mulhi, &st_sin_scaled, &st_cos_scaled, &st_atan,
                              &st_rotate, &st_build, &st_apply, &st_div3};

static unsigned long fsincos_last_bit;
static unsigned long fsincos_max_diff;
static unsigned long fsincos_exponent_differs;
static unsigned int rng_state = 0x9e3779b9u;

static unsigned int rnd32(void)
{
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 17;
    rng_state ^= rng_state << 5;
    return rng_state;
}

static int rnd_wide(void)
{
    int bits = (int)(rnd32() % 33);
    unsigned int v = bits == 0 ? 0 : rnd32() >> (32 - bits);
    if (rnd32() & 1)
        v = (unsigned int)0 - v;
    return (int)v;
}

static int rnd_full(void)
{
    return (int)rnd32();
}

static void mismatch(stat_t *s, const char *fmt, ...)
{
    va_list ap;
    s->mismatches++;
    if (s->mismatches <= 5) {
        printf("  %s mismatch: ", s->name);
        va_start(ap, fmt);
        vprintf(fmt, ap);
        va_end(ap);
        printf("\n");
    }
}

static int specials[160];
static int n_specials;
static int edges[] = {0, 1, -1, INT_MAX_, INT_MIN_, 0x10000, -0x10000, 1000000, -1000000, 12345678, -87654321, 0x7ffffff0};
#define N_EDGES ((int)(sizeof(edges) / sizeof(edges[0])))
static unsigned short angle_specials[] = {0, 1, 0x2aab, 0x3fff, 0x4000, 0x8000, 0xc000, 0xffff};
#define N_ANGLE_SPECIALS ((int)(sizeof(angle_specials) / sizeof(angle_specials[0])))

static void add_special(int v)
{
    specials[n_specials++] = v;
}

static void build_specials(void)
{
    int k;
    add_special(0);
    add_special(1);
    add_special(-1);
    for (k = 1; k <= 30; k++) {
        add_special(1 << k);
        add_special(-(1 << k));
        add_special((1 << k) - 1);
        add_special(-(1 << k) + 1);
    }
    add_special(INT_MAX_);
    add_special(INT_MIN_);
    add_special(INT_MAX_ - 1);
    add_special(INT_MIN_ + 1);
}

static void test_angles(void)
{
    unsigned int a;
    for (a = 0; a < 0x10000; a++) {
        int want = ORACLE_INT(FixedSine((unsigned short)a));
        int got = bx_FixedSine((bx_u16)a);
        st_sine.cases++;
        if (want != got)
            mismatch(&st_sine, "angle 0x%04x want %d got %d", a, want, got);
        want = ORACLE_INT(orig_cos((unsigned short)a));
        got = bx_fixed_cos((bx_u16)a);
        st_cos.cases++;
        if (want != got)
            mismatch(&st_cos, "angle 0x%04x want %d got %d", a, want, got);
    }
}

static void test_mul_hi(void)
{
    int i, j;
    for (i = 0; i < n_specials; i++)
        for (j = 0; j < n_specials; j++) {
            int want = ORACLE_INT(orig_mul_hi(specials[i], specials[j]));
            int got = bx_mul_hi32(specials[i], specials[j]);
            st_mulhi.cases++;
            if (want != got)
                mismatch(&st_mulhi, "a %d b %d want %d got %d", specials[i], specials[j], want, got);
        }
    for (i = 0; i < RANDOM_CASES; i++) {
        int a = (i & 1) ? rnd_full() : rnd_wide();
        int b = (i & 2) ? rnd_full() : rnd_wide();
        int want = ORACLE_INT(orig_mul_hi(a, b));
        int got = bx_mul_hi32(a, b);
        st_mulhi.cases++;
        if (want != got)
            mismatch(&st_mulhi, "a %d b %d want %d got %d", a, b, want, got);
    }
}

static void one_scaled(unsigned int a, int scale)
{
    int want = ORACLE_INT(orig_sin_scaled((short)a, scale));
    int got = bx_FUN_004b70ef((bx_i16)a, scale);
    st_sin_scaled.cases++;
    if (want != got)
        mismatch(&st_sin_scaled, "angle 0x%04x scale %d want %d got %d", a & 0xffff, scale, want, got);
    want = ORACLE_INT(orig_cos_scaled((short)a, scale));
    got = bx_FUN_004b7123((bx_i16)a, scale);
    st_cos_scaled.cases++;
    if (want != got)
        mismatch(&st_cos_scaled, "angle 0x%04x scale %d want %d got %d", a & 0xffff, scale, want, got);
}

static void test_scaled(void)
{
    unsigned int a;
    int i;
    for (a = 0; a < 0x10000; a++) {
        for (i = 0; i < n_specials; i++)
            one_scaled(a, specials[i]);
        for (i = 0; i < 8; i++)
            one_scaled(a, rnd_wide());
    }
    for (i = 0; i < RANDOM_CASES; i++)
        one_scaled(rnd32() & 0xffff, (i & 1) ? rnd_full() : rnd_wide());
}

static void one_atan(int x, int z)
{
    int want = ORACLE_INT(orig_atan(x, z));
    int got = bx_FUN_004b715a(x, z);
    st_atan.cases++;
    if (want != got)
        mismatch(&st_atan, "x %d z %d want %d got %d", x, z, want, got);
}

static void test_atan(void)
{
    int i, j;
    for (i = 0; i < n_specials; i++)
        for (j = 0; j < n_specials; j++)
            one_atan(specials[i], specials[j]);
    for (i = -300; i <= 300; i++)
        for (j = -300; j <= 300; j++)
            one_atan(i, j);
    for (i = 0; i < 3 * RANDOM_CASES; i++) {
        int x, z;
        switch (i % 3) {
        case 0: x = rnd_wide(); z = rnd_wide(); break;
        case 1: x = rnd_full(); z = rnd_full(); break;
        default:
            z = rnd_wide();
            x = (int)((unsigned int)z + (rnd32() % 5) - 2);
            if (rnd32() & 1)
                x = (int)((unsigned int)0 - (unsigned int)x);
            break;
        }
        one_atan(x, z);
    }
}

static void test_fsincos(void)
{
    int idx;
    for (idx = 0; idx < 0x10000; idx++) {
#ifdef _MSC_VER
        short a = (short)idx;
        double theta = (double)a * g_angle16ToRad;
        unsigned char sinb[10], cosb[10];
        const bx_u32 *e = bx_sincos_table + 5 * idx;
        unsigned char want[20];
        __asm {
            fld qword ptr theta
            fsincos
            fstp tbyte ptr cosb
            fstp tbyte ptr sinb
        }
        memcpy(want, sinb, 8);
        memcpy(want + 8, cosb, 8);
        st_fsincos.cases++;
        {
            bx_u32 sh = (bx_u32)sinb[4] | ((bx_u32)sinb[5] << 8) | ((bx_u32)sinb[6] << 16) | ((bx_u32)sinb[7] << 24);
            bx_u32 sl = (bx_u32)sinb[0] | ((bx_u32)sinb[1] << 8) | ((bx_u32)sinb[2] << 16) | ((bx_u32)sinb[3] << 24);
            bx_u32 ch = (bx_u32)cosb[4] | ((bx_u32)cosb[5] << 8) | ((bx_u32)cosb[6] << 16) | ((bx_u32)cosb[7] << 24);
            bx_u32 cl = (bx_u32)cosb[0] | ((bx_u32)cosb[1] << 8) | ((bx_u32)cosb[2] << 16) | ((bx_u32)cosb[3] << 24);
            bx_u32 se = (bx_u32)sinb[8] | ((bx_u32)sinb[9] << 8);
            bx_u32 ce = (bx_u32)cosb[8] | ((bx_u32)cosb[9] << 8);
            bx_u64 ws = ((bx_u64)sh << 32) | sl, wc = ((bx_u64)ch << 32) | cl;
            bx_u64 ts = ((bx_u64)e[0] << 32) | e[1], tc = ((bx_u64)e[2] << 32) | e[3];
            bx_u64 ds = ws > ts ? ws - ts : ts - ws, dc = wc > tc ? wc - tc : tc - wc;
            if (ds)
                fsincos_last_bit++;
            if (dc)
                fsincos_last_bit++;
            if (ds > fsincos_max_diff)
                fsincos_max_diff = (unsigned long)ds;
            if (dc > fsincos_max_diff)
                fsincos_max_diff = (unsigned long)dc;
            if (se != (e[4] >> 16) || ce != (e[4] & 0xffff))
                fsincos_exponent_differs++;
        }
#else
        (void)idx;
#endif
    }
}

static void one_rotate(unsigned int a, int x, int z)
{
    int w[2], g[2];
    w[0] = g[0] = x;
    w[1] = g[1] = z;
    ORACLE_WORDS(orig_rotate((short)a, w), w, 2);
    bx_FUN_004b7173((bx_i16)a, g);
    st_rotate.cases++;
    if (w[0] != g[0] || w[1] != g[1])
        mismatch(&st_rotate, "angle 0x%04x xz (%d, %d) want (%d, %d) got (%d, %d)", a & 0xffff, x, z, w[0], w[1], g[0], g[1]);
}

static void test_rotate(void)
{
    unsigned int a;
    int i, j;
    for (a = 0; a < 0x10000; a++) {
        for (i = 0; i < N_EDGES; i++)
            for (j = 0; j < N_EDGES; j++)
                one_rotate(a, edges[i], edges[j]);
        for (i = 0; i < 16; i++)
            one_rotate(a, rnd_wide(), rnd_wide());
    }
    for (i = 0; i < 4096; i++)
        for (j = 0; j < 256; j++)
            one_rotate((unsigned int)(((i >> 10) << 14) + (i & 1023) - 512), rnd_full(), rnd_full());
    for (i = 0; i < RANDOM_CASES; i++)
        one_rotate(rnd32() & 0xffff, (i & 1) ? rnd_full() : rnd_wide(), (i & 2) ? rnd_full() : rnd_wide());
}

static void one_build(unsigned short a0, unsigned short a1, unsigned short a2)
{
    unsigned char obj[0x14];
    bx_i16 angles[3];
    bx_i32 m[9];
    int i;

    memset(obj, 0, sizeof(obj));
    memcpy(obj + 0xc, &a0, 2);
    memcpy(obj + 0xe, &a1, 2);
    memcpy(obj + 0x10, &a2, 2);
    angles[0] = (bx_i16)a0;
    angles[1] = (bx_i16)a1;
    angles[2] = (bx_i16)a2;
    memset(g_rotationMatrix, 0x5a, sizeof(g_rotationMatrix));
    ORACLE_WORDS(orig_build(obj), g_rotationMatrix, 9);
    bx_build_rotation_matrix(angles, m);
    st_build.cases++;
    for (i = 0; i < 9; i++)
        if (m[i] != g_rotationMatrix[i]) {
            mismatch(&st_build, "angles (0x%04x, 0x%04x, 0x%04x) entry %d want %d got %d", a0, a1, a2, i, g_rotationMatrix[i], m[i]);
            return;
        }
}

static void test_build(void)
{
    unsigned int a;
    int axis, i, j;
    for (axis = 0; axis < 3; axis++)
        for (a = 0; a < 0x10000; a++)
            for (i = 0; i < N_ANGLE_SPECIALS; i++)
                for (j = 0; j < N_ANGLE_SPECIALS; j++) {
                    unsigned short v[3];
                    v[axis] = (unsigned short)a;
                    v[(axis + 1) % 3] = angle_specials[i];
                    v[(axis + 2) % 3] = angle_specials[j];
                    one_build(v[0], v[1], v[2]);
                }
    for (i = 0; i < RANDOM_CASES; i++)
        one_build((unsigned short)rnd32(), (unsigned short)rnd32(), (unsigned short)rnd32());
}

static void test_apply(void)
{
    int i, k;
    for (i = 0; i < RANDOM_CASES; i++) {
        int in[3], want[3], got[3], m[9];
        int wide = i & 1;
        for (k = 0; k < 9; k++)
            m[k] = (i & 2) ? (wide ? rnd_wide() : rnd_full()) : (int)((rnd32() % 16385) - 8192);
        for (k = 0; k < 3; k++)
            in[k] = (i & 4) ? edges[rnd32() % N_EDGES] : (wide ? rnd_wide() : rnd_full());
        memcpy(g_rotationMatrix, m, sizeof(m));
        ORACLE_WORDS(orig_apply(in, want), want, 3);
        bx_apply_rotation_matrix(m, in, got);
        st_apply.cases++;
        if (memcmp(want, got, sizeof(want)))
            mismatch(&st_apply, "in (%d, %d, %d) want (%d, %d, %d) got (%d, %d, %d)", in[0], in[1], in[2], want[0], want[1], want[2], got[0], got[1], got[2]);
        if (i % 4 == 0) {
            int w2[3], g2[3];
            memcpy(w2, in, sizeof(in));
            memcpy(g2, in, sizeof(in));
            ORACLE_WORDS(orig_apply(w2, w2), w2, 3);
            bx_apply_rotation_matrix(m, g2, g2);
            st_apply.cases++;
            if (memcmp(w2, g2, sizeof(w2)))
                mismatch(&st_apply, "in place (%d, %d, %d) want (%d, %d, %d) got (%d, %d, %d)", in[0], in[1], in[2], w2[0], w2[1], w2[2], g2[0], g2[1], g2[2]);
        }
    }
}

static void one_div3(int a, int b, int c)
{
    int want, got;
    if (c != 0) {
        bx_i64 p = (bx_i64)a * (bx_i64)b;
        bx_i64 q = p / (bx_i64)c;
        if (q > INT_MAX_ || q < INT_MIN_) {
            st_div3.skipped++;
            return;
        }
    }
    want = ORACLE_INT(orig_div3(a, b, c));
    got = bx_FUN_004b7381(a, b, c);
    st_div3.cases++;
    if (want != got)
        mismatch(&st_div3, "a %d b %d c %d want %d got %d", a, b, c, want, got);
}

static void test_div3(void)
{
    int i, j, k;
    for (i = 0; i < n_specials; i++)
        for (j = 0; j < n_specials; j++)
            for (k = 0; k < n_specials; k++)
                one_div3(specials[i], specials[j], specials[k]);
    for (i = 0; i < RANDOM_CASES; i++) {
        int a = (i & 1) ? rnd_full() : rnd_wide();
        int b = (i & 2) ? rnd_full() : rnd_wide();
        int c = (i & 4) ? rnd_full() : rnd_wide();
        if ((i & 7) == 7)
            c = 0;
        if ((i & 15) == 8)
            c = a;
        one_div3(a, b, c);
    }
}

#ifndef BX_REPLAY
static void *entry(int offset, const unsigned char *expect_prologue)
{
    unsigned char *p = (unsigned char *)FixedSine + offset;
    if (memcmp(p, expect_prologue, 3) != 0) {
        printf("entry at +0x%x of FixedSine does not start with push ebp / mov ebp, esp\n", offset);
        exit(2);
    }
    return p;
}
#endif

int main(int argc, char **argv)
{
#ifndef BX_REPLAY
    static const unsigned char prologue[3] = {0x55, 0x8b, 0xec};
#endif
    unsigned int cw;
    int i, bad = 0;

    _control87(0x9001F, 0xFFFFF);
    cw = _control87(0, 0);
    printf("fpu control word (_control87): 0x%05X\n", cw);

    printf("g_angle16ToRad %.17g\n", g_angle16ToRad);

    if (argc > 1 && !vec_open(argv[1]))
        return 2;
#ifndef BX_REPLAY
    orig_cos = (fn_angle)entry(0x20, prologue);
    orig_mul_hi = (fn_mul_hi)entry(0x40, prologue);
    orig_sin_scaled = (fn_scaled)entry(0x4f, prologue);
    orig_cos_scaled = (fn_scaled)entry(0x83, prologue);
    orig_atan = (fn_atan)entry(0xba, prologue);
    orig_rotate = (fn_rotate)entry(0xd3, prologue);
    orig_build = (fn_build)entry(0x107, prologue);
    orig_apply = (fn_apply)entry(0x242, prologue);
    orig_div3 = (fn_div3)entry(0x2e1, prologue);
#endif

    build_specials();
    test_angles();
    test_mul_hi();
    test_scaled();
    test_atan();
    test_fsincos();
    test_rotate();
    test_build();
    test_apply();
    test_div3();
    for (i = 0; i < (int)(sizeof(all_stats) / sizeof(all_stats[0])); i++) {
        printf("%s: %lu cases, %lu mismatches", all_stats[i]->name, all_stats[i]->cases, all_stats[i]->mismatches);
        if (all_stats[i]->skipped)
            printf(" (%lu skipped: quotient overflows, where the exe traps)", all_stats[i]->skipped);
        printf("\n");
        if (all_stats[i]->mismatches)
            bad = 1;
    }
    bad |= run_math_tests();
    if (argc > 1)
        bad |= vec_close();
    printf("info: the table's 80-bit fsincos values are exact roundings; this CPU's fsincos differs from them in %lu of %lu values"
           " (%lu with another exponent), by at most %lu in the last mantissa bits, mostly near a quarter turn\n",
           fsincos_last_bit, 2 * st_fsincos.cases, fsincos_exponent_differs, fsincos_max_diff);
    return bad;
}
