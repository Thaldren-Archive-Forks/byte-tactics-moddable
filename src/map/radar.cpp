// Decompiled by space-bunny-free, deepseek-v4.1-flash, Opus, Claude Opus 5.5, GPT-6, GPT-6.1-sol and mimo-v2.6-pro. Names are provisional.
// The radar: builds the minimap picture from the terrain icon map, keeps its
// view and zoom, and draws the units, projectiles and fog over it.
//
// <stdio.h>, <math.h> and <ddraw.h> stay: their symbol ids decide operand order
// in 0x466780, 0x466b70 and 0x466dc0.
#include <windows.h>
#include <stdio.h>
#include <math.h>
#include <ddraw.h>

struct Point_4665d0 {
    int x;
    int y;
};

struct Quad_4665d0 {
    Point_4665d0 p[4];
};

struct Surface_4665d0 {
    int width;                          // +0x0
    int height;                         // +0x4
    int pitch;                          // +0x8
    int bits;                           // +0xc
    int zPriority;                      // +0x10
    int colorKey;                       // +0x14
    unsigned short x;                   // +0x18
    unsigned short y;                   // +0x1a
    char unknown_1c[0x10];
    unsigned int flag0 : 1;             // +0x2c
    unsigned int flag1 : 1;
};

#include "../graphics/gaf_frame.h"

#pragma pack(push, 1)

struct IconSet_00466780 {
    int count;                      // +0x0
    unsigned char (*cell)[32][32];  // +0x4
};

struct Fx_00466c20 {
    char unknown_0[0xcc];
    unsigned char* colorMap;         // +0xcc
};

struct WeaponDef;

struct Flags241_00466dc0 {
    unsigned int :29;
    unsigned int bit29 : 1;
    unsigned int :2;
};

union Flags14281_00466dc0 {
    unsigned short all;
    struct {
        unsigned short bit0 : 1;
        unsigned short bit1 : 1;
        unsigned short :7;
        unsigned short bit9 : 1;
        unsigned short :5;
    } bits;
};

#include "../network/player_info.h"

#include "../network/player.h"

#include "../weapons/weapon_def.h"

// These headers' symbol ids replace those of seven declarations and a type that
// were here to balance the ids (docs/c2-regalloc.md).
#include "../util/angles.h"
#include "../weapons/unit_weapon_slot.h"

// A partial view of UnitDef (units/unit_def.h) with flags1 as a bit field, which the header lacks.
struct UnitType_00466dc0 {
    char unknown_0[0x204];
    short radardistance;                 // +0x204
    short sonardistance;                 // +0x206
    char unknown_208[0x20a - 0x208];
    short radardistancejam;              // +0x20a
    short sonardistancejam;              // +0x20c
    char unknown_20e[0x241 - 0x20e];
    Flags241_00466dc0 flags1;           // +0x241
    unsigned char flags2;                // +0x245
};

#include "../units/unit.h"
#include "../weapons/projectile.h"

// The four shorts at +0x142e7 are named minimapGadgetX/Y/W/H, originX/
// originY/sizeX/sizeY and the four-element `dim` the rectangle test indexes,
// depending on the function; the union in Game keeps every spelling addressable.

#include "../game/game.h"
#pragma pack(pop)

extern Game* g_game;
extern char g_radarPictureName[];
extern char g_radarPicTempName[];
extern char g_radarFinalName[];
extern char g_radarMappedName[];

void* __stdcall AllocFrame(char* name, int width, int height);
void __stdcall SurfaceFromFrame(Surface_4665d0* surface, void* pic);
void __stdcall DrawFrame(void* surface, void* frame, int x, int y);
void __stdcall FillSurface(Surface_4665d0* surface, int mode);
void __stdcall DrawFrameQuad(Surface_4665d0* surface, void* pic, Quad_4665d0* dst, Quad_4665d0* src);
void __cdecl GameFreeThunk(void* pic);

