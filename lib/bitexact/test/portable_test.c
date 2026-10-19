#include <stdio.h>
#include <string.h>

#include "../bitexact.h"

int bx_sscanf(const char *buf, const char *fmt, ...);

typedef struct {
    bx_u32 h[8];
    unsigned char block[64];
    bx_u32 used;
    bx_u64 total;
} sha256_t;

static const bx_u32 sha256_k[64] = {
    0x428a2f98u, 0x71374491u, 0xb5c0fbcfu, 0xe9b5dba5u, 0x3956c25bu, 0x59f111f1u, 0x923f82a4u, 0xab1c5ed5u,
    0xd807aa98u, 0x12835b01u, 0x243185beu, 0x550c7dc3u, 0x72be5d74u, 0x80deb1feu, 0x9bdc06a7u, 0xc19bf174u,
    0xe49b69c1u, 0xefbe4786u, 0x0fc19dc6u, 0x240ca1ccu, 0x2de92c6fu, 0x4a7484aau, 0x5cb0a9dcu, 0x76f988dau,
    0x983e5152u, 0xa831c66du, 0xb00327c8u, 0xbf597fc7u, 0xc6e00bf3u, 0xd5a79147u, 0x06ca6351u, 0x14292967u,
    0x27b70a85u, 0x2e1b2138u, 0x4d2c6dfcu, 0x53380d13u, 0x650a7354u, 0x766a0abbu, 0x81c2c92eu, 0x92722c85u,
    0xa2bfe8a1u, 0xa81a664bu, 0xc24b8b70u, 0xc76c51a3u, 0xd192e819u, 0xd6990624u, 0xf40e3585u, 0x106aa070u,
    0x19a4c116u, 0x1e376c08u, 0x2748774cu, 0x34b0bcb5u, 0x391c0cb3u, 0x4ed8aa4au, 0x5b9cca4fu, 0x682e6ff3u,
    0x748f82eeu, 0x78a5636fu, 0x84c87814u, 0x8cc70208u, 0x90befffau, 0xa4506cebu, 0xbef9a3f7u, 0xc67178f2u};

static bx_u32 rotr(bx_u32 x, int n)
{
    return (x >> n) | (x << (32 - n));
}

static void sha256_init(sha256_t *s)
{
    static const bx_u32 init[8] = {0x6a09e667u, 0xbb67ae85u, 0x3c6ef372u, 0xa54ff53au,
                                   0x510e527fu, 0x9b05688cu, 0x1f83d9abu, 0x5be0cd19u};
    memcpy(s->h, init, sizeof(init));
    s->used = 0;
    s->total = 0;
}

static void sha256_compress(sha256_t *s)
{
    bx_u32 w[64], v[8], t1, t2;
    int i;

    for (i = 0; i < 16; i++)
        w[i] = ((bx_u32)s->block[i * 4] << 24) | ((bx_u32)s->block[i * 4 + 1] << 16) |
               ((bx_u32)s->block[i * 4 + 2] << 8) | (bx_u32)s->block[i * 4 + 3];
    for (i = 16; i < 64; i++) {
        bx_u32 s0 = rotr(w[i - 15], 7) ^ rotr(w[i - 15], 18) ^ (w[i - 15] >> 3);
        bx_u32 s1 = rotr(w[i - 2], 17) ^ rotr(w[i - 2], 19) ^ (w[i - 2] >> 10);
        w[i] = w[i - 16] + s0 + w[i - 7] + s1;
    }
    for (i = 0; i < 8; i++)
        v[i] = s->h[i];
    for (i = 0; i < 64; i++) {
        t1 = v[7] + (rotr(v[4], 6) ^ rotr(v[4], 11) ^ rotr(v[4], 25)) + ((v[4] & v[5]) ^ (~v[4] & v[6])) +
             sha256_k[i] + w[i];
        t2 = (rotr(v[0], 2) ^ rotr(v[0], 13) ^ rotr(v[0], 22)) + ((v[0] & v[1]) ^ (v[0] & v[2]) ^ (v[1] & v[2]));
        v[7] = v[6];
        v[6] = v[5];
        v[5] = v[4];
        v[4] = v[3] + t1;
        v[3] = v[2];
        v[2] = v[1];
        v[1] = v[0];
        v[0] = t1 + t2;
    }
    for (i = 0; i < 8; i++)
        s->h[i] += v[i];
}

