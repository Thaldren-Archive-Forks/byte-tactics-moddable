#ifndef INTERFACES_FEATURES_H
#define INTERFACES_FEATURES_H

struct Cell;
struct Feature;
struct WeaponDef;
struct Spot;
struct Rot16;
struct Unit;
struct Vec3;
struct Point16;
struct List_00421f20;
class HapiBank;

// Major version of the features interface, changed when a slot is removed or its signature changes.
#define FEATURES_VERSION_MAJOR 1
// Minor version of the features interface, changed when a slot is added.
#define FEATURES_VERSION_MINOR 1

// Map features such as wrecks and trees.
struct FeaturesSeam {
    // Advances every map feature for one tick.
    void (__stdcall* UpdateFeatures)(void);
    // Reads the feature file list from the data files at match start. Hooks: none.
    void (__stdcall* LoadFeatureFileList)(void);
    // Releases the feature file list when a match shuts down. Hooks: none.
    void (__stdcall* FreeFeatureFileList)(void);
    // Does nothing in 3.1c, called after the feature file list is read. Hooks: none.
    void (__stdcall* NopAfterFeatureEnum)(void);
    // Links each feature to its dead and burnt versions by name. Hooks: none.
    void (__stdcall* ResolveFeatureLinks)(void);
    // Prepares the pool of feature animations for a map. Hooks: none.
    void (__stdcall* InitFeatureAnimPool)(List_00421f20* list);
    // Releases the pool of feature animations. Hooks: none.
    void (__stdcall* FreeFeaturePool)(void);
    // Places the features a mission file lists. Hooks: none.
    void (__stdcall* PlaceMissionFeatures)(void);
    // Adds the metal of the features to the metal map. Hooks: none.
    void (__stdcall* StampFeatureMetal)(void);
    // Returns the feature number on a cell, or a code for none or a part of a larger footprint. Hooks: none.
    unsigned short (__stdcall* GetCellFeature)(Cell* cell);
    // Puts a feature on a cell and returns its spot, or null when it keeps none. Hooks: none.
    Spot* (__stdcall* PlaceFeature)(Cell* cell, unsigned short feature, Vec3* pos, Rot16* rot, unsigned char owner);
    // Removes the feature on a cell, or on the cell its footprint starts from. Hooks: none.
    int (__stdcall* RemoveFeature)(Cell* cell, int flag);
    // Starts a unit reclaiming the feature at a position and returns whether it did. Hooks: none.
    int (__stdcall* ReclaimFeature)(Unit* unit, Vec3* pos);
    // Returns the number of a feature type by name, loading it when it is new. Hooks: none.
    unsigned short (__stdcall* FindOrLoadFeatureType)(char* name);
    // Damages the feature near a cell with a weapon's area damage. Hooks: none.
    void (__stdcall* DamageFeature)(Cell* cell, int x, int z, WeaponDef* weapon);
    // Returns the world position of the centre of a feature's footprint. Hooks: none.
    Vec3 (__stdcall* GetFootprintCentre)(Point16* cell, Feature* def);
    // Writes the features to a saved game. Hooks: none.
    void (__stdcall* SaveFeatures)(HapiBank* file);
    // Reads the features from a saved game. Hooks: none.
    void (__stdcall* LoadFeatures)(HapiBank* file);
};

#endif
