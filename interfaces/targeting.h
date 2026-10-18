#ifndef INTERFACES_TARGETING_H
#define INTERFACES_TARGETING_H

struct Unit;
struct Vec3;

// Major version of the targeting interface, changed when a slot is removed or its signature changes.
#define TARGETING_VERSION_MAJOR 1
// Minor version of the targeting interface, changed when a slot is added.
#define TARGETING_VERSION_MINOR 1

// Target selection for weapons.
struct TargetingSeam {
    // Aims a weapon at a unit. Hooks: none.
    void (__stdcall* SetWeaponTargetUnit)(Unit* unit, Unit* target, int weapon);
    // Aims a weapon at a ground position. Hooks: none.
    void (__stdcall* SetWeaponTargetPos)(Unit* unit, Vec3* pos, int weapon);
    // Stops a weapon aiming and tells the unit's script its target is cleared. Hooks: none.
    void (__stdcall* ClearWeaponTarget)(Unit* unit, int weapon);
    // Drops a weapon's target without telling the script. Hooks: none.
    void (__stdcall* ResetWeaponTarget)(Unit* unit, int weapon);
    // Returns the unit a weapon aims at, or null. Hooks: none.
    Unit* (__stdcall* GetWeaponTargetUnit)(Unit* unit, int weapon);
    // Writes the position a weapon aims at and returns whether it has a target. Hooks: none.
    int (__stdcall* GetWeaponTargetPos)(Unit* unit, Vec3* pos, int weapon);
    // Picks the best enemy for one weapon, within its range or the unit's sight. Hooks: FilterTargets.
    Unit* (__stdcall* FindWeaponTarget)(Unit* unit, unsigned char weapon, int useRange);
    // Picks the best enemy for a unit that fires at will, or returns null. Hooks: FilterTargets.
    Unit* (__stdcall* FindBestTargetIfFireAtWill)(Unit* unit);
    // Returns the weapon a unit fires first at its target (thiscall in 3.1c, this first). Hooks: none.
    unsigned char (__fastcall* ChooseWeapon)(Unit* unit);
};

#endif