static void sha256_byte(sha256_t *s, unsigned char b)
{
    s->block[s->used++] = b;
    s->total++;
    if (s->used == 64) {
        sha256_compress(s);
        s->used = 0;
    }
}

static void sha256_print(sha256_t *s, const char *name)
{
    bx_u64 bits = s->total * 8;
    int i;

    sha256_byte(s, 0x80);
    while (s->used != 56)
        sha256_byte(s, 0);
    for (i = 7; i >= 0; i--)
        sha256_byte(s, (unsigned char)(bits >> (i * 8)));
    printf("%s ", name);
    for (i = 0; i < 8; i++)
        printf("%08x", s->h[i]);
    printf("\n");
}

static void put_u32(sha256_t *s, bx_u32 v)
{
    int i;

    for (i = 0; i < 4; i++)
        sha256_byte(s, (unsigned char)(v >> (i * 8)));
}

static void put_u64(sha256_t *s, bx_u64 v)
{
    put_u32(s, (bx_u32)v);
    put_u32(s, (bx_u32)(v >> 32));
}

static void put_i32(sha256_t *s, bx_i32 v)
{
    put_u32(s, (bx_u32)v);
}

static bx_u64 bits_of(double d)
{
    bx_u64 b;
    memcpy(&b, &d, sizeof(b));
    return b;
}

static double from_bits(bx_u64 b)
{
    double d;
    memcpy(&d, &b, sizeof(d));
    return d;
}

static void put_double(sha256_t *s, double d)
{
    put_u64(s, bits_of(d));
}

static void put_float(sha256_t *s, float f)
{
    bx_u32 b;
    memcpy(&b, &f, sizeof(b));
    put_u32(s, b);
}

static bx_u32 rng_state = 0x2545f491u;

static bx_u32 next_random(void)
{
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 17;
    rng_state ^= rng_state << 5;
    return rng_state;
}

static bx_u64 next_random64(void)
{
    bx_u64 hi = next_random();
    return (hi << 32) | next_random();
}

static const bx_i32 edge_ints[] = {0, 1, -1, 2, -2, 3, 255, 256, 0x7fff, -0x8000, 0x8000, 0xffff, 0x10000, 65535,
                                   0x7fffffff, -0x7fffffff - 1, 0x7ffffffe, -0x7fffffff, 1000000, -1000000};
#define EDGE_INTS ((int)(sizeof(edge_ints) / sizeof(edge_ints[0])))

static const bx_i32 edge_scales[] = {0, 1, -1, 2, 100, -100, 4096, 0x10000, 0x7fffffff, -0x7fffffff - 1};
#define EDGE_SCALES ((int)(sizeof(edge_scales) / sizeof(edge_scales[0])))

static bx_i32 any_int(void)
{
    bx_u32 r = next_random();
    int kind = (int)(r & 3);

    r = next_random();
    if (kind == 0)
        return edge_ints[r % EDGE_INTS];
    if (kind == 1)
        return (bx_i32)(r & 0xffff) - 0x8000;
    if (kind == 2)
        return (bx_i32)(r >> (next_random() & 31));
    return (bx_i32)r;
}

static void run_sine(void)
{
    sha256_t s;
    bx_u32 a;

    sha256_init(&s);
    for (a = 0; a < 0x10000; a++)
        put_i32(&s, bx_FixedSine((bx_u16)a));
    sha256_print(&s, "bx_FixedSine");
}

static void run_cosine(void)
{
    sha256_t s;
    bx_u32 a;

    sha256_init(&s);
    for (a = 0; a < 0x10000; a++)
        put_i32(&s, bx_fixed_cos((bx_u16)a));
    sha256_print(&s, "bx_fixed_cos");
}