void __stdcall FrameFromSurface(GafFrame* dst, void* src);
void __stdcall DownsampleFrame(GafFrame* dst, GafFrame* src);
void __stdcall DrawPixel(void* picture, int x, int y, int pixel);
void* __stdcall AllocSurface(char* name, int width, int height);
void __stdcall FreeSurface(void* picture);

void __stdcall DrawRectangle(void* param_1, void* param_2, int param_3);
void __stdcall DrawSurface(void* dst, void* bmp, int x, int y);

void* __stdcall GetGafFrame(void* a, int index);
void __stdcall DrawCircle(void* surface, int x, int y, int radius, int color);
void __stdcall DrawDashedCircle(void* surface, int x, int y, int radius, int color,
                            int a6, int a7);

// Rescales a radar picture. The picture is first copied into a temp bitmap of
// its own size, then p is given the new size (x, y) and used as the surface to
// draw on, and the copy is blitted into it, scaled to fit (w - 0x20) by
// (h - 0x80) and centred: when the box is wider than tall the height is the
// scaled one and it is centred, otherwise the width is.

// FUNCTION: 0x4665d0
void __stdcall ResizeRadarPicture(GafFrame* pic, int x, int y, int w, int h)
{
    if (pic == 0) {
        return;
    }
    int dwx;
    int dhy;
    int sw;
    int sh;
    Quad_4665d0 src;
    Quad_4665d0 dst;
    Surface_4665d0 surface;
    int dw = w - 0x20;
    int dh = h - 0x80;
    int ox;
    int oy;
    if (dw >= dh) {
        dwx = x;
        dhy = dh * y / dw;
        sw = pic->width;
        sh = dh * pic->height / dw;
        ox = 0;
        oy = (y - dhy) / 2;
    } else {
        dwx = dw * x / dh;
        dhy = y;
        sw = dw * pic->width / dh;
        sh = pic->height;
        ox = (x - dwx) / 2;
        oy = 0;
    }
    void* temp = AllocFrame("TEMP RADAR PIC", pic->width, pic->height);
    SurfaceFromFrame(&surface, temp);
    DrawFrame(&surface, (short*)pic, 0, 0);
    pic->width = x;
    pic->height = y;
    SurfaceFromFrame(&surface, pic);
    FillSurface(&surface, 0);

    src.p[0].x = 0;
    src.p[0].y = 0;
    src.p[1].x = sw - 1;
    src.p[1].y = 0;
    src.p[2].x = sw - 1;
    src.p[2].y = sh - 1;
    src.p[3].x = 0;
    src.p[3].y = sh - 1;

    dst.p[0].x = ox;
    dst.p[0].y = oy;
    dst.p[1].x = ox + dwx;
    dst.p[1].y = oy;
    dst.p[2].x = ox + dwx;
    dst.p[2].y = oy + dhy;
    dst.p[3].x = ox;
    dst.p[3].y = oy + dhy;

    DrawFrameQuad(&surface, temp, &dst, &src);
    GameFreeThunk(temp);
}

// Rebuilds the radar picture: fits the map onto 126 pixels along its long side,
// then blits it onto the stored radar frame, or, when there is none, scales the
// 8x8 icon map up to twice that size through a temporary picture.
// FUNCTION: 0x466780
void __stdcall BuildRadarPicture()
{
    int mapWidth = g_game->mapWidth;
    int mapHeight = g_game->mapHeight;
    int width;
    int height;
    if (mapWidth >= mapHeight) {
        width = 126;
        height = mapHeight * 126 / mapWidth;
        g_game->posX = 0;
        g_game->posY = (126 - height) / 2;
    } else {
        width = mapWidth * 126 / mapHeight;
        height = 126;
        g_game->posX = (126 - width) / 2;
        g_game->posY = 0;
    }
    g_game->width_142eb = width;
    g_game->screen_h = height;
    g_game->pictureSurface = AllocSurface(g_radarPictureName, width, height);
    GafFrame frame;
    FrameFromSurface(&frame, g_game->pictureSurface);
    if ((GafFrame *)g_game->radarFrame) {
        DownsampleFrame((GafFrame *)g_game->radarFrame, &frame);
        return;
    }
    int h2 = height * 2;
    int w2 = width * 2;
    void* temp = AllocSurface(g_radarPicTempName, w2, h2);
    for (int j = 0; j < h2; j++) {
        for (int i = 0; i < w2; i++) {
            int x = g_game->mapWidth * i / w2;
            int y = g_game->mapHeight * j / h2;
            int index = (y / 32) * (g_game->width / 2) + x / 32;
            unsigned short value = g_game->mapValues[index];
            if (value >= g_game->iconSet->count) {
                value = 0;
            }
            int pixel = g_game->iconSet->cell[value][y % 32][x % 32];
            DrawPixel(temp, i, j, pixel);
        }
    }
    GafFrame tempFrame;
    FrameFromSurface(&tempFrame, temp);
    DownsampleFrame(&tempFrame, &frame);
    FreeSurface(temp);
}

