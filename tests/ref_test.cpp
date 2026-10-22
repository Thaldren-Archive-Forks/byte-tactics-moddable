#include <stdio.h>
#include <string.h>

#include "../interfaces/ref.h"
#include "../src/state/block.h"

void* GetAllocLock()
{
    static void* lock;
    if (lock == 0)
        lock = host_mutex_create();
    return lock;
}

void (*g_outOfMemoryHandler)() = 0;

static int g_failures;
static int g_testMode;
static int g_fatals;
static char g_fatalText[512];

int HarnessTestMode()
{
    return g_testMode;
}

extern "C" void HOST_CALL host_fatal(void*, const char*, const char* text)
{
    g_fatals++;
    strncpy(g_fatalText, text, sizeof(g_fatalText) - 1);
}

static void Check(bool ok, const char* what)
{
    if (!ok) {
        printf("FAIL: %s\n", what);
        g_failures++;
    }
}

struct Thing {
    int a;
    int b;
};

static_assert(sizeof(Ref<Thing>) == 4, "size");
static_assert(__is_trivial(Ref<Thing>) && __is_trivially_copyable(Ref<Thing>), "a Ref is plain data");

int main()
{
    Ref<Thing> zeroInit = {};
    Check(zeroInit.get() == 0, "empty braces give null");
    SimBlockInit();
    Thing* t = (Thing*)SimAlloc(sizeof(Thing));
    t->a = 7;
    t->b = 9;
    Thing* u = (Thing*)SimAlloc(sizeof(Thing));
    Thing outside;
    Thing* base = (Thing*)SimBlockBase();

    Ref<Thing> r = t;
    Check(r.offset == (unsigned)((char*)t - (char*)base), "a Ref holds the offset from the base");
    Check(r.offset != 0, "an object is never at offset 0");
    Check((Thing*)r == t && r.get() == t, "a Ref round-trips to its pointer");
    Check(r->a == 7 && (*r).b == 9, "-> and * reach the object");
    r->a = 11;
    Check(t->a == 11, "writing through -> writes the object");

    Ref<Thing> n;
    memset(&n, 0, sizeof(n));
    Check(n.get() == 0 && !n, "a zeroed Ref is null");
    Ref<Thing> n2 = 0;
    Ref<Thing> n3 = nullptr;
    Ref<Thing> n4 = (Thing*)0;
    Check(n2.offset == 0 && n3.offset == 0 && n4.offset == 0, "null converts to offset 0");
    Check(n == nullptr && nullptr == n && !(n != nullptr) && n == 0, "null compares equal to null");
    Check(r != nullptr && nullptr != r && r != 0, "an object is not null");
    Check(r, "a Ref to an object is true");

    Ref<Thing> r2 = t;
    Ref<Thing> r3 = u;
    Check(r == r2 && !(r != r2), "equal Refs compare equal");
    Check(r != r3 && !(r == r3), "different Refs compare unequal");
    Check(r == t && t == r && r3 != t && t != r3, "a Ref compares with a pointer");
    Check(r != n && n != r, "a Ref differs from null Ref");

    Thing* raw = r;
    Check(raw == t, "implicit conversion to a pointer");

    struct Holder {
        Ref<Thing> first;
        Ref<Thing> second;
        int tail;
    };
    Holder h = {};
    Check(h.first == nullptr && h.second == nullptr && h.tail == 0, "aggregate initialisation gives null");
    Holder h2 = { t, u, 3 };
    Check(h2.first == t && h2.second == u && sizeof(Holder) == 12, "aggregate initialisation from pointers");

    g_testMode = 0;
    Ref<Thing> quiet = &outside;
    Check(g_fatals == 0, "an outside pointer is stored quietly outside test mode");
    Check(quiet.offset == (unsigned)((char*)&outside - (char*)base), "an outside pointer wraps to its offset");
    Check(quiet.get() == &outside, "an outside pointer round-trips on this 32-bit target");

    g_testMode = 1;
    Ref<Thing> inside = t;
    Check(g_fatals == 0, "an inside pointer is fine in test mode");
    Ref<Thing> loud = &outside;
    Check(g_fatals == 1, "an outside pointer reaches host_fatal in test mode");
    Check(strstr(g_fatalText, "Thing") != 0 && strstr(g_fatalText, "pointer outside the state block") != 0, "the message names the type");
    Ref<Thing> nul = nullptr;
    Check(g_fatals == 1, "null is fine in test mode");
    Ref<Thing> below = (Thing*)((char*)base - 64);
    Check(g_fatals == 2, "a pointer below the block reaches host_fatal");
    Check(loud.offset == (unsigned)((char*)&outside - (char*)base), "the offset is stored after the report");
    (void)inside;
    (void)nul;
    (void)below;

    printf(g_failures ? "FAILED\n" : "ref_test: all passed\n");
    return g_failures != 0;
}
