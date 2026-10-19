#ifndef MODEL_TYPES_H
#define MODEL_TYPES_H

#ifndef TA_TYPES_H
#include "../util/vec3.h"
#endif
#include "gaf_frame.h"

struct Vector3s {
    short x;                           // +0x0
    short y;                           // +0x2
    short z;                           // +0x4
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
    int unknown_0;                     // +0x00
    int count;                         // +0x04
    int unknown_8;                     // +0x08
    unsigned short* indices;           // +0x0c
    void* pic;                         // +0x10
    int unknown_14;                    // +0x14
    unsigned short* color;             // +0x18
    FaceFlags_459c70 flags;            // +0x1c
};

struct Object3doView {
    char unknown_0[4];
    int vertexCount;                   // +0x04
    int faceCount;                     // +0x08
    int firstFace;                     // +0x0c
    int box_x;                         // +0x10
    int box_y;                         // +0x14
    int box_z;                         // +0x18
    const char* name;                  // +0x1c
    char unknown_20[4];
    Vec3* points;                      // +0x24
    Face_459c70* faces;                // +0x28
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
    int rot_x;                         // +0x04
    int rot_y;                         // +0x08
    int rot_z;                         // +0x0c
    short short_10;                    // +0x10
    short short_12;                    // +0x12
    short short_14;                    // +0x14
    Vec3 offset;                       // +0x16
    Vec3* points;                      // +0x22
    short modified;                    // +0x26
    PieceFlags_459c70 flags;           // +0x28
    Piece_459c70* sibling;             // +0x2a
    Piece_459c70* child;               // +0x2e
    Piece_459c70* parent;              // +0x32
};
#pragma pack(pop)

struct UnitDef;
struct Model_459200;

#pragma pack(push, 1)
struct Unit_459200 {
    char unknown_0[0x64];
    Vector3s angles;                   // +0x64
    int pos_x;                         // +0x6a
    int pos_y;                         // +0x6e
    int pos_z;                         // +0x72
    char unknown_76[0x86 - 0x76];
    int carrier;                       // +0x86
    Unit_459200* cargo;                // +0x8a
    Unit_459200* cargoNext;            // +0x8e
    UnitDef* def;                      // +0x92
    char unknown_96[8];
    Model_459200* state;               // +0x9e
    char unknown_a2[4];
    short unitDefIndex;                // +0xa6
    unsigned short id;                 // +0xa8
    char unknown_aa[0xff - 0xaa];
    unsigned char playerIndex;         // +0xff
    char unknown_100[4];
    float buildLeft;                   // +0x104
    char unknown_108[6];
    unsigned char activateFlags;       // +0x10e
    char cobStateFlags;
    unsigned int flags;                // +0x110
    unsigned char zBufferFlag;         // +0x114
};
#pragma pack(pop)

#pragma pack(push, 2)
struct Model_459200 {
    int count;                         // +0x00
    int cacheDrawCount;                // +0x04
    int animDirty;                     // +0x08
    Unit_459200* owner;                // +0x0c
    GafFrame* bitmap;                  // +0x10
    GafFrame* shadow;                  // +0x14
    Vector3s pos;                      // +0x18
    Piece_459c70* root;                // +0x1e
    Piece_459c70 pieces[1];            // +0x22
};
#pragma pack(pop)

#endif
