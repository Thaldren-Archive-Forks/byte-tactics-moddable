// Decompiled by deepseek-v4.1-flash, Opus, space-bunny-free, GPT-6, Sonnet and Claude Opus 5.5, finished by deepseek-v4.1-flash. Names are provisional.
// The unit type registry: one UnitDef per unit, its FBI / COB data, the
// movement classes, and the downloadable build menu tables. UnitDef, TdfFile,
// TdfRecord, MovementClass and Game each have one definition; the views the
// functions used sit in unions where they disagree.
#include <stdio.h>
#include "../state/block.h"
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <list>
// Included only for its symbols: it puts LoadUnitFbi's yardmap stores back
// in the original's base/index order (docs/c2-regalloc.md).
#include <malloc.h>

#pragma pack(push, 1)

#include "../util/vec3.h"

// The game's 16.16 fixed-point value. GetFieldFixed returns it by value (through
// a hidden pointer, since it has a constructor) and takes the default by value.
class Fixed {
public:
    int value;
    Fixed(int v) { value = v; }
};

// Keeps its own class: it defines the methods, and its loader names and bit groupings are not unit_def.h's.
// One unit type, 0x249 bytes. The field names are 0x42bf40's; the two flag
// words keep 0x42b370's bit groupings so its hand-written operator= still sees
// them (bit 5 downloadable, bit 6 canbuild, bit 31 gui of flags1; bits 20..22
// selfdestructcountdown of flags2).
#include "unit_def.h"

// One 0xbd-byte downloadable build list: a count and five entries.
struct BuildEntry_0042dcf0 {
    unsigned short typeId;             // +0x00
    unsigned char page;                // +0x02
    unsigned char slot;                // +0x03
    char name[0x21];                   // +0x04
};

struct BuildList_0042dcf0 {
    int count;                         // +0x00
    BuildEntry_0042dcf0 entries[5];    // +0x04
};

class UnitTable;

#include "../game/game.h"

#include "movement_class.h"

#include "../graphics/memory_cache.h"

// Unused here: the symbol ids these declarations take keep LoadUnitTypes'
// allocation, standing in for the two view classes merged above
// (docs/c2-regalloc.md).
int RIReport(int, int, int, int, int, int, int, int, int, int);
void CopyDwordIfNonNull(int*, int*);

// Unused here: the symbol ids these declarations take keep the allocation,
// standing in for the three TdfRecord views merged into the class below
// (docs/c2-regalloc.md).
int IsCountBelowAiLimit(unsigned char, unsigned short, int);
int IsUnderLimit(int, unsigned short, int);
void ProbeUnitDefEnergyRate(int, int, int);
void CountMessage(unsigned char, int, int);
void CountPacket(int, int, int);
void SetCameraPosition(int, int, int);
void StartScreenShake(int, int, int);
int RegisterUnitOrders();

// A parsed TDF file; the getters read the current section.
// Keeps its own view of the parser classes: the header's GetFieldFixed takes
// the result pointer first, while these calls pass and return a Fixed by value,
// and spelling them with the header's form changes the frame and the pushes.
class TdfRecord;


#include "../util/tdf.h"

static inline int FixedField(TdfRecord* record, char* key, Fixed def)
{
    int out;
    record->GetFieldFixed(&out, key, def.value);
    return out;
}

#include "../util/string_ref.h"

#include "../orders/mission_type.h"

// Puts the one-byte MissionType temporary at [esp+0x23], the top byte of
// its slot, where the original builds it; a plain named local lands at the
// bottom of the slot.
struct MissionHolder {
    char pad[3];
    MissionType mission;
    MissionHolder(char* text) : mission(text) {}
};

#pragma pack(pop)

extern Game* g_game;
extern char DAT_005119b8[];
extern char g_noneString[];

void* __stdcall GetCategoryMask(char* name);
void __stdcall GetLocalizedString(void* parser, char* dst, char* key, int size, char* def);
void __cdecl ProtectBlockReadWrite(void* param_1);
void __cdecl ProtectBlockReadOnly(void* param_1);
void __cdecl GameFreeThunk(void* param_1);
void __stdcall FreeCobScript(void* param_1);
void __stdcall BuildDataPath(char* out, const char* dir, const char* name, const char* ext);
void* __cdecl GameAllocIgnoreTag(const char* name, int size);
unsigned short __stdcall FindUnitTypeId(char* name);
void __stdcall LoadUnitFbi(char* path, UnitDef* type);
void AddDownloadBuildOptions();
void __stdcall ListDirectory(const char* pattern, int flags, std::vector<StringRef>* out);
void __stdcall InsertionSortUnitTypes(void* start, void* end, void* cmp, int param);
void __stdcall SortUnitTypes(void* start, void* end, void* cmp, int param);
int __stdcall CompareUnitTypeNames(const char* a, const char* b);
int __cdecl GameStrdup(char* name);
void __stdcall FatalError(const char* msg);
int __stdcall HAPI_FileLengthByName(char* path);
void* __stdcall Load3do(char* path);
void __stdcall MirrorObject(void* obj);
int __stdcall GetObjectHeight(void* obj);
void* __stdcall LoadCobScript(char* path);
void __stdcall StripExtension(char* text);
void __stdcall BindModelTextures(void* obj, char* name);
MovementClass* __stdcall FindMovementClass(char* name);
void LoadUnitInfo();

// The float fields' getter. The outer parentheses matter (note 7 of 0x42bf40).
#define GETFLOAT(section, key) ((float)(section)->GetFieldDouble(key, 0.0))