static void run_mul_hi32(void)
{
    sha256_t s;
    int i, j;

    sha256_init(&s);
    for (i = 0; i < EDGE_INTS; i++)
        for (j = 0; j < EDGE_INTS; j++)
            put_i32(&s, bx_mul_hi32(edge_ints[i], edge_ints[j]));
    for (i = 0; i < 100000; i++) {
        bx_i32 a = any_int(), b = any_int();
        put_i32(&s, bx_mul_hi32(a, b));
    }
    sha256_print(&s, "bx_mul_hi32");
}

typedef bx_i32 (*scaled_fn)(bx_i16, bx_i32);

static void run_scaled(scaled_fn fn, const char *name)
{
    sha256_t s;
    bx_i32 a;
    int j;

    sha256_init(&s);
    for (a = -0x8000; a < 0x8000; a++)
        for (j = 0; j < EDGE_SCALES; j++)
            put_i32(&s, fn((bx_i16)a, edge_scales[j]));
    for (j = 0; j < 200000; j++) {
        bx_i16 angle = (bx_i16)next_random();
        bx_i32 scale = any_int();
        put_i32(&s, fn(angle, scale));
    }
    sha256_print(&s, name);
}

static void run_atan2(void)
{
    sha256_t s;
    bx_i32 x, z;
    int i, j;

    sha256_init(&s);
    for (x = -40; x <= 40; x++)
        for (z = -40; z <= 40; z++)
            put_i32(&s, bx_FUN_004b715a(x, z));
    for (i = 0; i < EDGE_INTS; i++)
        for (j = 0; j < EDGE_INTS; j++)
            put_i32(&s, bx_FUN_004b715a(edge_ints[i], edge_ints[j]));
    for (i = 0; i < 100000; i++) {
        bx_i32 a = any_int(), b = any_int();
        put_i32(&s, bx_FUN_004b715a(a, b));
    }
    sha256_print(&s, "bx_FUN_004b715a");
}

static void run_acos(void)
{
    sha256_t s;
    static const double edges[] = {-1.0, 1.0, 0.0, 0.5, -0.5, 1e-300, -1e-300, 0.999999999999, -0.999999999999,
                                   0.7071067811865476, 4.9e-324};
    int i;

    sha256_init(&s);
    for (i = 0; i < (int)(sizeof(edges) / sizeof(edges[0])); i++)
        put_double(&s, bx_acos(edges[i]));
    for (i = 0; i < 20000; i++) {
        double x = (double)(next_random() >> 5) / 67108864.0 * 2.0 - 1.0;
        put_double(&s, bx_acos(x));
    }
    for (i = 0; i < 20000; i++) {
        double x = (double)(next_random() >> 5) / 67108864.0;
        put_double(&s, bx_acos(x * x * x * x - 1.0 + (i & 1) * 2.0 * (1.0 - x * x * x * x)));
    }
    sha256_print(&s, "bx_acos");
}

static void run_rotate(void)
{
    sha256_t s;
    bx_i32 a, xz[2];
    int j;

    sha256_init(&s);
    for (a = -0x8000; a < 0x8000; a++)
        for (j = 0; j < 4; j++) {
            xz[0] = j == 0 ? 1000 : j == 1 ? -77777 : any_int();
            xz[1] = j == 0 ? 0 : j == 1 ? 12345 : any_int();
            bx_FUN_004b7173((bx_i16)a, xz);
            put_i32(&s, xz[0]);
            put_i32(&s, xz[1]);
        }
    for (j = 0; j < EDGE_INTS * EDGE_INTS; j++) {
        xz[0] = edge_ints[j / EDGE_INTS];
        xz[1] = edge_ints[j % EDGE_INTS];
        bx_FUN_004b7173((bx_i16)(j * 977), xz);
        put_i32(&s, xz[0]);
        put_i32(&s, xz[1]);
    }
    sha256_print(&s, "bx_FUN_004b7173");
}

