#ifndef INTERFACES_PLAYERS_H
#define INTERFACES_PLAYERS_H

struct Player;

// Major version of the players interface, changed when a slot is removed or its signature changes.
#define PLAYERS_VERSION_MAJOR 1
// Minor version of the players interface, changed when a slot is added.
#define PLAYERS_VERSION_MINOR 2

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
};

#endif
