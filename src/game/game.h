#ifndef GAME_H
#define GAME_H

#ifndef TA_TYPES_H
#include "../graphics/rect.h"
#include "../ingame/frame_timers.h"
#include "../map/grid.h"
#include "../network/player.h"
#include "../network/settings.h"
#include "../util/vec3.h"
#endif
#include "../frontend/bits_00426e80.h"
#include "../frontend/conn_info_443ff0.h"
#include "../frontend/flags37f14_0045cf60.h"
#include "../frontend/menu_0045b800.h"
#include "../frontend/player_entry_004777a0.h"
#include "../frontend/v4i.h"
#include "../graphics/object3do.h"
#include "../graphics/piece_frame.h"
#include "../gui/gui.h"
#include "../ingame/arg_00419670.h"
#include "../ingame/camera_flags.h"
#include "../ingame/cursor_state_0041cc60.h"
#include "../ingame/flags16_00499200.h"
#include "../ingame/flags_004197d0.h"
#include "../ingame/flags_00495e90_37ebe.h"
#include "../ingame/flags_00495e90_37f06.h"
#include "../ingame/flags_00495e90_38a51.h"
#include "../ingame/flags_00495e90_3923b.h"
#include "../ingame/map_grid.h"
#include "../ingame/orders.h"
#include "../ingame/side_name.h"
#include "../ingame/snapshot_0046a860.h"
#include "../ingame/sub_004679a0_a.h"
#include "../ingame/sub_004679a0_b.h"
#include "../map/anim_manager_00421f20.h"
#include "../map/grid2.h"
#include "../map/map_info.h"
#include "../map/player_grid.h"
#include "../map/pool.h"
#include "../map/radar_timer_00466dc0.h"
#include "../network/chat_hud_entry.h"
#include "../network/entry_00463ca0.h"
#include "../network/player_entry_0046d6c0.h"
#include "../sound/flags_0045c570.h"
#include "../units/f14373.h"
#include "../units/name_004864b0.h"
#include "../util/point.h"
#include "../weapons/debris_00420f30.h"
#include "../weapons/def_0049d270.h"
#include "clock_flags_0042f9a0.h"
#include "flags_00496ce0.h"
#include "load_flags_00497f40.h"
#include "menu.h"
#include "menu_00497ce0.h"
#include "mission_flags_0042f9a0.h"
#include "player_slots_00497f40.h"
#include "settings_00496e10.h"
#include "side_00431a60.h"
#include "side_name_00497180.h"
#include "slot_0041e420.h"
#include "view_flags_497180.h"
#include "zero11_004917d0.h"

struct AnimEntry_00429700;
struct BuildList_0041ace0;
struct Cell;
class Class_00437c80;
struct Class_0045ae80;
class Class_0046d040;
struct Class_00472200;
struct Class_0047cb00;
class Class_004cdb40;
struct Conn_443ff0;
struct Ctx_00499890;
struct Desc_004437c0;
struct Entry_0042f740;
struct Entry_00479620;
struct Eye_00482130;
struct Feature;
struct FeatureSpot;
struct Font;
struct FrameTable;
struct IconSet_00466780;
struct List_00420960;
class Mission;
class MissionConditions;
struct Net_4517b0;
struct Obj_00406f50;
class PathMap;
struct PlayerInfo;
struct SideData;
struct Src_004ab400;
struct Struct_00499200_531;
struct Unit;
struct UnitDef;

