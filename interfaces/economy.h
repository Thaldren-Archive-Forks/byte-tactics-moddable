#ifndef INTERFACES_ECONOMY_H
#define INTERFACES_ECONOMY_H

struct Player;

// Major version of the economy interface, changed when a slot is removed or its signature changes.
#define ECONOMY_VERSION_MAJOR 1
// Minor version of the economy interface, changed when a slot is added.
#define ECONOMY_VERSION_MINOR 0

// Resource sharing between players.
struct EconomySeam {
    // Sends the given player's resource sharing changes to the other players.
    void (__stdcall* UpdateResourceSharing)(Player* player);
};

#endif
