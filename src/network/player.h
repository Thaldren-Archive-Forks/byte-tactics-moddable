#ifndef PLAYER_H
#define PLAYER_H

struct PlayerInfo;
struct Unit;
class SquadManager;
struct Group;
struct UnitResources;

#pragma pack(push, 1)
struct Player {
    int active;                        // +0x00, zero when the slot is unused
    unsigned int id;                   // +0x04
    int joinTime;                      // +0x08
    int lobbyDataSynced;                       // +0x0c, a team or a 1 to 10 id: the views disagree
    int messages;                      // +0x10
    int ping;                          // +0x14
    int syncTick;                      // +0x18
    int lastHeard;                     // +0x1c
    unsigned char progress;            // +0x20
    unsigned char keepaliveFlags;      // +0x21
    unsigned char rejectReason;        // +0x22
    char unknown_23[4];
    PlayerInfo* info;                  // +0x27
    char name[0x1e];                   // +0x2b
    char fullName[0x1e];               // +0x49
    Unit* unitsBegin;                  // +0x67
    Unit* unitsEnd;                    // +0x6b
    unsigned short firstIndex;         // +0x6f
    unsigned short lastIndex;          // +0x71
    unsigned char type;                // +0x73
    SquadManager* ai;                  // +0x74
    Group* groups;                     // +0x78
    unsigned char* explored;           // +0x7c
    int exploredWidth;                 // +0x80
    int exploredHeight;                // +0x84
    int exploredSize;                  // +0x88
    union {
        float energy;
        int energyBits;
    };
    float energyIncome;                // +0x90
    float energyUsage;                 // +0x94
    union {
        float metal;
        int metalBits;
    };
    float metalIncome;                 // +0x9c
    float metalUsage;                  // +0xa0
    union {
        float energyCapacity;
        int energyCapacityBits;
    };
    union {
        float metalCapacity;
        int metalCapacityBits;
    };
    union {
        double totalEnergyProduced;
        struct {
            int totalEnergyProducedLo;
            int totalEnergyProducedHi;
        };
    };
    union {
        double totalMetalProduced;
        struct {
            int totalMetalProducedLo;
            int totalMetalProducedHi;
        };
    };
    union {
        double totalEnergyConsumed;
        struct {
            int totalEnergyConsumedLo;
            int totalEnergyConsumedHi;
        };
    };
    union {
        double totalMetalConsumed;
        struct {
            int totalMetalConsumedLo;
            int totalMetalConsumedHi;
        };
    };
    union {
        double energyWasted;
        struct {
            int energyWastedLo;
            int energyWastedHi;
        };
    };
    union {
        double metalWasted;
        struct {
            int metalWastedLo;
            int metalWastedHi;
        };
    };
    float energyStorageBonus;          // +0xdc
    float metalStorageBonus;           // +0xe0
    float shareMetal;                  // +0xe4
    float shareEnergy;                 // +0xe8
    UnitResources* econ;               // +0xec
    int updateTime;                    // +0xf0
    int winLoseTime;                   // +0xf4
    int displayTimer;                  // +0xf8
    short kills;                       // +0xfc
    short losses;                      // +0xfe
    unsigned short unused_100;         // +0x100, set to 0xffff when the slot is reset, never read
    unsigned short unused_102;         // +0x102, same
    short commanderKills;              // +0x104
    short commanderLosses;             // +0x106
    unsigned char allied[11];          // +0x108
    unsigned char alliedBy[11];        // +0x113
    unsigned char shareLos[11];        // +0x11e
    unsigned char shareVision[11];     // +0x129
    unsigned char shareMapping[11];    // +0x134
    unsigned char alliance;            // +0x13f
    int unitsCreated;                  // +0x140
    unsigned short unitCount;          // +0x144
    unsigned char index;               // +0x146
    unsigned char startPos;            // +0x147
    union {
        struct {
            unsigned char rank;
            union {
                unsigned short flags;
                struct {
                    unsigned short storage : 1;
                    unsigned short bits_149_1 : 15;
                } bits_149;
            };
        };
        struct {
            unsigned short bits0 : 8;
            unsigned short started : 1;
        } bits_148;
    };

    Player();
    void FreeSideDataAndFogSightCounts();
    void SetType(int newType);
    int IsPlayableSlot();
    void Clear() { active = 0; type = 0; }
    float GetEnergy() { return energy; }
    float GetMetal() { return metal; }
};
#pragma pack(pop)

#endif
