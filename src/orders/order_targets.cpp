// Decompiled by Haiku, Sonnet, Opus, Space Bunny Free, deepseek-v4.1-flash, DeepSeek V4.1 Flash, Claude Opus 5.5 and GPT-6. Names are provisional.
// The order-target area classes: the OrderFx family (circles, rings
// and rectangles, with their hit tests, world-position writers, area loaders
// and constructors), the std::vector<Point_0044eec0> point-list builders and
// the class that writes a target to a bit stream, the AirManeuverOrder
// and PathOrder path-target classes, the PathGoal
// family with its AiSearchGoal path, the Pathfinder singleton, and the
// out-of-line std::vector<Point_0044eec0> members. The module's files
// gathered in address order; 0x44da00 and 0x44ec30 keep their own files
// (0x44da00's std::copy specialization changes the vector instantiations
// 0x44d0e0 and 0x44d560 share, and 0x44ec30 only matches compiled with /Gi,
// which moves 0x44e3c0's registers).
// Keep the header include: without it the operand order of 0x44d560's sum flips.
#include <memory.h>
#include "../state/block.h"
#include "../util/hapi_bank.h"
#include <stdlib.h>
#include <vector>
#include "../util/vec3.h"
#include <algorithm>
#include <math.h>

// The vtables the area classes store by hand; the base class's is 0x4fd2f8.
extern void* g_approachRadiusVtable[];
extern void* g_ringApproachVtable[];
extern void* g_pointMarkerVtable[];

extern void __cdecl operator delete(void* p);

#include "order_fx.h"

// Unused here: forward declarations of real functions; their symbol ids keep
// the allocation the merged OrderFx-family views moved (docs/c2-regalloc.md).
void RegisterUnitOrders();
void RegisterGroundOrders();
void EnableAICommands();
void RegisterAICommands();
void ResetAIPlayers();
void RegisterVtolOrders();
void StepAllGafSequences();
void ResetNetStats();
void InitCommands();
int UpdatePlacementGhostValidity();
void RefreshSelectionOrders();
void DispatchOrdersPanelPageFlags();
void ResetCameraState();
void FindLocalCommander();
void ClampCameraPosition();
void ClampCameraTarget();
void UpdateScreenShake();
void UpdateCameraFollow();
void BeginMouseScroll();
void EndMouseScroll();
void UpdateMouseScroll();
void UpdateEdgeScroll();
void CenterCameraOnRadarClick();
void CenterCameraOnStartPosition();
void RegisterDataArchives();
int GetCdPathMismatch();
void CreateGameObject();
void InitMissionStatus();
void SetUpEndMissionScreen();

// Unused here: forward declarations of real functions, one id each, where the
// merged ArcOrder view stood; they keep the allocation of the classes declared
// after it (docs/c2-regalloc.md).
void StartScreenFade();
void StepScreenFade();
void ScheduleFadeTick();
int IsFadeDone();
void StepPaletteFade();
void FillEndGameStatistics();
int AreStatBarsComplete();
void EnableEndMissionButtons();
void ShowEndMissionScreen();
void OpenCdCheckDialog();

typedef std::vector<Point16> Vec_0044d0e0;

#include "approach_areas.h"

typedef std::vector<Point16> Vec_0044d560;

unsigned short __stdcall GetHeadingBetween(void* from, void* to);

// Fixed-point trig helpers written in assembly.
int __cdecl FUN_004b70ef(short angle, int scale);
int __cdecl FUN_004b7123(short angle, int scale);

#pragma pack(push, 1)
#include "../units/unit_def.h"
#pragma pack(pop)

#pragma pack(push, 1)
// The second part's full view of the unit, with part 1's x and y at +0x76.
#include "../units/unit.h"
#include "../weapons/weapon_def.h"
#pragma pack(pop)

typedef std::vector<Point16> Vec_0044da00;

#pragma pack(push, 2)

class BitWriter {
public:
    int bit;                           // +0x0 current word index
    int index;                         // +0x4 bits used in the current word
    int capacity;                      // +0x8
    unsigned int* data;                // +0xc
    unsigned int buffer[0x100];        // +0x10
    void GrowBuffer();
    void WriteBits(int value, int bits);
};

// The path target: the flag word at +8, the linked order at +0x16 and the
// position at +0x26. One class in several files' views: the constructors and
// setters declared below are its own methods, so the views merge here
// (docs/c2-regalloc.md). The field names each view needs coexist in unions.
class BitReader;
struct Order;
#include "path_order_attach.h"

class __declspec(novtable) PathOrder : public OrderFx {
public:
    union {
        unsigned short flags;              // +0x8
        short field_8;                     // +0x8, the setters' word
        struct {
            unsigned short : 6;
            unsigned short headingFlag : 1;  // +0x8 bit 6, SetHeading
            unsigned short : 9;
        };
        struct {
            unsigned short : 4;
            unsigned short radiusFlag : 1;   // +0x8 bit 4, SetApproachRadius
            unsigned short : 11;
        };
    };
    short approachRadius;              // +0xa
    short altitude;                    // +0xc
    short heading;                     // +0xe
    short piece;                       // +0x10
    Unit* unit;
    PathOrderAttach ref;               // +0x16
    union {
        Vec3 pos;
        struct {
            int pos_x;                 // +0x26, the bit-stream loader's view
            int pos_y;                 // +0x2a
            int pos_z;                 // +0x2e
        };
    };
    int followOffset;                  // +0x32

    PathOrder(int owner, HapiBank* file, char* name);
    PathOrder(Unit* owner_, BitReader* reader);
    PathOrder(Order* order, Unit* unit);
    PathOrder(Order* source, int unit, short value);
    PathOrder(Order* source, const Vec3& p);
    PathOrder(Order* source, int unit, const Vec3& p);
    void SetAltitude(int param_1);
    void SetHeading(short v);
    void SetApproachRadius(short v);

    void SerializeToBits(BitWriter* stream);
    int GetType();
    int IsFxStyle();
    void* Destroy(unsigned char flag);
    int SerializeToSave(int unused, HapiBank* file, char* name);
    int KeepAfterComplete();
    int FillWorldPos(Vec3* out);
    int GetDesiredHeading(unsigned short* out);
    int IsComplete(Unit* arg);
};

#pragma pack(pop)

// The second part's own views and externs: the path targets, the
// Pathfinder, the bit reader and the game the loaders read.
class PathOrderAttach;

#include "path_order_attach.h"

class UnitRef {
public:
    void Unlink();
};

extern void* g_pathOrderVtable[];
extern void* g_airManeuverOrderVtable[];

#pragma pack(push, 2)
struct PathOrderRecord {
    int unknown_0;                     // +0x0
    int unknown_4;                     // +0x4
    short id1;                         // +0x8
    void* ref_vt;                      // +0xa
    void* ref_owner;                   // +0xe
    void* ref_next;                    // +0x12
    int ref_value;                     // +0x16
    short id2;                         // +0x1a
    short f1;                          // +0x1c
    short f2;                          // +0x1e
    short f3;                          // +0x20
    short f4;                          // +0x22
    short f5;                          // +0x24
    Vec3 pos;
    int i4;                            // +0x32
};

#pragma pack(pop)

// Reads bit fields from an array of dwords, lowest bits first.
// Bit reader, see src/network/net_stats.cpp.
class BitReader {
public:
    unsigned int* data;                // +0x00
    int index;                         // +0x04
    int bit;                           // +0x08
    int ReadBits(int bits);

    int ReadBit()
    {
        int r = (data[index] & (1 << bit)) != 0;
        if (++bit == 32) {
            bit = 0;
            index++;
        }
        return r;
    }
};

class Pathfinder;

#pragma pack(push, 1)
#include "../game/game.h"
#pragma pack(pop)

extern Game* g_game;

#include "../ai/pathfinder.h"

#include "../../interfaces/seams.h"

#pragma pack(push, 2)
#include "order.h"

#pragma pack(pop)

#pragma pack(push, 2)

#pragma pack(push, 2)
#include "air_maneuver_order.h"
#pragma pack(pop)

// Unused here: the symbol ids these declarations take keep the allocation
// after the AirManeuverOrder views were joined (docs/c2-regalloc.md).
int RIReport(int, int, int, int, int, int, int, int, int, int);
int DrawWrappedText(char*, char*, int, int, int, int, int);
void CmdMem(int);

