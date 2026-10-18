#ifndef FACE_FLAGS_459830_H
#define FACE_FLAGS_459830_H

#pragma pack(push, 1)
struct FaceFlags_459830 {
    unsigned int textured : 1;
    unsigned int usePic : 1;
    unsigned int shaded : 1;
    unsigned int rest : 29;
};
#pragma pack(pop)

#if !defined(_MSC_VER) || _MSC_VER >= 1600
static_assert(sizeof(FaceFlags_459830) == 4, "FaceFlags_459830");
#endif

#endif
