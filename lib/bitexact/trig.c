#include "bitexact.h"
#include "internal.h"
#include "bx_tables.h"

#define BX_INT_MIN (-2147483647 - 1)

typedef union {
    double d;
    bx_u32 w[2];
} bx_double_bits;

static double bx_double_from_words(bx_u32 hi, bx_u32 lo)
{
    bx_double_bits b;
    b.w[1] = hi;
    b.w[0] = lo;
    return b.d;
}

static bx_i32 bx_round_to_int(double v)
{
    bx_i64 t;
    double f;

    if (!(v > -4e18 && v < 4e18))
        return BX_INT_MIN;
    t = (bx_i64)v;
    f = v - (double)t;
    if (v >= 0) {
        if (f > 0.5 || (f == 0.5 && (t & 1)))
            t++;
    } else {
        if (f < -0.5 || (f == -0.5 && (t & 1)))
            t--;
    }
    if (t > 2147483647 || t < -2147483647 - 1)
        return BX_INT_MIN;
    return (bx_i32)t;
}

static bx_i32 bx_shrd13(bx_i64 product)
{
    return (bx_i32)(bx_u32)(bx_u32)((bx_u64)product >> 13);
}

static bx_i32 bx_mul13(bx_i32 a, bx_i32 b)
{
    return bx_shrd13((bx_i64)a * (bx_i64)b);
}

static bx_i32 bx_add(bx_i32 a, bx_i32 b)
{
    return (bx_i32)((bx_u32)a + (bx_u32)b);
}

static bx_i32 bx_sub(bx_i32 a, bx_i32 b)
{
    return (bx_i32)((bx_u32)a - (bx_u32)b);
}

static bx_i32 bx_neg(bx_i32 a)
{
    return (bx_i32)((bx_u32)0 - (bx_u32)a);
}

/* FUNCTION: 0x4b70a0 */
bx_i32 bx_FixedSine(bx_u16 angle)
{
    return bx_sin_table[(angle >> 7) & 0x1ff];
}

/* FUNCTION: 0x4b70c0 */
bx_i32 bx_fixed_cos(bx_u16 angle)
{
    return bx_sin_table[128 + ((angle >> 7) & 0x1ff)];
}

/* FUNCTION: 0x4b70e0 */
bx_i32 bx_mul_hi32(bx_i32 a, bx_i32 b)
{
    return (bx_i32)((bx_i64)a * (bx_i64)b >> 32);
}

static bx_i32 bx_scaled(bx_i32 entry, bx_i32 scale)
{
    return bx_shrd13((bx_i64)entry * (bx_i64)scale + 0x1000);
}

/* FUNCTION: 0x4b70ef */
bx_i32 bx_FUN_004b70ef(bx_i16 angle, bx_i32 scale)
{
    bx_u32 index = (((bx_u32)(bx_i32)angle + 0x20) >> 7) & 0x1ff;
    return bx_scaled(bx_sin_table[index], scale);
}

/* FUNCTION: 0x4b7123 */
bx_i32 bx_FUN_004b7123(bx_i16 angle, bx_i32 scale)
{
    bx_u32 index = (((bx_u32)(bx_u16)angle + 0x4020) >> 7) & 0x1ff;
    return bx_scaled(bx_sin_table[index], scale);
}

static void bx_two_sum(double a, double b, double *s, double *e)
{
    double t = a + b;
    double bb = t - a;
    *e = (a - (t - bb)) + (b - bb);
    *s = t;
}

static void bx_two_prod(double a, double b, double *p, double *e)
{
    double t, ah, al, bh, bl;

    *p = a * b;
    t = 134217729.0 * a;
    ah = t - (t - a);
    al = a - ah;
    t = 134217729.0 * b;
    bh = t - (t - b);
    bl = b - bh;
    *e = ((ah * bh - *p) + ah * bl + al * bh) + al * bl;
}

static void bx_dd_const(const bx_u32 *w, double *hi, double *lo)
{
    *hi = bx_double_from_words(w[0], w[1]);
    *lo = bx_double_from_words(w[2], w[3]);
}