// The same vector as 0x44ee90 (_Ucopy) and 0x44eec0 (_Ufill): 0x44d0e0,
// 0x44d560 and 0x44da00 call 0x44ee90 and then this with ecx set to it.
typedef std::vector<Point16> Vec_0044ee60;
typedef void (Vec_0044ee60::*DestroyFn_0044ee60)(Vec_0044ee60::iterator, Vec_0044ee60::iterator);

// _Destroy is protected: a derived class takes its address so it is emitted out of line.
struct PointVectorDestroyAccess : Vec_0044ee60 {
    static DestroyFn_0044ee60 fn;
};

typedef std::vector<Point16> Vec_0044ee70;
typedef Vec_0044ee70::size_type (Vec_0044ee70::*SizeFn_0044ee70)() const;

typedef std::vector<Point16> Vec_0044ee90;
typedef Vec_0044ee90::iterator (Vec_0044ee90::*UcopyFn_0044ee90)(
    Vec_0044ee90::const_iterator, Vec_0044ee90::const_iterator, Vec_0044ee90::iterator);

// _Ucopy is protected: a derived class takes its address so it is emitted out of line.
struct PointVectorUcopyAccess : Vec_0044ee90 {
    static UcopyFn_0044ee90 fn;
};

typedef std::vector<Point16> Vec_0044eec0;
typedef void (Vec_0044eec0::*UfillFn_0044eec0)(
    Vec_0044eec0::iterator, Vec_0044eec0::size_type, const Point16&);

// _Ufill is protected: a derived class takes its address so it is emitted out of line.
struct PointVectorUfillAccess : Vec_0044eec0 {
    static UfillFn_0044eec0 fn;
};

#include "order_target_types.h"

#include "../game/goal_owner.h"

// The object at +0x4 (see victory_490940.cpp); vtable 0x4fd2f8. Slot 8 is the
// "give me your position as a Vec3" call (an implementation is 0x44dc60); slot
// 8 of the base holds _purecall, slot 11 the stub KeepAfterComplete, which returns 0.
class Base_00490a10 {
public:
    virtual ~Base_00490a10();                           // slot 0
    virtual void SerializeSave();                       // slot 1
    virtual void GetType();                             // slot 2
    virtual void IsFxStyle();                           // slot 3
    virtual int ContainsUnit(Unit* owner);
    virtual int ContainsCell(int x, int y);             // slot 5
    virtual void FillGoalCells();                       // slot 6
    virtual void ApproxDist();                          // slot 7
    virtual int FillWorldPos(Vec3* out);
    virtual int TryGetDesiredHeading();                 // slot 9
    virtual void WriteBits();                           // slot 10
    virtual int KeepAfterComplete();                    // slot 11
};

#include "../ai/ai_search_goal.h"
#include "order_goals.h"

#include "unit_motion.h"

static inline UnitMotion* Motion(Unit* unit)
{
    return (UnitMotion*)unit->motion;
}

// Unused here: a forward declaration of a later function of this file; its
// symbol id keeps the allocation (docs/c2-regalloc.md).
void CreatePathfinder();

// Unused here: forward declarations of real functions; their symbol ids keep
// the allocation (docs/c2-regalloc.md).
void UpdateMenuSparks();
void OpenCloseCdPlayerDialog();
void OpenMainMenu();
void HandleFrontendDebugKey();
int ConnectToService();
void RunFrontendStateMachine();
void FreePictureCache();
void ClearPictureCache();
void OrLabelAttribs();
void EmptyPreFrontendInitHook();
int CodeChecksumFailed();
void EmptyPostSimStepHook();
void EmptyPostSimStepHook_B();
void EmptyPostSimStepHook_C();
int MainLoopContinueStub();
int MainLoopContinueStub_B();
int MainLoopContinueStub_C();
void EmptyMainLoopHook();
void EmptyMainLoopHook_B();
void EmptyMainLoopHook_C();
void ProtectUnitDefsReadOnly();
void ProtectUnitDefsReadWrite();
void CheckGpfVersion();
void LoadGameResources();
void FreeAnimFiles();
void LoadGameFonts();
void FreeGameFonts();
int LoadDefaultPalette();
void LoadTextureGafs();
void FreeTextureGafs();
void CheckDownloadableFlags();
void AddDownloadBuildOptions();
// Two more, for the declaration PatrolGoal::DeserializeNetUnitState no longer needs.
void StartScreenFade();
void StepScreenFade();
// Unused here: one id each, to keep the allocation the merged PathOrder views
// moved (docs/c2-regalloc.md).
extern int Pad6260_01;
extern int Pad6260_02;
extern int Pad6260_03;
extern int Pad6260_04;
extern int Pad6260_05;
extern int Pad6260_06;
extern int Pad6260_07;
extern int Pad6260_08;
extern int Pad6260_09;
extern int Pad6260_10;
extern int Pad6260_11;
extern int Pad6260_12;
extern int Pad6260_13;
extern int Pad6260_14;
extern int Pad6260_15;
extern int Pad6260_16;
extern int Pad6260_17;
extern int Pad6260_18;
extern int Pad6260_19;
extern int Pad6260_20;
extern int Pad6260_21;
extern int Pad6260_22;
extern int Pad6260_23;
extern int Pad6260_24;
extern int Pad6260_25;
extern int Pad6260_26;
extern int Pad6260_27;
extern int Pad6260_28;
extern int Pad6260_29;
extern int Pad6260_30;
extern int Pad6260_31;
extern int Pad6260_32;
extern int Pad6260_33;
extern int Pad6260_34;
extern int Pad6260_35;
extern int Pad6260_36;
extern int Pad6260_37;
extern int Pad6260_38;
extern int Pad6260_39;
extern int Pad6260_40;
extern int Pad6260_41;
extern int Pad6260_42;
extern int Pad6260_43;
extern int Pad6260_44;
extern int Pad6260_45;
extern int Pad6260_46;
extern int Pad6260_47;
extern int Pad6260_48;
extern int Pad6260_49;
extern int Pad6260_50;
extern int Pad6260_51;
extern int Pad6260_52;
extern int Pad6260_53;
extern int Pad6260_54;
extern int Pad6260_55;
extern int Pad6260_56;
extern int Pad6260_57;
extern int Pad6260_58;
extern int Pad6260_59;
extern int Pad6260_60;
extern int Pad6260_61;
extern int Pad6260_62;
extern int Pad6260_63;
extern int Pad6260_64;
extern int Pad6260_65;
extern int Pad6260_66;
extern int Pad6260_67;

void __cdecl FUN_004b7173(short angle, int* xy);

// FUNCTION: 0x44ce40
int OrderFx::GetType()
{
    return 1;
}

// FUNCTION: 0x44ce50
void* OrderFx::Destroy(unsigned char flag)
{
    SetVtable(g_orderFxVtable);
    if (flag & 1) {
        operator delete(this);
    }
    return this;
}

// FUNCTION: 0x44ce70 ??0OrderFx@@QAE@HHH@Z
OrderFx::OrderFx(int arg1, int arg2, int arg3)
{
    SetVtable(g_orderFxVtable);
}

// FUNCTION: 0x44ce80
int __stdcall SerializeSave(int, int, int)
{
    return 0;
}

// Slot 6 of the vtable at 0x4fd2f8: the default implementation ignores the
// object and empties the caller's list (an inlined vector::clear()).
// FUNCTION: 0x44ce90
void OrderFx::FillGoalCells(void* cells)
{
    ((std::vector<void*>*)cells)->clear();
}

// FUNCTION: 0x44cec0
int OrderFx::ApproxDist(int, int)
{
    return 0;
}

// The second part's callers (0x44ef90, 0x44f080, 0x44f1a0, 0x44f2a0) call
// this out of line in the original; in one file MSVC would inline it.
#pragma auto_inline(off)
// FUNCTION: 0x44ced0
void OrderFx::AddFlags(int param_1)
{
    int val = source;
    if (val != 0) {
        int* target = (int*)(val + 0x4e);
        *target |= param_1;
    }
}
#pragma auto_inline(on)

// FUNCTION: 0x44cef0
int OrderFx::KeepAfterComplete()
{
    return 0;
}

// FUNCTION: 0x44cf00
int OrderFx::ContainsUnit(Unit* unit)
{
    short esi = unit->cell.y;
    short eax = unit->cell.x;

    return ContainsCell(eax, esi);
}

// FUNCTION: 0x44cf20
int OrderFx::ContainsCell(int, int)
{
    return 0;
}

// FUNCTION: 0x44cf30
int OrderFx::IsFxStyle()
{
    return 1;
}

// FUNCTION: 0x44cf40
int __stdcall TryGetDesiredHeading(int)
{
    return 0;
}

