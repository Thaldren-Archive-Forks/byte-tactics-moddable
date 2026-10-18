#include <vector>

#include "../../interfaces/seams.h"
#include "../game/unit_resources.h"
#include "../units/cob_script.h"
#include "../util/vec3.h"
#include "../map/mission.h"
#include "../units/unit.h"

struct Player;

class MeteorParams {
public:
    void LoadMeteorDefaults();
};

void __cdecl UpdateWind(void);
void __stdcall UpdateMeteors(void);
void __stdcall InitMeteors(void);
void __stdcall EmptyShutdownPreCleanup(void);
void __stdcall EnableMeteors(void);
void __stdcall DisableMeteors(void);
void __stdcall SetMeteorParams(MeteorParams* params);
void __stdcall SaveMeteors(HapiBank* file);
void __stdcall LoadMeteors(HapiBank* file);
void __stdcall CmdMeteor(CommandArgs* args);
int __stdcall SpawnProjectile(WeaponDef* weapon, Vec3* pos, Vec3* vel, int flag);
void __stdcall UpdateResourceSharing(Player* player);
void __stdcall UpdateFeatures(void);

struct Cell;
struct Feature;
struct Weapon;
struct Spot;
struct Rot16;
struct List_00421f20;
struct Pos_00485070;
struct Obj_0043e3c0;
struct Vec3_0049a0c0;
struct Vec3_0048a1e0;
struct Vec;
struct UnitDef;
struct WeaponDef;
struct Callback;

float __stdcall GetEnergyUse(UnitDef* def);
void __stdcall UpdatePlayerEconomy(Player* p);
void __stdcall UpdateMetalExtraction(Unit* unit);

void __stdcall LoadFeatureFileList(void);
void __stdcall FreeFeatureFileList(void);
void __stdcall NopAfterFeatureEnum(void);
void __stdcall ResolveFeatureLinks(void);
void __stdcall InitFeatureAnimPool(List_00421f20* list);
void __stdcall FreeFeaturePool(void);
void __stdcall PlaceMissionFeatures(void);
void __stdcall StampFeatureMetal(void);
unsigned short __stdcall GetCellFeature(Cell* cell);
Spot* __stdcall PlaceFeature(Cell* cell, unsigned short feature, Vec3* pos, Rot16* rot, unsigned char owner);
int __stdcall RemoveFeature(Cell* cell, int flag);
int __stdcall ReclaimFeature(Unit* unit, Vec3* pos);
unsigned short __stdcall FindOrLoadFeatureType(char* name);
void __stdcall DamageFeature(Cell* cell, int x, int z, Weapon* weapon);
Vec3 __stdcall GetFootprintCentre(Point16* cell, Feature* def);
void __stdcall SaveFeatures(HapiBank* file);
void __stdcall LoadFeatures(HapiBank* file);

void __stdcall SetWeaponTargetUnit(char* param_1, char* param_2, int param_3);
void __stdcall SetWeaponTargetPos(char* param_1, int* param_2, int param_3);
void __stdcall ClearWeaponTarget(Unit* unit, int index);
void __stdcall ResetWeaponTarget(Unit* obj, int index);
Unit* __stdcall GetWeaponTargetUnit(Unit* obj, int index);
int __stdcall GetWeaponTargetPos(Unit* unit, Vec3_0048a1e0* pos, int index);
Unit* __stdcall FindWeaponTarget(Unit* unit, unsigned char weapon, int useRange);
int __stdcall FindBestTargetIfFireAtWill(Unit* unit);

Cell* __stdcall GetMapCell(int x, int y);
void __stdcall CmdShareMapping(int unused);
void __stdcall CmdShareRadar(int unused);
void __stdcall TransferMetal(unsigned char from, unsigned char to, float amount, int flag);
void __stdcall TransferEnergy(unsigned char from, unsigned char to, float amount, int flag);
int __stdcall GetGroundHeight(Pos_00485070* p);
void __stdcall GetSweetSpot(Obj_0043e3c0* obj, int out);
int __stdcall GetCellMeanHeight(Pos_00485070* p);
void __stdcall RefreshAllPassMaps(Point16 a, Point16 b);
void __stdcall ApplyAreaDamageAt(void* owner, Vec3_0049a0c0* pos);
char* __stdcall FindWeaponByName(char* name);
int __stdcall GetWeaponRange(int param1, unsigned int param2);
int __stdcall WeaponCanReachUnit(Unit* unit1, Unit* unit2, unsigned char weapon);
void __stdcall GetVisibleEnemiesInRadius(int player, const Vec* pos, int radius, int flags, std::vector<Unit*>* out);

static void __stdcall ResetNative(UnitResources* self, unsigned char playerIndex)
{
    self->Reset(playerIndex);
}

static void __stdcall SaveUnitAccountsNative(UnitResources* self, UnitInfo* info, HapiBank* file)
{
    self->SaveUnitAccounts(info, file);
}

static void __stdcall LoadUnitAccountsNative(UnitResources* self, UnitInfo* info, HapiBank* file)
{
    self->LoadUnitAccounts(info, file);
}