/* FUNCTION: 0x4b715a */
bx_i32 bx_FUN_004b715a(bx_i32 x, bx_i32 z)
{
    double n, d, num, den, u1, u2, w, p, pe, rem, poly, tail;
    double ch, cl, ah, ae, al, qh, ql, th, te, tl, k_rad, v;
    double c_hi, c_lo, sign;
    bx_u32 ax, bz;
    int k, swap, z_neg;

    if (x == 0 && z == 0)
        return 0;
    ax = x < 0 ? (bx_u32)0 - (bx_u32)x : (bx_u32)x;
    bz = z < 0 ? (bx_u32)0 - (bx_u32)z : (bx_u32)z;
    z_neg = z < 0;
    swap = ax > bz;
    n = (double)(swap ? bz : ax);
    d = (double)(swap ? ax : bz);

    k = (int)(n * 16.0 / d + 0.5);
    num = 16.0 * n - (double)k * d;
    den = 16.0 * d + (double)k * n;
    u1 = num / den;
    bx_two_prod(u1, den, &p, &pe);
    rem = (num - p) - pe;
    u2 = rem / den;
    w = u1 * u1;
    poly = 1.0 / 17.0;
    poly = w * poly - 1.0 / 15.0;
    poly = w * poly + 1.0 / 13.0;
    poly = w * poly - 1.0 / 11.0;
    poly = w * poly + 1.0 / 9.0;
    poly = w * poly - 1.0 / 7.0;
    poly = w * poly + 1.0 / 5.0;
    poly = w * poly - 1.0 / 3.0;
    tail = u1 * w * poly + u2 / (1.0 + w);
    bx_dd_const(bx_atan_sixteenths + 4 * k, &ch, &cl);
    bx_two_sum(ch, u1, &ah, &ae);
    al = cl + ae + tail;

    if (!swap && !z_neg) {
        c_hi = 0.0;
        c_lo = 0.0;
        sign = 1.0;
    } else if (!swap) {
        bx_dd_const(bx_pi_pi_half, &c_hi, &c_lo);
        sign = -1.0;
    } else {
        bx_dd_const(bx_pi_pi_half + 4, &c_hi, &c_lo);
        sign = z_neg ? 1.0 : -1.0;
    }
    qh = sign * ah;
    ql = sign * al;
    bx_two_sum(c_hi, qh, &th, &te);
    tl = te + c_lo + ql;

    k_rad = bx_double_from_words(bx_rad_to_angle16_bits[0], bx_rad_to_angle16_bits[1]);
    bx_two_prod(th, k_rad, &p, &pe);
    v = p + (pe + tl * k_rad);
    k = bx_round_to_int(v);
    return x < 0 ? bx_neg(k) : k;
}

static int bx_bit_length(bx_u64 v)
{
    int n = 0;
    while (v) {
        n++;
        v >>= 1;
    }
    return n;
}

static double bx_mul_ext(bx_i32 x, bx_u32 mant_hi, bx_u32 mant_lo, bx_u32 sign_exp)
{
    bx_u32 ax;
    bx_u64 phi, plo, carry, top, sticky, r, rem;
    bx_u32 low32;
    int sh, neg, exp;
    bx_double_bits b;

    if (x == 0)
        return 0.0;
    ax = x < 0 ? (bx_u32)0 - (bx_u32)x : (bx_u32)x;
    neg = (x < 0) != ((sign_exp & 0x8000) != 0);
    phi = (bx_u64)ax * (bx_u64)mant_hi;
    plo = (bx_u64)ax * (bx_u64)mant_lo;
    carry = phi + (plo >> 32);
    low32 = (bx_u32)(plo & 0xffffffffu);
    sh = bx_bit_length(carry) - 32;
    top = (carry << (32 - sh)) | ((bx_u64)low32 >> sh);
    sticky = (bx_u64)low32 & (((bx_u64)1 << sh) - 1);
    r = top >> 11;
    rem = top & 0x7ff;
    if (rem > 0x400 || (rem == 0x400 && (sticky || (r & 1))))
        r++;
    exp = (int)(sign_exp & 0x7fff) - 16383 + sh;
    if (r == ((bx_u64)1 << 53)) {
        r >>= 1;
        exp++;
    }
    b.w[1] = ((bx_u32)(exp + 1023) << 20) | (bx_u32)((r >> 32) & 0xfffff);
    b.w[0] = (bx_u32)(r & 0xffffffffu);
    if (neg)
        b.w[1] |= 0x80000000u;
    return b.d;
}

/* FUNCTION: 0x4b7173 */
void bx_FUN_004b7173(bx_i16 angle, bx_i32 *xz)
{
    const bx_u32 *e;
    double xc, yc, xs, ys;
    bx_i32 x, y;

    if (angle == 0)
        return;
    e = bx_sincos_table + 5 * (bx_u32)(bx_u16)angle;
    x = xz[0];
    y = xz[1];
    xc = bx_mul_ext(x, e[2], e[3], e[4] & 0xffff);
    yc = bx_mul_ext(y, e[2], e[3], e[4] & 0xffff);
    xs = bx_mul_ext(x, e[0], e[1], e[4] >> 16);
    ys = bx_mul_ext(y, e[0], e[1], e[4] >> 16);
    xz[0] = bx_round_to_int(xc - ys);
    xz[1] = bx_round_to_int(yc + xs);
}

