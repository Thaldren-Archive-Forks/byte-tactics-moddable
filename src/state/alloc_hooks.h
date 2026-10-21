#ifndef STATE_ALLOC_HOOKS_H
#define STATE_ALLOC_HOOKS_H

extern "C" void* _ReturnAddress(void);

struct AllocHooks {
    void(__cdecl* enter)(void* site, const char* tag);
    void(__cdecl* leave)();
    void(__cdecl* added)(void* p, unsigned int size);
    void(__cdecl* removed)(void* p);
};

extern const AllocHooks* g_allocHooks;

struct AllocSite {
    bool on;
    AllocSite(void* site, const char* tag)
    {
        on = g_allocHooks != 0;
        if (on)
            g_allocHooks->enter(site, tag);
    }
    ~AllocSite()
    {
        if (on)
            g_allocHooks->leave();
    }
};

#endif