static void run_matrix(void)
{
    sha256_t build, apply;
    bx_i16 angles[3];
    bx_i32 m[9], in[3], out[3], inplace[3];
    int i, k;

    sha256_init(&build);
    sha256_init(&apply);
    for (i = 0; i < 30000; i++) {
        for (k = 0; k < 3; k++)
            angles[k] = i < 27 ? (bx_i16)(((i / (k == 0 ? 1 : k == 1 ? 3 : 9)) % 3 - 1) * 0x4000)
                               : (bx_i16)next_random();
        bx_build_rotation_matrix(angles, m);
        for (k = 0; k < 9; k++)
            put_i32(&build, m[k]);
        for (k = 0; k < 3; k++) {
            in[k] = (i & 3) == 0 ? edge_ints[next_random() % EDGE_INTS] : any_int() / 4096;
            inplace[k] = in[k];
        }
        bx_apply_rotation_matrix(m, in, out);
        bx_apply_rotation_matrix(m, inplace, inplace);
        for (k = 0; k < 3; k++) {
            put_i32(&apply, out[k]);
            put_i32(&apply, inplace[k]);
        }
    }
    sha256_print(&build, "bx_build_rotation_matrix");
    sha256_print(&apply, "bx_apply_rotation_matrix");
}

static void run_muldiv(void)
{
    sha256_t s;
    int i, j, k;

    sha256_init(&s);
    for (i = 0; i < EDGE_INTS; i++)
        for (j = 0; j < EDGE_INTS; j++)
            for (k = 0; k < EDGE_INTS; k++)
                put_i32(&s, bx_FUN_004b7381(edge_ints[i], edge_ints[j], edge_ints[k]));
    for (i = 0; i < 100000; i++) {
        bx_i32 a = any_int(), b = any_int(), c = any_int();
        put_i32(&s, bx_FUN_004b7381(a, b, (i & 15) == 0 ? 0 : c));
    }
    sha256_print(&s, "bx_FUN_004b7381");
}

static double finite_double(int max_exponent_span)
{
    bx_u64 b = next_random64();
    bx_u32 exponent = (bx_u32)((b >> 52) & 0x7ff);

    if (max_exponent_span > 0)
        exponent = 1023 - (bx_u32)max_exponent_span + exponent % (2 * (bx_u32)max_exponent_span + 1);
    else if (exponent == 0x7ff)
        exponent = 0x7fe;
    b = (b & ~(bx_u64)0x7ff0000000000000u) | ((bx_u64)exponent << 52);
    return from_bits(b);
}

static void run_sqrt(void)
{
    sha256_t s;
    static const double edges[] = {0.0, 4.9e-324, 2.2250738585072009e-308, 2.2250738585072014e-308, 0.25, 1.0, 2.0, 3.0,
                                   4.0, 1.7976931348623157e308, 1e-10, 1e10, 0.1, 1.0000000000000002};
    int i;

    sha256_init(&s);
    for (i = 0; i < (int)(sizeof(edges) / sizeof(edges[0])); i++)
        put_double(&s, bx_sqrt(edges[i]));
    for (i = 0; i < 100000; i++) {
        double x = finite_double(i & 1 ? 40 : 0);
        put_double(&s, bx_sqrt(from_bits(bits_of(x) & 0x7fffffffffffffffull)));
    }
    for (i = 0; i < 100000; i++)
        put_double(&s, bx_sqrt((double)(next_random() >> (i & 15))));
    sha256_print(&s, "bx_sqrt");
}

static void run_hypot(void)
{
    sha256_t s;
    static const double edges[] = {0.0, 1.0, -1.0, 3.0, 4.0, -4.0, 1e-300, 1e150, 4.9e-324, 0.5, 123456.789};
    int i, j, n = (int)(sizeof(edges) / sizeof(edges[0]));

    sha256_init(&s);
    for (i = 0; i < n; i++)
        for (j = 0; j < n; j++)
            put_double(&s, bx_hypot(edges[i], edges[j]));
    for (i = 0; i < 100000; i++) {
        double x = finite_double(i & 1 ? 20 : 400);
        double y = finite_double(i & 1 ? 20 : 400);
        put_double(&s, bx_hypot(x, y));
    }
    for (i = 0; i < 50000; i++) {
        double x = (double)(bx_i32)next_random();
        double y = (double)(bx_i32)next_random();
        put_double(&s, bx_hypot(x, y));
    }
    sha256_print(&s, "bx_hypot");
}