// FUNCTION: 0x44cf50
void OrderFx::WriteBits(int)
{
}

// Constructor of an OrderFx subclass that stores a point converted
// from fixed-point world coordinates relative to the map origin, a radius
// and (radius / 16) squared. It stores the same vtable as 0x44d010,
// so this is probably a second constructor of that class.
// FUNCTION: 0x44cf60 ??0ApproachRadius@@QAE@PAUOrder@@HHH@Z
ApproachRadius::ApproachRadius(Order* source, int x, int y, int r)
    : OrderFx((int)source)
{
    SetVtable(g_approachRadiusVtable);
    Point16 org = source->unit->footprint;
    Point16 p;
    p.x = (x - (org.x << 19) + 0x80000) >> 20;
    p.y = (y - (org.y << 19) + 0x80000) >> 20;
    pos = *(Point16*)&p;
    radius = r;
    radiusSq = (r / 16) * (r / 16);
}

// FUNCTION: 0x44cfe0
int ApproachRadius::GetType()
{
    return 4;
}

// FUNCTION: 0x44cff0
void* ApproachRadius::Destroy(unsigned char flag)
{
    SetVtable(g_orderFxVtable);
    if (flag & 1) {
        operator delete(this);
    }
    return this;
}

// Constructor of an OrderFx subclass that reads a 16-byte header from
// a named entry of an open file and keeps its last three dwords.
// FUNCTION: 0x44d010 ??0ApproachRadius@@QAE@HPAVHapiBank@@PAD@Z
ApproachRadius::ApproachRadius(int owner, HapiBank* file, char* name)
    : OrderFx(owner)
{
    SetVtable(g_approachRadiusVtable);
    int bad = 0;
    if (owner == 0 || file == 0)
        bad = 1;
    if (name == 0)
        bad = 1;
    if (!bad) {
        file->OpenNamedBox(name);
        file->SeekBox(0);
        ApproachRadiusRecord hdr;
        if (file->ReadBox(&hdr, 16) == 16) {
            v.x = hdr.v.x;
            v.y = hdr.v.y;
            v.z = hdr.v.z;
        }
    }
}

// Saving counterpart of 0x44d010's constructor: writes a 16-byte
// header whose last three dwords are the stored values (the first dword is
// left uninitialised, as in the original).
// FUNCTION: 0x44d090
int ApproachRadius::Serialize(int unused, HapiBank* file, char* name)
{
    ApproachRadiusRecord hdr;
    hdr.v.x = v.x;
    hdr.v.y = v.y;
    hdr.v.z = v.z;
    file->OpenNamedBox(name);
    file->SeekBox(0);
    file->WriteBox(&hdr, 16);
    return 1;
}

// An inlined std::vector insert. The vector holds 4-byte points (two shorts,
// the element type of 0x44ee60/0x44ee90/0x44eec0/0x44eef0), and the method
// resets the list to a single point taken from this->+8, that is an inlined
// vector::assign(1, pos) or clear() plus push_back(pos):
//
//   list->clear();          // erase(begin(), end())
//   list->push_back(pos);   // inserts *(Point*)(this + 8) at the end
// FUNCTION: 0x44d0e0
void ApproachRadius::AppendGoalCell(void* cells)
{
    Vec_0044d0e0* list = (Vec_0044d0e0*)cells;
    list->clear();
    list->push_back(*(Point16*)&pos);
}

// Point version of 0x44d310: whether (px, py) lies within the circle around
// (x, y).
// FUNCTION: 0x44d290
int ApproachRadius::ContainsCell(int px, int py)
{
    int dy = py - pos.y;
    int dx = px - pos.x;
    return dx * dx + dy * dy <= radiusSq;
}

// Virtual method of the circle area class (compare 0x44d290 and 0x44d310,
// which use the same centre at +0x8): writes the centre, offset by a point
// of the owner, as 16.16 world coordinates.

static inline void ToWorld(int* out, Point16 a, Point16 b)
{
    out[0] = (a.x * 2 + b.x) << 19;
    out[2] = (a.y * 2 + b.y) << 19;
}

// FUNCTION: 0x44d2c0
int ApproachRadius::FillWorldPos(int* out)
{
    ToWorld(out, pos, ((Order*)source)->unit->footprint);
    return 1;
}

// Whether obj lies within the circle around (x, y).
// FUNCTION: 0x44d310
int ApproachRadius::ContainsUnit(Unit* obj)
{
    int dy = obj->cell.y - pos.y;
    int dx = obj->cell.x - pos.x;
    return dx * dx + dy * dy <= radiusSq;
}

// Approximate distance from (px, py) to the edge of the circle around
// (x, y), 0 when inside: 18 * major + 7 * minor axis distance.
// FUNCTION: 0x44d350
int ApproachRadius::ApproxDistExcess(int px, int py)
{
    int dx = abs(px - pos.x);
    int dy = abs(py - pos.y);
    int d;
    if (dx > dy)
        d = dy * 7 + dx * 18;
    else
        d = dx * 7 + dy * 18;
    if (d < radius)
        return 0;
    return d - radius;
}

// Constructor of an OrderFx subclass (vtable 0x4fd358, the same class
// as 0x44d470's file-loading constructor) that stores a point converted
// from fixed-point world coordinates relative to the map origin, two radii
// and each (radius / 16) squared. Two-radius version of 0x44cf60.
// FUNCTION: 0x44d3b0 ??0RingApproach@@QAE@PAUOrder@@HHHH@Z
RingApproach::RingApproach(Order* source, int x, int y, int r1, int r2)
    : OrderFx((int)source)
{
    SetVtable(g_ringApproachVtable);
    Point16 org = source->unit->footprint;
    Point16 p;
    p.x = (x - (org.x << 19) + 0x80000) >> 20;
    p.y = (y - (org.y << 19) + 0x80000) >> 20;
    pos = *(Point16*)&p;
    // Stays after the point conversion.
    inner = r2;
    outer = r1;
    outerSq = (r1 / 16) * (r1 / 16);
    innerSq = (r2 / 16) * (r2 / 16);
}

// FUNCTION: 0x44d440
int RingApproach::GetType()
{
    return 5;
}

// FUNCTION: 0x44d450
void* RingApproach::Destroy(unsigned char flag)
{
    SetVtable(g_orderFxVtable);
    if ((flag & 1) != 0) {
        operator delete(this);
    }
    return this;
}

// Constructor of an OrderFx subclass that reads a 24-byte header from
// a named entry of an open file and keeps its last five dwords.
// FUNCTION: 0x44d470 ??0RingApproach@@QAE@HPAVHapiBank@@PAD@Z
RingApproach::RingApproach(int owner, HapiBank* file, char* name)
    : OrderFx(owner)
{
    SetVtable(g_ringApproachVtable);
    int bad = 0;
    if (owner == 0 || file == 0)
        bad = 1;
    if (name == 0)
        bad = 1;
    if (!bad) {
        file->OpenNamedBox(name);
        file->SeekBox(0);
        RingApproachRecord hdr;
        if (file->ReadBox(&hdr, 24) == 24) {
            data.a = hdr.pos;
            data.b = hdr.inner;
            data.c = hdr.outer;
            data.d = hdr.innerSq;
            data.e = hdr.outerSq;
        }
    }
}

// Saving counterpart of 0x44d470's constructor: writes a 24-byte header
// whose last five dwords are the stored values. The first dword is left
// uninitialised, exactly as in 0x44d090 and 0x44d9a0.
// FUNCTION: 0x44d500
int RingApproach::Serialize(int unused, HapiBank* file, char* name)
{
    RingApproachRecord hdr;
    hdr.pos = data.a;
    hdr.inner = data.b;
    hdr.outer = data.c;
    hdr.innerSq = data.d;
    hdr.outerSq = data.e;
    file->OpenNamedBox(name);
    file->SeekBox(0);
    file->WriteBox(&hdr, 24);
    return 1;
}

// Clears a std::vector<Point_0044eec0> and then appends a copy of the point
// at +0x8, with its y moved by (radius2 + radius1) / 32.
// FUNCTION: 0x44d560
void RingApproach::AppendGoalCell(void* cells)
{
    Vec_0044d560* list = (Vec_0044d560*)cells;
    list->clear();
    Point16 p = *(Point16*)&pos;
    p.y = p.y + (outer + inner) / 32;
    list->push_back(p);
}

