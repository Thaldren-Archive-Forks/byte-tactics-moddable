#include "bitexact.h"
#include "bx_tables.h"

typedef struct {
    bx_u16 w[6];
} bx_ld12;

typedef struct {
    int max_exp;
    int min_exp;
    int precision;
    int exp_bits;
    int total_bits;
    int bias;
} bx_fp_format;

static const bx_fp_format bx_float_format = {128, -127, 24, 8, 32, 127};

static bx_u32 bx_get32(const bx_u16 *w, int i)
{
    return (bx_u32)w[i] | ((bx_u32)w[i + 1] << 16);
}

static void bx_set32(bx_u16 *w, int i, bx_u32 v)
{
    w[i] = (bx_u16)(v & 0xffffu);
    w[i + 1] = (bx_u16)(v >> 16);
}

static int bx_addl(bx_u32 x, bx_u32 y, bx_u32 *sum)
{
    *sum = x + y;
    return *sum < x || *sum < y;
}

static void bx_shl_12(bx_u32 *x)
{
    x[2] = (x[2] << 1) | (x[1] >> 31);
    x[1] = (x[1] << 1) | (x[0] >> 31);
    x[0] <<= 1;
}

static void bx_shr_12(bx_u32 *x)
{
    x[0] = (x[0] >> 1) | (x[1] << 31);
    x[1] = (x[1] >> 1) | (x[2] << 31);
    x[2] >>= 1;
}

static void bx_add_12(bx_u32 *x, const bx_u32 *y)
{
    if (bx_addl(x[0], y[0], &x[0])) {
        if (bx_addl(x[1], 1, &x[1]))
            x[2]++;
    }
    if (bx_addl(x[1], y[1], &x[1]))
        x[2]++;
    bx_addl(x[2], y[2], &x[2]);
}

static void bx_mtold12(const unsigned char *digits, int count, bx_ld12 *out)
{
    bx_u32 x[3], t[3], d[3];
    int exp = 0x404e, i;

    x[0] = x[1] = x[2] = 0;
    for (i = 0; i < count; i++) {
        t[0] = x[0];
        t[1] = x[1];
        t[2] = x[2];
        bx_shl_12(x);
        bx_shl_12(x);
        bx_add_12(x, t);
        bx_shl_12(x);
        d[0] = digits[i];
        d[1] = 0;
        d[2] = 0;
        bx_add_12(x, d);
    }
    while (x[2] == 0) {
        x[2] = x[1] >> 16;
        x[1] = (x[0] >> 16) | (x[1] << 16);
        x[0] <<= 16;
        exp -= 16;
    }
    while (!(x[2] & 0x8000u)) {
        bx_shl_12(x);
        exp--;
    }
    bx_set32(out->w, 0, x[0]);
    bx_set32(out->w, 2, x[1]);
    bx_set32(out->w, 4, x[2]);
    out->w[5] = (bx_u16)exp;
}

static void bx_ld12_infinity(bx_ld12 *x, int negative)
{
    bx_set32(x->w, 0, 0);
    bx_set32(x->w, 2, 0);
    bx_set32(x->w, 4, negative ? 0xffff8000u : 0x7fff8000u);
}

static void bx_ld12_zero(bx_ld12 *x)
{
    bx_set32(x->w, 0, 0);
    bx_set32(x->w, 2, 0);
    bx_set32(x->w, 4, 0);
}

