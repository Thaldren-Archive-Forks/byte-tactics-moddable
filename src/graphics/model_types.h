#ifndef MODEL_TYPES_H
#define MODEL_TYPES_H

#include "../util/vec3.h"
#include "gaf_frame.h"

struct Vector3s {
    short x;
    short y;
    short z;
};

struct FaceFlags_459c70 {
    union {
        unsigned int raw;
        struct {
            unsigned int textured : 1;
            unsigned int usePic : 1;
            unsigned int shaded : 1;
            unsigned int rest : 29;
        } bits;
    };
};

struct Face_459c70 {
    int unknown_0;
    int count;
    int unknown_8;
    unsigned short* indices;
    void* pic;
    int unknown_14;
    unsigned short* color;
    FaceFlags_459c70 flags;
};

struct Object3doView {
    char unknown_0[4];
    int vertexCount;
    int faceCount;
    int firstFace;
    int box_x;
    int box_y;
    int box_z;
    const char* name;
    char unknown_20[4];
    Vec3* points;
    Face_459c70* faces;
    Object3doView* sibling;
    Object3doView* child;
};

#pragma pack(push, 2)
union PieceFlags_459c70 {
    unsigned short word;
    struct {
        unsigned short visible : 1;
        unsigned short colored : 1;
        unsigned short lit : 1;
        unsigned short rest : 13;
    } bits;
};

struct Piece_459c70 {
    Object3doView* object;
    int rot_x;
    int rot_y;
    int rot_z;
    short short_10;
    short short_12;
    short short_14;
    Vec3 offset;
    Vec3* points;
    short modified;
    PieceFlags_459c70 flags;
    Piece_459c70* sibling;
    Piece_459c70* child;
    Piece_459c70* parent;
};
#pragma pack(pop)

struct UnitDef;
struct Model_459200;

#pragma pack(push, 1)
struct Unit_459200 {
    void* motion;
    char unknown_4[0x60];
    Vector3s angles;
    int pos_x;
    int pos_y;
    int pos_z;
    char unknown_76[0x86 - 0x76];
    int carrier;
    Unit_459200* cargo;
    Unit_459200* cargoNext;
    UnitDef* def;
    char unknown_96[8];
    Model_459200* state;
    char unknown_a2[4];
    short unitDefIndex;
    unsigned short id;
    char unknown_aa[0xff - 0xaa];
    unsigned char playerIndex;
    char unknown_100[4];
    float buildLeft;
    char unknown_108[6];
    unsigned char activateFlags;
    char cobStateFlags;
    unsigned int flags;
    unsigned char zBufferFlag;
};
#pragma pack(pop)

#pragma pack(push, 2)
struct Model_459200 {
    int count;
    int cacheDrawCount;
    int animDirty;
    Unit_459200* owner;
    GafFrame* bitmap;
    GafFrame* shadow;
    Vector3s pos;
    Piece_459c70* root;
    Piece_459c70 pieces[1];
};
#pragma pack(pop)

#pragma pack(push, 1)
struct Fixed_459200 {
    unsigned short frac;
    short whole;
};

struct Pos_459200 {
    Fixed_459200 x;
    Fixed_459200 y;
    Fixed_459200 z;
};

struct Ints_459200 {
    int x;
    int y;
    int z;
};

union Vec3_459200 {
    int v[3];
    Pos_459200 p;
    Ints_459200 i;
};
#pragma pack(pop)

#endif
