#include "../../interfaces/seams.h"
// Decompiled by Claude Opus 5.5. Names are provisional.
// The unit type loader: loads every Weapons\*.tdf into the weapon TDF table,
// reads the UNITINFO section of every units\*.fbi into the unit type table at
// g_game+0x1439b, then drops the units whose version or copyright does not
// check out and compacts the table.

#include <windows.h>
#include "../state/block.h"
#include <ddraw.h>
#include <dsound.h>
#include <dplay.h>
#include <stdio.h>
#include <vector>
#include <list>
#include <map>
#include <shlobj.h>
#include <imagehlp.h>
#include <string.h>
#include <math.h>
#include <io.h>
#include <process.h>
#include <mbstring.h>
#include <tchar.h>
#include <time.h>
#include <float.h>
#include "../util/tdf.h"
#include "unit_def.h"
#include "../game/game.h"

#include "../util/string_ref.h"

// One file name of ListDirectory's list (a reference-counted string handle).
struct Elem_00432be0 {
    char* data;                        // +0x0

    ~Elem_00432be0() { ((StringRef*)this)->ReleaseRef(); }
    // Used instead of `.data`: keeps the path temporary's order.
    operator char*() const { return data; }
};

typedef std::vector<Elem_00432be0> FileList;

void __stdcall ListDirectory(const char* pattern, int dirs, FileList* out);

// Unused here: the symbol ids these declarations take keep the allocation,
// standing in for the three TdfRecord views merged into the class below
// (docs/c2-regalloc.md).
void ProbeUnitDefEnergyRate(int, int, int);
void CountMessage(unsigned char, int, int);
void SetCameraPosition(int, int, int);
void StartScreenShake(int, int, int);
void AccumulateScreenShake(int, int, int);
int RegisterUnitOrders();

// The override file (units\NAME.OVR).
class HapiBank {
public:
    HapiBank* InitBank();
    void CloseBank();
    int OpenBank(char* path, char* type, int flag);
    int OpenAccount(char* name);
    int GetIntegerItem(char* name, int def);
};

// The override file object: 0x4b3620 builds it and 0x4b3630 frees it.
class OvrFile : public HapiBank {
public:
    void* table;
    OvrFile() { InitBank(); }
    ~OvrFile() { CloseBank(); }
};

// Unused here: the symbol ids these declarations take keep the allocation,
// standing in for the HapiBank views merged into the class above and for
// the placeholder class removed beside it (docs/c2-regalloc.md).
void SetMissionStatus(int, int, int);
void StartFeatureBurning(int, int, int);
void KillFeature(int, int, int);
void ReplaceFeatureWithDead(int, int, int);
void FillWaypointWorldPos(int, int, int);
void ResetAIPlayers(void);
void RegisterGroundOrders(void);
void EnableAICommands(void);
void RegisterAICommands(void);
void InitCommands(void);
void RegisterVtolOrders(void);
void StepAllGafSequences(void);
void ResetNetStats(void);
void ResetCameraState(void);
void CountPacket(int, int, int);
void SetCameraPosition(int, int, int);
void StartScreenShake(int, int, int);
void RegisterEmptyAtexitB(void);
bool __cdecl ReadGdperf(unsigned long, void*);

extern Game* g_game;
// File statics, not externs: with an extern count new[] reads its own copy.
static TdfFile* s_weaponTdfParsers;
static int s_weaponTdfLoadedCount;
static int s_weaponTdfAllocCount;

extern char DAT_005119b8[];

