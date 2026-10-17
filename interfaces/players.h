#ifndef INTERFACES_PLAYERS_H
#define INTERFACES_PLAYERS_H

// Major version of the players interface, changed when a slot is removed or its signature changes.
#define PLAYERS_VERSION_MAJOR 1
// Minor version of the players interface, changed when a slot is added.
#define PLAYERS_VERSION_MINOR 0

// Per-player state.
struct PlayersSeam {
    // Advances every player's state for one tick.
    void (__stdcall* UpdatePlayers)(void);
};

#endif
