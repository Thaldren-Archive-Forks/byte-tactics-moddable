#include "../../interfaces/seams.h"

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
char* __stdcall FindWeaponByName(char* name);
void __stdcall UpdateResourceSharing(Player* player);
void __stdcall UpdateFeatures(void);
void __stdcall UpdateProjectiles(void);
void __stdcall UpdateExplosions(void);
void __stdcall UpdateAllUnits(void);
void __stdcall UpdatePlayers(void);

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
    s_features.UpdateFeatures = UpdateFeatures;
    s_weapons.UpdateProjectiles = UpdateProjectiles;
    s_weapons.UpdateExplosions = UpdateExplosions;
    s_weapons.SpawnProjectile = SpawnProjectile;
    s_weapons.FindWeaponByName = FindWeaponByName;
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