// Virtual method of the ring area class (compare 0x44d2c0, which writes the
// same centre): writes the centre as 16.16 world coordinates, then moves it
// back towards the owner by the mean of the two radii at +0xc and +0x10.
// The mean needs its own inline helper to load +0xc first.

static inline void ToWorld(Vec3* out, Point16 a, Point16 b)
{
    out->x = (a.x * 2 + b.x) << 19;
    out->z = (a.y * 2 + b.y) << 19;
}

// Inlined copy of DirectionFromAngle.
static inline Vec3 Direction(short angle, int scale)
{
    Vec3 v;
    v.x = -FUN_004b70ef(angle, scale);
    v.y = 0;
    // The casts stay: without them 0x44d720's radius loads come out in the other order.
    v.z = -FUN_004b7123((short)angle, (int)scale);
    return v;
}

static inline int MeanRadius(RingApproach* c)
{
    // The locals keep the loads of the inner radius first; a single sum commutes them.
    int r1 = c->inner;
    int r2 = c->outer;
    return (r1 + r2) / 2;
}

// FUNCTION: 0x44d720
int RingApproach::FillWorldPos(Vec3* out)
{
    Unit* inner = ((Order*)source)->unit;
    ToWorld(out, pos, inner->footprint);
    short angle = GetHeadingBetween(&inner->pos, out);
    Vec3 d = Direction(angle, MeanRadius(this) << 16);
    out->x -= d.x;
    out->y -= d.y;
    out->z -= d.z;
    return 1;
}

// FUNCTION: 0x44d7c0
int RingApproach::ContainsCell(int px, int py)
{
    int dy = py - y;
    int dx = px - x;
    int distSq = dy * dy + dx * dx;
    return distSq <= outerSq && distSq >= innerSq;
}

// Same test as 0x44d7c0, on a unit's position: 1 when the squared distance
// from the centre lies within [radius2Sq, radius1Sq].
// FUNCTION: 0x44d800
int RingApproach::ContainsUnit(Unit* unit)
{
    int dy = unit->cell.y - y;
    int dx = unit->cell.x - x;
    int distSq = dx * dx + dy * dy;
    return distSq <= outerSq && distSq >= innerSq;
}

// Approximate distance from (px, py) to the ring around (x, y) with the
// inner radius +0xc and the outer radius +0x10, 0 between the radii.
// Same 18 * major + 7 * minor metric as 0x44d350.
// FUNCTION: 0x44d840
int RingApproach::ApproxDistExcess(int px, int py)
{
    int dx = abs(px - x);
    int dy = abs(py - y);
    int d;
    if (dx > dy)
        d = dy * 7 + dx * 18;
    else
        d = dx * 7 + dy * 18;
    if (d > outer)
        return d - outer;
    if (d < inner)
        return inner - d;
    return 0;
}

// Second constructor of the OrderFx subclass with vtable 0x4fd388
// (compare 0x44d930): the rectangle comes from a position, made relative to
// the owner's view origin, and a size.
// FUNCTION: 0x44d8a0 ??0PointMarker@@QAE@PAUOrder@@UPoint16@@1@Z
PointMarker::PointMarker(Order* owner, Point16 pos, Point16 size)
    : OrderFx((int)owner)
{
    SetVtable(g_pointMarkerVtable);
    x1 = pos.x - owner->unit->footprint.x;
    y1 = pos.y - owner->unit->footprint.y;
    x2 = size.x + pos.x;
    y2 = size.y + pos.y;
}

// FUNCTION: 0x44d900
int PointMarker::GetType()
{
    return 6;
}

// FUNCTION: 0x44d910
void* PointMarker::Destroy(unsigned char should_delete)
{
    PointMarker* esi = this;
    esi->SetVtable(g_orderFxVtable);
    if (should_delete & 1) {
        operator delete(esi);
    }
    return esi;
}

// Constructor of an OrderFx subclass that reads a 20-byte header from
// a named entry of an open file and keeps its last four dwords.
// FUNCTION: 0x44d930 ??0PointMarker@@QAE@HPAVHapiBank@@PAD@Z
PointMarker::PointMarker(int owner, HapiBank* file, char* name)
    : OrderFx(owner)
{
    SetVtable(g_pointMarkerVtable);
    file->OpenNamedBox(name);
    file->SeekBox(0);
    PointMarkerRecord hdr;
    if (file->ReadBox(&hdr, 20) == 20) {
        x1 = hdr.x1;
        x2 = hdr.x2;
        y1 = hdr.y1;
        y2 = hdr.y2;
    }
}

// Saving counterpart of 0x44d930's constructor: writes a 20-byte
// header whose last four dwords are the stored values (the first dword is
// left uninitialised, as in the original), like 0x44d090.
// FUNCTION: 0x44d9a0
int PointMarker::Serialize(int unused, HapiBank* file, char* name)
{
    PointMarkerRecord hdr;
    hdr.x1 = x1;
    hdr.x2 = x2;
    hdr.y1 = y1;
    hdr.y2 = y2;
    file->OpenNamedBox(name);
    file->SeekBox(0);
    file->WriteBox(&hdr, 20);
    return 1;
}

static inline int MidX_44dc60(PointMarker* w)
{
    return (w->x1 + w->x2) / 2;
}

// FUNCTION: 0x44dc60
int PointMarker::FillWorldPos(int* out)
{
    short avg = (short)MidX_44dc60(this);
    short z = (short)y2;
    Point16 pos = ((Order*)source)->unit->footprint;
    out[0] = (pos.x + avg * 2) << 0x13;
    out[2] = (pos.y + z * 2) << 0x13;
    return 1;
}

// Same rectangle layout as 0x44dc60 (x1/x2 at +8/+0xc, y1/y2 at +0x10/+0x14):
// is the point (x, y) on the rectangle's border?
// FUNCTION: 0x44dcb0
int PointMarker::ContainsCell(int x, int y)
{
    if ((x == x1 || x == x2) && y >= y1 && y <= y2) {
        return 1;
    }
    if ((y == y1 || y == y2) && x >= x1 && x <= x2) {
        return 1;
    }
    return 0;
}

// Same rectangle layout as 0x44dc60/0x44dcb0 (x1/x2 at +8/+0xc, y1/y2 at
// +0x10/+0x14): approximate distance from (x, y) to the rectangle, 0 inside.
// Outside a corner it is 16 * major + 6 * minor axis distance (compare the
// 18/7 ring metric of 0x44d350/0x44d840); straight out from an edge it is
// 16 * distance; inside it is 16 * distance to the nearest edge.

// Stays a macro: as a function or an if chain the first argument gets another register.
#define Min_0044dd00(a, b) ((a) < (b) ? (a) : (b))

// FUNCTION: 0x44dd00
int PointMarker::ApproxDist(int x, int y)
{
    int dx;
    if (x < x1) {
        dx = x1 - x;
    }
    else if (x > x2) {
        dx = x - x2;
    }
    else {
        if (y < y1)
            return (y1 - y) * 16;
        if (y > y2)
            return (y - y2) * 16;
        int mx = Min_0044dd00(x - x1, x2 - x);
        int my = Min_0044dd00(y - y1, y2 - y);
        return Min_0044dd00(mx, my) * 16;
    }

    int dy;
    if (y < y1)
        dy = y1 - y;
    else if (y > y2)
        dy = y - y2;
    else
        return dx * 16;

    if (dx > dy)
        return dx * 16 + dy * 6;
    return dy * 16 + dx * 6;
}

// Slot 0 of vtable 0x4fd3e0: writes the flag word and, per flag bit, the
// short fields (or the three dwords) to a bit stream.
// FUNCTION: 0x44ddc0
void PathOrder::SerializeToBits(BitWriter* stream)
{
    stream->WriteBits(flags, 8);
    if ((flags & 1) != 0) {
        stream->WriteBits(piece, 0x10);
        stream->WriteBits((int)(unsigned short)(ref.owner == 0 ? 0 : (unsigned short)((short)ref.owner->id)), 0x10);
    }
    if ((flags & 0x10) != 0) {
        stream->WriteBits(approachRadius, 0x10);
    }
    if ((flags & 8) != 0) {
        stream->WriteBits(altitude, 0x10);
    }
    if ((flags & 0x40) != 0) {
        stream->WriteBits(heading, 0x10);
    }
    if ((flags & 0x20) != 0) {
        stream->WriteBits(pos.x, 0x20);
        stream->WriteBits(pos.y, 0x20);
        stream->WriteBits(pos.z, 0x20);
    }
}