static int __stdcall RequestEnergyNative(UnitResources* self, UnitResources* r, float amount)
{
    return self->RequestEnergy(r, amount);
}

static int __stdcall RequestEnergyAndMetalNative(UnitResources* self, float energy, float metal)
{
    return self->RequestEnergyAndMetal(energy, metal);
}

static int __stdcall SpendEnergyNative(UnitResources* self, float amount)
{
    return self->SpendEnergy(amount);
}

static int __stdcall SpendMetalNative(UnitResources* self, float amount)
{
    return self->SpendMetal(amount);
}

static int __stdcall SpendEnergyAndMetalNative(UnitResources* self, float energy, float metal)
{
    return self->SpendEnergyAndMetal(energy, metal);
}

static int __stdcall FindScriptNative(CobScript* script, const char* name)
{
    return script->FindScript(name);
}

static int __stdcall StartScriptWithArgsNative(CobScript* script, char* name, Callback* callback, int update, int count, int a, int b, int c, int d)
{
    return script->StartScriptWithArgs(name, callback, update, count, a, b, c, d);
}

static void __stdcall SetStateBitsNative(Unit* unit, int mask, int set)
{
    unit->SetStateBits(mask, set);
}

void __stdcall UpdateProjectiles(void);
void __stdcall UpdateExplosions(void);
void __stdcall UpdateAllUnits(void);
void __stdcall UpdatePlayers(void);

static unsigned char __fastcall ChooseWeaponNative(Unit* self)
{
    return self->ChooseWeapon();
}

static int __fastcall GetGameTypeNative(Mission* self)
{
    return self->GetGameType();
}

static void __stdcall SetWeaponTargetUnitNative(Unit* unit, Unit* target, int weapon)
{
    SetWeaponTargetUnit((char*)unit, (char*)target, weapon);
}

static void __stdcall SetWeaponTargetPosNative(Unit* unit, Vec3* pos, int weapon)
{
    SetWeaponTargetPos((char*)unit, &pos->x, weapon);
}

static int __stdcall GetWeaponTargetPosNative(Unit* unit, Vec3* pos, int weapon)
{
    return GetWeaponTargetPos(unit, (Vec3_0048a1e0*)pos, weapon);
}

static void __stdcall DamageFeatureNative(Cell* cell, int x, int z, WeaponDef* weapon)
{
    DamageFeature(cell, x, z, (Weapon*)weapon);
}

static Unit* __stdcall FindBestTargetIfFireAtWillNative(Unit* unit)
{
    return (Unit*)FindBestTargetIfFireAtWill(unit);
}

static int __stdcall GetGroundHeightNative(Vec3* pos)
{
    return GetGroundHeight((Pos_00485070*)pos);
}

static void __stdcall GetSweetSpotNative(Unit* unit, Vec3* out)
{
    GetSweetSpot((Obj_0043e3c0*)unit, (int)out);
}

static int __stdcall GetCellMeanHeightNative(Vec3* pos)
{
    return GetCellMeanHeight((Pos_00485070*)pos);
}

static void __stdcall ApplyAreaDamageAtNative(void* owner, Vec3* pos)
{
    ApplyAreaDamageAt(owner, (Vec3_0049a0c0*)pos);
}

static WeaponDef* __stdcall FindWeaponByNameNative(char* name)
{
    return (WeaponDef*)FindWeaponByName(name);
}

static int __stdcall GetWeaponRangeNative(Unit* unit, unsigned int weapon)
{
    return GetWeaponRange((int)unit, weapon);
}

static void __stdcall GetVisibleEnemiesInRadiusNative(int player, const Vec3* pos, int radius, int flags, std::vector<Unit*>* out)
{
    GetVisibleEnemiesInRadius(player, (const Vec*)pos, radius, flags, out);
}

Seams g_seams;

static WindMeteorsSeam s_wind_meteors;
static EconomySeam s_economy;
static FeaturesSeam s_features;
static AiSeam s_ai;
static PathfindingSeam s_pathfinding;
static WeaponsSeam s_weapons;
static TargetingSeam s_targeting;
static LosRadarSeam s_los_radar;
static OrdersSeam s_orders;
static UnitsSeam s_units;
static PlayersSeam s_players;
static MapSeam s_map;

static void __fastcall LoadMeteorDefaultsNative(MeteorParams* params)
{
    params->LoadMeteorDefaults();
}

