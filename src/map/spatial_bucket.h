#ifndef SPATIAL_BUCKET_H
#define SPATIAL_BUCKET_H

struct Unit;

class SpatialBucket {
public:
    char unknown_0[6];
    void* head;
    void UnlinkUnit(Unit* node);
    void PrependUnit(int node);
};

#endif