// Loading counterpart of the virtual save method 0x44dfb0 (vtable
// g_pathOrderVtable, slot 1): reads a 0x36-byte record from a named entry of an
// open file and copies its fields into this object. The record it reads is
// built by 0x44dfb0 and by 0x44e330 / 0x44e250.
// FUNCTION: 0x44de80 ??0PathOrder@@QAE@HPAVHapiBank@@PAD@Z
PathOrder::PathOrder(int owner, HapiBank* file, char* name)
    : OrderFx(owner), ref(0, 0)
{
    SetVtable(g_pathOrderVtable);
    PathOrderRecord rec;
    ((PathOrderAttach*)&rec.ref_vt)->PathOrderAttach::PathOrderAttach(0, 0);
    file->OpenNamedBox(name);
    file->SeekBox(0);
    if (file->ReadBox(&rec, 0x36) == 0x36) {
        unit = g_seams.units->LoadUnit(rec.id1, file);
        ref.SetUnit((Unit*)g_seams.units->LoadUnit(rec.id2, file));
        field_8 = rec.f1;
        approachRadius = rec.f2;
        altitude = rec.f3;
        heading = rec.f4;
        piece = rec.f5;
        pos = *(Vec3*)&rec.pos;
        followOffset = rec.i4;
    }
    g_seams.units->UnitRefUnlink(((UnitRef*)&rec.ref_vt));
}

// FUNCTION: 0x44df70
int PathOrder::GetType()
{
    return 2;
}

// Scalar-deleting-destructor shape: unlink the embedded list node at +0x16
// (via UnitRef::Unlink, the same unlink method used elsewhere),
// restore this object's own vtable, conditionally operator delete, and
// return `this`.
// FUNCTION: 0x44df80
void* PathOrder::Destroy(unsigned char flag)
{
    g_seams.units->UnitRefUnlink(((UnitRef*)((char*)this + 0x16)));
    SetVtable(g_orderFxVtable);
    if (flag & 1) {
        operator delete(this);
    }
    return this;
}

// Virtual save method (vtable 0x4fd3b8, slot 1) for the class built by
// 0x44e330 / 0x44e250. Builds a 0x36-byte record on the stack, writing the
// referenced unit's id, the embedded link's owner id, five flag shorts and
// the stored position, then writes it to the file via
// HapiBank::WriteBox. The read counterpart is 0x44de80.
//
// The record's first 8 bytes are never assigned and are still written out:
// 0x36 bytes of stack, 8 of them uninitialised, go to the save file. The read
// counterpart (0x44de80) reads all 0x36 bytes but never looks at 0..7, so the
// bytes are only leaked, never used.
// FUNCTION: 0x44dfb0
int PathOrder::SerializeToSave(int unused, HapiBank* file, char* name)
{
    // The first 8 bytes of rec stay unassigned, as in the original.
    PathOrderRecord rec;
    PathOrderAttach* ref = (PathOrderAttach*)&rec.ref_vt;
    ref->PathOrderAttach::PathOrderAttach(0, 0);
    if (unit == 0)
        rec.id1 = 0;
    else
        rec.id1 = ((short)unit->id);
    if (this->ref.owner == 0)
        rec.id2 = 0;
    else
        rec.id2 = ((short)this->ref.owner->id);
    rec.f1 = flags;
    rec.f2 = approachRadius;
    rec.f3 = altitude;
    rec.f4 = heading;
    rec.f5 = piece;
    rec.pos = pos;
    rec.i4 = followOffset;
    file->OpenNamedBox(name);
    file->SeekBox(0);
    file->WriteBox(&rec, 0x36);
    g_seams.units->UnitRefUnlink(((UnitRef*)&rec.ref_vt));
    return 1;
}

// FUNCTION: 0x44e080 ??0PathOrder@@QAE@PAUUnit@@PAVBitReader@@@Z
PathOrder::PathOrder(Unit* owner_, BitReader* reader)
    : OrderFx(0), unit(owner_), ref(0, 0)
{
    SetVtable(g_pathOrderVtable);
    flags = reader->ReadBits(8);
    if (flags & 1) {
        piece = reader->ReadBits(0x10);
        unsigned short index = reader->ReadBits(0x10);
        ref.SetUnit(index == 0 ? 0 : (Unit*)&g_game->units[index]);
    }
    if (flags & 0x10)
        approachRadius = reader->ReadBits(0x10);
    else
        approachRadius = 0;
    if (flags & 8)
        altitude = reader->ReadBits(0x10);
    else
        altitude = 0;
    if (flags & 0x40)
        heading = reader->ReadBits(0x10);
    else
        heading = 0;
    if (flags & 0x20) {
        pos_x = reader->ReadBits(0x20);
        pos_y = reader->ReadBits(0x20);
        pos_z = reader->ReadBits(0x20);
    }
}

// Another constructor of the class built by 0x44e250 / 0x44e2d0 / 0x44e330
// (vtable g_pathOrderVtable): it copies the position from the order's unit and
// picks a type of 7 or 1 from the unit definition's flag bit 11.
// FUNCTION: 0x44e190 ??0PathOrder@@QAE@PAUOrder@@PAUUnit@@@Z
PathOrder::PathOrder(Order* order, Unit* unit)
    : OrderFx((int)order), ref(0, 0)
{
    SetVtable(g_pathOrderVtable);
    ref.SetUnit((Unit*)unit);
    approachRadius = 0;
    altitude = 0;
    this->unit = order->unit;
    pos = this->unit->pos;
    piece = -1;
    if ((unsigned char)(ref.owner->def->flags1 >> 11) & 1) {
        field_8 = 7;
        int t = this->unit->weapons[0].weapon->range;
        if (t != 0)
            followOffset = t << 16;
        else
            followOffset = 0x640000;
    } else {
        field_8 = 1;
    }
}

// A second constructor for the class built by 0x44e330 (same vtable
// g_pathOrderVtable): it takes its position from the source's unit instead.
// FUNCTION: 0x44e250 ??0PathOrder@@QAE@PAUOrder@@HF@Z
PathOrder::PathOrder(Order* source, int unit, short value)
    : OrderFx((int)source), piece(value), ref(0, 0)
{
    SetVtable(g_pathOrderVtable);
    approachRadius = 0;
    altitude = 0;
    field_8 = 5;
    this->unit = source->unit;
    pos = this->unit->pos;
    ref.SetUnit((Unit*)unit);
}

// Another constructor of the class built by 0x44e330 and 0x44e250 (vtable
// g_pathOrderVtable), with a type of 0x20 and the position passed in.
// FUNCTION: 0x44e2d0 ??0PathOrder@@QAE@PAUOrder@@ABUVec3@@@Z
PathOrder::PathOrder(Order* source, const Vec3& p)
    : OrderFx((int)source), ref(0, 0), pos(p)
{
    SetVtable(g_pathOrderVtable);
    approachRadius = 0;
    altitude = 0;
    piece = -1;
    field_8 = 0x20;
    unit = source->unit;
}

// FUNCTION: 0x44e330 ??0PathOrder@@QAE@PAUOrder@@HABUVec3@@@Z
PathOrder::PathOrder(Order* source, int unit, const Vec3& p)
    : OrderFx((int)source), ref(0, 0)
{
    SetVtable(g_pathOrderVtable);
    ref.SetUnit((Unit*)unit);
    pos = p;
    approachRadius = 0;
    altitude = 0;
    piece = -1;
    this->unit = source->unit;
    field_8 = 0xa3;
}

// FUNCTION: 0x44e3a0
int PathOrder::KeepAfterComplete() {
    if ((flags & 1) == 0 || ref.owner == 0) {
        return 0;
    }
    return 1;
}

// Virtual method (g_pathOrderVtable slot) of the OrderFx family: works out
// where the object should be. When the flags say it is active and not fully
// set, it aims at the reference unit's predicted position, optionally adding
// a direction offset from the fixed-point trig helpers; otherwise it just
// places itself at terrain level using the unit definition's height.
static inline Vec3 Direction_0044e3c0(short angle, int scale)
{
    Vec3 v;
    v.x = -FUN_004b70ef(angle, scale);
    v.y = 0;
    v.z = -FUN_004b7123(angle, scale);
    return v;
}