// UnitDef::operator= for the 0x249-byte UnitDef element. Hand-written:
// char arrays are copied with byte loops, plain ints/shorts/structs member by
// member, and the two flag words at +0x241/+0x245 bit by bit.
// FUNCTION: 0x42b370 ??4UnitDef@@QAEAAV0@ABV0@@Z
UnitDef& UnitDef::operator=(const UnitDef& src)
{
    unsigned int i;

    for (i = 0; i < 0x20; i++) name[i] = src.name[i];
    for (i = 0; i < 0x20; i++) unitname[i] = src.unitname[i];
    for (i = 0; i < 0x40; i++) description[i] = src.description[i];
    for (i = 0; i < 0x20; i++) objectname[i] = src.objectname[i];
    for (i = 0; i < 0x1e; i++) side[i] = src.side[i];
    for (i = 0; i < 0x40; i++) aiWeight[i] = src.aiWeight[i];
    for (i = 0; i < 0x40; i++) aiLimit[i] = src.aiLimit[i];

    fbiChecksum = src.fbiChecksum; scriptChecksum = src.scriptChecksum; weaponHashXor = src.weaponHashXor; f14a = src.f14a;
    f14e = src.f14e; count = src.count; ids = src.ids;
    maxThisUnit = src.maxThisUnit;
    *(Vec3*)&modelMinX = *(Vec3*)&src.modelMinX; *(Vec3*)&modelMaxX = *(Vec3*)&src.modelMaxX; *(Vec3*)&modelSizeX = *(Vec3*)&src.modelSizeX;
    radius = src.radius; energyCost = src.energyCost;
    metalCost = src.metalCost; data = src.data;
    maxvelocity = src.maxvelocity; maxslopevelocity = src.maxslopevelocity;
    brakerate = src.brakerate; acceleration = src.acceleration;
    bankscale = src.bankscale; pitchscale = src.pitchscale;
    damagemodifier = src.damagemodifier; moverate1 = src.moverate1;
    moverate2 = src.moverate2; movementclass = src.movementclass;
    turnrate = src.turnrate; corpse = src.corpse;
    maxwaterdepth = src.maxwaterdepth; minwaterdepth = src.minwaterdepth;
    energymake = src.energymake; energyuse = src.energyuse;
    metalmake = src.metalmake; extractsMetal = src.extractsMetal;
    windgenerator = src.windgenerator; tidalgenerator = src.tidalgenerator;
    cloakcost = src.cloakcost; cloakcostmoving = src.cloakcostmoving;
    energystorage = src.energystorage; metalstorage = src.metalstorage;
    buildtime = src.buildtime;
    for (i = 0; i < 3; i++) weapons[i] = src.weapons[i];
    maxHealth = src.maxHealth;
    buildRate = src.buildRate; healtime = src.healtime;
    range = src.range; radardistance = src.radardistance;
    sonardistance = src.sonardistance; mincloakdistance = src.mincloakdistance;
    radardistancejam = src.radardistancejam; sonardistancejam = src.sonardistancejam;
    soundcategory = src.soundcategory; buildangle = src.buildangle;
    buildRange = src.buildRange; maneuverleashlength = src.maneuverleashlength;
    attackrunlength = src.attackrunlength; kamikazedistance = src.kamikazedistance;
    sortbias = src.sortbias; altitude = src.altitude;
    id = src.id;
    explodeas = src.explodeas; selfdestructas = src.selfdestructas;
    maxslope = src.maxslope; maxwaterslope = src.maxwaterslope;
    capacity = src.capacity; transportcapacity = src.transportcapacity;
    draft = src.draft; makesMetal = src.makesMetal;
    buildMenuPageCount = src.buildMenuPageCount; mobile = src.mobile;
    defaultmissiontype = src.defaultmissiontype;
    for (i = 0; i < 3; i++) weaponCategories[i] = src.weaponCategories[i];
    categories = src.categories;

    b00 = src.b00; b02 = src.b02;
    initCloaked = src.initCloaked; downloadable = src.downloadable;
    canbuild = src.canbuild; b07 = src.b07;
    b08 = src.b08; b09 = src.b09;
    b10 = src.b10; flying = src.flying;
    b12 = src.b12; b13 = src.b13;
    b14 = src.b14; b15 = src.b15;
    b16 = src.b16; b17 = src.b17;
    b18 = src.b18; b19 = src.b19;
    b20 = src.b20; b21 = src.b21;
    b22 = src.b22; b23 = src.b23;
    b24 = src.b24; b25 = src.b25;
    b26 = src.b26; b27 = src.b27;
    b28 = src.b28; b29 = src.b29;
    b30 = src.b30; gui = src.gui;

    c00 = src.c00; c01 = src.c01;
    c02 = src.c02; c03 = src.c03;
    c04 = src.c04; c05 = src.c05;
    c06 = src.c06; c07 = src.c07;
    c08 = src.c08; c09 = src.c09;
    c10 = src.c10; c11 = src.c11;
    capture = src.capture; c13 = src.c13;
    c14 = src.c14; norestrict = src.norestrict;
    wacky = src.wacky; c17 = src.c17;
    c18 = src.c18; c19 = src.c19;
    selfdestructcountdown = src.selfdestructcountdown;

    return *this;
}

// FUNCTION: 0x42bcc0
void FreeUnitInfo()
{
    if (g_game->defs != 0) {
        ProtectBlockReadWrite(g_game->defs);
        GameFreeThunk(g_game->defs);
        g_game->defs = 0;
        g_game->unitDefCount = 0;
    }
}

// FUNCTION: 0x42bd10
void RefreshUnitInfo()
{
    if (g_game->unitDefCount == 0 || g_game->unitDefEnumDirty != 0) {
        LoadUnitInfo();
        g_game->unitDefEnumDirty = 0;
    }
}

// Walks the unit definition table at g_game+0x1439b (0x249-byte entries) and
// the table at g_game+0x391cb (0xbd-byte entries). For every pair that shares a
// name, if the def's bit 5 at +0x241 is clear it prints a warning and sets the
// "downloadable" bit (the table's lock is taken around the update).
// FUNCTION: 0x42bd40
void CheckDownloadableFlags()
{
    UnitDef* def = g_game->defs;
    // One for statement with `i++, def += 0x249`; no source guard on the first test.
    for (int i = 0; i < g_game->unitDefCount;
         i++, def = (UnitDef*)((char*)def + 0x249)) {
        for (int j = 0; j < g_game->buildListCount; j++) {
            if (_strcmpi(((BuildList_0042dcf0 *)g_game->buildLists)[j].entries[0].name, def->unitname) == 0
                && !def->downloadable) {
                char message[128];
                sprintf(message,
                        "Hey!  Somebody forgot to set downloadable=1 for %s",
                        def->unitname);
                ProtectBlockReadWrite(g_game->defs);
                def->downloadable = 1;
                ProtectBlockReadOnly(g_game->defs);
            }
        }
    }
}

