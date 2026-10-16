#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../bitexact.h"
#include "vectors.h"

#ifdef BX_REPLAY
#define ORACLE_DOUBLE(call) vec_get_double()
#define ORACLE_BITS(call) vec_get()
#else
static double oracle_double(double value)
{
    vec_put_double(value);
    return value;
}
#define ORACLE_DOUBLE(call) oracle_double(call)
#define ORACLE_BITS(call) vec_put(call)
#endif

#define MATH_CASES 2000000
#define ACOS_ALLOWED 50

typedef struct {
    const char *name;
    unsigned long cases;
    unsigned long mismatches;
    char example[320];
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

static double random_exp_double(unsigned int lo, unsigned int span, int signed_value)
{
    bx_u32 exp = lo + mrnd() % span;
    bx_u32 hi = (exp << 20) | (mrnd() & 0xfffffu);
    bx_u32 low = mrnd();
    if (signed_value && (mrnd() & 1))
        hi |= 0x80000000u;
    return from_bits(hi, low);
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
        want = ORACLE_DOUBLE(x87_sqrt(x));
        got = bx_sqrt(x);
        record_u(sd, bits_of(want) == bits_of(got), "x %.17g want %.17g got %.17g", x, want, got);
        want = ORACLE_DOUBLE(sqrt(x));
        record_u(sc, bits_of(want) == bits_of(got), "x %.17g want %.17g got %.17g", x, want, got);
        if ((i & 3) == 1)
            f = (float)random_int_double();
        else
            f = float_from_bits(mrnd() & 0x7f7fffffu);
        if (f < 0)
            f = -f;
        {
            float wantf = float_from_bits(ORACLE_BITS(bits_of_float(x87_sqrt_float(f))));
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

static void test_hypot(math_stat_t *simple, math_stat_t *extended, math_stat_t *by_class, math_stat_t *port,
                       math_stat_t *port_class)
{
    int i;
    for (i = 0; i < MATH_CASES; i++) {
        double x, y, want, got;
        switch (i % 6) {
        case 0:
            x = random_int_double();
            y = random_int_double();
            break;
        case 4:
            x = (double)(int)(mrnd() % 129u) - 64.0;
            y = (double)(int)(mrnd() % 129u) - 64.0;
            break;
        case 5:
            x = random_exp_double(1023u - 900u, 1801u, 1);
            y = random_exp_double(1023u - 900u, 1801u, 1);
            if (mrnd() & 1)
                y = x * random_exp_double(1023u - 60u, 121u, 0);
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
        want = ORACLE_DOUBLE(_hypot(x, y));
        got = simple_hypot(x, y);
        record_b(simple, bits_of(want) == bits_of(got), "x %.17g y %.17g want %.17g got %.17g", x, y, want, got);
        record_b(&by_class[i % 6], bits_of(want) == bits_of(got), "x %.17g y %.17g want %.17g got %.17g", x, y,
                 want, got);
        got = extended_hypot(x, y);
        record_b(extended, bits_of(want) == bits_of(got), "x %.17g y %.17g want %.17g got %.17g", x, y, want, got);
        got = bx_hypot(x, y);
        record_b(port, bits_of(want) == bits_of(got), "x %.17g y %.17g want %.17g got %.17g", x, y, want, got);
        record_b(&port_class[i % 6], bits_of(want) == bits_of(got), "x %.17g y %.17g want %.17g got %.17g", x, y,
                 want, got);
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

static void test_acos(math_stat_t *s, math_stat_t *port, math_stat_t *by_class)
{
    int i;
    for (i = 0; i < MATH_CASES / 4; i++) {
        double x = (double)(mrnd() >> 1) / 1073741824.0 - 1.0;
        double want, got;
        int c = i & 7;
        if (c == 1 || c == 5)
            x = (double)(mrnd() % 1000001u) / 1000000.0;
        else if (c == 2)
            {
            double n = (double)(mrnd() & 0xfffffu);
            double d1 = (double)(1u << (20 + mrnd() % 11u));
            double d2 = (double)(1u << (mrnd() % 20u));
            x = 1.0 - n / d1 / d2;
        }
        else if (c == 3)
            x = random_exp_double(1023u - 60u, 60u, 0);
        else if (c == 4)
            x = (double)((int)(mrnd() % 2001u) - 1000) / 1000.0;
        else if (c == 6)
            x = (double)((int)(mrnd() % 65u) - 32) / 32.0;
        if (c != 0 && (mrnd() & 1) && c != 4 && c != 6)
            x = -x;
        if (i < 5)
            x = i == 0 ? 1.0 : i == 1 ? -1.0 : i == 2 ? 0.0 : i == 3 ? 0.5 : -0.5;
        want = ORACLE_DOUBLE(acos(x));
        got = simple_acos(x);
        record_u(s, bits_of(want) == bits_of(got), "x %.17g want %.17g got %.17g", x, want, got);
        got = bx_acos(x);
        record_u(port, bits_of(want) == bits_of(got), "x %.17g want %.17g got %.17g", x, want, got);
        if (bits_of(want) != bits_of(got) && port->mismatches > ACOS_ALLOWED)
            printf("  acos over the limit: x 0x%08x%08x want 0x%08x%08x got 0x%08x%08x\n", (bx_u32)(bits_of(x) >> 32),
                   (bx_u32)bits_of(x), (bx_u32)(bits_of(want) >> 32), (bx_u32)bits_of(want),
                   (bx_u32)(bits_of(got) >> 32), (bx_u32)bits_of(got));
        record_u(&by_class[c], bits_of(want) == bits_of(got), "x %.17g want %.17g got %.17g", x, want, got);
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
        want = ORACLE_DOUBLE(atof(text));
        if (!(i & 1)) {
            float scanned_value = 0;
            float exact_value = (float)got;
#ifdef BX_REPLAY
            scanned_value = float_from_bits(vec_get());
#else
            sscanf(text, "%f", &scanned_value);
            vec_put(bits_of_float(scanned_value));
#endif
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


static void make_float_text(char *text)
{
    int kind = (int)(mrnd() % 6u), digits, frac, j, p = 0;
    char d[48];

    switch (kind) {
    case 0:
        digits = 1 + (int)(mrnd() % 12u);
        frac = -1;
        break;
    case 1:
        digits = 1 + (int)(mrnd() % 9u);
        frac = (int)(mrnd() % (unsigned)(digits + 1));
        break;
    case 2:
        digits = 8 + (int)(mrnd() % 18u);
        frac = (int)(mrnd() % (unsigned)(digits + 1));
        break;
    case 3:
        digits = 15 + (int)(mrnd() % 26u);
        frac = (mrnd() & 1) ? -1 : (int)(mrnd() % (unsigned)(digits + 1));
        break;
    default:
        digits = 1 + (int)(mrnd() % 10u);
        frac = (mrnd() & 1) ? -1 : (int)(mrnd() % (unsigned)(digits + 1));
        break;
    }
    for (j = 0; j < digits; j++)
        d[j] = (char)('0' + (int)(mrnd() % 10u));
    if (kind == 3 && (mrnd() & 1))
        for (j = digits / 2; j < digits; j++)
            d[j] = '0';
    if ((mrnd() & 3) == 0) {
        int lead = (int)(mrnd() % 4u);
        for (j = 0; j < lead && j < digits; j++)
            d[j] = '0';
    }
    if (mrnd() & 1)
        text[p++] = '-';
    else if ((mrnd() & 7) == 0)
        text[p++] = '+';
    for (j = 0; j < digits; j++) {
        if (j == frac)
            text[p++] = '.';
        text[p++] = d[j];
    }
    if (frac == digits)
        text[p++] = '.';
    if (kind >= 4) {
        int ex = (int)(mrnd() % 100u) - 50;
        if (kind == 5)
            ex = (int)(mrnd() % 9u) - 4;
        if (mrnd() & 1)
            text[p++] = 'e';
        else
            text[p++] = 'E';
        if (ex < 0) {
            text[p++] = '-';
            ex = -ex;
        } else if (mrnd() & 1) {
            text[p++] = '+';
        }
        if (ex >= 10 || (mrnd() & 1))
            text[p++] = (char)('0' + ex / 10);
        text[p++] = (char)('0' + ex % 10);
    }
    text[p] = 0;
}

static void test_strtof(math_stat_t *s)
{
    int i;
    for (i = 0; i < MATH_CASES; i++) {
        char text[96];
        float want = 0, got;
        bx_u32 wb, gb;
        make_float_text(text);
#ifdef BX_REPLAY
        want = float_from_bits(vec_get());
#else
        sscanf(text, "%f", &want);
        vec_put(bits_of_float(want));
#endif
        got = bx_strtof_msvc5(text, 0);
        wb = bits_of_float(want);
        gb = bits_of_float(got);
        s->cases++;
        if (wb != gb) {
            s->mismatches++;
            if (s->mismatches == 1)
                sprintf(s->example, "\"%s\" sscanf %.9g (0x%08x) port %.9g (0x%08x)", text, (double)want, wb,
                        (double)got, gb);
        }
    }
}

int __cdecl bx_sscanf(const char *buf, const char *fmt, ...);

typedef struct {
    int ret;
    int i[2];
    unsigned int f[3];
    char s[64];
} scan_out;

static const char *scan_formats[] = {
    " %d %d", " %f %f", " %[a-zA-Z0-9_.]", " %f %f %f", " %[a-zA-Z0-9_.] %d %f %f", " %f %d", " %d",
    " %[a-zA-Z0-9.]", "%d %f", "%f,%f", "%s %f", "%f%d", "%f %[abc]", " %f %n", "%*d %f", " %f %",
    "%*d%d", " %f %n%d"};
#define N_SCAN_FORMATS ((int)(sizeof(scan_formats) / sizeof(scan_formats[0])))

static void scan_word(char *text, int *p, const char *alphabet)
{
    int n = 1 + (int)(mrnd() % 12u), len = (int)strlen(alphabet), j;
    for (j = 0; j < n; j++)
        text[(*p)++] = alphabet[mrnd() % (unsigned)len];
}

static void make_scan_text(int k, char *text)
{
    const char *f = scan_formats[k];
    int p = 0, spaces = (int)(mrnd() % 3u), cut, j;
    char piece[96];

    for (j = 0; j < spaces; j++)
        text[p++] = ' ';
    for (; *f; f++) {
        if (*f == ' ') {
            spaces = 1 + (int)(mrnd() % 3u);
            for (j = 0; j < spaces; j++)
                text[p++] = ' ';
        } else if (*f != '%') {
            text[p++] = *f;
        } else if (f[1] == 'n' || f[1] == 0) {
            if (f[1])
                f++;
        } else if (f[1] == 'd' || f[1] == '*') {
            int digits = 1 + (int)(mrnd() % 6u);
            if (mrnd() & 1)
                text[p++] = '-';
            for (j = 0; j < digits; j++)
                text[p++] = (char)('0' + (int)(mrnd() % 10u));
            f += f[1] == '*' ? 2 : 1;
        } else if (f[1] == 'f') {
            make_float_text(piece);
            strcpy(text + p, piece);
            p += (int)strlen(piece);
            f++;
        } else if (f[1] == '[') {
            scan_word(text, &p, f[2] == 'a' && f[3] == 'b' ? "abcabcxyz" : "abcXYZ019_.");
            while (*f != ']')
                f++;
        } else {
            scan_word(text, &p, "abcXYZ019_.");
            f++;
        }
    }
    text[p] = 0;
    cut = (int)(mrnd() % 16u);
    if (cut == 0 && p > 0)
        text[mrnd() % (unsigned)p] = 0;
    else if (cut == 1)
        strcat(text, " zz");
    else if (cut == 2)
        strcat(text, "x");
    else if (cut == 3 && p > 0)
        text[mrnd() % (unsigned)p] = 'q';
}

static void scan_case(int k, const char *text, int port, scan_out *o)
{
    void *a[4] = {0, 0, 0, 0};
    const char *fmt = scan_formats[k];

    memset(o, 0, sizeof(*o));
    o->i[0] = o->i[1] = 0x5a5a5a5a;
    o->f[0] = o->f[1] = o->f[2] = 0x7f7f1234u;
    memset(o->s, '#', sizeof(o->s) - 1);
    switch (k) {
    case 0: a[0] = &o->i[0]; a[1] = &o->i[1]; break;
    case 1: a[0] = &o->f[0]; a[1] = &o->f[1]; break;
    case 2: case 7: a[0] = o->s; break;
    case 3: a[0] = &o->f[0]; a[1] = &o->f[1]; a[2] = &o->f[2]; break;
    case 4: a[0] = o->s; a[1] = &o->i[0]; a[2] = &o->f[0]; a[3] = &o->f[1]; break;
    case 5: a[0] = &o->f[0]; a[1] = &o->i[0]; break;
    case 6: a[0] = &o->i[0]; break;
    case 8: a[0] = &o->i[0]; a[1] = &o->f[0]; break;
    case 9: a[0] = &o->f[0]; a[1] = &o->f[1]; break;
    case 10: a[0] = o->s; a[1] = &o->f[0]; break;
    case 11: a[0] = &o->f[0]; a[1] = &o->i[0]; break;
    case 13: a[0] = &o->f[0]; a[1] = &o->i[0]; break;
    case 14: case 15: a[0] = &o->f[0]; break;
    case 16: a[0] = &o->i[0]; break;
    case 17: a[0] = &o->f[0]; a[1] = &o->i[0]; a[2] = &o->i[1]; break;
    default: a[0] = &o->f[0]; a[1] = o->s; break;
    }
    if (port)
        o->ret = bx_sscanf(text, fmt, a[0], a[1], a[2], a[3]);
    else
        o->ret = sscanf(text, fmt, a[0], a[1], a[2], a[3]);
}

static void test_sscanf(math_stat_t *s)
{
    int i;
    for (i = 0; i < 400000; i++) {
        char text[512];
        scan_out want, got;
        int k = i % N_SCAN_FORMATS;
        make_scan_text(k, text);
#ifdef BX_REPLAY
        vec_get_bytes(&want, sizeof(want));
#else
        scan_case(k, text, 0, &want);
        vec_put_bytes(&want, sizeof(want));
#endif
        scan_case(k, text, 1, &got);
        s->cases++;
        if (memcmp(&want, &got, sizeof(want)) != 0) {
            s->mismatches++;
            if (s->mismatches == 1)
                sprintf(s->example, "format \"%s\" text \"%.60s\" return %d vs %d, ints %d %d vs %d %d, floats %08x %08x vs %08x %08x", scan_formats[k], text, want.ret,
                        got.ret, want.i[0], want.i[1], got.i[0], got.i[1], want.f[0], want.f[1], got.f[0], got.f[1]);
        }
    }
}

static int report_up_to(const math_stat_t *s, unsigned long limit)
{
    printf("%s: %lu cases, %lu mismatches (at most %lu allowed: the CPU's fpatan is not always the correctly rounded 64-bit value)\n",
           s->name, s->cases, s->mismatches, limit);
    if (s->mismatches && s->example[0])
        printf("  first mismatch: %s\n", s->example);
    return s->mismatches > limit;
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
                       hc[6] = {{"  hypot, integer arguments"}, {"  hypot, doubles over 1e-60 to 1e60"},
                                {"  hypot, one argument far smaller"}, {"  hypot, float arguments"},
                                {"  hypot, integers from -64 to 64"}, {"  hypot, exponents from -900 to 900"}},
                       hp = {"bx_hypot vs _hypot"},
                       hpc[6] = {{"  bx_hypot, integer arguments"}, {"  bx_hypot, doubles over 1e-60 to 1e60"},
                                 {"  bx_hypot, one argument far smaller"}, {"  bx_hypot, float arguments"},
                                 {"  bx_hypot, integers from -64 to 64"}, {"  bx_hypot, exponents from -900 to 900"}},
ac = {"simple acos vs acos"}, acp = {"bx_acos vs acos"},
                       acc[8] = {{"  bx_acos, random in [-1, 1)"}, {"  bx_acos, random in [0, 1] to 6 digits"},
                                 {"  bx_acos, within 2^-50 of 1"}, {"  bx_acos, below 2^-1 in magnitude"},
                                 {"  bx_acos, thousandths"}, {"  bx_acos, random in [0, 1] to 6 digits, again"},
                                 {"  bx_acos, multiples of 1/32"}, {"  bx_acos, random in [-1, 1), again"}},
                       at1 = {"correctly rounded atof, up to 9 digits vs atof"},
                       at2 = {"correctly rounded atof, 10 to 15 digits vs atof"},
                       sc3 = {"sscanf %f vs the correctly rounded double rounded to float"},
                       sf1 = {"bx_strtof_msvc5 vs sscanf %f"}, ss1 = {"bx_sscanf vs sscanf"};
    int bad = 0, i;

    test_sqrt(&sd, &sf, &sc);
    test_hypot(&hy, &hx, hc, &hp, hpc);
    test_acos(&ac, &acp, acc);
    test_atof(&at1, &at2, &sc3);
    test_strtof(&sf1);
    test_sscanf(&ss1);
    bad |= report(&sd, 1);
    bad |= report(&sf, 1);
    bad |= report(&sc, 1);
    report(&hy, 0);
    report(&hc[0], 0);
    report(&hc[1], 0);
    report(&hc[2], 0);
    report(&hc[3], 0);
    report(&hc[4], 0);
    report(&hc[5], 0);
    bad |= report(&hp, 1);
    for (i = 0; i < 6; i++)
        bad |= report(&hpc[i], 1);
    bad |= report(&hx, 1);
    report(&ac, 0);
    bad |= report_up_to(&acp, ACOS_ALLOWED);
    for (i = 0; i < 8; i++)
        report(&acc[i], 0);
    report(&at1, 0);
    report(&at2, 0);
    report(&sc3, 0);
    bad |= report(&sf1, 1);
    bad |= report(&ss1, 1);
    return bad;
}