static void make_number_text(char *buf, int variant)
{
    int digits = 1 + (int)(next_random() % 30), point = (int)(next_random() % (digits + 1)), n = 0, i;
    int exponent = (int)(next_random() % 90) - 45;

    if (next_random() & 1)
        buf[n++] = '-';
    for (i = 0; i < digits; i++) {
        if (i == point)
            buf[n++] = '.';
        buf[n++] = (char)('0' + next_random() % 10);
    }
    if (variant & 1)
        n += sprintf(buf + n, "e%d", exponent);
    buf[n] = 0;
}

static const char *const number_edges[] = {
    "0", "-0", "1", "0.1", "0.5", "3.14159265358979", "16777216", "16777217", "16777217.5", "1e38", "3.4028235e38",
    "3.4028236e38", "1e39", "1e-45", "1.4e-45", "7e-46", "1e-46", "1e-50", "1.17549435e-38", "1e300", "1.7976931348623157e308",
    "1e309", "4.9e-324", "2.4703282292062328e-324", "1e-400", "  12.5abc", "\t+7.25e+1x", ".5", "5.", "123456789012345678901234567890",
    "0.000000000000000000000000000001", "9.999999999999999999999999e22", "8.5e-5", "100", "1e+2", "2.5E3", "-.75"};

static void run_strtof_atof(void)
{
    sha256_t f, d;
    char buf[96];
    const char *end;
    int i, n = (int)(sizeof(number_edges) / sizeof(number_edges[0]));

    sha256_init(&f);
    sha256_init(&d);
    for (i = 0; i < n + 40000; i++) {
        if (i < n)
            strcpy(buf, number_edges[i]);
        else
            make_number_text(buf, i);
        end = 0;
        put_float(&f, bx_strtof_msvc5(buf, &end));
        put_u32(&f, (bx_u32)(end - buf));
        put_double(&d, bx_atof_msvc5(buf));
    }
    sha256_print(&f, "bx_strtof_msvc5");
    sha256_print(&d, "bx_atof_msvc5");
}

static void run_sscanf(void)
{
    sha256_t s;
    static const char *const inputs[] = {"1.5 2", "  7 -3.25", "abc", "12", "0.1,0.2", "1e3 x", "", "-0 0", "3.14159 42",
                                         "16777217 1", "2.5e-3,9"};
    static const char *const formats[] = {"%f", "%f %d", "%d %f", "%f,%f", "%d", "%f%n", "%*d %f", "x%f"};
    char buf[48];
    int i, j, k, count, i0, i1;
    float f0, f1;

    sha256_init(&s);
    for (i = 0; i < (int)(sizeof(inputs) / sizeof(inputs[0])); i++)
        for (j = 0; j < (int)(sizeof(formats) / sizeof(formats[0])); j++) {
            f0 = f1 = -1.0f;
            i0 = i1 = -1;
            if (j == 0 || j == 6 || j == 7)
                count = bx_sscanf(inputs[i], formats[j], &f0);
            else if (j == 1)
                count = bx_sscanf(inputs[i], formats[j], &f0, &i0);
            else if (j == 2)
                count = bx_sscanf(inputs[i], formats[j], &i0, &f0);
            else if (j == 3)
                count = bx_sscanf(inputs[i], formats[j], &f0, &f1);
            else if (j == 4)
                count = bx_sscanf(inputs[i], formats[j], &i0);
            else
                count = bx_sscanf(inputs[i], formats[j], &f0, &i0);
            put_i32(&s, count);
            put_float(&s, f0);
            put_float(&s, f1);
            put_i32(&s, i0);
            put_i32(&s, i1);
        }
    for (k = 0; k < 20000; k++) {
        char text[96];
        make_number_text(text, k);
        sprintf(buf, "%.40s", text);
        f0 = -1.0f;
        count = bx_sscanf(buf, "%f", &f0);
        put_i32(&s, count);
        put_float(&s, f0);
    }
    sha256_print(&s, "bx_sscanf");
}

