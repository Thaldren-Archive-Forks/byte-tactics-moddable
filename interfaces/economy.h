#ifndef INTERFACES_ECONOMY_H
#define INTERFACES_ECONOMY_H

struct Player;
struct Unit;
struct UnitDef;
struct UnitInfo;
class HapiBank;
class UnitResources;

// Major version of the economy interface, changed when a slot is removed or its signature changes.
#define ECONOMY_VERSION_MAJOR 1
// Minor version of the economy interface, changed when a slot is added.
#define ECONOMY_VERSION_MINOR 1

// Resource sharing between players, and the resource accounts of units.
struct EconomySeam {
    // Sends the given player's resource sharing changes to the other players.
    void (__stdcall* UpdateResourceSharing)(Player* player);
    // Returns the energy a unit type uses, or minus the energy it makes from wind or tide. Hooks: none.
    float (__stdcall* GetEnergyUse)(UnitDef* def);
    // Settles one player's units for a tick: charges their energy use and adds their income. Hooks: ResourceIncome, StatsAccumulate.
    void (__stdcall* UpdatePlayerEconomy)(Player* player);
    // Sets a metal extractor's rate from the metal under its footprint and tells its script. Hooks: none.
    void (__stdcall* UpdateMetalExtraction)(Unit* unit);
    // Clears a unit's resource account and points it at the given player (thiscall in 3.1c, this first). Hooks: none.
    void (__stdcall* Reset)(UnitResources* self, unsigned char playerIndex);
    // Writes a unit's resource account to a saved game (thiscall in 3.1c, this first). Hooks: none.
    void (__stdcall* SaveUnitAccounts)(UnitResources* self, UnitInfo* info, HapiBank* file);
    // Reads a unit's resource account from a saved game (thiscall in 3.1c, this first). Hooks: none.
    void (__stdcall* LoadUnitAccounts)(UnitResources* self, UnitInfo* info, HapiBank* file);
    // Asks for energy and returns 1 when it is not stalled (thiscall in 3.1c, this first; the account is passed again as r). Hooks: none.
    int (__stdcall* RequestEnergy)(UnitResources* self, UnitResources* r, float amount);
    // Asks for energy and metal and returns 1 when neither is stalled (thiscall in 3.1c, this first). Hooks: none.
    int (__stdcall* RequestEnergyAndMetal)(UnitResources* self, float energy, float metal);
    // Takes energy from the owner's store and returns 1, or returns 0 when there is too little (thiscall in 3.1c, this first). Hooks: none.
    int (__stdcall* SpendEnergy)(UnitResources* self, float amount);
    // Takes metal from the owner's store and returns 1, or returns 0 when there is too little (thiscall in 3.1c, this first). Hooks: none.
    int (__stdcall* SpendMetal)(UnitResources* self, float amount);
    // Takes energy and metal from the owner's store and returns 1, or returns 0 when either is short (thiscall in 3.1c, this first). Hooks: none.
    int (__stdcall* SpendEnergyAndMetal)(UnitResources* self, float energy, float metal);
};

#endif