// The two rectangle edges are `size + pos - 1`.
// FUNCTION: 0x4669b0
void InitRadar()
{
    BuildRadarPicture();
    g_game->finalSurface = AllocSurface(g_radarFinalName, g_game->dim.v[2], g_game->dim.v[3]);
    g_game->mappedSurface = AllocSurface(g_radarMappedName, g_game->dim.v[2], g_game->dim.v[3]);
    g_game->viewLeft = g_game->dim.v[0];
    g_game->viewTop = g_game->dim.v[1];
    // viewRight indexes the four radar shorts, viewBottom goes through a char*:
    // the shape of each address decides the load order.
    g_game->viewRight = g_game->dim.v[2] + g_game->dim.v[0] - 1;
    {
        char* c = (char*)g_game;
        g_game->viewBottom = *(short*)(c + 0x142ed) + *(short*)(c + 0x142e9) - 1;
    }
    g_game->timer.word.flags.mapChanged = 1;
    g_game->timer.word.blinkTimer = 7;
    g_game->timer.word.flags.blinkOn = 0;
}

// FUNCTION: 0x466aa0
void FreeRadar()
{
    FreeSurface(g_game->pictureSurface);
    FreeSurface(g_game->mappedSurface);
    FreeSurface(g_game->finalSurface);
    g_game->pictureSurface = 0;
    g_game->mappedSurface = 0;
    g_game->finalSurface = 0;
}

// FUNCTION: 0x466b00
void __stdcall DrawRadar(void* param_1)
{
    if (g_game->timer.word.flags.pending) {
        g_game->timer.word.flags.pending = 0;
        DrawSurface(param_1, g_game->finalSurface, g_game->posX, g_game->posY);
        DrawRectangle(param_1, g_game->field_142cb, g_game->unknown_dd9);
    }
}

// FUNCTION: 0x466b70
void __stdcall CalcRadarViewportRect(int* param_1)
{
    param_1[0] = g_game->width_142eb * g_game->scrollX / g_game->mapWidth + g_game->posX;
    param_1[1] = g_game->screen_h * g_game->scrollY / g_game->mapHeight + g_game->posY;
    param_1[2] = g_game->width_142eb * g_game->screenTilesX * 16 / g_game->mapWidth + param_1[0] - 1;
    param_1[3] = g_game->screen_h * g_game->screenTilesY * 16 / g_game->mapHeight + param_1[1] - 1;
}

// FUNCTION: 0x466c20
void UpdateRadarMapped()
{
    if (g_game->timer.word.flags.mapChanged) {
        g_game->timer.word.flags.mapChanged = 0;
        unsigned char fog = g_game->textColor;
        Player* t = &g_game->players_1b63_14b[g_game->playerIndex];
        unsigned int mask = 1 << g_game->playerIndex;
        unsigned char* dst = *(unsigned char**)((char*)g_game->mappedSurface + 0xc);
        unsigned char* src = *(unsigned char**)((char*)g_game->pictureSurface + 0xc);
        int halfWidth = g_game->width / 2;
        int halfHeight = g_game->height / 2;
        for (int i = 0; i < g_game->screen_h; i++) {
            int mapY = i * halfHeight;
            int mapX = 0;
            int j = 0;
            // A while loop, tail in this order: a for loop computes dst+1 before the store.
            while (j < g_game->width_142eb) {
                int index = (mapY / g_game->screen_h) * halfWidth + mapX / g_game->width_142eb;
                // Outputs assigned directly in each branch, no pixel temporary;
                // the unsigned short cast stays.
                if (!(unsigned short)(g_game->visibilityMask[index] & mask)) {
                    *dst = fog;
                } else if (t->explored[index]) {
                    *dst = *src;
                } else {
                    *dst = ((Fx_00466c20 *)g_game->unknown_c)->colorMap[*src];
                }
                j++;
                mapX += halfWidth;
                src++;
                dst++;
            }
        }
        g_game->timer.word.flags.pending = 1;
    }
}

