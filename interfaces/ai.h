#ifndef INTERFACES_AI_H
#define INTERFACES_AI_H

namespace std {
template<class T> class allocator;
template<class T, class A> class vector;
}

struct Unit;
struct Player;
struct Vec3;
class SquadManager;

// Major version of the ai interface, changed when a slot is removed or its signature changes.
#define AI_VERSION_MAJOR 1
// Minor version of the ai interface, changed when a slot is added.
#define AI_VERSION_MINOR 3

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
};

#endif