/* FUNCTION: 0x4b71a7 */
void bx_build_rotation_matrix(const bx_i16 *angles, bx_i32 m[9])
{
    bx_i32 c0 = bx_fixed_cos((bx_u16)angles[0]);
    bx_i32 s0 = bx_FixedSine((bx_u16)angles[0]);
    bx_i32 c1 = bx_fixed_cos((bx_u16)angles[1]);
    bx_i32 s1 = bx_FixedSine((bx_u16)angles[1]);
    bx_i32 c2 = bx_fixed_cos((bx_u16)angles[2]);
    bx_i32 s2 = bx_FixedSine((bx_u16)angles[2]);

    m[0] = bx_mul13(c1, c2);
    m[1] = bx_add(bx_mul13(s2, c0), bx_mul13(bx_mul13(s1, s0), c2));
    m[2] = bx_sub(bx_mul13(s0, s2), bx_mul13(bx_mul13(c0, s1), c2));
    m[3] = bx_neg(bx_mul13(c1, s2));
    m[4] = bx_sub(bx_mul13(c0, c2), bx_mul13(bx_mul13(s0, s1), s2));
    m[5] = bx_add(bx_mul13(bx_mul13(c0, s1), s2), bx_mul13(s0, c2));
    m[6] = s1;
    m[7] = bx_neg(bx_mul13(s0, c1));
    m[8] = bx_mul13(c0, c1);
}

/* FUNCTION: 0x4b72e2 */
void bx_apply_rotation_matrix(const bx_i32 m[9], const bx_i32 *in, bx_i32 *out)
{
    out[0] = bx_add(bx_add(bx_mul13(in[0], m[0]), bx_mul13(in[1], m[3])), bx_mul13(in[2], m[6]));
    out[1] = bx_add(bx_add(bx_mul13(in[0], m[1]), bx_mul13(in[1], m[4])), bx_mul13(in[2], m[7]));
    out[2] = bx_add(bx_add(bx_mul13(in[0], m[2]), bx_mul13(in[1], m[5])), bx_mul13(in[2], m[8]));
}

/* FUNCTION: 0x4b7381 */
bx_i32 bx_FUN_004b7381(bx_i32 a, bx_i32 b, bx_i32 c)
{
    bx_i64 q;

    if (c == 0)
        return 0;
    q = (bx_i64)a * (bx_i64)b / (bx_i64)c;
    if (q > 2147483647 || q < -2147483647 - 1)
        return BX_INT_MIN;
    return (bx_i32)q;
}

typedef struct {
    double hi;
    double lo;
} bx_dd;

static bx_dd bx_dd_make(double hi, double lo)
{
    bx_dd r;
    r.hi = hi + lo;
    r.lo = lo - (r.hi - hi);
    return r;
}

static bx_dd bx_dd_neg(bx_dd a)
{
    a.hi = -a.hi;
    a.lo = -a.lo;
    return a;
}

static bx_dd bx_dd_add(bx_dd a, bx_dd b)
{
    double s, e, t, f;
    bx_dd r;

    bx_two_sum(a.hi, b.hi, &s, &e);
    bx_two_sum(a.lo, b.lo, &t, &f);
    e += t;
    r = bx_dd_make(s, e);
    r.lo += f;
    return bx_dd_make(r.hi, r.lo);
}

static bx_dd bx_dd_mul(bx_dd a, bx_dd b)
{
    double p, e;

    bx_two_prod(a.hi, b.hi, &p, &e);
    e += a.hi * b.lo + a.lo * b.hi;
    return bx_dd_make(p, e);
}

static bx_dd bx_dd_mul_d(bx_dd a, double b)
{
    double p, e;

    bx_two_prod(a.hi, b, &p, &e);
    e += a.lo * b;
    return bx_dd_make(p, e);
}

static bx_dd bx_dd_div(bx_dd a, bx_dd b)
{
    double q1, q2, q3;
    bx_dd r, q;

    q1 = a.hi / b.hi;
    r = bx_dd_add(a, bx_dd_neg(bx_dd_mul_d(b, q1)));
    q2 = r.hi / b.hi;
    r = bx_dd_add(r, bx_dd_neg(bx_dd_mul_d(b, q2)));
    q3 = r.hi / b.hi;
    q = bx_dd_make(q1, q2);
    return bx_dd_add(q, bx_dd_make(q3, 0.0));
}

static bx_dd bx_dd_from_double(double a)
{
    bx_dd r;
    r.hi = a;
    r.lo = 0.0;
    return r;
}