#pragma pack(push, 1)
struct Game {
    union {
        struct {
            char unknown_0;
            unsigned char field_1;
            unsigned char field_2;
            unsigned char field_3;
        };
        int field_0;
    };
    char* build_date;
    char* build_time;
    Ctx_00499890* unknown_c;
    Class_004cdb40* field_10;
    union {
        struct {
            char session[4];
            char unknown_18[492];
            Font* font;
            int colour1;
            int colour2;
            int colour3;
            char unknown_214[561];
            V4i* guids;
            char unknown_449[40];
            Settings settings;
            char unknown_4c1[8];
            int lobby2;
            int local_id;
            char unknown_4d1[12];
        };
        struct {
            char session_14_1[1];
            char unknown_15[1156];
            int field_499;
            char unknown_49d[64];
        };
        char session_14[1117];
        char net_14[1225];
    };
    char unknown_4dd[8];
    Net_4517b0* field_4e5;
    char unknown_4e9[4];
    int field_4ed;
    int netMode;
    char unknown_4f5[4];
    int count;
    int field_4fd;
    char unknown_501[16];
    Player* slowest;
    int lag;
    union {
        struct {
            char menu[1];
            char unknown_51a[3];
            void* gaf;
            int field_521;
            int field_525;
            char unknown_529[4];
            int field_52d;
            Struct_00499200_531* layer;
            char unknown_535[72];
            int team_index;
            int selected;
            char unknown_585[4];
            int field_589;
            char unknown_58d[2110];
            unsigned char textColor;
            unsigned char field_dcc;
            char unknown_dcd[1];
            unsigned char field_dce;
            unsigned char field_dcf;
            char unknown_dd0[3];
            unsigned char color2;
            unsigned char field_dd4;
            unsigned char field_dd5;
            char unknown_dd6[1];
            unsigned char field_dd7;
            char unknown_dd8[1];
            unsigned char unknown_dd9;
            unsigned char field_dda;
            char unknown_ddb[1040];
            void* surface;
            char unknown_11ef[256];
            Entry_00463ca0 entries[30];
            int field_1b5f;
            Player players[10];
            char unknown_2851[331];
            int field_299c;
        };
        struct {
            int field_519;
            char unknown_51d[2222];
            unsigned char colors[16];
            char unknown_ddb_a[1300];
            char chatHudRing[4];
            char unknown_12f3[1868];
            PlayerInfo* infos[11];
            char unknown_1a6b[287];
            SideData* p1b8a;
            char unknown_1b8e[327];
            SideData* p1cd5;
            char unknown_1cd9[281];
            short field_1df2;
            char unknown_1df4[2988];
        };
        struct {
            char menu_519[16];
            char unknown_529_a[2210];
            unsigned char colors_dcb[42];
            char unknown_df5[1274];
            char chatHudRing_12ef[30][72];
            char unknown_1b5f[4];
            Player players_1b63_14b[1];
            char unknown_1cae[3314];
        };
        struct {
            char message[24];
            char unknown_531[3559];
            ChatHudEntry entries_1318[30];
            char unknown_1b88[2];
            PlayerEntry_004777a0 players_1b8a[10];
            char unknown_2878[296];
        };
        struct {
            Menu_00497ce0 menu_519_18;
            char unknown_531_a[5686];
            PlayerEntry_0046d6c0 players_1b67[10];
            char unknown_2855[331];
        };
        struct {
            Menu_0045b800 menu_519_1c;
            char unknown_535_a[5802];
            PlayerGrid players_1bdf[10];
            char unknown_28cd[211];
        };
        struct {
            Menu gui_519;
            char unknown_559[5642];
            Player players_1b63[11];
            char unknown_299c[4];
        };
        char menu_519_8b2[2226];
        Gui gui;
        char menu_519_164a[5706];
        char message_519[5745];
        char messages[9351];
    };
    Entry_00479620* options;
    union {
        struct {
            int loaded[10];
            char unknown_29cc[4];
            int field_29d0[11];
            int field_29fc[10];
            char unknown_2a24[4];
            int field_2a28;
            char unknown_2a2c[4];
        };
        int shareVisionReady[11];
        PlayerSlots_00497f40 slots_29a4;
    };
    Class_0046d040* field_2a30;
    int field_2a34;
    unsigned char* buffer;
    unsigned short field_2a3c;
    unsigned short tail;
    unsigned short head;
    unsigned char localPlayer;
    unsigned char playerIndex;
    union {
        unsigned short flags_2a44;
        unsigned char netGameFlags;
        struct {
            unsigned char leaving : 1;
            unsigned char rest_2a44 : 7;
        } bits_2a44_2a44;
        Flags16_00499200 netGameFlags_2a44;
        struct {
            unsigned short bit0 : 1;
            unsigned short bit1 : 1;
            unsigned short bit2 : 1;
            unsigned short bit3 : 1;
            unsigned short bits4_2a44 : 12;
        } bits_2a44;
        struct {
            unsigned short pad_2a44 : 2;
            unsigned short bit2 : 1;
            unsigned short rest_2a44 : 13;
        } bits_2a44_1;
    };
    unsigned char field_2a46;
    union {
        struct {
            char unknown_2a47[4];
            void* data[15];
        };
        void* data16[16];
    };
    char unknown_2a87[20];
    void* list;
    V4i* sessions;
    Conn_443ff0* conns;
    Desc_004437c0* desc;
    char* shared;
    union {
        struct {
            unsigned short bit0 : 1;
            unsigned short bit1 : 1;
            unsigned short bits2 : 14;
        };
        Bits_00426e80 dplayAddressDialogFlags;
        unsigned short dplayAddressDialogFlags_2aaf;
    };
    union {
        struct {
            char unknown_2ab1[155];
            unsigned short low_2b4c : 4;
            unsigned short flag_2b4c : 1;
            unsigned short rest_2b4c : 11;
            char unknown_2b4e[28];
        };
        struct {
            char unknown_2ab1_a[155];
            Flags_00496ce0 flags_2b4c;
            char unknown_2b4e_a[28];
        };
        char buffer_2ab1[185];
    };
    union {
        char game_info[84];
        struct {
            char unknown_2b6a[56];
            V4i field_2ba2;
            char unknown_2bb2[12];
        };
    };
    unsigned char field_2bbe;
    unsigned char field_2bbf;
    unsigned char field_2bc0;
    union {
        struct {
            char gameName[16];
            char field_2bd1;
        };
        char gameName_2bc1[17];
    };
    char nickname[17];
    union {
        struct {
            char password[11];
            unsigned short lobbyUiDirtyFlags;
            unsigned char mode_2bf0;
            unsigned char chatRecipients[11];
            char unknown_2bfc[44];
            int table_2c28[10];
            char unknown_2c50[36];
            unsigned char lockFlags;
            char unknown_2c75[1];
            Arg_00419670 view;
            short x;
            short y;
            int boxStartX;
            int boxStartHeight;
            int boxStartZ;
            int boxEndX;
            int boxEndHeight;
            int boxEndZ;
            Vec3 pos;
            int boxStartTick;
            unsigned short hoverUnitId;
            unsigned short cellFeature;
            char cursorMode;
            char unknown_2cbf[4];
            unsigned char orderMode;
            unsigned short buildTypeIndex;
            unsigned char pad_2cc6 : 5;
            unsigned char bit5_2cc6 : 1;
            unsigned char bit6_2cc6 : 1;
            unsigned char rest_2cc6 : 1;
            CursorState_0041cc60 cursor;
            Def_0049d270 weapons[256];
            int projectileCount;
            Obj_00406f50* projectiles;
            void* field_141fb;
            void* field_141ff;
            void* field_14203;
            PathMap* pathfinder;
            FeatureSpot* spots;
            Unit* unit;
            char unknown_14213[8];
            int list_1421b;
            void** field_1421f;
            int baseX;
            int baseY;
            int mapWidth;
            int mapHeight;
            int width;
            int height;
            int screenTilesX;
            int screenTilesY;
            char unknown_14243[16];
            int featureCount;
            int scanIndex;
            int windSpeedMin;
            int windSpeedMax;
            int gravity;
            float tidal;
            void* radarFrame;
            Feature* features;
            unsigned short* visibilityMask;
            int count_14277;
            Eye_00482130* eyes;
            unsigned char seaLevel;
            unsigned char debugMode;
            unsigned short mapFlags;
            IconSet_00466780* iconSet;
            Cell* cells;
            unsigned short* mapValues;
            Grid grid1;
            Class_0047cb00* cells_1429f;
            unsigned int width_142a3;
            unsigned int height_142a7;
            char unknown_142ab[4];
            int field_142af;
            int field_142b3;
            int overflowBucket;
            Rect viewLimit;
            char field_142cb[16];
            void* finalSurface;
            void* mappedSurface;
            void* pictureSurface;
            short posX;
            short posY;
            short width_142eb;
            short screen_h;
            short blinkTimer;
            unsigned short flags_142f1;
            int value_142f3;
            Obj_00406f50* tracked;
            int xs[4];
            int ys[4];
            unsigned char valid[4];
            int scrollX;
            int scrollY;
            int x2;
            int y2;
            int value_1432f;
            int value_14333;
            int sum_x;
            int sum_y;
            Vec3 trackedPos;
            short value_1434b;
        };
        struct {
            char unknown_2be3[11];
            struct {
                unsigned short dirty : 1;
                unsigned short bits1 : 15;
            } bits_2bee_1;
            char unknown_2bf0[1];
            unsigned char chatRecipients_2bf1[10];
            char unknown_2bfb[45];
            int playerIds[11];
            char unknown_2c54[32];
            struct {
                unsigned short locked : 1;
                unsigned short locked_rest : 15;
            } bits_2c74;
            int cursorScreenX;
            int cursorScreenY;
            int cursorKeyFlags;
            char unknown_2c82[12];
            Point16 point;
            char unknown_2c92[26];
            short xWhole;
            char unknown_2cae[2];
            short yWhole;
            char unknown_2cb2[2];
            short zWhole;
            char unknown_2cb6[16];
            unsigned char inputFlags;
            char unknown_2cc7[24];
            int middleScrollActive;
            char unknown_2ce3[70952];
            Pool pool;
            char unknown_1421f[98];
            ViewFlags_497180 mapFlags_14281;
            char unknown_14283[28];
            Grid spatialGrid;
            char unknown_142af[12];
            int viewLeft;
            int viewTop;
            int viewRight;
            int viewBottom;
            char unknown_142cb[28];
            struct {
                short v[4];
            } dim;
            char unknown_142ef[2];
            unsigned char viewDirtyFlags_142f1;
            char field_142f2;
            char unknown_142f3[90];
        };
        struct {
            char unknown_2be3_a[11];
            struct {
                unsigned short dirty : 1;
            } bits_2bee_3;
            char unknown_2bf0_a[1];
            Zero11_004917d0 chatRecipients_2bf1_b;
            char unknown_2bfc_a[122];
            Point view_2c76;
            char unknown_2c7e[44];
            void* pos_2caa;
            char unknown_2cae_a[24];
            Flags_004197d0 inputFlags_2cc6;
            char unknown_2cc7_a[70964];
            AnimManager_00421f20 anim;
            char unknown_14273[14];
            struct {
                unsigned short bit0 : 1;
                unsigned short bit1 : 1;
                unsigned short flag2 : 1;
                unsigned short flag3 : 1;
                unsigned short rest : 12;
            } bits_14281_1;
            char unknown_14283_a[28];
            Grid2 spatialGrid_1429f;
            char unknown_142b7[20];
            char field_142cb_142cb[38];
            CameraFlags viewDirtyFlags;
            char unknown_142f3_a[90];
        };
        struct {
            char unknown_2be3_b[11];
            Bits_00426e80 lobbyUiDirtyFlags_2bee;
            char unknown_2bf0_b[134];
            char orders[28];
            char unknown_2c92_a[71017];
            MapInfo info_141fb;
            char unknown_14277[10];
            struct {
                unsigned short bit0_14281 : 1;
                unsigned short bit1_14281 : 1;
                unsigned short bit2_14281 : 1;
                unsigned short rest_14281 : 13;
            } bits_14281;
            char unknown_14283_b[110];
            struct {
                unsigned short blinkOn : 1;
                unsigned short rest_142f1 : 15;
            } bits_142f1;
            char unknown_142f3_b[90];
        };
        struct {
            char unknown_2be3_c[11];
            struct {
                unsigned short bits0 : 4;
                unsigned short flag4 : 1;
                unsigned short bits5 : 11;
            } bits_2bee;
            char unknown_2bf0_c[134];
            char orders_2c76[68];
            char unknown_2cba[70977];
            MapGrid sortUnits;
            char unknown_1428b[102];
            struct {
                unsigned short flags_142f1_bit0 : 1;
                unsigned short flags_142f1_bit1 : 1;
                unsigned short flags_142f1_mapChanged : 1;
                unsigned short flags_142f1_rest : 13;
            } bits_142f1_1;
            char unknown_142f3_c[90];
        };
        struct {
            char unknown_2be3_d[11];
            struct {
                unsigned short pad_2bee : 5;
                unsigned short flags_2bee : 3;
                unsigned short rest_2bee : 8;
            } bits_2bee_2;
            char unknown_2bf0_d[71425];
            struct {
                unsigned short f0 : 1;
                unsigned short f1 : 1;
                unsigned short flagA : 1;
                unsigned short f3 : 5;
                unsigned short fhi : 8;
            } bits_142f1_2;
            char unknown_142f3_d[90];
        };
        struct {
            char password_2be3[17];
            char unknown_2bf4[71419];
            RadarTimer_00466dc0 timer;
            char unknown_142f3_e[90];
        };
        char name_2be3[64];
        char password_2be3_1176a[71530];
    };
    unsigned char scrollSpeed;
    unsigned char flags_1434e;
    unsigned short unitsPerPlayer;
    unsigned short poolCount;
    int f14353;
    Unit* units;
    Unit* unitsEnd;
    unsigned short* list_1435f;
    void* hotRadar;
    int count_14367;
    int hotRadarCount;
    unsigned short focusUnitId;
    unsigned short autoFollowTicks;
    union {
        struct {
            unsigned int bit0_14373 : 1;
            unsigned int autoFollow : 1;
            unsigned int rest : 30;
        };
        F14373 autoFollowFlags;
        unsigned int autoFollowFlags_14373;
    };
    Class_0045ae80** types;
    Class_00437c80* unitTable;
    char unknown_1437f[4];
    void* xform;
    void* projected;
    void* field_1438b;
    int count_1438f;
    int unitDefCountBits;
    int unitDefEnumDirty;
    UnitDef* defs;
    char unknown_1439f[8];
    union {
        struct {
            char field_143a7[4];
            char unknown_143ab[1020];
        };
        char paletteRgba[1024];
    };
    int baseHeight;
    int count_147ab;
    AnimEntry_00429700* entries_147af;
    char unknown_147b3[8];
    void* gaf_147bb;
    void* gaf_147bf;
    void* gaf_147c3;
    void* gaf_147c7;
    void* gaf_147cb;
    void* unknown_147cf;
    void* unknown_147d3;
    int fire1;
    int alfboom1;
    void* radlogo;
    void* radlogohigh;
    void* nuclogo;
    void* src1;
    void* src2;
    void* unknown_147f3;
    union {
        struct {
            void* src3;
            int explode2;
            int explode3;
            int explode4;
            int explode5;
            int nuke1;
        };
        void* sources[6];
    };
    void* gaf_1480f;
    unsigned short* image_14813;
    unsigned short* image_14817;
    int igpaused;
    union {
        struct {
            unsigned short* sidePanelTopSeq[5];
            unsigned short* sidePanelBotSeq[5];
        };
        unsigned short* sidePanelRows[10];
    };
    unsigned short* sidePanelSideSeq[5];
    FrameTable* losTable;
    union {
        void* black[4];
        struct {
            int black1Seq;
            int black2;
            int black3;
            int black4;
        };
    };
    union {
        void* gray[4];
        struct {
            int gray1Seq;
            int gray2;
            int gray3;
            int gray4;
        };
    };
    union {
        struct {
            Src_004ab400* table[1];
            int cursorAttack;
            int cursorAirstrike;
            int cursorTooFar;
            int cursorCapture;
            int cursorDefend;
            int cursorRepair;
            int cursorPatrol;
            int cursorPickup;
            int cursorTeleport;
            int cursorRevive;
            int cursorReclamate;
            int cursorLoad;
            int cursorUnload;
            int cursorMove;
            int cursorSelect;
            int cursorFindSite;
            int cursorRed;
            int cursorGrn;
            unsigned short* cursorNormal;
            int cursorHourglass;
        };
        void* cursorModeSeq[21];
    };
    int pathIcon;
    void* logos;
    void* logos32;
    int count_148df;
    void** blocks;
    int count_148e7;
    int* data_148eb;
    int gafAnimClearedSlot;
    int fxGaf;
    int igTitles;
    int vismasks;
    int fog;
    int field_14903;
    union {
        struct {
            int array_14907[1];
            char unknown_1490b[16];
        };
        int panelTop[5];
    };
    int count_1491b;
    Debris_00420f30 debris[300];
    List_00420960* lists[3];
    void* explosionLensFrame;
    Object3do slots[300];
    union {
        char vertices[300][96];
        Vec3 vertices_1e88f[300][8];
    };
    PieceFrame frames[300][6];
    int soundCount;
    union {
        struct {
            int soundIds[1];
            char unknown_33a17[1020];
        };
        int sounds[256];
    };
    union {
        struct {
            char soundNames[1][32];
            char unknown_33e33[8160];
            char soundFiles[256][32];
        };
        char soundNames_33e13[256][32];
        char soundNames_33e13_4000[512][32];
    };
    Entry_0042f740* entries_37e13;
    int entry_count;
    int screen;
    int width_37e1f;
    int field_37e23;
    union {
        struct {
            Rect rect;
            int viewWidth;
            int viewHeight;
            Sub_004679a0_a a;
            Sub_004679a0_b b;
            int field_37e98;
            unsigned short unitIndex;
            unsigned short unitDefIndex;
            char name[30];
            unsigned short flags_37ebe;
            unsigned short moveOrder : 3;
            unsigned short cloak : 2;
            unsigned short activation : 2;
            unsigned short flags_7 : 9;
            char unknown_37ec2[2];
            unsigned int windCounter;
            int windSpeedDenominator;
        };
        struct {
            int viewCullMinX;
            int viewCullMinY;
            int viewCullMaxX;
            int viewCullMaxY;
            char unknown_37e37[89];
            int panel;
            void* sprite;
            char unknown_37e98[38];
            unsigned char ordersPanelFlags;
            char unknown_37ebf[13];
        };
        struct {
            int viewport[4];
            char unknown_37e37_a[41];
            Snapshot_0046a860 selectionInfoCache;
            char unknown_37e9c[34];
            Flags_00495e90_37ebe ordersPanelFlags_37ebe;
            char unknown_37ec0[12];
        };
        struct {
            char unknown_37e27[151];
            struct {
                unsigned short bit0_37ebe : 1;
                unsigned short pad_37ebe : 10;
                unsigned short bit11_37ebe : 1;
                unsigned short rest_37ebe : 4;
            } bits_37ebe;
            char unknown_37ec0_a[12];
        };
        struct {
            char unknown_37e27_a[151];
            struct {
                unsigned short bit0_37ebe : 1;
                unsigned short bit1_37ebe : 1;
                unsigned short bit2_37ebe : 1;
                unsigned short bit3_37ebe : 1;
                unsigned short bit4_37ebe : 1;
                unsigned short bit5_37ebe : 1;
                unsigned short bit6_37ebe : 1;
                unsigned short bit7_37ebe : 1;
                unsigned short bits8_37ebe : 3;
                unsigned short bit11_37ebe : 1;
                unsigned short rest_37ebe : 4;
            } bits_37ebe_1;
            char unknown_37ec0_b[12];
        };
        struct {
            char unknown_37e27_b[151];
            struct {
                unsigned short flags_0 : 4;
                unsigned short loaded : 1;
                unsigned short flags_5 : 11;
            } bits_37ebe_2;
            char unknown_37ec0_c[12];
        };
        struct {
            char unknown_37e27_c[151];
            struct {
                unsigned short pad_37ebe : 6;
                unsigned short bit6_37ebe : 1;
                unsigned short rest_37ebe : 9;
            } bits_37ebe_3;
            char unknown_37ec0_d[12];
        };
        struct {
            char unknown_37e27_d[151];
            struct {
                unsigned short low_37ebe : 11;
                unsigned short bit11_37ebe : 1;
                unsigned short high_37ebe : 4;
            } bits_37ebe_4;
            char unknown_37ec0_e[12];
        };
        struct {
            char unknown_37e27_e[151];
            Orders ordersPanelFlags_37ebe_6;
            char unknown_37ec4[8];
        };
        char region_37e27[165];
    };
    union {
        struct {
            int windX;
            int windY;
            int windZ;
        };
        Vec3 wind;
    };
    unsigned short windDirection;
    int windSpeed;
    float windStrength;
    int windEnabled;
    union {
        struct {
            unsigned short field_37ee6;
            unsigned short lobbyInitScratch;
            unsigned short maxUnits;
            unsigned short maxUnits_37eec;
            int difficulty;
            int flag_37ef2;
            int commanderDeath;
            int interfaceType;
            int mode;
            int screenchat;
            Flags_00495e90_37f06 flags_37f06;
            int brightness;
            int volume1;
            int volume2;
            unsigned short flags_37f14;
            unsigned char cdMode;
            unsigned char unitChat;
            unsigned char unitChatText;
            Flags_0045c570 soundFlags;
            int width_37f1b;
            int height_37f1f;
            int textScroll;
            int textLines;
            int mousespeed;
            unsigned short uiOptionFlags;
            unsigned int lobbyTimeoutSecs;
            int netSendDropPct;
        };
        struct {
            char unknown_37ee6[12];
            unsigned char side;
            char unknown_37ef3[19];
            unsigned char visualFlagsByte;
            char unknown_37f07[5];
            short volume1Word;
            char unknown_37f0e[2];
            short volume2Word;
            char unknown_37f12[2];
            unsigned char musicMode;
            unsigned char field_37f15;
            char unknown_37f16[3];
            unsigned char soundFlags_37f19;
            char unknown_37f1a[1];
            unsigned short width_37f1b_2;
            char unknown_37f1d[2];
            unsigned short height_37f1f_2;
            char unknown_37f21[14];
            unsigned char uiOptionFlags_37f2f_1;
            unsigned char matchFlags;
            char unknown_37f31[8];
        };
        struct {
            char unknown_37ee6_a[32];
            unsigned short visualFlags;
            char unknown_37f08[12];
            Flags37f14_0045cf60 flags_37f14_37f14;
            char unknown_37f16_a[3];
            unsigned short soundFlags_37f19_2;
            char unknown_37f1b[20];
            ClockFlags_0042f9a0 uiOptionFlags_37f2f;
            char unknown_37f31_a[8];
        };
        struct {
            char unknown_37ee6_b[32];
            struct {
                unsigned short bit0 : 1;
                unsigned short bit1 : 1;
                unsigned short bit2 : 1;
                unsigned short bit3 : 1;
                unsigned short bit4 : 1;
                unsigned short bit5 : 1;
                unsigned short bit6 : 1;
                unsigned short rest : 9;
            } bits_37f06;
            char unknown_37f08_a[39];
            struct {
                unsigned short b0 : 1;
                unsigned short b1 : 1;
                unsigned short b2 : 1;
                unsigned short b3 : 1;
                unsigned short b4 : 1;
                unsigned short b5 : 1;
                unsigned short b6 : 1;
                unsigned short b7 : 1;
                unsigned short b8 : 1;
                unsigned short b9 : 1;
                unsigned short flag10 : 1;
                unsigned short rest_37f2f : 5;
            } bits_37f2f;
            char unknown_37f31_b[8];
        };
        struct {
            char unknown_37ee6_c[32];
            struct {
                unsigned short b0 : 1;
                unsigned short b1 : 1;
                unsigned short b2 : 1;
                unsigned short b3 : 1;
                unsigned short b4 : 1;
                unsigned short b5 : 1;
                unsigned short b6 : 1;
                unsigned short b7 : 1;
                unsigned short b8 : 1;
                unsigned short b9 : 1;
                unsigned short b10 : 1;
                unsigned short b11 : 1;
                unsigned short b12 : 1;
                unsigned short b13 : 1;
                unsigned short b14 : 1;
                unsigned short b15 : 1;
            } bits_37f06_1;
            char unknown_37f08_b[39];
            struct {
                unsigned short pad_37f2f : 7;
                unsigned short bit7_37f2f : 1;
                unsigned short bit8_37f2f : 1;
                unsigned short bit9_37f2f : 1;
                unsigned short rest_37f2f : 6;
            } bits_37f2f_1;
            char unknown_37f31_c[8];
        };
        struct {
            char unknown_37ee6_d[73];
            struct {
                unsigned short screenBitA : 1;
                unsigned short screenBitB : 1;
            } bits_37f2f_2;
            char unknown_37f31_d[8];
        };
        char block[83];
    };
    int sides;
    union {
        struct {
            char names[1][562];
            char unknown_3816f[2248];
            unsigned int lastTick;
            int frames_38a3b;
            int scrollScale;
            int carry;
            int ticks;
            unsigned short gameSpeed;
            short effectiveGameSpeed;
            short streak;
            unsigned short otherFlags;
            char installPath[1];
            char unknown_38a54[255];
            char text_38b53[256];
            int movieCaptureIndex;
            int movieOutputRate;
            unsigned int nextMovieFrameTick;
            int valueSet;
            int valueSet_38c63;
            int unusedAfterMovieOutputRateDirty;
            char saveName[1];
            char unknown_38c6c[255];
            int pendingSaveStore;
            char progress;
            unsigned char loadProgress;
            unsigned char loadPctUnits;
            unsigned char loadPercent;
            unsigned char progress_38d73;
            unsigned char shade;
            unsigned short net_flags;
            Class_00472200* lists_38d77;
            void* moviePlayer;
            unsigned short flags_38d7f;
            int playerCount;
            FrameTimers timers;
            int profileBarsEnabled;
            Slot_0041e420 slots_38dd9[10];
            char unknown_3901d[58];
            int field_39057;
            unsigned int deadline;
            unsigned int nextTime;
            int done;
            int steps;
            int field_3906b;
            int savedPaletteBrightness;
            int skip;
            void* lastFrame;
            void* image_3907b;
            void* palette_3907f;
            void* currentPalette;
            void* desiredPalette;
            void* fadeTable;
            char glamour[256];
        };
        struct {
            char unknown_37f3d[30];
            SideName_00497180 sideNames[2];
            char unknown_383bf[1682];
            unsigned char pauseFlags;
            char unknown_38a52[1];
            char field_38a53[256];
            char unknown_38b53[280];
            char saveName_38c6b[256];
            char unknown_38d6b[4];
            unsigned char progress_38d6f[6];
            unsigned char flags_38d75;
            char unknown_38d76[9];
            MissionFlags_0042f9a0 allMissionsFlags;
            char unknown_38d81[8];
            int total;
            int values[8];
            char unknown_38dad[994];
        };
        struct {
            char unknown_37f3d_a[34];
            char startPos[2792];
            char unknown_38a47[10];
            struct {
                unsigned short bit_38a51 : 1;
            } bits_38a51;
            char imageOutputDirectory[516];
            char unknown_38c57[286];
            LoadFlags_00497f40 loadingFlags;
            char unknown_38d77[1048];
        };
        struct {
            Side_00431a60 sideDefs[5];
            char unknown_38a37[26];
            struct {
                unsigned short flag_38a51 : 1;
                unsigned short bits_38a51 : 15;
            } bits_38a51_2;
            char path[524];
            char unknown_38c5f[278];
            struct {
                unsigned short : 2;
                unsigned short net_bit2 : 1;
            } net_bits;
            char unknown_38d79[1046];
        };
        struct {
            char unknown_37f3d_b[2836];
            Flags_00495e90_38a51 flags_38a51;
            char unknown_38a53[802];
            struct {
                unsigned short : 3;
                unsigned short synced : 1;
            } netBits;
            char unknown_38d79_a[1046];
        };
        struct {
            char unknown_37f3d_c[2836];
            struct {
                unsigned short bits_38a51 : 15;
            } bits_38a51_1;
            char unknown_38a53_a[1852];
        };
        struct {
            char unknown_37f3d_d[2836];
            struct {
                unsigned short paused : 1;
                unsigned short lagging : 1;
                unsigned short faster : 1;
                unsigned short rest : 13;
            } bits_38a51_3;
            char unknown_38a53_b[1852];
        };
        struct {
            char unknown_37f3d_e[34];
            Name_004864b0 names_37f5f[8];
            char unknown_390ef[160];
        };
        struct {
            char unknown_37f3d_f[30];
            SideName sideNames_37f5b[8];
            char unknown_390eb[164];
        };
    };
    union {
        int maxStats[7];
        struct {
            int maxKills;
            int maxLosses;
            int max_26;
            int max_2a;
            int max_2e;
            int max_32;
            int maxScore;
        };
    };
    int mission;
    int won;
    int unitInfoHoverTrack;
    unsigned short unitInfoHoverUnitId;
    int unitBuilderHoverTrack;
    unsigned short unitBuilderHoverUnitId;
    int showRanges;
    int showBps;
    int buildListCount;
    BuildList_0041ace0* buildLists;
    union {
        struct {
            char field_391cf[24];
            char field_391e7;
            char field_391e8;
        };
        char campaignMissionStatus_391cf[25];
        char campaignMissionStatus[26];
    };
    Mission* net;
    MissionConditions* list_391ed;
    int mode_391f1;
    void (__stdcall *handler)(void);
    int fontComix;
    void* fontSmlFont;
    union {
        struct {
            char field_39201[1];
            char unknown_39202[15];
            int field_39211;
            int field_39215;
        };
        char providerGuid[16];
        ConnInfo_443ff0 info;
    };
    union {
        struct {
            int singleCommanderDeath;
            int singleMapping;
            int singleLineOfSight;
            int singleLOSType;
        };
        Settings_00496e10 singleSettings;
    };
    int multiCommanderDeath;
    int multiMapping;
    int multiLineOfSight;
    int multiLOSType;
    short endGameCountdown;
    union {
        unsigned short endGameFlags;
        unsigned char flagsByte_3923b;
        Flags_00495e90_3923b endGameFlags_3923b;
        struct {
            unsigned short bits0_3923b : 2;
            unsigned short flag2_3923b : 1;
            unsigned short bit3_3923b : 1;
            unsigned short flag4_3923b : 1;
            unsigned short rest_3923b : 11;
        } bits_3923b;
        struct {
            unsigned short bits_3923b : 2;
            unsigned short flag2 : 1;
            unsigned short bit3 : 1;
            unsigned short flag4 : 1;
            unsigned short flag5 : 1;
            unsigned short flag6 : 1;
            unsigned short rest_3923b : 9;
        } bits;
        struct {
            unsigned short bit0_3923b : 1;
            unsigned short rest_3923b : 15;
        } bits_3923b_1;
        struct {
            unsigned short : 2;
            unsigned short bit2_3923b : 1;
            unsigned short : 1;
            unsigned short bit4_3923b : 1;
        } bits_3923b_2;
    };
    int playMovie;
    int introMovieLoop;
    int cmdlineSkipIntroMovie;
    int restartMissionRequest;
    Game(void);
    Player* Current(void);
    Player* Current2(void);
};
#pragma pack(pop)

