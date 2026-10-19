// Decompiled by space-bunny-free, finished by deepseek-v4.1-flash, finished by space-bunny-free, finished by deepseek-v4.1-flash. finished by Sonnet 5.5, finished by deepseek-v4.1-flash, finished by Claude Opus 5.5. Names are provisional.
// FLAGS: /Gi
// std::vector<UnitSyncPlayer>::insert(iterator, size_type, const T&) from
// MSVC 5's <vector>, for the 0x5c-byte element (four vectors and a sub-struct,
// see unit_sync.cpp) of the vector that 0x46dad0 appends to with an
// inlined push_back. The element's copy constructor (0x470390), destructor
// (0x46ded0) and operator= (0x470040) are out-of-line calls.
#include <vector>

struct SyncPlayerRecord {
public:
    char unknown_0[0x5c];
    SyncPlayerRecord& operator=(const SyncPlayerRecord& rhs);
};

// Must declare its copy constructor and operator=: the original calls both.
class UnitSyncPlayer : public SyncPlayerRecord {
public:
    UnitSyncPlayer(const UnitSyncPlayer& other);
    ~UnitSyncPlayer();
    UnitSyncPlayer& operator=(const UnitSyncPlayer& rhs)
    {
        return (UnitSyncPlayer&)SyncPlayerRecord::operator=(rhs);
    }
};

typedef std::vector<UnitSyncPlayer> Vec_0046f7a0;
typedef void (Vec_0046f7a0::*InsertFn_0046f7a0)(
    Vec_0046f7a0::iterator, Vec_0046f7a0::size_type,
    const UnitSyncPlayer&);

// FUNCTION: 0x46f7a0 ?insert@?$vector@VUnitSyncPlayer@@V?$allocator@VUnitSyncPlayer@@@std@@@std@@QAEXPAVUnitSyncPlayer@@IABV3@@Z
InsertFn_0046f7a0 g_insert_0046f7a0 = &Vec_0046f7a0::insert;
