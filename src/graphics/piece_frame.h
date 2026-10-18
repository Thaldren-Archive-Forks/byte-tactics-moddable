#ifndef PIECE_FRAME_H
#define PIECE_FRAME_H

#include "face_flags_459830.h"

#pragma pack(push, 1)
struct PieceFrame {
    int unknown_0;
    int count;
    int unknown_8;
    unsigned short* indices;
    char unknown_10[8];
    unsigned short* color;
    FaceFlags_459830 flags;
};
#pragma pack(pop)

#if !defined(_MSC_VER) || _MSC_VER >= 1600
static_assert(sizeof(PieceFrame) == 32, "PieceFrame");
#endif

#endif