int FUN_0041d8a0();
int GetCdPathMismatch();
void __stdcall BuildDataPath(char* out, const char* dir, const char* name, const char* ext);
void* __cdecl GameAllocIgnoreTag(char* name, int size);
void __cdecl GameFreeThunk(void* p);
void __cdecl ProtectBlockReadOnly(void* p);
void __cdecl ProtectBlockReadWrite(void* p);
void* __stdcall HAPI_OpenFileRead(char* path);
int __stdcall HAPI_CloseFile(void* file);
int __stdcall HAPI_IsInArchive(void* file);
int __stdcall HAPI_readfromfile(void* file, void* buf, int size);
int __stdcall HAPI_FileLength(void* file);
unsigned int __stdcall ComputeChecksum(char* data, int len);
void __stdcall ShowErrorBox(const char* text, const char* caption);
char* __stdcall Translate(char* text);
void __stdcall GetLocalizedString(void* parser, char* dst, char* key, int size, char* def);

#define COPYRIGHT "Copyright 0000 Humongous Entertainment. All rights reserved."

// Loads every Weapons\*.tdf into the weapon TDF table.
// Must stay an inline helper (like the TDF getters): it sets the inline budget.
static inline void LoadWeaponTDFs()
{
    FileList files;
    ListDirectory("Weapons\\*.tdf", 0, &files);
    if (files.size() == 0)
        return;
    s_weaponTdfAllocCount = files.size();
    s_weaponTdfParsers = new (kSim) TdfFile[s_weaponTdfAllocCount];
    for (Elem_00432be0* it = files.begin(); it < files.end(); it++) {
        char path[256];
        TdfFile* tdf = &s_weaponTdfParsers[s_weaponTdfLoadedCount];
        BuildDataPath(path, "Weapons", it->data, "TDF");
        if (tdf->LoadFile(path)) {
            if (tdf->flags != 0 || FUN_0041d8a0() == 0)
                s_weaponTdfLoadedCount++;
        }
    }
}

// The id of the weapon named `name` in the weapon TDFs, 0 when there is none.
static inline int FindWeapon(char* name)
{
    if (name != 0 && *name != 0) {
        for (int k = 0; k < s_weaponTdfLoadedCount; k++) {
            TdfFile* tdf = &s_weaponTdfParsers[k];
            tdf->ResetCurrentRecord();
            if (tdf->SelectRecord(name))
                return *(int*)((char*)tdf->current + 0x25);
        }
        return 0;
    }
    return 0;
}