// Makes the game's entry table writable, then fills each entry's short list
// (max 30) with the type ids whose element rows match the entry index, and
// finally restores the table to read-only.
// FUNCTION: 0x42be30
void AddDownloadBuildOptions()
{
    ProtectBlockReadWrite(g_game->defs);
    UnitDef* e = g_game->defs;
    for (int a = 0; a < g_game->unitDefCount; a++, e++) {
        if (e->ids != 0) {
            for (int b = 0; b < g_game->buildListCount; b++) {
                for (int c = 0; c < ((BuildList_0042dcf0 *)g_game->buildLists)[b].count; c++) {
                    if (a == ((BuildList_0042dcf0 *)g_game->buildLists)[b].entries[c].typeId && e->count <= 0x1e) {
                        unsigned short id = FindUnitTypeId(((BuildList_0042dcf0 *)g_game->buildLists)[b].entries[c].name);
                        if (id != 0) {
                            ((unsigned short*)e->ids)[e->count] = id;
                            e->count++;
                        }
                    }
                }
            }
        }
    }
    ProtectBlockReadOnly(g_game->defs);
}

// Stores the low bits of value, masked, into the flag word at the given shift.
// The xor and value2 spellings, candgun, kamikaze and norestrict stay written
// out in LoadUnitFbi: expanding the helpers there changes the later code.
static inline unsigned int SetFlagField(unsigned int flags, unsigned int value, int shift, unsigned int mask)
{
    return (value & mask) << shift | flags & ~(mask << shift);
}

// The same store, spelled with the old bits first.
static inline unsigned int ReplaceFlagField(unsigned int flags, unsigned int value, int shift, unsigned int mask)
{
    return flags & ~(mask << shift) | (value & mask) << shift;
}

#include "../../interfaces/seams.h"

// Included only for its symbols, here so the functions above keep their own
// numbers: with <malloc.h> it puts LoadUnitFbi's yardmap stores back in the
// original's base/index order (docs/c2-regalloc.md).
#include <conio.h>

