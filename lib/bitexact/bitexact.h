#ifndef BITEXACT_H
#define BITEXACT_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* A signed 16-bit integer. */
typedef short bx_i16;
/* An unsigned 16-bit integer. */
typedef unsigned short bx_u16;
/* A signed 32-bit integer. */
typedef int bx_i32;
/* An unsigned 32-bit integer. */
typedef unsigned int bx_u32;
/* A signed 64-bit integer. */
#if defined(_MSC_VER)
typedef __int64 bx_i64;
#else
__extension__ typedef long long bx_i64;
#endif
/* An unsigned 64-bit integer. */
#if defined(_MSC_VER)
typedef unsigned __int64 bx_u64;
#else
__extension__ typedef unsigned long long bx_u64;
#endif

/* FixedSine, 0x4b70a0: the sine table entry (sin * 0x2000) for the top 10 bits of a 16-bit angle (0x10000 is a full turn). */
bx_i32 bx_FixedSine(bx_u16 angle);

/* The cosine at 0x4b70c0: bx_FixedSine a quarter turn on, read from the table's own cosine view. */
bx_i32 bx_fixed_cos(bx_u16 angle);

/* The (a * b) >> 32 at 0x4b70e0: the high word of the signed 64-bit product. */
bx_i32 bx_mul_hi32(bx_i32 a, bx_i32 b);

/* FUN_004b70ef: scale * sin(angle), the angle rounded to a table entry and the product rounded to nearest. */
bx_i32 bx_FUN_004b70ef(bx_i16 angle, bx_i32 scale);

/* FUN_004b7123: scale * cos(angle), the angle rounded to a table entry and the product rounded to nearest. */
bx_i32 bx_FUN_004b7123(bx_i16 angle, bx_i32 scale);

/* FUN_004b715a: the angle (0x8000 is half a turn) of the vector (z, x), that is atan2(x, z), rounded to nearest even; needs a 53-bit FPU precision and round to nearest, as the game sets (_control87 0x9001F), and agrees with the exe's fpatan except where the exact result is within about 1e-13 of a half. */
bx_i32 bx_FUN_004b715a(bx_i32 x, bx_i32 z);

/* The result of fpatan(sqrt((1 + x) * (1 - x)), x), the x87 code of MSVC 5's acos under 53-bit precision, stored as a double; x in [-1, 1], an x outside giving the x87 indefinite NaN; agrees with the CPU's fpatan except where its 64-bit result is not the correctly rounded one. */
double bx_acos(double x);

/* FUN_004b7173: rotates xz[0] (x) and xz[1] (z) by angle in place, doing nothing when the angle is 0; exact for the game's FPU mode (_control87 0x9001F), a result out of int range becoming INT_MIN as fistp stores it. */
void bx_FUN_004b7173(bx_i16 angle, bx_i32 *xz);

/* The matrix builder at 0x4b71a7: fills the caller's 3x3 matrix m from angles[0], angles[1] and angles[2], the words the original reads at +0xc, +0xe and +0x10 of its argument. */
void bx_build_rotation_matrix(const bx_i16 *angles, bx_i32 m[9]);

/* The matrix apply at 0x4b72e2: writes the vector in (3 ints) times the matrix m to out, reading and writing in the original's order so that in and out may be the same. */
void bx_apply_rotation_matrix(const bx_i32 m[9], const bx_i32 *in, bx_i32 *out);

/* FUN_004b7381: a * b / c with a 64-bit product, truncated toward zero, or 0 when c is 0; the exe traps when the quotient does not fit in an int, here it is INT_MIN. */
bx_i32 bx_FUN_004b7381(bx_i32 a, bx_i32 b, bx_i32 c);

/* The correctly rounded square root, equal to x87 fsqrt at 53-bit precision (and to SSE2 sqrtsd and wasm f64.sqrt), done in integer code; finite non-negative x, a negative x giving the x87 indefinite NaN. */
double bx_sqrt(double x);

/* MSVC 5's _hypot: big = max(|x|, |y|), then sqrt((x / big)^2 + (y / big)^2) * big with each step rounded to a 64-bit mantissa as the x87 does at control word 0x133f and then stored as a double; finite arguments, results within double range. */
double bx_hypot(double x, double y);

/* The float MSVC 5's sscanf("%f") stores for text: LIBCMT's strgtold12 (24 digits, 80-bit powers of ten), then the rounding of ld12cvt to 24 bits, which is not always the correctly rounded float; *end gets the first character not used, text when there is no number. */
float bx_strtof_msvc5(const char *text, const char **end);

/* The state of one rand stream: LIBCMT keeps it per thread, the port takes it as a parameter so callers name the stream. */
typedef struct {
    bx_u32 holdrand;
} bx_rand_state;

/* LIBCMT srand: restarts the stream s from seed. */
void bx_srand(bx_rand_state *s, bx_u32 seed);

/* LIBCMT rand: advances the stream s and returns the next value, 0 to 0x7fff. */
int bx_rand(bx_rand_state *s);

/* std::random_shuffle(first, last) as MSVC 5's STL compiles it for an int array: draws last - first - 1 times from s, none for an empty or one-element range. */
void bx_random_shuffle_int(bx_rand_state *s, int *first, int *last);

/* The number of rand calls BuildExplosionFrame (0x420d20) makes for a size x size frame: one per pixel. */
bx_u32 bx_explosion_frame_draws(int size);

/* The number of rand calls InitExplosions (0x420620) makes in all: its three explosions' frames summed. */
bx_u32 bx_init_explosions_draws(void);

/* Advances the stream s by the draws of InitExplosions (0x420620), the load thread's draws between the seed and the start-position shuffle. */
void bx_init_explosions_rand(bx_rand_state *s);

/* LIBCMT qsort: sorts num elements of width bytes with the library's own partition and swap order, so equal elements end in the same order and cmp sees the same calls; not stable. */
void bx_qsort(void *base, size_t num, size_t width, int (*cmp)(const void *, const void *));

#ifdef __cplusplus
}
#endif

#endif