// FUNCTION: 0x44e3c0
int PathOrder::FillWorldPos(Vec3* out)
{
    unsigned short f = flags;
    if ((f & 1) && !(f & 0x80)) {
        if (ref.owner == 0 || ref.owner->spatialBucket == g_game->overflowBucket)
            return 0;
        // Through a local pointer: keeps the struct-assignment destination in edi.
        Vec3* p = &pos;
        *p = g_seams.units->GetPiecePosition(ref.owner, piece);
        if (flags & 2) {
            short angle = ref.owner->heading;
            if (flags & 0x40)
                angle += heading;
            Vec3 d = Direction_0044e3c0(angle, followOffset);
            p->x -= d.x;
            p->y -= d.y;
            p->z -= d.z;
        }
        pos.y += altitude << 16;
    } else {
        if ((f & 8) == 0) {
            UnitDef* def = unit->def;
            if (def->b22)
                pos.y = (def->altitude + g_game->seaLevel) << 16;
            else
                pos.y = (def->altitude + *(unsigned char*)(unit->spatialBucket + 1)) << 16;
        }
    }
    if (pos.y > 0x1ff0000)
        pos.y = 0x1ff0000;
    *out = pos;
    return 1;
}

// FUNCTION: 0x44e530
int PathOrder::GetDesiredHeading(unsigned short* out)
{
    unsigned short f = flags;
    if ((f & 0x84) && ref.owner != 0) {
        if (f & 2) {
            *out = GetHeadingBetween(&unit->pos, &ref.owner->pos);
            return 1;
        }
        if (f & 0x40) {
            *out = heading;
            return 1;
        }
        *out = ref.owner->heading;
        return 1;
    }
    if (f & 0x40) {
        *out = heading;
        return 1;
    }
    return 0;
}

// A virtual method of the big class whose vtable starts at 0x4fd2f8 (it shares
// that vtable with 0x44e530, 0x44e3a0 and friends). It fetches a position from
// itself through vtable slot +0x20, measures the horizontal (x/z) distance to
// `arg`, converts it from 16.16 to pixels, then tests range/flags.
// FUNCTION: 0x44e5b0
int PathOrder::IsComplete(Unit* arg)
{
    Vec3 at;
    FillWorldPos(&at);
    float dist = (float)_hypot((double)(arg->pos.x - at.x), (double)(arg->pos.z - at.z)) * 1.52587890625e-05f;
    unsigned short flags = this->flags;
    if (flags & 0x10) {
        return (double)approachRadius > dist;
    }
    else {
        if (dist > 0.5f)
            return 0;
        if ((flags & 1) && ref.owner == 0)
            return 0;
        if ((flags & 4) && (unsigned short)arg->heading != (unsigned short)ref.owner->heading)
            return 0;
        if ((flags & 8) && abs(arg->pos.y - at.y) > 0x10000)
            return 0;
        return 1;
    }
    return 0;
}

// FUNCTION: 0x44e6b0
int PathOrder::IsFxStyle()
{
    return 0;
}

#define max(a, b) (((a) > (b)) ? (a) : (b))

// FUNCTION: 0x44e6c0
void PathOrder::SetAltitude(int param_1)
{
    field_8 |= 8;
    altitude = param_1;
    if ((unsigned char)field_8 & 0x20) {
        pos.y = (max(g_seams.units->GetGroundHeight((Vec3*)&pos), g_game->seaLevel) + param_1) << 16;
        if (pos.y > 0x1ff0000)
            pos.y = 0x1ff0000;
    }
}

// Sets a flag bit in the word at +0x8 (a 1-bit unsigned short bitfield, which
// MSVC sets with `or byte ptr` straight to memory) and stores a short.
// FUNCTION: 0x44e720
void PathOrder::SetHeading(short v)
{
    headingFlag = 1;
    heading = v;
}

// Sets a flag bit in the word at +0x8 (a 1-bit unsigned short bitfield, which
// MSVC sets with `or byte ptr` straight to memory) and stores a short.
// FUNCTION: 0x44e730
void PathOrder::SetApproachRadius(short v)
{
    radiusFlag = 1;
    approachRadius = v;
}

// The constructor.
// FUNCTION: 0x44e740 ??0AirManeuverOrder@@QAE@PAUOrder@@ABUVec3@@1@Z
AirManeuverOrder::AirManeuverOrder(Order* source, const Vec3& a, const Vec3& b)
    : OrderFx((int)source)
{
    SetVtable(g_airManeuverOrderVtable);
    self = source->unit;
    flags = 0;
    saveOnly22 = 0;
    target = a;
    other = b;
}

// FUNCTION: 0x44e7a0
int AirManeuverOrder::GetType()
{
    return 3;
}

// FUNCTION: 0x44e7b0
void* AirManeuverOrder::Destroy(int param_1)
{
    SetVtable(g_orderFxVtable);
    if ((param_1 & 1) != 0) {
        operator delete(this);
    }
    return this;
}

// The load constructor: reads a 0x2a-byte
// header from a named entry of an open file, then looks up the unit it names.
// FUNCTION: 0x44e7d0 ??0AirManeuverOrder@@QAE@HPAVHapiBank@@PAD@Z
AirManeuverOrder::AirManeuverOrder(int owner, HapiBank* file, char* name)
{
    source = owner;
    SetVtable(g_airManeuverOrderVtable);
    file->OpenNamedBox(name);
    file->SeekBox(0);
    AirManeuverOrderRecord hdr;
    if (file->ReadBox(&hdr, 0x2a) == 0x2a) {
        self = g_seams.units->LoadUnit(hdr.id, file);
        flags = hdr.flag;
        target = hdr.target;
        other = hdr.other;
        saveOnly22 = hdr.value_24;
        heading = hdr.heading;
        value_26 = hdr.value_28;
    }
}

// Slot 1, the saving counterpart of the
// loading constructor 0x44e7d0: packs this object's fields into a 0x2a-byte
// header (whose first eight bytes are left as they are, exactly as 0x44d500
// leaves its magic dword alone) and appends it to the file. The reader takes
// the id as a dword but only its low word (0x487080 masks with 0xffff), so
// writing it as a word is harmless.
// FUNCTION: 0x44e880
int AirManeuverOrder::SerializeToSave(Order* order, HapiBank* file, char* name)
{
    AirManeuverOrderRecord hdr;
    // if/else, not a ternary: puts the store of 0 on the fallthrough path.
    if (!self) {
        hdr.unit_id = 0;
    } else {
        hdr.unit_id = self->id;
    }
    hdr.flag = flags;
    hdr.target = target;
    hdr.other = other;
    hdr.value_24 = saveOnly22;
    hdr.heading = heading;
    hdr.value_28 = value_26;
    file->OpenNamedBox(name);
    file->SeekBox(0);
    file->WriteBox(&hdr, 0x2a);
    return 1;
}

// Writes the flag word, the two
// points, and (when flag bit 0 is set) the word at +0x24 to a bit stream.
// FUNCTION: 0x44e930
void AirManeuverOrder::SerializeToBits(BitWriter* stream)
{
    stream->WriteBits(flags, 1);
    stream->WriteBits(target.x, 0x20);
    stream->WriteBits(target.y, 0x20);
    stream->WriteBits(target.z, 0x20);
    stream->WriteBits(other.x, 0x20);
    stream->WriteBits(other.y, 0x20);
    stream->WriteBits(other.z, 0x20);
    if ((flags & 1) != 0) {
        stream->WriteBits(heading, 0x10);
    }
}

// FUNCTION: 0x44e9c0 ??0AirManeuverOrder@@QAE@PAUUnit@@PAVBitReader@@@Z
AirManeuverOrder::AirManeuverOrder(Unit* owner, BitReader* reader)
{
    source = 0;
    self = owner;
    SetVtable(g_airManeuverOrderVtable);
    flags = reader->ReadBits(1);
    target.x = reader->ReadBits(0x20);
    target.y = reader->ReadBits(0x20);
    target.z = reader->ReadBits(0x20);
    other.x = reader->ReadBits(0x20);
    other.y = reader->ReadBits(0x20);
    other.z = reader->ReadBits(0x20);
    if (flags & 1) {
        heading = reader->ReadBits(0x10);
    }
}

// FUNCTION: 0x44ea50
int AirManeuverOrder::KeepAfterComplete()
{
    return 0;
}

