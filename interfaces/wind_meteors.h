#ifndef INTERFACES_WIND_METEORS_H
#define INTERFACES_WIND_METEORS_H

class HapiBank;
class MeteorParams;
class CommandArgs;

// Major version of the wind_meteors interface, changed when a slot is removed or its signature changes.
#define WIND_METEORS_VERSION_MAJOR 1
// Minor version of the wind_meteors interface, changed when a slot is added.
#define WIND_METEORS_VERSION_MINOR 1

// Wind and meteor phases of a game tick.
struct WindMeteorsSeam {
    // Advances the wind for one tick.
    void (__cdecl* UpdateWind)(void);
    // Advances the meteors for one tick.
    void (__stdcall* UpdateMeteors)(void);
    // Resets the meteor schedule and looks up the meteor weapon at match start. Hooks: none.
    void (__stdcall* InitMeteors)(void);
    // Does nothing in 3.1c, called when a match shuts down. Hooks: none.
    void (__stdcall* EmptyShutdownPreCleanup)(void);
    // Switches the meteor showers on. Hooks: none.
    void (__stdcall* EnableMeteors)(void);
    // Switches the meteor showers off. Hooks: none.
    void (__stdcall* DisableMeteors)(void);
    // Sets the meteor weapon and the shower timing from a map's parameters. Hooks: none.
    void (__stdcall* SetMeteorParams)(MeteorParams* params);
    // Fills the parameters from the default meteor data file (thiscall in 3.1c: this in ecx). Hooks: none.
    void (__fastcall* LoadMeteorDefaults)(MeteorParams* params);
    // Writes the meteor state to the "Meteor" account of a saved game. Hooks: none.
    void (__stdcall* SaveMeteors)(HapiBank* file);
    // Reads the meteor state from the "Meteor" account of a saved game. Hooks: none.
    void (__stdcall* LoadMeteors)(HapiBank* file);
    // Console command Meteor: switches the showers on or off, or starts one. Hooks: none.
    void (__stdcall* CmdMeteor)(CommandArgs* args);
};

#endif
