#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../bitexact.h"

#define MATH_CASES 2000000

typedef struct {
    const char *name;
    unsigned long cases;
    unsigned long mismatches;
    char example[160];
} math_stat_t;

static unsigned int m_state = 0x2545f491u;

static unsigned int mrnd(void)
{
    m_state ^= m_state << 13;
    m_state ^= m_state >> 17;
    m_state ^= m_state << 5;
    return m_state;
}

static bx_u64 bits_of(double d)
{
    union {
        double d;
        bx_u32 w[2];
    } b;
    b.d = d;
    return ((bx_u64)b.w[1] << 32) | b.w[0];
}

static double from_bits(bx_u32 hi, bx_u32 lo)
{
    union {
        double d;
        bx_u32 w[2];
    } b;
    b.w[1] = hi;
    b.w[0] = lo;
    return b.d;
}

static float float_from_bits(bx_u32 v)
{
    union {
        float f;
        bx_u32 u;
    } b;
    b.u = v;
    return b.f;
}

static bx_u32 bits_of_float(float f)
{
    union {
        float f;
        bx_u32 u;
    } b;
    b.f = f;
    return b.u;
}

static double random_positive(void)
{
    bx_u32 hi = mrnd() & 0x7fffffffu;
    bx_u32 exp = (hi >> 20) & 0x7ffu;
    if (exp == 0x7ffu)
        hi &= 0x7fefffffu;
    if ((mrnd() & 63) == 0)
        hi &= 0x800fffffu & 0x7fffffffu;
    return from_bits(hi, mrnd());
}

static double random_moderate(void)
{
    bx_u32 exp = 1023u - 200u + (mrnd() % 401u);
    bx_u32 hi = (exp << 20) | (mrnd() & 0xfffffu);
    if (mrnd() & 1)
        hi |= 0x80000000u;
    return from_bits(hi, mrnd());
}

static double random_int_double(void)
{
    int bits = (int)(mrnd() % 33);
    bx_u32 v = bits == 0 ? 0 : mrnd() >> (32 - bits);
    return (mrnd() & 1) ? -(double)v : (double)v;
}

static void record_u(math_stat_t *s, int same, const char *fmt, double a, double want, double got)
{
    s->cases++;
    if (!same) {
        s->mismatches++;
        if (s->mismatches == 1)
            sprintf(s->example, fmt, a, want, got);
    }
}

static void record_b(math_stat_t *s, int same, const char *fmt, double a, double b, double want, double got)
{
    s->cases++;
    if (!same) {
        s->mismatches++;
        if (s->mismatches == 1)
            sprintf(s->example, fmt, a, b, want, got);
    }
}

static double x87_sqrt(double x)
{
    double r;
    __asm {
        fld qword ptr [x]
        fsqrt
        fstp qword ptr [r]
    }
    return r;
}

static float x87_sqrt_float(float x)
{
    float r;
    __asm {
        fld dword ptr [x]
        fsqrt
        fstp dword ptr [r]
    }
    return r;
}

static void test_sqrt(math_stat_t *sd, math_stat_t *sf, math_stat_t *sc)
{
    int i;
    for (i = 0; i < MATH_CASES; i++) {
        double x = (i & 3) == 0 ? random_int_double() : random_positive();
        double want, got;
        float f;
        if (x < 0)
            x = -x;
        want = x87_sqrt(x);
        got = bx_sqrt(x);
        record_u(sd, bits_of(want) == bits_of(got), "x %.17g want %.17g got %.17g", x, want, got);
        want = sqrt(x);
        record_u(sc, bits_of(want) == bits_of(got), "x %.17g want %.17g got %.17g", x, want, got);
        if ((i & 3) == 1)
            f = (float)random_int_double();
        else
            f = float_from_bits(mrnd() & 0x7f7fffffu);
        if (f < 0)
            f = -f;
        {
            float wantf = x87_sqrt_float(f);
            float gotf = (float)bx_sqrt((double)f);
            record_u(sf, bits_of_float(wantf) == bits_of_float(gotf), "f %.9g want %.9g got %.9g", (double)f,
                     (double)wantf, (double)gotf);
        }
    }
}