static void bx_ld12mul(bx_ld12 *px, const bx_ld12 *py)
{
    bx_u16 acc[6];
    bx_u32 esum, t;
    int ex, ey, sign, i, j, bx, sticky = 0, n;
    short sbx;

    for (i = 0; i < 6; i++)
        acc[i] = 0;
    sign = (px->w[5] ^ py->w[5]) & 0x8000;
    ex = px->w[5] & 0x7fff;
    ey = py->w[5] & 0x7fff;
    esum = (bx_u32)(ex + ey);
    if (ex >= 0x7fff || ey >= 0x7fff || (bx_u16)esum > 0xbffdu) {
        bx_ld12_infinity(px, sign != 0);
        return;
    }
    if ((bx_u16)esum <= 0x3fbfu) {
        bx_ld12_zero(px);
        return;
    }
    if (ex == 0) {
        esum++;
        if ((bx_get32(px->w, 4) & 0x7fffffffu) == 0 && bx_get32(px->w, 2) == 0 && bx_get32(px->w, 0) == 0) {
            px->w[5] = 0;
            return;
        }
    }
    if (ey == 0) {
        esum++;
        if ((bx_get32(py->w, 4) & 0x7fffffffu) == 0 && bx_get32(py->w, 2) == 0 && bx_get32(py->w, 0) == 0) {
            bx_ld12_zero(px);
            return;
        }
    }
    for (i = 0; i < 5; i++) {
        for (j = 0; j < 5 - i; j++) {
            bx_u32 product = (bx_u32)px->w[i + j] * (bx_u32)py->w[4 - j];
            bx_u32 cur = bx_get32(acc, i);
            if (bx_addl(cur, product, &t))
                acc[i + 2]++;
            bx_set32(acc, i, t);
        }
    }
    bx = (int)((esum + 0xc002u) & 0xffffu);
    sbx = (short)bx;
    while (sbx > 0 && !(bx_get32(acc, 4) & 0x80000000u)) {
        bx_u32 s[3];
        s[0] = bx_get32(acc, 0);
        s[1] = bx_get32(acc, 2);
        s[2] = bx_get32(acc, 4);
        bx_shl_12(s);
        bx_set32(acc, 0, s[0]);
        bx_set32(acc, 2, s[1]);
        bx_set32(acc, 4, s[2]);
        sbx = (short)(sbx - 1);
    }
    if (sbx <= 0) {
        sbx = (short)(sbx - 1);
        if (sbx < 0) {
            n = -sbx;
            sbx = (short)(sbx + n);
            while (n-- > 0) {
                bx_u32 s[3];
                if (acc[0] & 1)
                    sticky++;
                s[0] = bx_get32(acc, 0);
                s[1] = bx_get32(acc, 2);
                s[2] = bx_get32(acc, 4);
                bx_shr_12(s);
                bx_set32(acc, 0, s[0]);
                bx_set32(acc, 2, s[1]);
                bx_set32(acc, 4, s[2]);
            }
        }
        if (sticky)
            acc[0] |= 1;
    }
    bx = (int)(bx_u16)sbx;
    if (acc[0] > 0x8000u || (bx_get32(acc, 0) & 0x1ffffu) == 0x18000u) {
        if (bx_get32(acc, 1) != 0xffffffffu) {
            bx_set32(acc, 1, bx_get32(acc, 1) + 1);
        } else {
            bx_set32(acc, 1, 0);
            if (bx_get32(acc, 3) != 0xffffffffu) {
                bx_set32(acc, 3, bx_get32(acc, 3) + 1);
            } else {
                bx_set32(acc, 3, 0);
                if (acc[5] != 0xffffu) {
                    acc[5]++;
                } else {
                    acc[5] = 0x8000u;
                    bx++;
                }
            }
        }
    }
    if ((bx_u16)bx >= 0x7fffu) {
        bx_ld12_infinity(px, sign != 0);
        return;
    }
    px->w[0] = acc[1];
    bx_set32(px->w, 1, bx_get32(acc, 2));
    bx_set32(px->w, 3, bx_get32(acc, 4));
    px->w[5] = (bx_u16)((bx_u16)bx | sign);
}

static void bx_multtenpow12(bx_ld12 *x, int power)
{
    const bx_u32 *table = power < 0 ? bx_pow10_neg : bx_pow10_pos;
    int group = 0, digit;
    bx_ld12 entry;
    bx_u32 v;
    int i;

    if (power == 0)
        return;
    if (power < 0)
        power = -power;
    x->w[0] = 0;
    while (power != 0) {
        digit = power & 7;
        power >>= 3;
        if (digit) {
            const bx_u32 *src = table + 3 * (group * 7 + digit - 1);
            for (i = 0; i < 3; i++)
                bx_set32(entry.w, 2 * i, src[i]);
            if (entry.w[0] >= 0x8000u) {
                v = bx_get32(entry.w, 1);
                bx_set32(entry.w, 1, v - 1);
            }
            bx_ld12mul(x, &entry);
        }
        group++;
    }
}

static int bx_zero_tail(const bx_u32 *man, int n)
{
    int idx = n >> 5, bit = n & 31, i;
    bx_u32 mask = ~((bx_u32)0xffffffffu << (31 - bit));

    if (man[idx] & mask)
        return 0;
    for (i = idx + 1; i < 3; i++)
        if (man[i])
            return 0;
    return 1;
}

static int bx_inc_man(bx_u32 *man, int n)
{
    int idx = n >> 5, bit = n & 31, carry;

    carry = bx_addl(man[idx], (bx_u32)1 << (31 - bit), &man[idx]);
    idx--;
    while (idx >= 0 && carry) {
        carry = bx_addl(man[idx], 1, &man[idx]);
        idx--;
    }
    return carry;
}

static int bx_round_man(bx_u32 *man, int precision)
{
    int idx = precision >> 5, bit = precision & 31, carry = 0, i;

    if (man[idx] & ((bx_u32)1 << (31 - bit))) {
        if (!bx_zero_tail(man, precision + 1))
            carry = bx_inc_man(man, precision);
    }
    man[idx] &= (bx_u32)0xffffffffu << (31 - bit);
    for (i = idx + 1; i < 3; i++)
        man[i] = 0;
    return carry;
}

