// Decompiled by GPT-5.6-Terra, finished by Sonnet 5.5. Names are provisional.
#include <list>
#include <map>
#include <vector>
#include <xmemory>

#pragma pack(push, 2)
struct Elem_0046faf0 {
    int a;
    int b;
    int c;
    short d;
};
#pragma pack(pop)

typedef std::list<int> List_0046ca60;
typedef List_0046ca60::iterator (List_0046ca60::*EraseFn_0046ca60)(List_0046ca60::iterator);
typedef List_0046ca60::iterator (List_0046ca60::iterator::*PostIncFn_0046ca60)(int);

EraseFn_0046ca60 g_erase_0046ca60 = &List_0046ca60::erase;
PostIncFn_0046ca60 g_postinc_0046ca60 = &List_0046ca60::iterator::operator++;

struct UnitSyncEntry {                 // the map mapped type, 0x10 bytes
    int x;
    int y;
    short w;
    short h;
    int limit;
};

class VecInt_0046ca60 {               // std::vector<int>
public:
    std::allocator<int> alloc;
    int* _First;
    int* _Last;
    int* _End;

    ~VecInt_0046ca60()
    {
        operator delete(_First);
        _First = 0, _Last = 0, _End = 0;
    }
};

class UnitSyncPlayer {                 // 0x5c bytes
public:
    int id;                            // +0x00
    VecInt_0046ca60 ids;               // +0x04
    VecInt_0046ca60 pairs;             // +0x14
    char unknown_24[0x18];             // +0x24
    VecInt_0046ca60 sentQueue;         // +0x3c
    VecInt_0046ca60 heldQueue;         // +0x4c
};

static inline void DestroyR0_0046ca60(UnitSyncPlayer* _F, UnitSyncPlayer* _L);
static inline void DestroyR1_0046ca60(UnitSyncPlayer* _F, UnitSyncPlayer* _L);
static inline void DestroyR2_0046ca60(UnitSyncPlayer* _F, UnitSyncPlayer* _L);
class VecElems_0046ca60 {              // std::vector<UnitSyncPlayer>
public:
    std::allocator<UnitSyncPlayer> alloc;
    UnitSyncPlayer* _First;
    UnitSyncPlayer* _Last;
    UnitSyncPlayer* _End;

    void _Destroy(UnitSyncPlayer* _F, UnitSyncPlayer* _L)
    {
        for (; _F != _L; ++_F)
            alloc.destroy(_F);
    }

    ~VecElems_0046ca60()
    {
        DestroyR0_0046ca60(_First, _Last);

        operator delete(_First);
        _First = 0, _Last = 0, _End = 0;
    }
};

static inline void DestroyR0_0046ca60(UnitSyncPlayer* _F, UnitSyncPlayer* _L)
{
    DestroyR1_0046ca60(_F, _L);
}

static inline void DestroyR1_0046ca60(UnitSyncPlayer* _F, UnitSyncPlayer* _L)
{
    DestroyR2_0046ca60(_F, _L);
}

static inline void DestroyR2_0046ca60(UnitSyncPlayer* _F, UnitSyncPlayer* _L)
{
    std::allocator<UnitSyncPlayer> alloc;
    for (UnitSyncPlayer* p = _F; p != _L; ++p)
        alloc.destroy(p);
}

class UnitSync {
public:
    // Real std::map: its erase keeps the symbols.csv name.
    std::map<unsigned int, UnitSyncEntry> rects;
    VecElems_0046ca60 elems;
    std::list<int> ids;
    int seqSent;
    int seqCur;
    int seqMax;
    std::vector<Elem_0046faf0> seqSentQueue;
    std::vector<Elem_0046faf0> seqHeldQueue;
    int direct;
    int pendingPlayerCount;
    int checksumProgress;
    int disabled;

    ~UnitSync() {}
    void ApplyToUnitTypes();
};

#include "../game/game.h"

extern Game* g_game;

// FUNCTION: 0x46ca60
// __fastcall: keeps the erase loop comparing the iterator slot directly.
void __fastcall FinishUnitSync()
{
    ((UnitSync *)g_game->sync)->ApplyToUnitTypes();
    delete (UnitSync *)g_game->sync;
    (*(UnitSync **)&g_game->sync) = 0;
}