// The unit loop's x multiply (`movsx eax, [ebx+0x6c]; movsx ecx,
// [esi+0x142eb]` in the original) is ordered by a key built from the symbol
// ids of the two bases plus the displacement. <windows.h> supplies the
// declaration count that gives the original's order; reading the type through
// a reference taken at the top of the loop body leaves the x multiply on `u`.
// The radar helpers use the ByteMap Index/Get methods (width materialised,
// `mov ecx, [edx+0x80]; imul ecx, edi`) and the slot loop reads `slot->weapon`
// at each use instead of a `shot` local.
static Player* Player_Get(unsigned char p)
{
    return &g_game->players_1b63_14b[0] + p;
}

// True when (px, py) is inside the current player's visible area. The two
// halves match the uint8 terrain bitmap and the packed 16-bit bitfield variant.
static inline int OnRadarByte_00466dc0(Player* pi, int px, int py)
{
    int tx = px >> 5;
    int ty = py >> 5;
    if ((unsigned int)tx < (unsigned int)pi->exploredWidth && (unsigned int)ty < (unsigned int)pi->exploredHeight &&
        pi->explored[(unsigned int)pi->exploredWidth * ty + tx])
        return 1;
    return 0;
}

static inline int OnRadarShort_00466dc0(Player* pi, int px, int py)
{
    int tx = px >> 5;
    int ty = py >> 5;
    if (!((unsigned int)tx < (unsigned int)pi->exploredWidth && (unsigned int)ty < (unsigned int)pi->exploredHeight)) {
        return 0;
    }
    Player* b = pi;
    return (g_game->visibilityMask[(unsigned int)b->exploredWidth * ty + tx] &
            (1 << g_game->playerIndex)) != 0;
}

static inline int OnRadar_00466dc0(int px, int py)
{
    Player* pi = Player_Get(g_game->playerIndex);
    if (((*(Flags14281_00466dc0*)&g_game->mapFlags_14281).all & 2) == 2)
        return OnRadarByte_00466dc0(pi, px, py);
    return OnRadarShort_00466dc0(pi, px, py);
}

// A unit's screen y (height folded into z) times the minimap scale.
static inline int ScaleY_00466dc0(Unit* u)
{
    return ((int)u->zWhole - ((int)u->yWhole >> 1)) * (int)g_game->screen_h;
}