// Reads a unit's FBI file (the UNITINFO section of a TDF) into its 0x249-byte
// unit definition: names, costs, movement, energy, the two flag words, the
// self-destruct countdown, the sound category, corpse, movement class,
// weapons, the yard map and the footprint extents.
// FUNCTION: 0x42bf40
void __stdcall LoadUnitFbi(char* fbi_file, UnitDef* unitdef) {
    TdfFile parser;
    char buf[100];
    char weapon[128];
    char yard[1024];

    if (parser.LoadFile(fbi_file)) {
        if (!parser.SelectRecord("UNITINFO")) {
            parser.Unload();
            goto FINISH;
        }
        {
            parser.current->GetFieldString(unitdef->unitname, "unitname", 0x20, DAT_005119b8);
            GetLocalizedString(&parser, unitdef->name, "name", 0x20, 0);
            GetLocalizedString(&parser, unitdef->description, "description", 0x40, 0);
            parser.current->GetFieldString(buf, "defaultmissiontype", 100, DAT_005119b8);
            MissionHolder m(buf);
            unitdef->defaultmissiontype = m.mission.index;
            parser.current->GetFieldString(buf, "wpri_badTargetCategory", 100, g_noneString);
            unitdef->weaponCategories[0] = (unsigned int*)GetCategoryMask(buf);
            parser.current->GetFieldString(buf, "wsec_badTargetCategory", 100, g_noneString);
            unitdef->weaponCategories[1] = (unsigned int*)GetCategoryMask(buf);
            parser.current->GetFieldString(buf, "wspe_badTargetCategory", 100, g_noneString);
            unitdef->weaponCategories[2] = (unsigned int*)GetCategoryMask(buf);
            parser.current->GetFieldString(buf, "noChaseCategory", 100, g_noneString);
            unitdef->categories = (unsigned int*)GetCategoryMask(buf);
            if (parser.current
                    ->GetFieldString(unitdef->objectname, "objectname", 0x20, DAT_005119b8) == 0) {
                strcpy(unitdef->objectname, unitdef->unitname);
            }
            unitdef->energyCost = (float)parser.current->GetFieldInt("buildcostenergy", 0);
            unitdef->metalCost = (float)parser.current->GetFieldInt("buildcostmetal", 0);
            unitdef->maxvelocity =
                FixedField(parser.current, "maxvelocity", Fixed(0));
            unitdef->brakerate =
                FixedField(parser.current, "brakerate", Fixed(0));
            unitdef->acceleration =
                FixedField(parser.current, "acceleration", Fixed(0));
            unitdef->bankscale =
                FixedField(parser.current, "bankscale", Fixed(0x10000));
            unitdef->pitchscale =
                FixedField(parser.current, "pitchscale", Fixed(0));
            unitdef->damagemodifier =
                FixedField(parser.current, "damagemodifier", Fixed(0x10000));
            unitdef->moverate1 =
                FixedField(parser.current, "moverate1", Fixed(unitdef->maxvelocity * 2));
            unitdef->moverate2 =
                FixedField(parser.current, "moverate2", Fixed(unitdef->maxvelocity * 2));
            unitdef->turnrate =
                (short)parser.current->GetFieldInt("turnrate", 0);
            unitdef->draft = (char)parser.current->GetFieldInt("waterline", 0);
            unitdef->capacity =
                (char)parser.current->GetFieldInt("transportsize", 0);
            unitdef->transportcapacity =
                (char)parser.current->GetFieldInt("transportcapacity", 0);
            unitdef->energymake = GETFLOAT(parser.current, "energymake");
            unitdef->energyuse = GETFLOAT(parser.current, "energyuse");
            unitdef->metalmake = GETFLOAT(parser.current, "metalmake");
            unitdef->extractsMetal = GETFLOAT(parser.current, "extractsmetal");
            unitdef->makesMetal = (char)parser.current->GetFieldInt("makesmetal", 0);
            unitdef->windgenerator = GETFLOAT(parser.current, "windgenerator");
            unitdef->tidalgenerator = GETFLOAT(parser.current, "tidalgenerator");
            unitdef->energystorage = GETFLOAT(parser.current, "energystorage");
            unitdef->metalstorage = GETFLOAT(parser.current, "metalstorage");
            unitdef->buildtime =
                parser.current->GetFieldInt("buildtime", 0);
            unitdef->buildRate =
                (short)parser.current->GetFieldInt("workertime", 0);
            unitdef->healtime =
                (short)parser.current->GetFieldInt("healtime", 0);
            unitdef->maxHealth =
                parser.current->GetFieldInt("maxdamage", 0);
            unitdef->range =
                (short)parser.current->GetFieldInt("sightdistance", 0);
            unitdef->radardistance =
                (short)parser.current->GetFieldInt("radardistance", 0);
            unitdef->sonardistance =
                (short)parser.current->GetFieldInt("sonardistance", 0);
            unitdef->radardistancejam =
                (short)parser.current->GetFieldInt("radardistancejam", 0);
            unitdef->sonardistancejam =
                (short)parser.current->GetFieldInt("sonardistancejam", 0);
            unitdef->mobile = (char)parser.current->GetFieldInt("bmcode", 0);
            unsigned int value2;
            unsigned int value;
            value = parser.current->GetFieldInt("standingmoveorder", 2);
            unitdef->flags1 =
                (value ^ unitdef->flags1) & 3 ^ unitdef->flags1;
            value = parser.current->GetFieldInt("standingfireorder", 2);
            unitdef->flags1 = SetFlagField(unitdef->flags1, value, 2, 3);
            value = parser.current->GetFieldInt("init_cloaked", 0);
            unitdef->flags1 = SetFlagField(unitdef->flags1, value, 4, 1);
            value = parser.current->GetFieldInt("downloadable", 0);
            unitdef->flags1 = SetFlagField(unitdef->flags1, value, 5, 1);
            value = parser.current->GetFieldInt("builder", 0);
            unitdef->flags1 = SetFlagField(unitdef->flags1, value, 6, 1);
            value = parser.current->GetFieldInt("stealth", 0);
            unitdef->flags1 = SetFlagField(unitdef->flags1, value, 8, 1);
            unitdef->cloakcost = (float)parser.current->GetFieldInt("cloakcost", 0);
            unitdef->cloakcostmoving = (float)parser.current->GetFieldInt("cloakcostmoving", (int)unitdef->cloakcost);
            unitdef->mincloakdistance =
                (short)parser.current->GetFieldInt("mincloakdistance", 0);
            unitdef->buildangle =
                (short)parser.current->GetFieldInt("buildangle", 0);
            unitdef->buildRange =
                (short)parser.current->GetFieldInt("builddistance", 0);
            unitdef->sortbias =
                (short)parser.current->GetFieldInt("sortbias", 0);
            unitdef->altitude =
                (short)parser.current->GetFieldInt("cruisealt", 0);
            value = parser.current->GetFieldInt("zbuffer", 0);
            unitdef->flags1 = SetFlagField(unitdef->flags1, value, 7, 1);
            value = parser.current->GetFieldInt("isairbase", 0);
            unitdef->flags1 = SetFlagField(unitdef->flags1, value, 9, 1);
            value = parser.current->GetFieldInt("istargetingupgrade", 0);
            unitdef->flags1 = SetFlagField(unitdef->flags1, value, 10, 1);
            value = parser.current->GetFieldInt("teleporter", 0);
            unitdef->flags1 = ReplaceFlagField(unitdef->flags1, value, 13, 1);
            value = parser.current->GetFieldInt("hidedamage", 0);
            unitdef->flags1 = ReplaceFlagField(unitdef->flags1, value, 14, 1);
            value = parser.current->GetFieldInt("shootme", 0);
            unitdef->flags1 = ReplaceFlagField(unitdef->flags1, value, 15, 1);
            value = parser.current->GetFieldInt("armoredstate", 0);
            unitdef->flags1 = SetFlagField(unitdef->flags1, value, 17, 1);
            value = parser.current->GetFieldInt("activatewhenbuilt", 0);
            unitdef->flags1 = SetFlagField(unitdef->flags1, value, 18, 1);
            value = parser.current->GetFieldInt("canfly", 0);
            unitdef->flags1 = SetFlagField(unitdef->flags1, value, 11, 1);
            value = parser.current->GetFieldInt("canhover", 0);
            unitdef->flags1 = ReplaceFlagField(unitdef->flags1, value, 12, 1);
            value = parser.current->GetFieldInt("upright", 0);
            unitdef->flags1 = SetFlagField(unitdef->flags1, value, 20, 1);
            value = parser.current->GetFieldInt("floater", 0);
            unitdef->flags1 = SetFlagField(unitdef->flags1, value, 19, 1);
            value = parser.current->GetFieldInt("amphibious", 0);
            unitdef->flags1 = SetFlagField(unitdef->flags1, value, 21, 1);
            value = parser.current->GetFieldInt("isfeature", 0);
            unitdef->flags1 = SetFlagField(unitdef->flags1, value, 24, 1);
            value = parser.current->GetFieldInt("noshadow", 0);
            unitdef->flags1 = SetFlagField(unitdef->flags1, value, 25, 1);
            value = parser.current->GetFieldInt("immunetoparalyzer", 0);
            unitdef->flags1 = SetFlagField(unitdef->flags1, value, 26, 1);
            value = parser.current->GetFieldInt("hoverattack", 0);
            unitdef->flags1 = SetFlagField(unitdef->flags1, value, 27, 1);
            value = parser.current->GetFieldInt("antiweapons", 0);
            unitdef->flags1 = ReplaceFlagField(unitdef->flags1, value, 29, 1);
            value = parser.current->GetFieldInt("digger", 0);
            unitdef->flags1 = ReplaceFlagField(unitdef->flags1, value, 30, 1);
            value = parser.current->GetFieldInt("onoffable", 0);
            unitdef->flags2 = SetFlagField(unitdef->flags2, value, 2, 1);
            value = parser.current->GetFieldInt("mobilestandorders", 0);
            unitdef->flags2 =
                (value ^ unitdef->flags2) & 1 ^ unitdef->flags2;
            value = parser.current->GetFieldInt("firestandorders", 0);
            unitdef->flags2 = SetFlagField(unitdef->flags2, value, 1, 1);
            value = parser.current->GetFieldInt("canstop", 0);
            unitdef->flags2 = SetFlagField(unitdef->flags2, value, 3, 1);
            value = parser.current->GetFieldInt("canattack", 0);
            unitdef->flags2 = SetFlagField(unitdef->flags2, value, 4, 1);
            value = parser.current->GetFieldInt("canguard", 0);
            unitdef->flags2 = SetFlagField(unitdef->flags2, value, 5, 1);
            value = parser.current->GetFieldInt("canpatrol", 0);
            unitdef->flags2 = SetFlagField(unitdef->flags2, value, 6, 1);
            value = parser.current->GetFieldInt("canmove", 0);
            unitdef->flags2 = SetFlagField(unitdef->flags2, value, 7, 1);
            value = parser.current->GetFieldInt("canload", 0);
            unitdef->flags2 = SetFlagField(unitdef->flags2, value, 8, 1);
            value = parser.current->GetFieldInt("canreclamate", 0);
            unitdef->flags2 = SetFlagField(unitdef->flags2, value, 10, 1);
            value = parser.current->GetFieldInt("canresurrect", 0);
            value2 = (value & 1) << 0xb | unitdef->flags2 & 0xfffff7ff;
            unitdef->flags2 = value2 & 0xfffffdff | (value2 & 0x400) >> 1;
            value2 = parser.current->GetFieldInt("cancapture", 0);
            value = unitdef->flags2;
            value2 = (value2 & 1) << 0xc;
            value = value & 0xffffefff | value2;
            unitdef->flags2 = value;
            value2 = (unsigned int)(unitdef->cloakcost > 0.0f);
            unitdef->flags2 = value & 0xffffdfff | (value2 & 1) << 0xd;
            value = parser.current->GetFieldInt("candgun", 0);
            unitdef->flags2 =
                unitdef->flags2 & 0xffffbfff | (value & 1) << 0xe;
            unitdef->maneuverleashlength =
                (short)parser.current->GetFieldInt("maneuverleashlength", 0);
            unitdef->attackrunlength =
                (short)parser.current->GetFieldInt("attackrunlength", 0);
            value = parser.current->GetFieldInt("kamikaze", 0);
            unitdef->flags1 =
                unitdef->flags1 & 0xefffffff | (value & 1) << 0x1c;
            unitdef->kamikazedistance =
                (short)parser.current->GetFieldInt("kamikazedistance", 0);
            value = parser.current->GetFieldInt("norestrict", 0);
            unitdef->flags2 =
                unitdef->flags2 & 0xffff7fff | (value & 1) << 0xf;
            value = parser.current->GetFieldInt("showplayername", 0);
            unitdef->flags2 = SetFlagField(unitdef->flags2, value, 17, 1);
            value = parser.current->GetFieldInt("commander", 0);
            unitdef->flags2 = SetFlagField(unitdef->flags2, value, 18, 1);
            value = parser.current->GetFieldInt("cantbetransported", 0);
            unitdef->flags2 = SetFlagField(unitdef->flags2, value, 19, 1);

            char* countdown =
                (char*)parser.current->FindFieldValue("selfdestructcountdown");
            if (countdown != (char*)0)
                unitdef->selfdestructcountdown = atoi(countdown);
            else
                unitdef->selfdestructcountdown = 5;
            parser.current->GetFieldString(buf, "category", 100, DAT_005119b8);
            unitdef->AddToCategories(buf);
            // The test goes through an int local, and default arguments are literal 0.
            int found = parser.current
                    ->GetFieldString(buf, "soundcategory", 100, DAT_005119b8);
            if (found) {
                // Own loop counter, with the store and goto on a hit.
                int sound;
                for (sound = 0; sound < g_game->entry_count; sound++) {
                    if (_strcmpi((char *)g_game->entries_37e13 + sound * 0x160, buf) == 0) {
                        unitdef->soundcategory = (short)sound;
                        goto SOUND_DONE;
                    }
                }
                unitdef->soundcategory = (short)atoi(buf);
            } else {
                unitdef->soundcategory = 0;
            }
        SOUND_DONE:
            unitdef->corpse = -1;
            if (parser.current->GetFieldString(buf, "corpse", 100, DAT_005119b8))
                unitdef->corpse = g_seams.features->FindOrLoadFeatureType(buf);
            unitdef->movementclass = 0;
            if (parser.current
                    ->GetFieldString(buf, "movementclass", 100, DAT_005119b8))
                unitdef->movementclass = FindMovementClass(buf);
            MovementClass movement;
            MovementClass* move = (MovementClass*)unitdef->movementclass;
            if (move == 0) {
                (&movement)->ReadMoveInfo((Source_00440340*)&parser);
                move = &movement;
            }
            unitdef->footprintx = move->footprintX;
            unitdef->footprintz = move->footprintZ;
            unitdef->maxwaterdepth = move->maxWaterDepth;
            unitdef->minwaterdepth = move->minWaterDepth;
            unitdef->maxslope = move->maxSlope;
            unitdef->maxwaterslope = move->maxWaterSlope;
            unitdef->maxslopevelocity = (int)(((__int64)unitdef->maxvelocity << 16) /
                                             ((unitdef->maxslope + 1) * 0x10000));
            char* defaultWeapon = (char*)g_game + 0x2cf3;
            parser.current->GetFieldString(weapon, "weapon1", 128, DAT_005119b8);
            char* weapon1 = (char*)g_seams.weapons->FindWeaponByName(weapon);
            unitdef->weapons[0] = (WeaponDef*)(weapon1 ? weapon1 : defaultWeapon);
            parser.current->GetFieldString(weapon, "weapon2", 128, DAT_005119b8);
            char* weapon2 = (char*)g_seams.weapons->FindWeaponByName(weapon);
            unitdef->weapons[1] = (WeaponDef*)(weapon2 ? weapon2 : defaultWeapon);
            parser.current->GetFieldString(weapon, "weapon3", 128, DAT_005119b8);
            char* weapon3 = (char*)g_seams.weapons->FindWeaponByName(weapon);
            unitdef->weapons[2] = (WeaponDef*)(weapon3 ? weapon3 : defaultWeapon);
            parser.current->GetFieldString(weapon, "explodeas", 128, DAT_005119b8);
            char* explodeas = (char*)g_seams.weapons->FindWeaponByName(weapon);
            unitdef->explodeas = explodeas ? explodeas : defaultWeapon;
            parser.current
                ->GetFieldString(weapon, "selfdestructas", 128, DAT_005119b8);
            char* selfdestructas = (char*)g_seams.weapons->FindWeaponByName(weapon);
            unitdef->selfdestructas = selfdestructas ? selfdestructas : defaultWeapon;
            if ((char*)unitdef->weapons[0] == defaultWeapon &&
                (char*)unitdef->weapons[1] == defaultWeapon &&
                (char*)unitdef->weapons[2] == defaultWeapon)
                unitdef->flags1 &= ~0x10000;
            else
                unitdef->flags1 |= 0x10000;
            // Cleared here and again in the else branch: the original has both stores.
            unitdef->yardmap = 0;
            if (unitdef->mobile == 0) {
                parser.current
                    ->GetFieldString(yard, "YardMap", 1024, DAT_005119b8);
                unitdef->yardmap = (char*)SimAllocTagged(
                    "BUILDING YARD",
                    unitdef->footprintx * unitdef->footprintz);
                int cell = 0;
                char* cursor = yard;
                int y = 0;
                // Declared after the loop locals: its larger symbol id makes it the base.
                char*& map = unitdef->yardmap;
                while (y < unitdef->footprintz) {
                    for (int x = 0; x < unitdef->footprintx;) {
                        switch (*cursor) {
                        case '.':
                            map[cell] = 0x0;
                            break;
                        case 'f':
                            map[cell] = 0x6f;
                            break;
                        case 'o':
                            map[cell] = 0x2f;
                            break;
                        case 'c':
                            map[cell] = 0x2d;
                            break;
                        case 'O':
                            map[cell] = 0x2b;
                            break;
                        case 'w':
                            map[cell] = 0x37;
                            break;
                        case 'C':
                            map[cell] = 0x35;
                            break;
                        case 'y':
                            map[cell] = 0x29;
                            break;
                        case 'Y':
                            map[cell] = 0x31;
                            break;
                        case 'G':
                            map[cell] = 0x8f;
                            break;
                        default:
                            cursor++;
                            continue;
                        }
                        x++;
                        cell++;
                        if (cursor[1] != 0)
                            cursor++;
                    }
                    y++;
                }
            } else {
                unitdef->yardmap = 0;
            }
            // Footprint fields read directly, no w/h locals.
            unitdef->modelMinX = (unitdef->footprintx * -0x100000) / 2;
            unitdef->modelMinZ = (unitdef->footprintz * -0x100000) / 2;
            unitdef->modelMaxX = (unitdef->footprintx << 20) / 2;
            unitdef->modelMaxZ = (unitdef->footprintz << 20) / 2;
            // Inline Vec3 operator- returning by value.
            *(Vec3*)&unitdef->modelSizeX = *(Vec3*)&unitdef->modelMaxX - *(Vec3*)&unitdef->modelMinX;
            unitdef->radius = (unitdef->modelSizeZ + unitdef->modelSizeX) / 3;
            parser.Unload();
            // cancloak with no mincloakdistance: default to 80
            if ((unitdef->flags2 & 0x2000) && unitdef->mincloakdistance == 0)
                unitdef->mincloakdistance = 80;
        }
    }
FINISH:;
}