void InstallNativeSeams()
{
    s_wind_meteors.UpdateWind = UpdateWind;
    s_wind_meteors.UpdateMeteors = UpdateMeteors;
    s_wind_meteors.InitMeteors = InitMeteors;
    s_wind_meteors.EmptyShutdownPreCleanup = EmptyShutdownPreCleanup;
    s_wind_meteors.EnableMeteors = EnableMeteors;
    s_wind_meteors.DisableMeteors = DisableMeteors;
    s_wind_meteors.SetMeteorParams = SetMeteorParams;
    s_wind_meteors.LoadMeteorDefaults = LoadMeteorDefaultsNative;
    s_wind_meteors.SaveMeteors = SaveMeteors;
    s_wind_meteors.LoadMeteors = LoadMeteors;
    s_wind_meteors.CmdMeteor = CmdMeteor;
    s_economy.UpdateResourceSharing = UpdateResourceSharing;
    s_economy.GetEnergyUse = GetEnergyUse;
    s_economy.UpdatePlayerEconomy = UpdatePlayerEconomy;
    s_economy.UpdateMetalExtraction = UpdateMetalExtraction;
    s_economy.Reset = ResetNative;
    s_economy.SaveUnitAccounts = SaveUnitAccountsNative;
    s_economy.LoadUnitAccounts = LoadUnitAccountsNative;
    s_economy.RequestEnergy = RequestEnergyNative;
    s_economy.RequestEnergyAndMetal = RequestEnergyAndMetalNative;
    s_economy.SpendEnergy = SpendEnergyNative;
    s_economy.SpendMetal = SpendMetalNative;
    s_economy.SpendEnergyAndMetal = SpendEnergyAndMetalNative;
    s_features.UpdateFeatures = UpdateFeatures;
    s_features.LoadFeatureFileList = LoadFeatureFileList;
    s_features.FreeFeatureFileList = FreeFeatureFileList;
    s_features.NopAfterFeatureEnum = NopAfterFeatureEnum;
    s_features.ResolveFeatureLinks = ResolveFeatureLinks;
    s_features.InitFeatureAnimPool = InitFeatureAnimPool;
    s_features.FreeFeaturePool = FreeFeaturePool;
    s_features.PlaceMissionFeatures = PlaceMissionFeatures;
    s_features.StampFeatureMetal = StampFeatureMetal;
    s_features.GetCellFeature = GetCellFeature;
    s_features.PlaceFeature = PlaceFeature;
    s_features.RemoveFeature = RemoveFeature;
    s_features.ReclaimFeature = ReclaimFeature;
    s_features.FindOrLoadFeatureType = FindOrLoadFeatureType;
    s_features.DamageFeature = DamageFeatureNative;
    s_features.GetFootprintCentre = GetFootprintCentre;
    s_features.SaveFeatures = SaveFeatures;
    s_features.LoadFeatures = LoadFeatures;
    s_targeting.SetWeaponTargetUnit = SetWeaponTargetUnitNative;
    s_targeting.SetWeaponTargetPos = SetWeaponTargetPosNative;
    s_targeting.ClearWeaponTarget = ClearWeaponTarget;
    s_targeting.ResetWeaponTarget = ResetWeaponTarget;
    s_targeting.GetWeaponTargetUnit = GetWeaponTargetUnit;
    s_targeting.GetWeaponTargetPos = GetWeaponTargetPosNative;
    s_targeting.FindWeaponTarget = FindWeaponTarget;
    s_targeting.FindBestTargetIfFireAtWill = FindBestTargetIfFireAtWillNative;
    s_targeting.ChooseWeapon = ChooseWeaponNative;
    s_los_radar.GetMapCell = GetMapCell;
    s_los_radar.CmdShareMapping = CmdShareMapping;
    s_los_radar.CmdShareRadar = CmdShareRadar;
    s_players.TransferMetal = TransferMetal;
    s_players.TransferEnergy = TransferEnergy;
    s_units.GetGroundHeight = GetGroundHeightNative;
    s_units.GetSweetSpot = GetSweetSpotNative;
    s_units.FindScript = FindScriptNative;
    s_units.StartScriptWithArgs = StartScriptWithArgsNative;
    s_units.SetStateBits = SetStateBitsNative;
    s_map.GetCellMeanHeight = GetCellMeanHeightNative;
    s_map.RefreshAllPassMaps = RefreshAllPassMaps;
    s_map.GetGameType = GetGameTypeNative;
    s_weapons.FindWeaponByName = FindWeaponByNameNative;
    s_weapons.ApplyAreaDamageAt = ApplyAreaDamageAtNative;
    s_weapons.GetWeaponRange = GetWeaponRangeNative;
    s_weapons.WeaponCanReachUnit = WeaponCanReachUnit;
    s_ai.GetVisibleEnemiesInRadius = GetVisibleEnemiesInRadiusNative;
    s_weapons.UpdateProjectiles = UpdateProjectiles;
    s_weapons.UpdateExplosions = UpdateExplosions;
    s_weapons.SpawnProjectile = SpawnProjectile;
    s_units.UpdateAllUnits = UpdateAllUnits;
    s_players.UpdatePlayers = UpdatePlayers;
    g_seams.wind_meteors = &s_wind_meteors;
    g_seams.economy = &s_economy;
    g_seams.features = &s_features;
    g_seams.ai = &s_ai;
    g_seams.pathfinding = &s_pathfinding;
    g_seams.weapons = &s_weapons;
    g_seams.targeting = &s_targeting;
    g_seams.los_radar = &s_los_radar;
    g_seams.orders = &s_orders;
    g_seams.units = &s_units;
    g_seams.players = &s_players;
    g_seams.map = &s_map;
}