// FUNCTION: 0x42a8d0
int LoadUnitInfo()
{
    int bad = 0;
    char path[256];

    LoadWeaponTDFs();

    if (g_game->defs) {
        ProtectBlockReadWrite(g_game->defs);
        GameFreeThunk(g_game->defs);
        g_game->defs = 0;
    }

    BuildDataPath(path, "units", "*", "FBI");
    FileList files;
    ListDirectory(path, 0, &files);
    int count = files.size() + 1;
    g_game->unitDefCount = count;
    int size = count * sizeof(UnitDef);
    g_game->defs = (UnitDef*)SimAllocTagged("UNITINFO", size);
    memset(g_game->defs, 0, size);
    strcpy(g_game->defs->unitname, "None");
    g_game->defs->flags1 |= 0x800000;
    int offset = strstr(COPYRIGHT, "0000") - COPYRIGHT;

    for (unsigned short i = 1; i < count; i++) {
        UnitDef* u = &g_game->defs[i];
        u->id = i;
        BuildDataPath(path, "units", files[i - 1], "FBI");
        void* f = HAPI_OpenFileRead(path);
        if (f) {
            int len = HAPI_FileLength(f);
            char* buf = (char*)GameAllocIgnoreTag(path, len);
            HAPI_readfromfile(f, buf, len);
            u->fbiChecksum = ComputeChecksum(buf, len);
            OvrFile ovr;
            char ovrpath[256];
            BuildDataPath(ovrpath, "units", files[i - 1], "OVR");
            if (ovr.OpenBank(ovrpath, "TA Unit Override", 0)) {
                if (ovr.OpenAccount("Compatability")) {
                    char num[16];
                    sprintf(num, "%u", u->fbiChecksum);
                    u->fbiChecksum = ovr.GetIntegerItem(num, u->fbiChecksum);
                }
            }
            TdfFile parser;
            (&parser)->LoadBuffer(buf, len, 0, "<NO FILE>");
            if (!(&parser)->SelectRecord("UNITINFO")) {
                // Original bug: this exit leaves the FBI file open (no
                // HAPI_CloseFile), the weapon TDF table allocated and the unit
                // table locked (no ProtectBlockReadOnly).
                GameFreeThunk(buf);
                return 0;
            }
            GetLocalizedString(&parser, u->name, "name", 0x20, 0);
            parser.GetString(u->unitname, "unitname", 0x20, DAT_005119b8);
            parser.GetString(u->side, "side", 0x1e, DAT_005119b8);
            parser.GetString(u->aiWeight, "ai_weight", 0x40, DAT_005119b8);
            parser.GetString(u->aiLimit, "ai_limit", 0x40, DAT_005119b8);
            if (parser.GetString(u->objectname, "objectname", 0x20, DAT_005119b8) == 0)
                strcpy(u->objectname, u->unitname);
            u->energyCost = (float)parser.GetInt("buildcostenergy", 0);
            u->metalCost = (float)parser.GetInt("buildcostmetal", 0);
            u->norestrict = parser.GetInt("norestrict", 0);
            u->wacky = parser.GetInt("wacky", 0);
            u->weaponHashXor ^= FindWeapon(parser.GetValue("weapon1"));
            u->weaponHashXor ^= FindWeapon(parser.GetValue("weapon2"));
            u->weaponHashXor ^= FindWeapon(parser.GetValue("weapon3"));
            u->weaponHashXor ^= FindWeapon(parser.GetValue("explodeas"));
            u->weaponHashXor ^= FindWeapon(parser.GetValue("selfdestructas"));
            double version = parser.GetDouble("Version", 0.0);
            int major = (int)floor(version);
            int minor = (int)floor((version - major) * 10.0);
            if (major < g_game->version)
                u->flags1 |= 0x800000;
            else if (major == g_game->version && minor <= g_game->versionMinor)
                u->flags1 |= 0x800000;
            else
                u->flags1 &= ~0x800000;
            if ((HAPI_IsInArchive(f) == 0 && FUN_0041d8a0() != 0) || GetCdPathMismatch() != 0) {
                u->flags1 &= ~0x800000;
                bad = 1;
            }
            // 128 bytes, so it sorts before the 0x100 path buffers.
            char copyright[128];
            parser.GetString(copyright, "Copyright", 0x80, "Run to the Village!  Warn your brother!");
            memcpy(copyright + offset, "0000", 4);
            if (strcmp(copyright, COPYRIGHT) != 0) {
                u->flags1 &= ~0x800000;
                bad = 1;
            }
            u->maxThisUnit = -1;
            HAPI_CloseFile(f);
            GameFreeThunk(buf);
        }
    }

    delete[] s_weaponTdfParsers;
    s_weaponTdfParsers = 0;
    s_weaponTdfLoadedCount = 0;
    s_weaponTdfAllocCount = 0;

    // Drop the units marked incompatible, moving the last kept one into each
    // hole; `size` is the byte offset of the end of the kept units.
    // Keep `size` and decrement it; indexing unitDefs[count - 1] loses ebp.
    int oldcount = count;
    for (unsigned short j = count - 1; j > 0; j--) {
        UnitDef* u = &g_game->defs[j];
        if (!(u->flags1 & 0x800000)) {
            if (j != count - 1) {
                *u = *(UnitDef*)((char*)g_game->defs + size - sizeof(UnitDef));
                u->id = j;
            }
            count--;
            size -= sizeof(UnitDef);
        }
    }
    g_game->unitDefCount = count;
    if (oldcount != count && !bad)
        ShowErrorBox(Translate("Incompatible units found.  They will be ignored.  Please download the latest version of the game."), DAT_005119b8);
    ProtectBlockReadOnly(g_game->defs);
    return 1;
}