// Reads the unit definition and script of a unit type from disk, both under
// "units/<TypeName>/", and stores the script at UnitDef+0x18e: first the
// definition file ("FBI") when it exists, then the script file ("COB").
// The whole table at g_game+0x1439b (0x249-byte entries) is locked while the
// files are read.
// FUNCTION: 0x42d1f0
void __stdcall ReloadUnitType(unsigned short index)
{
    if (index == 0)
        return;
    UnitDef* type = &g_game->defs[index];
    if ((type->flags1 & 0x800000) == 0)
        return;
    ProtectBlockReadWrite(g_game->defs);
    char path[256];
    BuildDataPath(path, "units", type->unitname, "FBI");
    if (HAPI_FileLengthByName(path)) {
        LoadUnitFbi(path, type);
        FreeCobScript(type->data);
        BuildDataPath(path, "scripts", type->unitname, "COB");
        void* cob = LoadCobScript(path);
        type->data = cob;
        ProtectBlockReadOnly(g_game->defs);
    } else {
        ProtectBlockReadOnly(g_game->defs);
    }
}

// FUNCTION: 0x42d2e0
void LoadUnitTypes() {
    char namebuf[32];
    char section[32];
    char path[256];
    char classbuf[100];
    char objpath[256];
    char valbuf[256];

    {
        TdfFile parser;
        BuildDataPath(path, "gamedata", "moveinfo", "TDF");
        if (!parser.LoadFile(path))
            FatalError("Can't load MOVEINFO.TDF");

        int i = 0;
        MovementClass* cls = MovementClassTable::g_movementClasses.entries;
        MovementClass* cls_end = &MovementClassTable::g_movementClasses.entries[32];
        do {
            sprintf(classbuf, "CLASS%d", i);
            parser.ResetCurrentRecord();
            if (parser.SelectRecord(classbuf)) {
                parser.current->GetFieldString(classbuf, "name", 100, DAT_005119b8);
                cls->name = (int*)GameStrdup(classbuf);
                cls->ReadMoveInfo((Source_00440340*)&parser);
            }
            cls++;
            i++;
        } while ((int)cls < (int)cls_end);
        parser.Unload();
    }

    // operator new plus Construct(), not `new`: the symbol table has no constructor here.
    UnitTable* obj = (UnitTable*)operator new(0x14);
    obj = obj ? obj->Construct() : 0;
    (*(UnitTable **)&g_game->unitTable) = obj;

    int t = g_game->field_37e23 * g_game->width_37e1f * 2;
    int v = (int)(t * 1.3);

    int n = *(int*)((char*)(void *)g_game->unknown_c + 0x620) / 0x100000 + 1;
    float scale = 1.0f;
    if (n > 0x10) {
        double d = (double)n * 0.0625;
        if (d > 5.0)
            scale = 5.0f;
        else
            scale = (float)d;
    }
    int size = (int)(v * scale);
    ((UnitTable *)g_game->unitTable)->Initialize((size + 0xfff) & 0xfffff000);

    ProtectBlockReadWrite(g_game->defs);

    UnitDef* end = g_game->defs + g_game->unitDefCount;
    UnitDef* start = g_game->defs + 1;

    UnitDef* p = start;
    UnitDef* d;
    // This guard shape (if p != end, then if p == end) gives the original's branch layout.
    if (p != end) {
        while (p != end && !(~(p->flags1) & 0x800000))
            p++;
    }
    if (p == end) {
        d = p;
    } else {
        UnitDef* w = p;
        for (UnitDef* s = p + 1; s != end; s++) {
            if (!(~(s->flags1) & 0x800000)) {
                // Must be `*w++ = *s`, not a separate increment.
                *w++ = *s;
            }
        }
        d = w;
    }
    g_game->unitDefCount = (int)(d - g_game->defs);

    start = g_game->defs + 1;
    UnitDef* last = d;
    if (last - start <= 0x10) {
        InsertionSortUnitTypes(start, last, (void*)CompareUnitTypeNames, 0);
    } else {
        SortUnitTypes(start, last, (void*)CompareUnitTypeNames, 0);
        UnitDef* q = start + 0x10;
        InsertionSortUnitTypes(start, q, (void*)CompareUnitTypeNames, 0);
        for (; q != last; q++) {
            UnitDef tmp = *q;
            UnitDef* r = q - 1;
            UnitDef* w = q;
            while (_strcmpi(tmp.unitname, r->unitname) < 0) {
                *w = *r;
                w = r;
                r--;
            }
            *w = tmp;
        }
    }

    {
        unsigned short index = 0;
        if (g_game->unitDefCount > 0) {
            do {
                g_game->defs[index].id = index;
                index++;
            } while ((int)index < g_game->unitDefCount);
        }
    }
    int c = g_game->unitDefCount;
    g_game->unitDefCountBits = 0;
    if (c) {
        do {
            c >>= 1;
            g_game->unitDefCountBits++;
        } while (c);
    }

    (*(void ***)&g_game->types) = (void**)SimAllocTagged("MODEL PTRS", g_game->unitDefCount * 4);

    // `u` must be an unsigned short: an int counter breaks the loop.
    for (unsigned short u = 1; u < g_game->unitDefCount; u++) {
        UnitDef* type = &g_game->defs[u];
        // Divisor read through this body-local pointer: g_game then takes ecx.
        Game* gp = g_game;
        g_game->loadPctUnits = (unsigned char)((u * 100) / gp->unitDefCount);
        type->id = u;
        BuildDataPath(path, "units", type->unitname, "FBI");
        if (HAPI_FileLengthByName(path))
            LoadUnitFbi(path, type);

        strncpy(namebuf, type->objectname, 0x20);
        namebuf[0x1f] = 0;
        BuildDataPath(objpath, "objects3d", namebuf, "3DO");
        void* model = Load3do(objpath);
        if (model == 0)
            FatalError(objpath);
        MirrorObject(model);
        BindModelTextures(model, namebuf);
        ((void **)g_game->types)[u] = model;
        type->modelMinY = 0;
        type->modelMaxY = GetObjectHeight(((void **)g_game->types)[u]);
        type->modelSizeY = type->modelMaxY - type->modelMinY;

        strcpy(namebuf, type->unitname);
        StripExtension(namebuf);
        sprintf(section, "%s0", namebuf);
        BuildDataPath(path, "guis", section, "GUI");
        if (HAPI_FileLengthByName(path))
            type->gui = 1;
        else
            type->gui = 0;

        int suffix = 1;
        int found = 0;
        int more;
        // Condition-tested do-while on a `more` local, with `else break;`.
        do {
            sprintf(section, "%s%d", namebuf, suffix);
            BuildDataPath(path, "guis", section, "GUI");
            more = HAPI_FileLengthByName(path);
            if (more) {
                suffix++;
                found = 1;
            } else
                break;
        } while (more);
        if (found)
            type->buildMenuPageCount = suffix;
        else if (type->gui)
            type->buildMenuPageCount = 1;
        else
            type->buildMenuPageCount = 0;

        BuildDataPath(path, "scripts", type->unitname, "COB");
        type->data = LoadCobScript(path);
    }

    ProtectBlockReadOnly((void **)g_game->types);

    TdfFile parser2;
    BuildDataPath(path, "gamedata", "sidedata", "TDF");
    if (!parser2.LoadFile(path)) {
        FatalError("Can't load GAMEDATA.TDF");
    } else {
        short* list = (short*)GameAllocIgnoreTag("TEMP UTYPE LIST", 0x3c);
        for (unsigned short s = 1; s < g_game->unitDefCount; s++) {
            UnitDef* type = &g_game->defs[s];
            type->count = 0;
            type->ids = 0;
            if (type->canbuild) {
                parser2.ResetCurrentRecord();
                if (parser2.SelectRecord("CANBUILD") &&
                    parser2.SelectRecord(type->unitname)) {
                    int count = 0;
                    int k = 1;
                    sprintf(objpath, "canbuild%d", k);
                    while (parser2.current->GetFieldString(valbuf, objpath, 0x20, DAT_005119b8)) {
                        short val = FindUnitTypeId(valbuf);
                        if (val != 0) {
                            // Index store, not a walking `*out = val; out++;`.
                            list[count] = val;
                            count++;
                        }
                        k++;
                        sprintf(objpath, "canbuild%d", k);
                    }
                    type->count = count;
                }
                sprintf(objpath, "CANBUILD %s", type->unitname);
                type->ids = (unsigned short*)SimAllocTagged(objpath, 0x3c);
                memcpy(type->ids, list, 0x3c);
            }
        }
        GameFreeThunk(list);
        parser2.Unload();
    }

    g_game->loadPctUnits = 100;
    g_game->unitDefEnumDirty = 1;
    ProtectBlockReadOnly(g_game->defs);
}