// Slot 8 of AirManeuverOrder (vtable 0x4fd3f8, constructor 0x44e740), the slot
// before 0x44eb40: copies the first point out, advances it by the second point
// and, when flag bit 0 is set, turns the second point (a step per frame) towards
// the stored heading, by at most an eighth of the unit type's turn rate.
// The rotation helper takes a pair of ints, so the (x, z) step is built in the
// local's x and y fields and read back from y into other.z.
// FUNCTION: 0x44ea60
int AirManeuverOrder::FillWorldPos(Vec3* out)
{
    *out = target;
    target.x += other.x;
    target.z += other.z;
    Vec3 v;
    v.x = 0;
    v.y = 0;
    v.z = 0;
    unsigned short h = GetHeadingBetween(&v, &other);
    if ((flags & 1) && h != heading) {
        short diff = heading - h;
        v.x = other.x;
        v.y = other.z;
        short step = diff;
        unsigned short max = self->def->turnrate;
        if (step >= (max >> 3))
            step = max >> 3;
        else if (step <= -(max >> 3))
            step = -(max >> 3);
        FUN_004b7173(-step, (int*)&v);
        other.x = v.x;
        other.z = v.y;
        other.y = 0;
    }
    return 1;
}

// Virtual slot 9 (vtable at 0x4fd3f8), the same slot as 0x44e530: writes the
// heading from the owner's position (+0x6a) to the target point at +0xa.
// FUNCTION: 0x44eb40
int AirManeuverOrder::GetDesiredHeading(unsigned short* out)
{
    *out = GetHeadingBetween(&self->pos, &target);
    return 1;
}

// Slot 4: done
// when the unit is within 48 world units of the target point, or, when flag
// bit 0 is set, when the heading from the origin to the second point equals
// the stored heading.
// FUNCTION: 0x44eb60
int AirManeuverOrder::IsComplete(Unit* unit)
{
    if ((float)_hypot(unit->pos.x - target.x, unit->pos.z - target.z) / 65536.0f < 48.0f)
        return 1;
    if (flags & 1) {
        Vec3 origin;
        origin.x = 0;
        origin.y = 0;
        origin.z = 0;
        if (GetHeadingBetween(&origin, &other) == heading)
            return 1;
    }
    return 0;
}

// FUNCTION: 0x44ec00
int AirManeuverOrder::IsFxStyle()
{
    return 0;
}

// An empty method: its one caller (0x412d40) calls it on
// the object it has just built with that class's constructor (0x44e740).
// FUNCTION: 0x44ec10
void AirManeuverOrder::SetAltitude(int)
{
}

// Sets a flag bit in the word at +0x8 (a 1-bit unsigned short bitfield, which
// MSVC sets with `or byte ptr` straight to memory) and stores a short.
// FUNCTION: 0x44ec20
void AirManeuverOrder::SetHeading(short v)
{
    headingSet = 1;
    heading = v;
}

// std::vector<Point_0044eec0>::_Destroy(first, last) from MSVC 5's <vector>:
// empty, since the element type is trivial.
// FUNCTION: 0x44ee60 ?_Destroy@?$vector@UPoint16@@V?$allocator@UPoint16@@@std@@@std@@IAEXPAUPoint16@@0@Z
DestroyFn_0044ee60 PointVectorDestroyAccess::fn = &PointVectorDestroyAccess::_Destroy;

// std::vector<Point_0044eec0>::size() for the 4-byte point (two shorts); its
// callers are the vector insert paths in 0x44d0e0 and 0x44d560.
// FUNCTION: 0x44ee70 ?size@?$vector@UPoint16@@V?$allocator@UPoint16@@@std@@@std@@QBEIXZ
SizeFn_0044ee70 g_size_0044ee70 = &Vec_0044ee70::size;

// std::vector<Point_0044eec0>::_Ucopy(first, last, dest) from MSVC 5's
// <vector>: copy-constructs [first, last) into raw storage at dest and
// returns the end of the copies. Its callers (around 0x44d190) set ecx to the
// vector whose _Ufill is 0x44eec0.
// FUNCTION: 0x44ee90 ?_Ucopy@?$vector@UPoint16@@V?$allocator@UPoint16@@@std@@@std@@IAEPAUPoint16@@PBU3@0PAU3@@Z
UcopyFn_0044ee90 PointVectorUcopyAccess::fn = &PointVectorUcopyAccess::_Ucopy;

// std::vector<Point_0044eec0>::_Ufill(first, n, value) from MSVC 5's <vector>:
// copy-constructs n copies of value into raw storage. Its callers (around
// 0x44d190) set ecx to the vector; its _Ucopy is 0x44ee90 and its copy is
// 0x44eef0 (4-byte elements, two shorts).
// FUNCTION: 0x44eec0 ?_Ufill@?$vector@UPoint16@@V?$allocator@UPoint16@@@std@@@std@@IAEXPAUPoint16@@IABU3@@Z
UfillFn_0044eec0 PointVectorUfillAccess::fn = &PointVectorUfillAccess::_Ufill;

// std::copy for 4-byte elements (two shorts), compiled with __stdcall as the
// default convention. Its one caller (0x44da00) is an inlined
// vector::erase(begin(), end()) whose _Destroy is 0x44ee60. Not a member of
// the PathGoal family next to it: it takes no `this`.
// FUNCTION: 0x44eef0
Point16* __stdcall CopyDwordRangeUnchecked(Point16* first, Point16* last, Point16* dest)
{
    for (; first != last; ++dest, ++first)
        *dest = *first;
    return dest;
}

// The base of a class family (vtable 0x4fd428, 11 slots; constructor
// 0x44ef20, ??_G 0x44ef60). Derived classes, all with this class's
// declaration copied verbatim:
//   AiSearchGoal  vtable 0x4fd458  ctor 0x44f010  dtor 0x44f450  ??_G 0x44f040
//   PatrolGoal  vtable 0x4fd488  ctor 0x44f570  ??_G 0x44f590
//   PackedGoal  vtable 0x4fd980  ctor 0x4905e0  ??_G 0x490630
//     LiteGoal  vtable 0x4fd9b0  ctor 0x4907e0  ??_G 0x490840
//     PackedPosGoal  vtable 0x4fd9e0  ctor 0x490940  dtor 0x4909e0  ??_G 0x4909a0

// The constructor.
// FUNCTION: 0x44ef20
// FUNCTION: 0x44ef60 ??_GPathGoal@@UAEPAXI@Z
PathGoal::PathGoal(Unit* p)
{
    owner = p;
    target = 0;
}

// Slot 3: does nothing.
// FUNCTION: 0x44ef40
void PathGoal::FillWaypointWorldPos(Vec3*, int, int)
{
}

// Slot 10: does nothing.
// FUNCTION: 0x44ef50
void PathGoal::DrawOnSurface(void*)
{
}

// Slot 5: whether there is an object at +0x4.
// FUNCTION: 0x44ef80
int PathGoal::HasReadyWaypoints()
{
    return target != 0;
}

// Slot 1: sets the object at +0x4, first telling the one it replaces 0x80.
// FUNCTION: 0x44ef90
void PathGoal::SetPathOrder(void* param)
{
    if (target != 0) {
        ((OrderFx*)target)->AddFlags(0x80);
    }
    target = (Base_00490a10*)param;
}

// Slot 2: does nothing.
// FUNCTION: 0x44efb0
void PathGoal::TickTowardGoal()
{
}

// Slot 8: does nothing.
// FUNCTION: 0x44efc0
void PathGoal::SerializeNetUnitState(BitWriter*)
{
}

// Slot 9: does nothing.
// FUNCTION: 0x44efd0
void PathGoal::DeserializeNetUnitState(BitReader*)
{
}

// Slot 7: 0.
// FUNCTION: 0x44efe0
int PathGoal::HasNetUnitState()
{
    return 0;
}

// Slot 6: none.
// FUNCTION: 0x44eff0
AiSearchGoal* PathGoal::TryClaimRepath()
{
    return 0;
}

// Slot 4: does nothing.
// FUNCTION: 0x44f000
void PathGoal::ExportGoalPose(Vec3*, Vec3*, short*)
{
}

// The constructor.
// FUNCTION: 0x44f010
AiSearchGoal::AiSearchGoal(Unit* p)
    : PathGoal(p)
{
    count = 0;
    active = 0;
    flag_1 = 0;
    flag_3 = 1;
    repathClaimTick = 0;
}

// Sets the path points (at most 20) and marks it active, or with no
// points asks the object at +0x4 about the owner and flags it (0x40) when that
// fails.
// FUNCTION: 0x44f080
void AiSearchGoal::SetWaypoints(Point16* src, int n)
{
    if (n == 0) {
        if (target && target->ContainsUnit(owner) == 0)
            ((OrderFx*)target)->AddFlags(0x40);
        active = 0;
    } else {
        if (n >= 20)
            n = 20;
        count = n;
        std::copy(src, src + n, points);
        active = 1;
    }
    flag_1 = 0;
    flag_3 = 1;
}