static bx_dd bx_dd_product(double a, double b)
{
    bx_dd r;
    bx_two_prod(a, b, &r.hi, &r.lo);
    return r;
}

static bx_dd bx_dd_atan_ratio(double n, double d)
{
    bx_dd num, den, u, w, p, atan_u, c;
    bx_dd table;
    double th, tl;
    int k, m;

    if (n == 0.0)
        return bx_dd_from_double(0.0);
    k = (int)(n * 16.0 / d + 0.5);
    num = bx_dd_add(bx_dd_from_double(16.0 * n), bx_dd_neg(bx_dd_product((double)k, d)));
    den = bx_dd_add(bx_dd_from_double(16.0 * d), bx_dd_product((double)k, n));
    u = bx_dd_div(num, den);
    w = bx_dd_mul(u, u);
    bx_dd_const(bx_atan_series + 4 * 11, &p.hi, &p.lo);
    for (m = 10; m >= 0; m--) {
        bx_dd_const(bx_atan_series + 4 * m, &c.hi, &c.lo);
        p = bx_dd_add(bx_dd_mul(p, w), c);
    }
    atan_u = bx_dd_mul(u, p);
    bx_dd_const(bx_atan_sixteenths + 4 * k, &th, &tl);
    table.hi = th;
    table.lo = tl;
    return bx_dd_add(table, atan_u);
}

static bx_dd bx_dd_atan2_nonneg(double s, double x)
{
    double ax = x < 0 ? -x : x, ch, cl;
    bx_dd a, c;

    if (s == 0.0 && ax == 0.0)
        return bx_dd_from_double(0.0);
    if (s <= ax) {
        a = bx_dd_atan_ratio(s, ax);
    } else {
        bx_dd_const(bx_pi_pi_half + 4, &ch, &cl);
        c.hi = ch;
        c.lo = cl;
        a = bx_dd_add(c, bx_dd_neg(bx_dd_atan_ratio(ax, s)));
    }
    if (x < 0) {
        bx_dd_const(bx_pi_pi_half, &ch, &cl);
        c.hi = ch;
        c.lo = cl;
        a = bx_dd_add(c, bx_dd_neg(a));
    }
    return a;
}

static bx_u64 bx_double_bits64(double d)
{
    bx_double_bits b;
    b.d = d;
    return ((bx_u64)b.w[1] << 32) | b.w[0];
}

static bx_u64 bx_round_shift(bx_u64 v, int shift)
{
    bx_u64 q, rem, half;

    if (shift <= 0)
        return v << (-shift);
    if (shift > 64)
        return 0;
    if (shift == 64) {
        q = 0;
        rem = v;
        half = (bx_u64)1 << 63;
    } else {
        q = v >> shift;
        rem = v & (((bx_u64)1 << shift) - 1);
        half = (bx_u64)1 << (shift - 1);
    }
    if (rem > half || (rem == half && (q & 1)))
        q++;
    return q;
}

static double bx_dd_to_x87_double(bx_dd v)
{
    bx_u64 hb = bx_double_bits64(v.hi), lb = bx_double_bits64(v.lo);
    bx_u64 mh, ml, n, r;
    int eh, el, neg, shift;

    if (v.hi == 0.0)
        return 0.0;
    eh = (int)(hb >> 52) & 0x7ff;
    mh = (hb & (((bx_u64)1 << 52) - 1)) | ((bx_u64)1 << 52);
    n = mh << 11;
    if (v.lo == 0.0)
        return bx_ext64_to_double(n, eh - 1075 - 11);
    neg = (int)(lb >> 63);
    el = (int)(lb >> 52) & 0x7ff;
    ml = (lb & (((bx_u64)1 << 52) - 1)) | ((bx_u64)1 << 52);
    shift = (eh - 1075 - 11) - (el - 1075);
    if (neg && mh == ((bx_u64)1 << 52)) {
        r = bx_round_shift(ml, shift + 1);
        if (r == 0)
            return bx_ext64_to_double(n, eh - 1075 - 11);
        return bx_ext64_to_double((bx_u64)0 - r, eh - 1075 - 12);
    }
    r = bx_round_shift(ml, shift);
    return bx_ext64_to_double(neg ? n - r : n + r, eh - 1075 - 11);
}

/* FUNCTION: 0x4e67f0 */
double bx_acos(double x)
{
    double s, ax = x < 0 ? -x : x;

    if (x != x)
        return x;
    if (ax > 1.0)
        return bx_sqrt(-1.0);
    s = bx_sqrt((1.0 + x) * (1.0 - x));
    return bx_dd_to_x87_double(bx_dd_atan2_nonneg(s, x));
}