static void bx_shr_man(bx_u32 *man, int n)
{
    int words = n >> 5, bits = n & 31, i;
    bx_u32 low_mask = ~((bx_u32)0xffffffffu << bits), carry = 0, w, low;

    for (i = 0; i < 3; i++) {
        w = man[i];
        low = w & low_mask;
        man[i] = (w >> bits) | carry;
        carry = bits ? low << (32 - bits) : 0;
    }
    for (i = 2; i >= 0; i--)
        man[i] = i >= words ? man[i - words] : 0;
}

static bx_u32 bx_ld12_to_float_bits(const bx_ld12 *x, const bx_fp_format *f)
{
    bx_u32 man[3], copy[3], sign, bits;
    int e, biased, i;

    sign = x->w[5] & 0x8000u ? 0x80000000u : 0;
    e = (int)(x->w[5] & 0x7fff) - 0x3fff;
    man[0] = bx_get32(x->w, 3);
    man[1] = bx_get32(x->w, 1);
    man[2] = (bx_u32)x->w[0] << 16;
    if (e == -0x3fff) {
        biased = 0;
        man[0] = man[1] = man[2] = 0;
    } else {
        for (i = 0; i < 3; i++)
            copy[i] = man[i];
        if (bx_round_man(man, f->precision))
            e++;
        if (e < f->min_exp - f->precision) {
            man[0] = man[1] = man[2] = 0;
            biased = 0;
        } else if (e <= f->min_exp) {
            for (i = 0; i < 3; i++)
                man[i] = copy[i];
            bx_shr_man(man, f->min_exp - e);
            bx_round_man(man, f->precision);
            bx_shr_man(man, f->exp_bits + 1);
            biased = 0;
        } else if (e >= f->max_exp) {
            man[0] = man[1] = man[2] = 0;
            man[0] = 0x80000000u;
            bx_shr_man(man, f->exp_bits);
            biased = f->bias + f->max_exp;
        } else {
            biased = f->bias + e;
            man[0] &= 0x7fffffffu;
            bx_shr_man(man, f->exp_bits);
        }
    }
    bits = (bx_u32)biased << (31 - f->exp_bits);
    return bits | sign | man[0];
}

static int bx_is_digit(char c)
{
    return c >= '0' && c <= '9';
}

static int bx_is_exp_letter(char c)
{
    return c == 'e' || c == 'E' || c == 'd' || c == 'D';
}

float bx_strtof_msvc5(const char *text, const char **end)
{
    const char *p = text;
    unsigned char digits[25];
    int count = 0, adjust = 0, negative = 0, seen = 0, exp_value = 0, exp_negative = 0;
    int power;
    bx_ld12 ld;
    bx_u32 bits;
    union {
        float f;
        bx_u32 u;
    } result;

    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
        p++;
    if (*p == '+' || *p == '-') {
        negative = *p == '-';
        p++;
    }
    if (*p == '.' && !bx_is_digit(p[1]))
        goto no_number;
    if (!bx_is_digit(*p) && *p != '.')
        goto no_number;
    seen = 1;
    while (*p == '0')
        p++;
    while (bx_is_digit(*p)) {
        if (count < 25)
            digits[count++] = (unsigned char)(*p - '0');
        else
            adjust++;
        p++;
    }
    if (*p == '.') {
        p++;
        if (count == 0)
            while (*p == '0') {
                adjust--;
                p++;
            }
        while (bx_is_digit(*p)) {
            if (count < 25) {
                digits[count++] = (unsigned char)(*p - '0');
                adjust--;
            }
            p++;
        }
    }
    if (bx_is_exp_letter(*p)) {
        const char *q = p + 1;
        if (*q == '+' || *q == '-') {
            exp_negative = *q == '-';
            q++;
        }
        if (bx_is_digit(*q)) {
            while (*q == '0')
                q++;
            while (bx_is_digit(*q)) {
                if (exp_value <= 0x1450) {
                    exp_value = exp_value * 10 + (*q - '0');
                    if (exp_value > 0x1450)
                        exp_value = 0x1451;
                }
                q++;
            }
            p = q;
        }
    }
    if (end)
        *end = p;
    if (count > 24) {
        if (digits[23] >= 5)
            digits[23]++;
        count = 24;
        adjust++;
    }
    while (count > 0 && digits[count - 1] == 0) {
        count--;
        adjust++;
    }
    if (count == 0) {
        bx_ld12_zero(&ld);
    } else {
        bx_mtold12(digits, count, &ld);
        power = (exp_negative ? -exp_value : exp_value) + adjust;
        if (power > 0x1450) {
            bx_ld12_infinity(&ld, 0);
        } else if (power < -0x1450) {
            bx_ld12_zero(&ld);
        } else {
            bx_multtenpow12(&ld, power);
        }
    }
    ld.w[5] = (bx_u16)(ld.w[5] | (negative ? 0x8000u : 0));
    bits = bx_ld12_to_float_bits(&ld, &bx_float_format);
    result.u = bits;
    return result.f;

no_number:
    if (end)
        *end = text;
    return 0.0f;
}
