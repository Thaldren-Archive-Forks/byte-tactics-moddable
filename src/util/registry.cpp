// Decompiled by Opus, Haiku, Space Bunny Free, GPT-6.1-sol, mimo-v2.6-pro and Sonnet. Names are provisional.
// The game's values under Software\Cavedog Entertainment, with the rectangle
// tests and the thread, sleep and display helpers that share this unit.
#include <windows.h>
#include <string.h>

#include "../graphics/rect.h"

// Returns 1 when rectangle a lies entirely inside rectangle b.
// FUNCTION: 0x4b6750
int __stdcall RectInsideRect(Rect* a, Rect* b)
{
    if (a->left < b->left) {
        return 0;
    }
    if (a->left > b->right) {
        return 0;
    }
    if (a->right < b->left) {
        return 0;
    }
    if (a->right > b->right) {
        return 0;
    }
    if (a->top < b->top) {
        return 0;
    }
    if (a->top > b->bottom) {
        return 0;
    }
    if (a->bottom < b->top) {
        return 0;
    }
    if (a->bottom > b->bottom) {
        return 0;
    }
    return 1;
}

// Returns 1 when rectangles a and b overlap.
// FUNCTION: 0x4b67d0
int __stdcall RectsOverlap(Rect* a, Rect* b)
{
    if (a->left > b->right) {
        return 0;
    }
    if (a->right < b->left) {
        return 0;
    }
    if (a->top > b->bottom) {
        return 0;
    }
    return a->bottom >= b->top ? 1 : 0;
}

// FUNCTION: 0x4b6820
int __stdcall CompareByte1Equal(int a, int b)
{
    int cl = *(char*)(a + 1);
    int bl = *(char*)(b + 1);
    return cl == bl ? 1 : 0;
}

// FUNCTION: 0x4b6840
void __stdcall SetBytes1To3(unsigned char* buf, unsigned char b2, unsigned char b3, unsigned char b4)
{
    buf[1] = b2;
    buf[2] = b3;
    buf[3] = b4;
}

// FUNCTION: 0x4b6a80
int __stdcall GetWindowsUserName(char* out)
{
    DWORD size;
    char name[256];
    size = 256;
    if (GetUserNameA(name, &size) && name[0] != 0) {
        strcpy(out, name);
        return 1;
    }
    return 0;
}

// The loop test was an inlined helper with one return per outcome. Its
// multi-block body stops MSVC from rotating the loop (moving the test to the
// bottom) and from merging the two identical "next line" branches; written
// as a plain `while (n != lines)` the loop is rotated and the branches merged.
static inline int NotDone(int wanted, int current)
{
    if (wanted == current) {
        return 0;
    }
    return 1;
}

// Returns a pointer to the start of line `n` of `text`; lines end at '\n' or '\0'.
// FUNCTION: 0x4b6af0
char* __stdcall SkipTextLines(char* text, int n)
{
    int i = 0;
    int lines = 0;
    while (NotDone(n, lines)) {
        if (text[i] == 0) {
            lines++;
            i++;
        } else if (text[i] == '\n') {
            lines++;
            i++;
        } else {
            i++;
        }
    }
    return text + i;
}

extern void* g_display;

// FUNCTION: 0x4b6b60
void __stdcall SetMediaNotifyCallback(int callback)
{
    *(int*)((char*)g_display + 0x80) = callback;
}