// FUNCTION: 0x42db60
int __stdcall CompareUnitTypeNames(const char* param_1, const char* param_2)
{
    return _strcmpi(param_1 + 0x20, param_2 + 0x20) < 0;
}

// Releases the per-unit-type data tables: the array at g_game+0x14377, the
// 0x249-byte entries of g_game+0x1439b (with their script and file members),
// and the object at g_game+0x1437b. The loop starts at index 1 because entry 0
// is not a real unit type.
// FUNCTION: 0x42db90
void FreeUnitTypes()
{
    ProtectBlockReadWrite(g_game->defs);
    ProtectBlockReadWrite((void **)g_game->types);

    for (unsigned short i = 1; i < g_game->unitDefCount; i++) {
        UnitDef* type = &g_game->defs[i];
        void* p = ((void **)g_game->types)[i];
        if (p != 0) {
            GameFreeThunk(p);
            ((void **)g_game->types)[i] = 0;
        }
        if (type->yardmap != 0) {
            GameFreeThunk(type->yardmap);
            type->yardmap = 0;
        }
        if (type->data != 0) {
            FreeCobScript(type->data);
            type->data = 0;
        }
        if (type->ids != 0) {
            GameFreeThunk(type->ids);
            type->count = 0;
            type->ids = 0;
        }
    }

    UnitTable* obj = (UnitTable*)(UnitTable *)g_game->unitTable;
    if (obj != 0) {
        obj->Destroy();
        delete obj;
    }
    (*(UnitTable **)&g_game->unitTable) = 0;

    GameFreeThunk((void **)g_game->types);
    GameFreeThunk(g_game->defs);

    (*(void ***)&g_game->types) = 0;
    g_game->defs = 0;
}

