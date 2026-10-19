#ifndef QUIRKS_QUIRKS_H
#define QUIRKS_QUIRKS_H

// The quirk flags: one bool per entry of quirks.def, all off meaning 3.1c.
struct Quirks {
#define QUIRK(member, name, scope, oa) bool member;
#include "quirks.def"
#undef QUIRK
};

// The flags in force for the match, resolved before the match loads.
extern Quirks g_quirks;

// Parses "<preset> [+flag|-flag ...]" into out; returns false and writes a message to err on an unknown preset or flag.
bool QuirksParse(const char* spec, Quirks* out, char* err, int errLen);

// Writes spec with its whitespace collapsed to single spaces; returns false if outLen is too small.
bool QuirksNormalise(const char* spec, char* out, int outLen);

// The FNV-1a 32-bit hash of the sim-scope flags, in quirks.def order, one byte per flag.
unsigned QuirksHash(const Quirks& q);

// True when every flag, sim or view, is off.
bool QuirksAllOff(const Quirks& q);

#endif
