#ifndef MISSION_FLAGS_0042F9A0_H
#define MISSION_FLAGS_0042F9A0_H

#pragma pack(push, 1)
struct MissionFlags_0042f9a0 {
    unsigned short allMissions : 1;
    unsigned short : 15;
};
#pragma pack(pop)

#if !defined(_MSC_VER) || _MSC_VER >= 1600
static_assert(sizeof(MissionFlags_0042f9a0) == 2, "MissionFlags_0042f9a0");
#endif

#endif
