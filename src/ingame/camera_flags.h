#ifndef CAMERA_FLAGS_H
#define CAMERA_FLAGS_H

#pragma pack(push, 1)
union CameraFlags {
    unsigned short raw;
    unsigned char lo;
};
#pragma pack(pop)

#if defined(__clang__) || !defined(_MSC_VER) || _MSC_VER >= 1600
static_assert(sizeof(CameraFlags) == 2, "CameraFlags");
#endif

#endif