// FUNCTION: 0x466dc0
void DrawRadarUnits(void)
{
    unsigned char* base = (unsigned char*)g_game + 0xdcb;
    unsigned short* out = (unsigned short *)g_game->hotRadar;

    g_game->hotRadarCount = 0;
    void* surface = g_game->finalSurface;
    DrawSurface(surface, g_game->mappedSurface, 0, 0);

    int enabled;
    if ((*(Flags14281_00466dc0*)&g_game->mapFlags_14281).bits.bit0 || (*(Flags14281_00466dc0*)&g_game->mapFlags_14281).bits.bit1)
        enabled = 0;
    else
        enabled = 1;
    if ((*(Flags14281_00466dc0*)&g_game->uiOptionFlags_37f2f).bits.bit9)
        enabled = 1;

    Unit* u = g_game->units;
    Unit* end = g_game->unitsEnd;
    if (u <= end) {
        do {
            UnitType_00466dc0*& type = *(UnitType_00466dc0**)&u->def;
            if (u->unitDefIndex != 0) {
                if (enabled != 0 || (u->flags & 0x300) != 0 ||
                    u->playerIndex == g_game->playerIndex) {
                    int x = u->xWhole * g_game->width_142eb /
                            g_game->mapWidth;
                    int y = ScaleY_00466dc0(u) / g_game->mapHeight;
                    if (u->recentlyDamagedTimer == 0 ||
                        (g_game->timer.byte.field_142f0.b.hi & 1) != 0) {
                        DrawFrame(surface,
                            GetGafFrame(g_game->radlogo,
                                Player_Get(u->playerIndex)->info->color),
                            x, y);
                    }
                    if ((short)u->id == (short)g_game->hoverUnitId) {
                        DrawFrame(surface,
                            GetGafFrame(g_game->radlogohigh, 0), x, y);
                    }
                    if (u->selected) {
                        if ((u->activateFlags & 1) != 0 ||
                            (type->flags2 & 4) == 0) {
                            if (type->radardistance != 0)
                                DrawCircle(surface, x, y,
                                    (int)g_game->width_142eb * type->radardistance /
                                    g_game->mapWidth, base[0xa]);
                            if (type->sonardistance != 0)
                                DrawCircle(surface, x, y,
                                    (int)g_game->width_142eb * type->sonardistance /
                                    g_game->mapWidth, base[0xa]);
                            if (type->radardistancejam != 0)
                                DrawCircle(surface, x, y,
                                    (int)g_game->width_142eb * type->radardistancejam /
                                    g_game->mapWidth, base[0xc]);
                            if (type->sonardistancejam != 0)
                                DrawCircle(surface, x, y,
                                    (int)g_game->width_142eb * type->sonardistancejam /
                                    g_game->mapWidth, base[0xc]);
                        }
                        if (type->flags1.bit29) {
                            UnitWeaponSlot* slot = u->weapons;
                            int n = 3;
                            do {
                                if (slot->weapon->flags.b.interceptor) {
                                    int r = ((int)g_game->width_142eb *
                                             (slot->weapon->coverage - 0x200)) /
                                            g_game->mapWidth;
                                    if (slot->stockpile != 0)
                                        DrawDashedCircle(surface, x, y, r, base[0xf],
                                                     0x20,
                                                     g_game->timer.byte.field_142f0.b.hi & 1);
                                    else
                                        DrawCircle(surface, x, y, r, base[0xf]);
                                }
                                slot++;
                                n--;
                            } while (n != 0);
                        }
                    }
                    out[0] = u->id;
                    *(int*)(out + 1) = g_game->posX + x;
                    *(int*)(out + 3) = g_game->posY + y;
                    out += 5;
                    g_game->hotRadarCount++;
                }
            }
            u = (Unit*)((char*)u + 0x118);
        } while (u <= g_game->unitsEnd);
    }

    Projectile* p = g_game->projectiles;
    int i = 0;
    if (g_game->projectileCount > 0) {
        do {
            int px = p->positionWords.xWhole;
            int x = (int)g_game->width_142eb * px / g_game->mapWidth;
            int py = p->positionWords.zWhole - ((int)p->positionWords.yWhole >> 1);
            int y = (int)g_game->screen_h * py / g_game->mapHeight;
            if ((p->type->flags.value & 0x60000000) == 0) {
                if ((p->type->flags.value & 0x40) == 0) {
                    if (OnRadar_00466dc0(px, py) ||
                        p->owner ==
                            g_game->playerIndex) {
                        DrawPixel(surface, x, y, base[0xe]);
                    }
                }
            } else {
                if (OnRadar_00466dc0(px, py) ||
                    p->attacker->playerIndex ==
                        g_game->playerIndex) {
                    DrawFrame(surface,
                        GetGafFrame(g_game->nuclogo,
                            Player_Get(
                                p->owner)->info->color),
                        x, y);
                }
            }
            i++;
            p++;
        } while (i < g_game->projectileCount);
    }

    g_game->timer.byte.field_142f0.bits.bit1 = 1;
}
