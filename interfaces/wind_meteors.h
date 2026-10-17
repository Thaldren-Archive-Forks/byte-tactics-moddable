#ifndef INTERFACES_WIND_METEORS_H
#define INTERFACES_WIND_METEORS_H

// Major version of the wind_meteors interface, changed when a slot is removed or its signature changes.
#define WIND_METEORS_VERSION_MAJOR 1
// Minor version of the wind_meteors interface, changed when a slot is added.
#define WIND_METEORS_VERSION_MINOR 0

// Wind and meteor phases of a game tick.
struct WindMeteorsSeam {
    // Advances the wind for one tick.
    void (__cdecl* UpdateWind)(void);
    // Advances the meteors for one tick.
    void (__stdcall* UpdateMeteors)(void);
};

#endif
