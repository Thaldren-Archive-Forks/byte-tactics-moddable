#ifndef OBJECT3DO_H
#define OBJECT3DO_H

struct PieceFrame;

#pragma pack(push, 1)
struct Object3do {
    unsigned char state;
    char unknown_1[3];
    int vertexCount;
    int faceCount;
    int firstFace;
    int f10;
    int f14;
    unsigned short* color;
    char* name;
    int active;
    void* vertices;
    PieceFrame* faces;
    Object3do* sibling;
    Object3do* child;
};
#pragma pack(pop)

#if defined(__clang__) || !defined(_MSC_VER) || _MSC_VER >= 1600
static_assert(sizeof(Object3do) == 52, "Object3do");
#endif

#endif
