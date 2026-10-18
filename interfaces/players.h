#ifndef INTERFACES_PLAYERS_H
#define INTERFACES_PLAYERS_H


// Major version of the players interface, changed when a slot is removed or its signature changes.
#define PLAYERS_VERSION_MAJOR 1
// Minor version of the players interface, changed when a slot is added.
#define PLAYERS_VERSION_MINOR 1

// Per-player state.
struct PlayersSeam {
    // Advances every player's state for one tick.
    void (__stdcall* UpdatePlayers)(void);
    // Moves metal from one player to another, clamped to what the sender has, and announces it when flag is set. Hooks: none.
    void (__stdcall* TransferMetal)(unsigned char from, unsigned char to, float amount, int flag);
    // Moves energy from one player to another, clamped to what the sender has, and announces it when flag is set. Hooks: none.
    void (__stdcall* TransferEnergy)(unsigned char from, unsigned char to, float amount, int flag);
};

#endif