#if defined(__clang__) || !defined(_MSC_VER) || _MSC_VER >= 1600
#include <stddef.h>
static_assert(sizeof(Game) == 0x3924d, "Game is 0x3924d bytes");
static_assert(offsetof(Game, unknown_0) == 0x0, "unknown_0");
static_assert(offsetof(Game, field_0) == 0x0, "field_0");
static_assert(offsetof(Game, session) == 0x14, "session");
static_assert(offsetof(Game, session_14_1) == 0x14, "session_14_1");
static_assert(offsetof(Game, session_14) == 0x14, "session_14");
static_assert(offsetof(Game, net_14) == 0x14, "net_14");
static_assert(offsetof(Game, menu) == 0x519, "menu");
static_assert(offsetof(Game, field_519) == 0x519, "field_519");
static_assert(offsetof(Game, menu_519) == 0x519, "menu_519");
static_assert(offsetof(Game, message) == 0x519, "message");
static_assert(offsetof(Game, menu_519_18) == 0x519, "menu_519_18");
static_assert(offsetof(Game, menu_519_1c) == 0x519, "menu_519_1c");
static_assert(offsetof(Game, gui_519) == 0x519, "gui_519");
static_assert(offsetof(Game, menu_519_8b2) == 0x519, "menu_519_8b2");
static_assert(offsetof(Game, gui) == 0x519, "gui");
static_assert(offsetof(Game, menu_519_164a) == 0x519, "menu_519_164a");
static_assert(offsetof(Game, message_519) == 0x519, "message_519");
static_assert(offsetof(Game, messages) == 0x519, "messages");
static_assert(offsetof(Game, loaded) == 0x29a4, "loaded");
static_assert(offsetof(Game, shareVisionReady) == 0x29a4, "shareVisionReady");
static_assert(offsetof(Game, slots_29a4) == 0x29a4, "slots_29a4");
static_assert(offsetof(Game, flags_2a44) == 0x2a44, "flags_2a44");
static_assert(offsetof(Game, netGameFlags) == 0x2a44, "netGameFlags");
static_assert(offsetof(Game, bits_2a44_2a44) == 0x2a44, "bits_2a44_2a44");
static_assert(offsetof(Game, netGameFlags_2a44) == 0x2a44, "netGameFlags_2a44");
static_assert(offsetof(Game, bits_2a44) == 0x2a44, "bits_2a44");
static_assert(offsetof(Game, bits_2a44_1) == 0x2a44, "bits_2a44_1");
static_assert(offsetof(Game, data) == 0x2a4b, "data");
static_assert(offsetof(Game, data16) == 0x2a47, "data16");
static_assert(offsetof(Game, dplayAddressDialogFlags) == 0x2aaf, "dplayAddressDialogFlags");
static_assert(offsetof(Game, dplayAddressDialogFlags_2aaf) == 0x2aaf, "dplayAddressDialogFlags_2aaf");
static_assert(offsetof(Game, flags_2b4c) == 0x2b4c, "flags_2b4c");
static_assert(offsetof(Game, buffer_2ab1) == 0x2ab1, "buffer_2ab1");
static_assert(offsetof(Game, game_info) == 0x2b6a, "game_info");
static_assert(offsetof(Game, field_2ba2) == 0x2ba2, "field_2ba2");
static_assert(offsetof(Game, gameName) == 0x2bc1, "gameName");
static_assert(offsetof(Game, gameName_2bc1) == 0x2bc1, "gameName_2bc1");
static_assert(offsetof(Game, password) == 0x2be3, "password");
static_assert(offsetof(Game, bits_2bee_1) == 0x2bee, "bits_2bee_1");
static_assert(offsetof(Game, bits_2bee_3) == 0x2bee, "bits_2bee_3");
static_assert(offsetof(Game, lobbyUiDirtyFlags_2bee) == 0x2bee, "lobbyUiDirtyFlags_2bee");
static_assert(offsetof(Game, bits_2bee) == 0x2bee, "bits_2bee");
static_assert(offsetof(Game, bits_2bee_2) == 0x2bee, "bits_2bee_2");
static_assert(offsetof(Game, password_2be3) == 0x2be3, "password_2be3");
static_assert(offsetof(Game, name_2be3) == 0x2be3, "name_2be3");
static_assert(offsetof(Game, password_2be3_1176a) == 0x2be3, "password_2be3_1176a");
static_assert(offsetof(Game, autoFollowFlags) == 0x14373, "autoFollowFlags");
static_assert(offsetof(Game, autoFollowFlags_14373) == 0x14373, "autoFollowFlags_14373");
static_assert(offsetof(Game, field_143a7) == 0x143a7, "field_143a7");
static_assert(offsetof(Game, paletteRgba) == 0x143a7, "paletteRgba");
static_assert(offsetof(Game, src3) == 0x147f7, "src3");
static_assert(offsetof(Game, sources) == 0x147f7, "sources");
static_assert(offsetof(Game, sidePanelTopSeq) == 0x1481f, "sidePanelTopSeq");
static_assert(offsetof(Game, sidePanelRows) == 0x1481f, "sidePanelRows");
static_assert(offsetof(Game, black) == 0x1485f, "black");
static_assert(offsetof(Game, black1Seq) == 0x1485f, "black1Seq");
static_assert(offsetof(Game, gray) == 0x1486f, "gray");
static_assert(offsetof(Game, gray1Seq) == 0x1486f, "gray1Seq");
static_assert(offsetof(Game, table) == 0x1487f, "table");
static_assert(offsetof(Game, cursorModeSeq) == 0x1487f, "cursorModeSeq");
static_assert(offsetof(Game, array_14907) == 0x14907, "array_14907");
static_assert(offsetof(Game, panelTop) == 0x14907, "panelTop");
static_assert(offsetof(Game, vertices) == 0x1e88f, "vertices");
static_assert(offsetof(Game, vertices_1e88f) == 0x1e88f, "vertices_1e88f");
static_assert(offsetof(Game, soundIds) == 0x33a13, "soundIds");
static_assert(offsetof(Game, sounds) == 0x33a13, "sounds");
static_assert(offsetof(Game, soundNames) == 0x33e13, "soundNames");
static_assert(offsetof(Game, soundNames_33e13) == 0x33e13, "soundNames_33e13");
static_assert(offsetof(Game, soundNames_33e13_4000) == 0x33e13, "soundNames_33e13_4000");
static_assert(offsetof(Game, rect) == 0x37e27, "rect");
static_assert(offsetof(Game, viewCullMinX) == 0x37e27, "viewCullMinX");
static_assert(offsetof(Game, viewport) == 0x37e27, "viewport");
static_assert(offsetof(Game, bits_37ebe) == 0x37ebe, "bits_37ebe");
static_assert(offsetof(Game, bits_37ebe_1) == 0x37ebe, "bits_37ebe_1");
static_assert(offsetof(Game, bits_37ebe_2) == 0x37ebe, "bits_37ebe_2");
static_assert(offsetof(Game, bits_37ebe_3) == 0x37ebe, "bits_37ebe_3");
static_assert(offsetof(Game, bits_37ebe_4) == 0x37ebe, "bits_37ebe_4");
static_assert(offsetof(Game, ordersPanelFlags_37ebe_6) == 0x37ebe, "ordersPanelFlags_37ebe_6");
static_assert(offsetof(Game, region_37e27) == 0x37e27, "region_37e27");
static_assert(offsetof(Game, windX) == 0x37ecc, "windX");
static_assert(offsetof(Game, wind) == 0x37ecc, "wind");
static_assert(offsetof(Game, field_37ee6) == 0x37ee6, "field_37ee6");
static_assert(offsetof(Game, side) == 0x37ef2, "side");
static_assert(offsetof(Game, visualFlags) == 0x37f06, "visualFlags");
static_assert(offsetof(Game, bits_37f06) == 0x37f06, "bits_37f06");
static_assert(offsetof(Game, bits_37f06_1) == 0x37f06, "bits_37f06_1");
static_assert(offsetof(Game, bits_37f2f_2) == 0x37f2f, "bits_37f2f_2");
static_assert(offsetof(Game, block) == 0x37ee6, "block");
static_assert(offsetof(Game, names) == 0x37f3d, "names");
static_assert(offsetof(Game, sideNames) == 0x37f5b, "sideNames");
static_assert(offsetof(Game, startPos) == 0x37f5f, "startPos");
static_assert(offsetof(Game, sideDefs) == 0x37f3d, "sideDefs");
static_assert(offsetof(Game, flags_38a51) == 0x38a51, "flags_38a51");
static_assert(offsetof(Game, bits_38a51_1) == 0x38a51, "bits_38a51_1");
static_assert(offsetof(Game, bits_38a51_3) == 0x38a51, "bits_38a51_3");
static_assert(offsetof(Game, names_37f5f) == 0x37f5f, "names_37f5f");
static_assert(offsetof(Game, sideNames_37f5b) == 0x37f5b, "sideNames_37f5b");
static_assert(offsetof(Game, maxStats) == 0x3918f, "maxStats");
static_assert(offsetof(Game, maxKills) == 0x3918f, "maxKills");
static_assert(offsetof(Game, field_391cf) == 0x391cf, "field_391cf");
static_assert(offsetof(Game, campaignMissionStatus_391cf) == 0x391cf, "campaignMissionStatus_391cf");
static_assert(offsetof(Game, campaignMissionStatus) == 0x391cf, "campaignMissionStatus");
static_assert(offsetof(Game, field_39201) == 0x39201, "field_39201");
static_assert(offsetof(Game, providerGuid) == 0x39201, "providerGuid");
static_assert(offsetof(Game, info) == 0x39201, "info");
static_assert(offsetof(Game, singleCommanderDeath) == 0x39219, "singleCommanderDeath");
static_assert(offsetof(Game, singleSettings) == 0x39219, "singleSettings");
static_assert(offsetof(Game, endGameFlags) == 0x3923b, "endGameFlags");
static_assert(offsetof(Game, flagsByte_3923b) == 0x3923b, "flagsByte_3923b");
static_assert(offsetof(Game, endGameFlags_3923b) == 0x3923b, "endGameFlags_3923b");
static_assert(offsetof(Game, bits_3923b) == 0x3923b, "bits_3923b");
static_assert(offsetof(Game, bits) == 0x3923b, "bits");
static_assert(offsetof(Game, bits_3923b_1) == 0x3923b, "bits_3923b_1");
static_assert(offsetof(Game, bits_3923b_2) == 0x3923b, "bits_3923b_2");
#endif

#endif
