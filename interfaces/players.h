#ifndef INTERFACES_PLAYERS_H
#define INTERFACES_PLAYERS_H

struct Player;
struct Unit;
struct OwnershipTransferPacket;
class HapiBank;
class MissionConditions;
class TdfFile;

// Major version of the players interface, changed when a slot is removed or its signature changes.
#define PLAYERS_VERSION_MAJOR 1
// Minor version of the players interface, changed when a slot is added.
#define PLAYERS_VERSION_MINOR 3

// Per-player state.
struct PlayersSeam {
    // Advances every player's state for one tick.
    void (__stdcall* UpdatePlayers)(void);
    // Moves metal from one player to another, clamped to what the sender has, and announces it when flag is set. Hooks: none.
    void (__stdcall* TransferMetal)(unsigned char from, unsigned char to, float amount, int flag);
    // Moves energy from one player to another, clamped to what the sender has, and announces it when flag is set. Hooks: none.
    void (__stdcall* TransferEnergy)(unsigned char from, unsigned char to, float amount, int flag);
    // Returns the energy a player makes per tick. Hooks: none.
    float (__stdcall* GetEnergyIncome)(Player* player);
    // Returns the metal a player makes per tick. Hooks: none.
    float (__stdcall* GetMetalIncome)(Player* player);
    // Returns the energy a player makes less what it uses per tick. Hooks: none.
    float (__stdcall* GetNetEnergy)(Player* player);
    // Returns the metal a player makes less what it uses per tick. Hooks: none.
    float (__stdcall* GetNetMetal)(Player* player);
    // Runs the AI script of the mission, or the default one, and parses the AI weight scripts of the computer players. Hooks: none.
    void (__stdcall* LoadDefaultAIScript)(void);
    // Sets up the player slots at the start of a match. Hooks: none.
    void (__stdcall* InitPlayers)(void);
    // Frees the player slots at the end of a match. Hooks: none.
    void (__stdcall* FreePlayers)(void);
    // Gives every player the metal and energy the mission starts with. Hooks: none.
    void (__stdcall* InitPlayerResources)(void);
    // Reads which player is a human and which a computer from a saved game. Hooks: none.
    void (__stdcall* LoadPlayerControllers)(HapiBank* file);
    // Reads the players of a saved game from a bank. Hooks: none.
    void (__stdcall* LoadPlayers)(HapiBank* file);
    // Writes the players of a saved game to a bank. Hooks: none.
    void (__stdcall* SavePlayers)(HapiBank* file);
    // Rebuilds the feature cells every computer player keeps. Hooks: none.
    void (__stdcall* RebuildAIFeatureCells)(void);
    // Fills a player slot as a human, a computer or an empty one. Hooks: none.
    void (__stdcall* SetupPlayerSlot)(int player, char type);
    // Steps the blinking of the player markers. Hooks: none.
    void (__stdcall* UpdateBlink)(void);
    // Kills every unit of a player. Hooks: none.
    void (__stdcall* KillPlayerUnits)(unsigned char player);
    // Hands a unit to another player, with the record that tells the new owner, or none. Hooks: none.
    void (__stdcall* GiveUnitToPlayer)(Unit* unit, Player* other, OwnershipTransferPacket* packet);
    // Stops the victory and defeat conditions of the mission from being checked (thiscall in 3.1c: this in ecx). Hooks: none.
    void (__fastcall* DeactivateConditions)(MissionConditions* conditions);
    // Reads the conditions state of a saved game from a bank (thiscall in 3.1c: this in ecx). Hooks: none.
    void (__fastcall* LoadConditions)(MissionConditions* conditions, int unusedEdx, HapiBank* file);
    // Writes the conditions state of a saved game to a bank (thiscall in 3.1c: this in ecx). Hooks: none.
    void (__fastcall* SaveConditions)(MissionConditions* conditions, int unusedEdx, HapiBank* file);
    // Reads the victory and defeat conditions from a mission file (thiscall in 3.1c: this in ecx). Hooks: none.
    void (__fastcall* RegisterConditions)(MissionConditions* conditions, int unusedEdx, TdfFile* mission);
    // Frees the condition objects (thiscall in 3.1c: this in ecx). Hooks: none.
    void (__fastcall* FreeConditions)(MissionConditions* conditions);
    // Frees the condition objects and then the conditions object itself. Hooks: none.
    void (__stdcall* DeleteConditions)(MissionConditions* conditions);
    // Tells the conditions that a unit was created (thiscall in 3.1c: this in ecx). Hooks: none.
    void (__fastcall* NotifyUnitCreated)(MissionConditions* conditions, int unusedEdx, Unit* unit);
    // Tells the conditions that a unit died (thiscall in 3.1c: this in ecx). Hooks: none.
    void (__fastcall* NotifyUnitDied)(MissionConditions* conditions, int unusedEdx, Unit* unit);
};

#endif
