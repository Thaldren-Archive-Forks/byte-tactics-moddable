#ifndef INTERFACES_UNITS_H
#define INTERFACES_UNITS_H

// Major version of the units interface, changed when a slot is removed or its signature changes.
#define UNITS_VERSION_MAJOR 1
// Minor version of the units interface, changed when a slot is added.
#define UNITS_VERSION_MINOR 0

// Unit simulation.
struct UnitsSeam {
    // Advances every unit for one tick.
    void (__stdcall* UpdateAllUnits)(void);
};

#endif
