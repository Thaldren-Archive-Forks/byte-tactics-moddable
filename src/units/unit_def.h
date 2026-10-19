#ifndef UNIT_DEF_H
#define UNIT_DEF_H

#pragma pack(push, 1)

struct WeaponDef;

class UnitDef {
public:
    char name[0x20];                   // +0x000
    char unitname[0x20];               // +0x020
    char description[0x40];            // +0x040
    char objectname[0x20];             // +0x080
    char side[0x1e];                   // +0x0a0
    char aiWeight[0x40];               // +0x0be, the FBI's ai_weight
    char aiLimit[0x40];                // +0x0fe, the FBI's ai_limit
    unsigned int fbiChecksum;          // +0x13e
    int scriptChecksum;                // +0x142
    int weaponHashXor;                 // +0x146
    union {
        int f14a;                      // +0x14a
        struct {
            short footprintx;          // +0x14a
            short footprintz;          // +0x14c
        };
    };
    union {
        int f14e;                      // +0x14e
        char* yardmap;                 // +0x14e
    };
    int count;                         // +0x152
    // The unit type ids it can build; vtol_orders_414380.cpp tests it as
    // canBuild, but every indexing read is unsigned short.
    unsigned short* ids;               // +0x156
    int maxThisUnit;                   // +0x15a
    // The model bounding box (nModelMinX..nModelSizeZ in Thaldren's UnitDef);
    // each file that reads it as a Vec3 casts at the use.
    int modelMinX;                     // +0x15e
    int modelMinY;                     // +0x162
    int modelMinZ;                     // +0x166
    int modelMaxX;                     // +0x16a
    union {
        int modelMaxY;                 // +0x16e
        struct {
            short unknown_16e;
            short field_170;           // +0x170
        };
    };
    int modelMaxZ;                     // +0x172
    int modelSizeX;                    // +0x176
    int modelSizeY;                    // +0x17a
    union {
        int modelSizeZ;
        struct {
            short unknown_17e;
            short height180;
        };
    };
    union {
        int radius;
        struct {
            short unknown_182;
            short radius184;
        };
    };
    float energyCost;                  // +0x186
    float metalCost;                   // +0x18a
    void* data;                        // +0x18e
    int maxvelocity;                   // +0x192
    int maxslopevelocity;              // +0x196
    int brakerate;                     // +0x19a
    int acceleration;                  // +0x19e
    int bankscale;                     // +0x1a2
    int pitchscale;                    // +0x1a6
    int damagemodifier;                // +0x1aa
    int moverate1;                     // +0x1ae
    int moverate2;                     // +0x1b2
    void* movementclass;               // +0x1b6
    short turnrate;                    // +0x1ba
    short corpse;                      // +0x1bc
    short maxwaterdepth;               // +0x1be
    short minwaterdepth;               // +0x1c0
    float energymake;                  // +0x1c2
    float energyuse;                   // +0x1c6
    float metalmake;                   // +0x1ca
    float extractsMetal;               // +0x1ce
    float windgenerator;               // +0x1d2
    float tidalgenerator;              // +0x1d6
    float cloakcost;                   // +0x1da
    float cloakcostmoving;             // +0x1de
    float energystorage;               // +0x1e2
    float metalstorage;                // +0x1e6
    int buildtime;                     // +0x1ea
    // The three weapon type pointers (0x115 bytes each); the loader stores what
    // FindWeaponByName returns.
    WeaponDef* weapons[3];             // +0x1ee
    unsigned int maxHealth;            // +0x1fa
    unsigned short buildRate;          // +0x1fe
    short healtime;                    // +0x200
    short range;                       // +0x202
    short radardistance;               // +0x204
    short sonardistance;               // +0x206
    short mincloakdistance;            // +0x208
    short radardistancejam;            // +0x20a
    short sonardistancejam;            // +0x20c
    short soundcategory;               // +0x20e
    short buildangle;                  // +0x210
    unsigned short buildRange;         // +0x212
    short maneuverleashlength;         // +0x214
    unsigned short attackrunlength;    // +0x216
    short kamikazedistance;            // +0x218
    short sortbias;                    // +0x21a
    short altitude;                    // +0x21c
    unsigned short id;                 // +0x21e
    char* explodeas;                   // +0x220
    char* selfdestructas;              // +0x224
    unsigned char maxslope;            // +0x228
    unsigned char maxwaterslope;       // +0x229
    unsigned char capacity;            // +0x22a
    char transportcapacity;            // +0x22b
    unsigned char draft;               // +0x22c
    unsigned char makesMetal;          // +0x22d
    unsigned char buildMenuPageCount;  // +0x22e
    unsigned char mobile;              // +0x22f
    char defaultmissiontype;           // +0x230
    unsigned int* weaponCategories[3]; // +0x231
    unsigned int* categories;          // +0x23d
    union {
        unsigned int flags1;
        unsigned char flags1Byte;
        struct {
            unsigned int b00 : 2;
            unsigned int b02 : 2;
            unsigned int initCloaked : 1;
            unsigned int downloadable : 1;
            unsigned int canbuild : 1;
            unsigned int b07 : 1;
            unsigned int b08 : 1;
            unsigned int b09 : 1;
            unsigned int b10 : 1;
            unsigned int flying : 1;
            unsigned int b12 : 1;
            unsigned int b13 : 1;
            unsigned int b14 : 1;
            unsigned int b15 : 1;
            unsigned int b16 : 1;
            unsigned int b17 : 1;
            unsigned int b18 : 1;
            unsigned int b19 : 1;
            unsigned int b20 : 1;
            unsigned int b21 : 1;
            unsigned int b22 : 1;
            unsigned int b23 : 1;
            unsigned int b24 : 1;
            unsigned int b25 : 1;
            unsigned int b26 : 1;
            unsigned int b27 : 1;
            unsigned int b28 : 1;
            unsigned int b29 : 1;
            unsigned int b30 : 1;
            unsigned int gui : 1;
        };
    };
    union {
        unsigned int flags2;
        unsigned char flags2Byte;
        struct {
            unsigned int c00 : 1;
            unsigned int c01 : 1;
            unsigned int c02 : 1;
            unsigned int c03 : 1;
            unsigned int c04 : 1;
            unsigned int c05 : 1;
            unsigned int c06 : 1;
            unsigned int c07 : 1;
            unsigned int c08 : 1;
            unsigned int c09 : 1;
            unsigned int c10 : 1;
            unsigned int c11 : 1;
            unsigned int capture : 1;
            unsigned int c13 : 1;
            unsigned int c14 : 1;
            unsigned int norestrict : 1;
            unsigned int wacky : 1;
            unsigned int c17 : 1;
            unsigned int c18 : 1;
            unsigned int c19 : 1;
            unsigned int selfdestructcountdown : 3;
        };
    };

    UnitDef& operator=(const UnitDef& src);
    void AddToCategories(char* category);
    int HasField1ce() { return extractsMetal != 0.0f; }
    float EnergyCost() { return (float)(energyCost * 30); }
    float MetalCost() { return (float)(metalCost * 30); }
    unsigned int MaxHealth() { return maxHealth; }
};

#pragma pack(pop)

#endif