// Drops the first n path points, clears the active flag when fewer
// than two are left and sets flag 3.
// FUNCTION: 0x44f100
void AiSearchGoal::TruncateWaypointsFrom(int n)
{
    if (n != 0) {
        std::copy(points + n, points + count, points);
        count -= n;
        if (count < 2)
            active = 0;
        flag_3 = 1;
    }
}

// Slot 3: fills n positions from the path points, repeating the last point past the
// end of the path.
// FUNCTION: 0x44f150
void AiSearchGoal::FillWaypointWorldPos(Vec3* out, int unused, int n)
{
    for (int i = 0; i < n; i++) {
        int j = i < count ? i : count - 1;
        out[i].x = points[j].x << 16;
        out[i].y = 0;
        out[i].z = points[j].y << 16;
    }
}

// Slot 2: tells the object at +0x4 about the owner, then, when the unit has reached
// its second path point (within 5 units of the owner), drops that point with
// an overlapping std::copy and refreshes the flags.
// FUNCTION: 0x44f1a0
void AiSearchGoal::TickTowardGoal()
{
    if (target) {
        if (target->ContainsUnit(owner)) {
            ((OrderFx*)target)->AddFlags(0x20);
            if (!target->KeepAfterComplete())
                SetPathOrder(0);
        }
    }
    if (count >= 2) {
        int dx = owner->zWhole - points[1].y;
        int dy = owner->xWhole - points[1].x;
        if (dy * dy + dx * dx <= 25) {
            // std::copy, not memmove: memmove stays a library call.
            std::copy(points + 1, points + count, points);
            int n = --count;
            if (n < 2)
                active = 0;
            flag_3 = 1;
        }
    }
    if (target && (Motion(owner)->flags & 4 || count < 2))
        flag_1 = 1;
}

// Slot 6: returns this object when its flag bit 1 is set and the counter at
// g_game+0x38a47 has reached repathClaimTick + 0x3c (storing the counter in
// repathClaimTick), or null. The path search scheduler (0x40eb70) calls it through
// slot 6 to pick the path to search for next.
// FUNCTION: 0x44f260
AiSearchGoal* AiSearchGoal::TryClaimRepath()
{
    if (flags & 2) {
        unsigned int limit = (unsigned int)g_game->ticks;
        if (limit >= repathClaimTick + 0x3c) {
            repathClaimTick = limit;
            return this;
        }
    }
    return 0;
}

// Slot 5: the active flag (bit 0 of +0x64).
// FUNCTION: 0x44f290
int AiSearchGoal::HasReadyWaypoints()
{
    return flags & 1;
}

// Slot 1: hands the object at +0x4 over to the
// path logic, then, unless the path is already active, steps the path
// forwards: it asks the object for its position (its slot 8) and marks the
// path active when the next path point is at most half as far from that
// object as the owner is. With no usable point left it resets the path to
// two points taken from the owner and the object's position.
// FUNCTION: 0x44f2a0
void AiSearchGoal::SetPathOrder(void* param)
{
    g_seams.pathfinding->AbortIfGoalMatch(g_game->pathfinder, this);
    if (target)
        ((OrderFx*)target)->AddFlags(0x80);
    active = 0;
    target = (Base_00490a10*)param;
    if (param == 0) {
        flag_1 = 0;
    } else {
        flag_1 = 1;
        if (count >= 3) {
            if (target->ContainsCell(points[count - 1].x >> 4, points[count - 1].y >> 4)) {
                active = 1;
                flag_1 = 0;
            }
        }
        if (!active) {
            Vec3 p;
            if (target->FillWorldPos(&p)) {
                if (count >= 3) {
                    int sx = points[count - 1].x << 16;
                    int sz = points[count - 1].y << 16;
                    int d1 = (int)_hypot(owner->pos.x - p.x,
                                         owner->pos.z - p.z);
                    int d2 = (int)_hypot(sx - p.x,
                                         sz - p.z);
                    if (d2 * 2 < d1)
                        active = 1;
                }
                if (!active) {
                    Order* t = owner->list;
                    if (t && !(t->flags_42 & 0x800000)) {
                        count = 2;
                        points[0].x = owner->xWhole;
                        points[0].y = owner->zWhole;
                        points[1].x = (short)(p.x >> 16);
                        points[1].y = (short)(p.z >> 16);
                        active = 1;
                    }
                }
            }
        }
    }
    if (repathClaimTick <= (unsigned int)g_game->ticks - 10)
        repathClaimTick = 0;
    flag_3 = 1;
}

// The out-of-line destructor: it stores its own vtable, unregisters the object, then the empty inline base destructor
// stores 0x4fd428.
// FUNCTION: 0x44f040 ??_GAiSearchGoal@@UAEPAXI@Z
// FUNCTION: 0x44f450
AiSearchGoal::~AiSearchGoal()
{
    g_seams.pathfinding->AbortIfGoalMatch(g_game->pathfinder, this);
}

// Slot 7: true when there is something to send: flag 3 of +0x64 (the path changed) is
// set, or flag 2 no longer matches bit 2 of the owner's target. Slot 8
// (0x44f4a0) writes the path out and brings both flags up to date.
// FUNCTION: 0x44f480
int AiSearchGoal::HasNetUnitState()
{
    return (flags & 8) || ((Motion(owner)->flags ^ flags) & 4);
}

// Slot 8, the write counterpart of the reader 0x44f5c0. It sets the stream's next bit when the
// unit's mode asks for it, writes the point count in 2 bits, then up to three
// points of 16 bits each, and finally copies the unit's mode into flag 2 while
// clearing flag 3 (the "changed" flag the readers rely on).
// FUNCTION: 0x44f4a0
void AiSearchGoal::SerializeNetUnitState(BitWriter* stream)
{
    int n;
    if (active) {
        n = count < 3 ? count : 3;
    } else {
        n = 0;
    }
    if (Motion(owner)->flags & 4) {
        stream->data[stream->bit] |= 1 << stream->index;
    }
    stream->index++;
    if (stream->index == 0x20) {
        stream->index = 0;
        stream->bit++;
        if (stream->bit == stream->capacity) {
            stream->GrowBuffer();
        }
        stream->data[stream->bit] = 0;
    }
    stream->WriteBits(n, 2);
    for (int i = 0; i < n; i++) {
        stream->WriteBits(points[i].x, 0x10);
        stream->WriteBits(points[i].y, 0x10);
    }
    flag_2 = (Motion(owner)->flags & 4) != 0;
    flag_3 = 0;
}

// The constructor, with the base constructor inlined. Its vtable reference
// makes the compiler emit the scalar deleting destructor here too: the
// destructor is trivial, so the dead store of this class's vtable disappears
// and only the inlined base destructor's store of 0x4fd428 is left.
// FUNCTION: 0x44f570
// FUNCTION: 0x44f590 ??_GPatrolGoal@@UAEPAXI@Z
PatrolGoal::PatrolGoal(Unit* p)
    : PathGoal(p)
{
    count = 0;
}

// Slot 5: whether the patrol has at least two waypoints.
// FUNCTION: 0x44f5b0
int PatrolGoal::HasReadyWaypoints()
{
    return count >= 2;
}

// Slot 9: reads the owner's target flag (bit 2 of +0x2e) and up to three waypoints from the stream.
// FUNCTION: 0x44f5c0
void PatrolGoal::DeserializeNetUnitState(BitReader* reader)
{
    Motion(owner)->flag = reader->ReadBit();
    count = reader->ReadBits(2);
    for (int i = 0; i < count; i++) {
        points[i].x = reader->ReadBits(16);
        points[i].y = reader->ReadBits(16);
    }
}

// Slot 3: converts up to `n` short 2D points into 16.16 fixed-point 3D vectors
// (x, 0, y), repeating the last point once the list runs out.
// FUNCTION: 0x44f650
void PatrolGoal::FillWaypointWorldPos(Vec3* out, int unused, int n)
{
    for (int i = 0; i < n; i++) {
        int j = i < count ? i : count - 1;
        out[i].x = points[j].x << 16;
        out[i].y = 0;
        out[i].z = points[j].y << 16;
    }
}

// Creates the object at g_game+0x14207 (deleted again by 0x44f6e0).
// FUNCTION: 0x44f6a0
void CreatePathfinder()
{
    g_game->pathfinder = new (kSim) Pathfinder;
}

// Deletes the object at g_game+0x14207 and clears the pointer.
// FUNCTION: 0x44f6e0
void DestroyPathfinder()
{
    g_seams.pathfinding->Destroy(g_game->pathfinder);
    g_game->pathfinder = 0;
}
