#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "quirks.h"

Quirks g_quirks;

namespace {

struct QuirkInfo {
    const char* name;
    bool Quirks::*member;
    bool sim;
};

const QuirkInfo s_quirks[] = {
#define QUIRK(member, name, scope, oa) { name, &Quirks::member, QUIRK_SCOPE_##scope },
#define QUIRK_SCOPE_sim true
#define QUIRK_SCOPE_view false
#include "quirks.def"
#undef QUIRK_SCOPE_sim
#undef QUIRK_SCOPE_view
#undef QUIRK
};

const int s_quirkCount = (int)(sizeof s_quirks / sizeof s_quirks[0]);

void SetAll(Quirks* q, bool on)
{
    for (int i = 0; i < s_quirkCount; i++)
        q->*s_quirks[i].member = on;
}

void Fail(char* err, int errLen, const char* what, const char* token)
{
    if (err != NULL && errLen > 0)
        _snprintf(err, errLen, "%s \"%s\"", what, token);
    if (err != NULL && errLen > 0)
        err[errLen - 1] = '\0';
}

const char* NextToken(const char* p, char* token, int tokenLen)
{
    while (*p != '\0' && isspace((unsigned char)*p))
        p++;
    int n = 0;
    while (*p != '\0' && !isspace((unsigned char)*p)) {
        if (n < tokenLen - 1)
            token[n++] = *p;
        p++;
    }
    token[n] = '\0';
    return p;
}

}

bool QuirksParse(const char* spec, Quirks* out, char* err, int errLen)
{
    Quirks q;
    char token[96];
    const char* p = NextToken(spec, token, sizeof token);
    if (token[0] == '\0') {
        Fail(err, errLen, "empty quirk spec, expected a preset", "");
        return false;
    }
    if (strcmp(token, "3.1c") == 0)
        SetAll(&q, false);
    else if (strcmp(token, "recommended") == 0)
        SetAll(&q, true);
    else {
        Fail(err, errLen, "unknown quirk preset", token);
        return false;
    }
    for (;;) {
        p = NextToken(p, token, sizeof token);
        if (token[0] == '\0')
            break;
        if (token[0] != '+' && token[0] != '-') {
            Fail(err, errLen, "quirk flag needs a + or - in front", token);
            return false;
        }
        int found = -1;
        for (int i = 0; i < s_quirkCount; i++) {
            if (strcmp(token + 1, s_quirks[i].name) == 0)
                found = i;
        }
        if (found < 0) {
            Fail(err, errLen, "unknown quirk flag", token + 1);
            return false;
        }
        q.*s_quirks[found].member = token[0] == '+';
    }
    *out = q;
    return true;
}

bool QuirksNormalise(const char* spec, char* out, int outLen)
{
    char token[96];
    int n = 0;
    const char* p = spec;
    for (;;) {
        p = NextToken(p, token, sizeof token);
        if (token[0] == '\0')
            break;
        int len = (int)strlen(token);
        if (n + (n > 0) + len + 1 > outLen)
            return false;
        if (n > 0)
            out[n++] = ' ';
        memcpy(out + n, token, len);
        n += len;
    }
    if (n + 1 > outLen)
        return false;
    out[n] = '\0';
    return true;
}

unsigned QuirksHash(const Quirks& q)
{
    unsigned hash = 2166136261u;
    for (int i = 0; i < s_quirkCount; i++) {
        if (!s_quirks[i].sim)
            continue;
        hash ^= q.*s_quirks[i].member ? 1u : 0u;
        hash *= 16777619u;
    }
    return hash;
}

bool QuirksAllOff(const Quirks& q)
{
    for (int i = 0; i < s_quirkCount; i++) {
        if (q.*s_quirks[i].member)
            return false;
    }
    return true;
}