static double simple_hypot(double x, double y)
{
    double a = x < 0 ? -x : x;
    double b = y < 0 ? -y : y;
    double big = b > a ? b : a;
    double ra, rb;
    if (big == 0)
        return 0;
    ra = a / big;
    rb = b / big;
    return bx_sqrt(ra * ra + rb * rb) * big;
}

static double extended_hypot(double x, double y)
{
    volatile double ra, rb, sum, root, prod;
    double a = x < 0 ? -x : x;
    double b = y < 0 ? -y : y;
    double big = b > a ? b : a;
    unsigned int saved = _control87(0, 0);
    if (big == 0)
        return 0;
    _control87(0x00000000, 0x00030000);
    ra = a / big;
    rb = b / big;
    sum = ra * ra + rb * rb;
    root = x87_sqrt(sum);
    prod = root * big;
    _control87(saved, 0x00030000);
    return prod;
}

static void test_hypot(math_stat_t *simple, math_stat_t *extended, math_stat_t *by_class)
{
    int i;
    for (i = 0; i < MATH_CASES; i++) {
        double x, y, want, got;
        switch (i & 3) {
        case 0:
            x = random_int_double();
            y = random_int_double();
            break;
        case 1:
            x = random_moderate();
            y = random_moderate();
            break;
        case 2:
            x = random_moderate();
            y = x * (double)(mrnd() % 1000) / 1.0e6;
            break;
        default:
            x = (double)float_from_bits(mrnd() & 0x7f7fffffu);
            y = (double)float_from_bits(mrnd() & 0x7f7fffffu);
            if (x > 1.0e30)
                x = 1.0e30;
            if (y > 1.0e30)
                y = 1.0e30;
            break;
        }
        want = _hypot(x, y);
        got = simple_hypot(x, y);
        record_b(simple, bits_of(want) == bits_of(got), "x %.17g y %.17g want %.17g got %.17g", x, y, want, got);
        record_b(&by_class[i & 3], bits_of(want) == bits_of(got), "x %.17g y %.17g want %.17g got %.17g", x, y,
                 want, got);
        got = extended_hypot(x, y);
        record_b(extended, bits_of(want) == bits_of(got), "x %.17g y %.17g want %.17g got %.17g", x, y, want, got);
    }
}

static double simple_atan(double x)
{
    double sign = 1, r = 0, term, x2;
    int k, n;
    if (x < 0) {
        sign = -1;
        x = -x;
    }
    if (x > 1)
        return sign * (1.5707963267948966 - simple_atan(1 / x));
    for (k = 0; k < 3; k++)
        x = x / (1 + bx_sqrt(1 + x * x));
    x2 = x * x;
    term = x;
    for (n = 0; n < 14; n++) {
        r += term / (double)(2 * n + 1);
        term = -term * x2;
    }
    return sign * 8 * r;
}

static double simple_acos(double x)
{
    double s = bx_sqrt((1 + x) * (1 - x));
    if (x == 0)
        return 1.5707963267948966;
    if (x > 0)
        return simple_atan(s / x);
    return 3.141592653589793 - simple_atan(s / -x);
}

static void test_acos(math_stat_t *s)
{
    int i;
    for (i = 0; i < MATH_CASES / 4; i++) {
        double x = (double)(mrnd() >> 1) / 1073741824.0 - 1.0;
        double want, got;
        if (i & 1)
            x = (double)(mrnd() % 1000001u) / 1000000.0;
        want = acos(x);
        got = simple_acos(x);
        record_u(s, bits_of(want) == bits_of(got), "x %.17g want %.17g got %.17g", x, want, got);
    }
}

static const double pow10_table[] = {1e0, 1e1, 1e2, 1e3, 1e4, 1e5, 1e6, 1e7, 1e8, 1e9, 1e10, 1e11, 1e12, 1e13, 1e14, 1e15};

