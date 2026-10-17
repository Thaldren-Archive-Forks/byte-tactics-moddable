#ifndef INTERFACES_FEATURES_H
#define INTERFACES_FEATURES_H

// Major version of the features interface, changed when a slot is removed or its signature changes.
#define FEATURES_VERSION_MAJOR 1
// Minor version of the features interface, changed when a slot is added.
#define FEATURES_VERSION_MINOR 0

// Map features such as wrecks and trees.
struct FeaturesSeam {
    // Advances every map feature for one tick.
    void (__stdcall* UpdateFeatures)(void);
};

#endif
