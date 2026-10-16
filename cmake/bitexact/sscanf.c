#include <stdarg.h>
#include <stdio.h>

#include "bitexact.h"

#define MAX_FORWARD 12

static int is_space(char c)
{
    return c == ' ' || (c >= 9 && c <= 13);
}

static int is_digit(char c)
{
    return c >= '0' && c <= '9';
}

static int spec_length(const char *f)
{
    const char *p = f + 1;

    if (*p == '*')
        p++;
    while (is_digit(*p))
        p++;
    while (*p == 'h' || *p == 'l' || *p == 'L')
        p++;
    if (*p == '[') {
        p++;
        if (*p == '^')
            p++;
        if (*p == ']')
            p++;
        while (*p && *p != ']')
            p++;
        return *p ? (int)(p + 1 - f) : 0;
    }
    return *p ? (int)(p + 1 - f) : 0;
}

static int is_plain_float(const char *f, int len)
{
    return len == 2 && f[1] == 'f';
}

static int scan_float_token(const char *cur, char *token, int size, int *at_end)
{
    int n = 0, digits = 0;

    if ((*cur == '+' || *cur == '-') && n < size - 1)
        token[n++] = *cur++;
    while (is_digit(*cur) && n < size - 1) {
        token[n++] = *cur++;
        digits++;
    }
    if (*cur == '.' && n < size - 1) {
        token[n++] = *cur++;
        while (is_digit(*cur) && n < size - 1) {
            token[n++] = *cur++;
            digits++;
        }
    }
    if (digits && (*cur == 'e' || *cur == 'E') && n < size - 1) {
        token[n++] = *cur++;
        if ((*cur == '+' || *cur == '-') && n < size - 1)
            token[n++] = *cur++;
        while (is_digit(*cur) && n < size - 1)
            token[n++] = *cur++;
    }
    token[n] = 0;
    *at_end = *cur == 0;
    return digits ? n : -1;
}

int __cdecl bx_sscanf(const char *buf, const char *fmt, ...)
{
    va_list ap;
    const char *f, *cur = buf;
    void *args[MAX_FORWARD];
    int own_float = 0, conversions = 0, assigned = 0, matched = 0, eof = 0, hit_end = 0, len, i;

    for (f = fmt; *f;) {
        if (*f != '%') {
            f++;
            continue;
        }
        if (f[1] == '%') {
            f += 2;
            continue;
        }
        len = spec_length(f);
        if (len == 0)
            break;
        if (is_plain_float(f, len))
            own_float = 1;
        if (f[1] != '*')
            conversions++;
        f += len;
    }
    va_start(ap, fmt);
    if (!own_float && conversions <= MAX_FORWARD) {
        for (i = 0; i < MAX_FORWARD; i++)
            args[i] = i < conversions ? va_arg(ap, void *) : 0;
        va_end(ap);
        return sscanf(buf, fmt, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8],
                      args[9], args[10], args[11]);
    }
    for (f = fmt; *f;) {
        if (is_space(*f)) {
            while (is_space(*f))
                f++;
            while (is_space(*cur))
                cur++;
            continue;
        }
        if (*f != '%' || f[1] == '%') {
            if (*f == '%')
                f++;
            if (*cur == 0) {
                eof = 1;
                break;
            }
            if (*cur != *f)
                break;
            cur++;
            f++;
            hit_end = 0;
            continue;
        }
        len = spec_length(f);
        if (len == 0)
            break;
        if (is_plain_float(f, len)) {
            char token[400];
            int used;
            float *dest = va_arg(ap, float *);
            while (is_space(*cur))
                cur++;
            if (*cur == 0) {
                eof = 1;
                break;
            }
            used = scan_float_token(cur, token, sizeof(token), &eof);
            if (used < 0)
                break;
            eof = 0;
            *dest = bx_strtof_msvc5(token, 0);
            cur += used;
            assigned++;
            matched = 1;
            hit_end = *cur == 0;
        } else {
            char sub[64];
            int consumed = -1, r;
            void *dest = f[1] == '*' ? 0 : va_arg(ap, void *);
            if (len > 60)
                break;
            if (dest && f[len - 1] == 'n') {
                if (hit_end) {
                    eof = 1;
                    break;
                }
                *(int *)dest = (int)(cur - buf);
                f += len;
                continue;
            }
            for (i = 0; i < len; i++)
                sub[i] = f[i];
            sub[len] = '%';
            sub[len + 1] = 'n';
            sub[len + 2] = 0;
            r = dest ? sscanf(cur, sub, dest, &consumed) : sscanf(cur, sub, &consumed);
            if (consumed < 0) {
                eof = r == EOF;
                break;
            }
            cur += consumed;
            matched = 1;
            hit_end = *cur == 0;
            if (dest)
                assigned++;
        }
        f += len;
    }
    va_end(ap);
    return assigned == 0 && eof && !matched ? EOF : assigned;
}