static void test_atof(math_stat_t *typical, math_stat_t *wide, math_stat_t *scanned)
{
    int i;
    for (i = 0; i < MATH_CASES; i++) {
        math_stat_t *s = (i & 1) ? wide : typical;
        int digits = (i & 1) ? 10 + (int)(mrnd() % 6) : 1 + (int)(mrnd() % 9);
        int frac = (int)(mrnd() % (unsigned)(digits + 1));
        bx_u64 m = 0;
        char text[40], rev[40];
        int n = 0, j, neg = (int)(mrnd() & 1);
        double want, got;
        for (j = 0; j < digits; j++)
            m = m * 10 + (mrnd() % 10);
        for (j = 0; j < digits; j++) {
            rev[n++] = (char)('0' + (int)(m % 10));
            m /= 10;
            if (n == frac && frac < digits)
                rev[n++] = '.';
        }
        if (frac == digits)
            rev[n++] = '.';
        {
            int p = 0;
            if (neg)
                text[p++] = '-';
            if (frac == digits)
                text[p++] = '0';
            while (n > 0)
                text[p++] = rev[--n];
            text[p] = 0;
        }
        m = 0;
        for (j = 0; text[j]; j++)
            if (text[j] >= '0' && text[j] <= '9')
                m = m * 10 + (bx_u64)(text[j] - '0');
        got = (double)(bx_i64)m / pow10_table[frac];
        if (neg)
            got = -got;
        want = atof(text);
        if (!(i & 1)) {
            float scanned_value = 0;
            float exact_value = (float)got;
            sscanf(text, "%f", &scanned_value);
            scanned->cases++;
            if (bits_of_float(scanned_value) != bits_of_float(exact_value)) {
                scanned->mismatches++;
                if (scanned->mismatches == 1)
                    sprintf(scanned->example, "\"%s\" sscanf %.9g (double rounded to float) %.9g", text,
                            (double)scanned_value, (double)exact_value);
            }
        }
        s->cases++;
        if (bits_of(want) != bits_of(got)) {
            s->mismatches++;
            if (s->mismatches == 1)
                sprintf(s->example, "\"%s\" atof %.17g exact %.17g", text, want, got);
        }
    }
}

static int report(const math_stat_t *s, int fatal)
{
    if (fatal)
        printf("%s: %lu cases, %lu mismatches\n", s->name, s->cases, s->mismatches);
    else
        printf("info: %s: %lu cases, %lu mismatches (%.3f%%)\n", s->name, s->cases, s->mismatches,
               100.0 * (double)s->mismatches / (double)s->cases);
    if (s->mismatches && s->example[0])
        printf("  first mismatch: %s\n", s->example);
    return fatal && s->mismatches != 0;
}

int run_math_tests(void)
{
    static math_stat_t sd = {"bx_sqrt vs x87 fsqrt"}, sf = {"bx_sqrt vs x87 fsqrt, float in and out"},
                       sc = {"bx_sqrt vs CRT sqrt"}, hy = {"simple hypot (double steps, 53-bit) vs _hypot"},
                       hx = {"hypot with 64-bit steps (x87 precision control) vs _hypot"},
                       hc[4] = {{"  hypot, integer arguments"}, {"  hypot, doubles over 1e-60 to 1e60"},
                                {"  hypot, one argument far smaller"}, {"  hypot, float arguments"}},
ac = {"simple acos vs acos"},
                       at1 = {"correctly rounded atof, up to 9 digits vs atof"},
                       at2 = {"correctly rounded atof, 10 to 15 digits vs atof"},
                       sc3 = {"sscanf %f vs the correctly rounded double rounded to float"};
    int bad = 0;

    test_sqrt(&sd, &sf, &sc);
    test_hypot(&hy, &hx, hc);
    test_acos(&ac);
    test_atof(&at1, &at2, &sc3);
    bad |= report(&sd, 1);
    bad |= report(&sf, 1);
    bad |= report(&sc, 1);
    report(&hy, 0);
    report(&hc[0], 0);
    report(&hc[1], 0);
    report(&hc[2], 0);
    report(&hc[3], 0);
    bad |= report(&hx, 1);
    report(&ac, 0);
    report(&at1, 0);
    report(&at2, 0);
    report(&sc3, 0);
    return bad;
}
