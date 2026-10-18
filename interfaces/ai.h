#ifndef INTERFACES_AI_H
#define INTERFACES_AI_H

namespace std {
template<class T> class allocator;
template<class T, class A> class vector;
}

struct Unit;
struct Vec3;

// Major version of the ai interface, changed when a slot is removed or its signature changes.
#define AI_VERSION_MAJOR 1
// Minor version of the ai interface, changed when a slot is added.
#define AI_VERSION_MINOR 1

// Computer player decisions.
struct AiSeam {
    // Appends to out the enemies a player sees within a radius of a position. Hooks: none.
    void (__stdcall* GetVisibleEnemiesInRadius)(int player, const Vec3* pos, int radius, int flags, std::vector<Unit*, std::allocator<Unit*> >* out);
};

#endif