static void run_rand(void)
{
    sha256_t s;
    static const bx_u32 seeds[] = {0, 1, 2, 0x7fff, 0x8000, 0xffffffffu, 0x80000000u, 12345, 0xdeadbeefu};
    bx_rand_state st;
    int i, j;

    sha256_init(&s);
    for (i = 0; i < (int)(sizeof(seeds) / sizeof(seeds[0])) + 200; i++) {
        bx_srand(&st, i < (int)(sizeof(seeds) / sizeof(seeds[0])) ? seeds[i] : next_random());
        put_u32(&s, st.holdrand);
        for (j = 0; j < 2000; j++)
            put_i32(&s, bx_rand(&st));
        put_u32(&s, st.holdrand);
    }
    sha256_print(&s, "bx_srand");
}

static void run_shuffle(void)
{
    sha256_t s;
    bx_rand_state st;
    int a[64], n, seed, i;

    sha256_init(&s);
    for (n = 0; n <= 64; n++)
        for (seed = 0; seed < 40; seed++) {
            bx_srand(&st, (bx_u32)(seed * 7919 + n));
            for (i = 0; i < n; i++)
                a[i] = i * 3 - 5;
            bx_random_shuffle_int(&st, a, a + n);
            for (i = 0; i < n; i++)
                put_i32(&s, a[i]);
            put_u32(&s, st.holdrand);
        }
    sha256_print(&s, "bx_random_shuffle_int");
}

static void run_explosions(void)
{
    sha256_t draws, advance;
    bx_rand_state st;
    int size, i;

    sha256_init(&draws);
    for (size = 0; size <= 300; size++)
        put_u32(&draws, bx_explosion_frame_draws(size));
    put_u32(&draws, bx_init_explosions_draws());
    sha256_print(&draws, "bx_explosion_frame_draws");

    sha256_init(&advance);
    put_u32(&advance, bx_init_explosions_draws());
    for (i = 0; i < 3; i++) {
        bx_srand(&st, (bx_u32)i * 0x9e3779b9u + 1);
        bx_init_explosions_rand(&st);
        put_u32(&advance, st.holdrand);
        put_i32(&advance, bx_rand(&st));
    }
    sha256_print(&advance, "bx_init_explosions_rand");
}

typedef struct {
    bx_i32 key;
    bx_i32 order;
} keyed_t;

static int compare_keys(const void *a, const void *b)
{
    bx_i32 x = ((const keyed_t *)a)->key, y = ((const keyed_t *)b)->key;
    return x < y ? -1 : x > y;
}

static void run_qsort(void)
{
    sha256_t s;
    keyed_t items[300];
    int n, trial, i, range;

    sha256_init(&s);
    for (n = 0; n <= 300; n += n < 40 ? 1 : 13)
        for (trial = 0; trial < 12; trial++) {
            range = trial < 4 ? 2 + trial : trial < 8 ? 20 : 100000;
            for (i = 0; i < n; i++) {
                items[i].key = trial == 8 ? i : trial == 9 ? n - i : trial == 10 ? 0 : (bx_i32)(next_random() % range);
                items[i].order = i;
            }
            bx_qsort(items, (size_t)n, sizeof(keyed_t), compare_keys);
            for (i = 0; i < n; i++) {
                put_i32(&s, items[i].key);
                put_i32(&s, items[i].order);
            }
        }
    sha256_print(&s, "bx_qsort");
}

int main(void)
{
    run_sine();
    run_cosine();
    run_mul_hi32();
    run_scaled(bx_FUN_004b70ef, "bx_FUN_004b70ef");
    run_scaled(bx_FUN_004b7123, "bx_FUN_004b7123");
    run_atan2();
    run_acos();
    run_rotate();
    run_matrix();
    run_muldiv();
    run_sqrt();
    run_hypot();
    run_strtof_atof();
    run_sscanf();
    run_rand();
    run_shuffle();
    run_explosions();
    run_qsort();
    return 0;
}
