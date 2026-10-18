// SpatialBucket: one cell of the spatial grid at g_game+0x1429f, the head of
// the list of units standing in it, linked through the units' cargoNext. The
// one declaration of the class for the files that call its methods.
#ifndef SPATIAL_BUCKET_H
#define SPATIAL_BUCKET_H

struct Unit;

class SpatialBucket {
public:
    char unknown_0[6];
    void* head;                        // +6
    void UnlinkUnit(Unit* node);
    void PrependUnit(int node);
};

#endif
