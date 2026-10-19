// Decompiled by DeepSeek V4.1 Flash, deepseek-v4.1-flash, Opus, Sonnet and Haiku. Names are provisional.

#pragma pack(push, 1)
struct Rect_0041d0f0 {
    int x;                             // +0x0
    int y;                             // +0x4
    int unknown_8[4];
};

struct Entry_0041d1f0 {
    int type;                          // +0x0
    int id;                            // +0x4
    short x;                           // +0x8
    short z;                           // +0xa
};

class Class_0041d1f0_net {
public:
    char unknown_0[0xdb4];
    Entry_0041d1f0* entries;           // +0xdb4
    int entry_count;                   // +0xdb8
};

#include "../game/game.h"
#pragma pack(pop)

#include "../util/hapi_bank.h"

// GLOBAL: 0x511de8
extern Game* g_game;

extern char g_cameraAccount[]; // "Camera"
extern char g_cameraZPosition[]; // "Z Position"
extern char g_cameraXPosition[]; // "X Position"

extern int g_commandLineUnusedFlagL;
extern int g_cdBypassDriveScan;

void ClampCameraPosition(void);

static inline void SetPos(int x, int y)
{
    g_game->scrollX = x;
    g_game->scrollY = y;
}

// Centres the camera on the screen position stored at +0x2c76, then clamps it.
// FUNCTION: 0x41d0f0
void CenterCameraOnRadarClick()
{
    Rect_0041d0f0 r = (*(Rect_0041d0f0*)&g_game->view);
    int y = g_game->mapHeight * (r.y - g_game->posY) / g_game->screen_h - g_game->viewHeight / 2;
    int x = g_game->mapWidth * (r.x - g_game->posX) / g_game->width_142eb - g_game->viewWidth / 2;
    g_game->scrollX = x;
    g_game->scrollY = y;
    g_game->viewDirtyFlags_142f1 |= 2;
    ClampCameraPosition();
    g_game->x2 = g_game->scrollX;
    g_game->y2 = g_game->scrollY;
    g_game->mapFlags &= 0xfff7;
    g_game->value_1434b = 0;
    g_game->value_142f3 = 0;
    (*(int*)&g_game->tracked) = 0;
}

// FUNCTION: 0x41d1f0
void CenterCameraOnStartPosition()
{
    int i = 0;
    Entry_0041d1f0* e = ((Class_0041d1f0_net *)g_game->net)->entries;
    int count = ((Class_0041d1f0_net *)g_game->net)->entry_count;
    for (; i < count; i++, e++) {
        if (e->type == 1 && e->id == 0) {
            SetPos(e->x - g_game->viewWidth / 2, e->z - g_game->viewHeight / 2);
            g_game->viewDirtyFlags_142f1 |= 2;
            ClampCameraPosition();
            g_game->x2 = g_game->scrollX;
            g_game->y2 = g_game->scrollY;
            g_game->mapFlags &= 0xfff7;
            return;
        }
    }
}

// Reads the camera X and Z positions from the "Camera" section of a parsed
// text file (defaults to the current position), then clamps the view.
// FUNCTION: 0x41d2b0
void __stdcall ReadCameraPosition(HapiBank* file)
{
    file->OpenAccount(g_cameraAccount);
    int z = file->GetIntegerItem(g_cameraZPosition, g_game->scrollY);
    int x = file->GetIntegerItem(g_cameraXPosition, g_game->scrollX);
    g_game->scrollX = x;
    g_game->scrollY = z;
    g_game->viewDirtyFlags_142f1 |= 2;
    ClampCameraPosition();
    g_game->x2 = g_game->scrollX;
    g_game->y2 = g_game->scrollY;
    g_game->mapFlags &= 0xfff7;
}

// Writes the camera position to a section ("Camera", "X Position", "Z Position").
// FUNCTION: 0x41d360
void __stdcall WriteCameraPosition(HapiBank* file)
{
    file->OpenAccount(g_cameraAccount);
    file->SetIntegerItem(g_cameraXPosition, g_game->scrollX);
    file->SetIntegerItem(g_cameraZPosition, g_game->scrollY);
}

// FUNCTION: 0x41d3b0
void __stdcall SaveCameraPosition(int slot)
{
    g_game->xs[slot] = g_game->scrollX;
    g_game->ys[slot] = g_game->scrollY;
    g_game->valid[slot] = 1;
}

// FUNCTION: 0x41d3f0
void __stdcall RestoreCameraPosition(int index)
{
    if (g_game->valid != 0) {
        g_game->value_1434b = 0;
        g_game->value_142f3 = 0;
        (*(int*)&g_game->tracked) = 0;
        SetPos(g_game->xs[index], g_game->ys[index]);
        g_game->viewDirtyFlags_142f1 |= 2;
        ClampCameraPosition();
        g_game->x2 = g_game->scrollX;
        g_game->y2 = g_game->scrollY;
        g_game->mapFlags &= 0xfff7;
    }
}

// FUNCTION: 0x41d4a0
void __stdcall SetCommandLineUnusedFlagL(int val)
{
    g_commandLineUnusedFlagL = val;
}

// FUNCTION: 0x41d4b0
void __stdcall SetBypassDriveScan(int val)
{
    g_cdBypassDriveScan = val;
}
