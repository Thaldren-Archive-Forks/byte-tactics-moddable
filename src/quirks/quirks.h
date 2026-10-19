#ifndef QUIRKS_QUIRKS_H
#define QUIRKS_QUIRKS_H

struct Quirks {
#define QUIRK(member, flagName, scope, openAnnihilationId) bool member;
#include "quirks.def"
#undef QUIRK
};

extern Quirks g_quirks;

bool QuirksParse(const char* spec, Quirks* out, char* err, int errLen);

bool QuirksNormalise(const char* spec, char* out, int outLen);

unsigned QuirksHash(const Quirks& q);

bool QuirksAllOff(const Quirks& q);

bool QuirksFlipFirstSim(Quirks* q);

#endif
