#ifndef SPOT_STATE_H
#define SPOT_STATE_H

#include "model_types.h"

struct Owner_459c70;
struct Bitmap_00437b50;

#pragma pack(push, 1)
struct SpotState {
    int count;
    int cacheDrawCount;
    int animDirty;
    union { Owner_459c70* owner; void* ownerPointer; };
    Bitmap_00437b50* bitmap;
    int shadow;
    Vector3s pos;
    Piece_459c70* root;
    Piece_459c70 pieces[1];
    char unknown_58[18];
    int pos_x;
    int pos_y;
    int pos_z;
};
#pragma pack(pop)

#endif