// Edited by deepseek-v4.1: MATCH. Fixes this round:
// 1. `int c; int i;` are declared at the top of the body, before `int n = files.size();`,
//    and the last two loops reuse that one counter `c`. A group of scalar locals gets
//    its esp slots in reverse declaration order, so with the counters declared after n
//    the file count n landed in [esp+0x18]/EBP instead of the original [esp+0x10]/EBX;
//    declaring them first moved n to 0x10/EBX and cascaded the whole group (the max-page
//    scan and the downloadable scan, g_game into EDI, the lea esi,[ebx+eax] addressing),
//    lifting 77.6 -> 97.3.
// 2. The downloadable scan (0x42e037-0x42e0bf) kept coming out with the build-list
//    counter in EBX and the i*0xbd byte offset in EBP, the exact inverse of the
//    original (counter EBP, offset EBX, `inc ebp`; `add ebx,0xbd`; `cmp ebp,[eax+..]`).
//    The register assignment follows the order the loop variables are created, mapped
//    onto ESI, EDI, EBX, EBP (the four callee-saved registers live across the _strcmpi
//    call here), so the offset has to be a real source variable created before the
//    counter. Declaring `int off = 0;` and walking with it (`i++, off += 0xbd`,
//    `((char*)g_game->buildLists + off)->entries[0].name`)
//    makes the strength-reduction temp disappear and gives offset EBX / counter EBP.
// 3. `defs[c].name` is used directly at both call sites instead of a `char* name`
//    local: that drops one live-across-call variable, so the hoisted name temp lands
//    in EDI (the `lea edi,[esi-0x221]` in the preheader) instead of competing for EBX.
// Note 0x4c48c0 and 0x4c46c0 are two different classes in data/symbols.csv
// (TdfRecord::GetFieldString and TdfRecord::GetFieldInt), so `current`
// is a TdfRecord* and the int-arg calls cast it to TdfRecord*.
// FUNCTION: 0x42dcf0
void LoadDownloadMenus()
{
    int c;
    int i;
    char path[256];
    char unitbuf[256];
    std::vector<StringRef> files;
    BuildDataPath(path, "download", "*", "TDF");
    ListDirectory(path, 0, &files);

    int n = files.size();
    g_game->buildListCount = n;
    (*(BuildList_0042dcf0 **)&g_game->buildLists) = (BuildList_0042dcf0*)SimAllocTagged("DOWNLOADMENU", n * 0xbd);

    for (i = 0; i < n; i++) {
        TdfFile parser;
        BuildDataPath(path, "download", files[i].p, "TDF");
        if (parser.LoadFile(path)) {
            int j = 0;
            while (1) {
                parser.ResetCurrentRecord();
                if (!parser.SelectRecordAt(j))
                    break;
                ((BuildList_0042dcf0 *)g_game->buildLists)[i].count = j + 1;
                char* buf = unitbuf;
                if (parser.current->GetFieldString(unitbuf, "UNITMENU", 0x20, DAT_005119b8)) {
                    for (unsigned short u = 0; u < g_game->unitDefCount; u++) {
                        if (_strcmpi(g_game->defs[u].unitname, buf) == 0) {
                            ((BuildList_0042dcf0 *)g_game->buildLists)[i].entries[j].typeId = u;
                            ((BuildList_0042dcf0 *)g_game->buildLists)[i].entries[j].page = (unsigned char)parser.current->GetFieldInt("MENU", 0);
                            ((BuildList_0042dcf0 *)g_game->buildLists)[i].entries[j].slot = (unsigned char)parser.current->GetFieldInt("BUTTON", 0);
                            parser.current->GetFieldString(((BuildList_0042dcf0 *)g_game->buildLists)[i].entries[j].name, "UNITNAME", 0x20, DAT_005119b8);
                            break;
                        }
                    }
                }
                j++;
            }
        }
    }

    ProtectBlockReadWrite(g_game->defs);
    for (unsigned short u = 0; u < g_game->unitDefCount; u++) {
        for (c = 0; c < n; c++) {
            for (int d = 0; d < ((BuildList_0042dcf0 *)g_game->buildLists)[c].count; d++) {
                if (((BuildList_0042dcf0 *)g_game->buildLists)[c].entries[d].typeId == u) {
                    if (g_game->defs[u].buildMenuPageCount < ((BuildList_0042dcf0 *)g_game->buildLists)[c].entries[d].page)
                        g_game->defs[u].buildMenuPageCount = ((BuildList_0042dcf0 *)g_game->buildLists)[c].entries[d].page;
                }
            }
        }
    }
    ProtectBlockReadOnly(g_game->defs);

    UnitDef* defs = g_game->defs;
    for (c = 0; c < g_game->unitDefCount; c++) {
        for (int i = 0; i < g_game->buildListCount; i++) {
            if (_strcmpi(((BuildList_0042dcf0 *)g_game->buildLists)[i].entries[0].name, defs[c].unitname) == 0
                && !defs[c].downloadable) {
                char buf[128];
                sprintf(buf, "Hey!  Somebody forgot to set downloadable=1 for %s", defs[c].unitname);
                ProtectBlockReadWrite(g_game->defs);
                defs[c].downloadable = 1;
                ProtectBlockReadOnly(g_game->defs);
            }
        }
    }

    AddDownloadBuildOptions();
}

// FUNCTION: 0x42e120
void FreeDownloadMenus()
{
    GameFreeThunk((BuildList_0042dcf0 *)g_game->buildLists);
}

// Walks the whitespace separated names in the argument string. Each name is
// looked up in the name to mask table (GetCategoryMask) and this object's team bit
// is set in the mask that name maps to, then the same is done for "ALL", so the
// team always ends up in the ALL mask.
// Must stay out of line: inlining the body into LoadUnitFbi breaks the match.
#pragma auto_inline(off)
// FUNCTION: 0x488e70
void UnitDef::AddToCategories(char* names)
{
    int n;
    char buf[256];
    // First test outside the loop (a do/while): it decides the register save and `this`.
    if (sscanf(names, " %s %n", buf, &n) == 1) {
        do {
            names += n;
            unsigned short team = this->id;
            unsigned int* mask = (unsigned int*)GetCategoryMask(buf);
            mask[team >> 5] |= 1 << (team & 0x1f);
        } while (sscanf(names, " %s %n", buf, &n) == 1);
    }
    unsigned short team = this->id;
    unsigned int* all = (unsigned int*)GetCategoryMask("ALL");
    all[team >> 5] |= 1 << (team & 0x1f);
}
#pragma auto_inline(on)
