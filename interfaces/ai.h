#ifndef INTERFACES_AI_H
#define INTERFACES_AI_H

namespace std {
template<class T> class allocator;
template<class T, class A> class vector;
}

struct Unit;
struct Player;
struct _iobuf;
struct Vec3;
class SquadManager;

// Major version of the ai interface, changed when a slot is removed or its signature changes.
#define AI_VERSION_MAJOR 1
// Minor version of the ai interface, changed when a slot is added.
#define AI_VERSION_MINOR 4

// Computer player decisions.
struct AiSeam {
    // Appends to out the enemies a player sees within a radius of a position. Hooks: none.
    void (__stdcall* GetVisibleEnemiesInRadius)(int player, const Vec3* pos, int radius, int flags, std::vector<Unit*, std::allocator<Unit*> >* out);
    // Returns whether a player has a ready unit within a range of a position. Hooks: none.
    bool (__stdcall* HasReadyUnitInRange)(int player, Vec3* pos, int range);
    // Returns 1 when a position is inside what a player's side can see. Hooks: none.
    int (__stdcall* IsPointVisible)(Player* player, Vec3* pos);
    // Lets a unit's player react to an attack by another unit. Hooks: none.
    void (__stdcall* ReactToAttack)(Unit* attacker, Unit* unit, int amount);
    // Marks a squad manager's owner for a network update after a damage split (thiscall in 3.1c: this in ecx, edx unused). Hooks: none.
    void (__fastcall* MarkOwnerNetDirtyFromDamageSplit)(SquadManager* self, int unusedEdx, void* weapon, int enemyDamage, int friendlyDamage);
    // Writes a report on the AI of a player to a file. Hooks: none.
    void (__stdcall* DumpPlayerAI)(int player, _iobuf* file);
    // Resets the unit tables of the computer players and loads the default AI script again. Hooks: none.
    void (__stdcall* ResetAIPlayers)(void);
    // Appends to out the factories a player has within a radius of a position. Hooks: none.
    void (__stdcall* GetFactoriesInRadius)(int player, const Vec3* pos, int radius, std::vector<Unit*, std::allocator<Unit*> >* out);
    // Registers the console commands that tune the AI. Hooks: none.
    void (__stdcall* RegisterAICommands)(void);
    // Creates the AI state of a player. Hooks: none.
    void (__stdcall* CreatePlayerAI)(int player);
    // Frees the AI state of a player. Hooks: none.
    void (__stdcall* DestroyPlayerAI)(int player);
    // Advances the AI of a player for one tick. Hooks: none.
    void (__stdcall* UpdatePlayerAI)(int player);
    // Rebuilds the feature cells an AI player keeps. Hooks: none.
    void (__stdcall* RebuildFeatureCells)(int player);
    // Parses the AI weight scripts a player downloaded. Hooks: none.
    void (__stdcall* ParseDownloadableAiWeightScripts)(int player);
    // Parses a player's AI weight scripts again unless the unit limit is sticky. Hooks: none.
    void (__stdcall* ReparseAiWeightScriptsIfLimitNotSticky)(int player);
    // Creates the squads of a player. Hooks: none.
    void (__stdcall* CreateSquads)(Player* player);
    // Frees the squads of a player. Hooks: none.
    void (__stdcall* FreeSquads)(Player* player);
    // Moves a unit to a squad of its player, or out of any squad when the index is -1. Hooks: none.
    void (__stdcall* SetUnitSquad)(Unit* unit, int index);
    // Runs one tick of a squad manager when its player is an active computer player (thiscall in 3.1c: this in ecx, edx unused). Hooks: none.
    void (__fastcall* SquadManagerTickIfActive)(SquadManager* self, int unusedEdx);
    // Deletes the timers of a squad manager (thiscall in 3.1c: this in ecx, edx unused). Hooks: none.
    void (__fastcall* SquadManagerDeleteTimers)(SquadManager* self, int unusedEdx);
};

#endif
