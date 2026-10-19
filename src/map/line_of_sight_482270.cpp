// Decompiled by deepseek-v4.1-flash, finished by space-bunny-free, deepseek-v4.1-flash, GPT-6, GPT-6.1-sol, deepseek-v4.1-flash, finished by deepseek-v4.1-flash, finished by GPT-6.1-sol, finished by mimo-v2.6-pro, finished by Space Bunny Free. Names are provisional.
// Not gathered into line_of_sight.cpp: merged, the player grid address folds in
// the other operand order.
// Unused by the code but needed: it flips the operand order of `i * frame->width`.
#include <math.h>
#include <windows.h>

#include <memory>
#include "los_tables.h"

#pragma pack(push, 1)

class LosLine {
public:
    short GetLosLineStepCount();
    void GetLosLineStep(short i, int* a, int* b);
};

class LosTable {
public:
    short GetLosLineCount();
    LosLine* GetLosLine(short i);
};

// Unused here: the symbol ids these declarations take keep the allocation (docs/c2-regalloc.md).
void CopyDwordIfNonNull(int*, int*);
int ScanDirectory(char*, char*, char*, int, int, int);
void RegisterUnitOrders(void);
void RegisterGroundOrders(void);
void EnableAICommands(void);
void RegisterAICommands(void);
void FUN_00406f40(void);
void ResetAIPlayers(void);
void FUN_004161f0(void);
void InitCommands(void);
void ResetNetStats(void);

extern LosTables g_losTables;

struct MapSize_482270 {
    unsigned int width;                // +0x0
    unsigned int height;               // +0x4
};

struct ByteMap_482270 {
    unsigned char* data;               // +0x0
    MapSize_482270 size;               // +0x4
    unsigned char& at(int x, int y) { return data[y * size.width + x]; }
};

#include "grid.h"

struct Player_482270 {
    char unknown_0[0x7c];
    ByteMap_482270 grid;              // +0x7c
    char unknown_88[0x146 - 0x88];
    unsigned char playerIndex;         // +0x146
};

#include "sight_query.h"

#include "../game/game.h"

#include "../graphics/gaf_frame.h"

#pragma pack(pop)

extern Game* g_game;

GafFrame* __stdcall GetGafFrame(unsigned short* table, int index);

// FUNCTION: 0x482270
void __stdcall AddLineOfSight(SightQuery* params)
{
    if (((Player_482270*)params->player)->playerIndex == g_game->playerIndex) {
        g_game->bits_14281_1.flag3 = 0;
        g_game->bits_142f1_2.flagA = 1;
    }
    int halfW = g_game->width / 2;
    int halfH = g_game->height / 2;
    int x = params->cacheCell[0];
    int y = params->cacheCell[1];
    if (g_game->bits_14281_1.flag2 == 1) {
        Grid* grid = &g_game->grid1;
        if ((unsigned)x >= grid->width)
            return;
        if ((unsigned)y >= grid->height)
            return;
        // Clamp written inline as the GetLosTable argument, with no temporary.
        // One cast stays: without it the register allocation of the loop below moves.
        LosTable* table = (LosTable*)((LosTables*)&g_losTables)->GetLosTable(
            (params->sightDistance / 32 < 0 ? 0 : params->sightDistance / 32) <
                    (short)g_losTables.GetLosTableCount() - 1
                ? (params->sightDistance / 32 < 0 ? 0 : params->sightDistance / 32)
                : (short)g_losTables.GetLosTableCount() - 1);
        short count = table->GetLosLineCount();
        short i = 0;
        ((Player_482270*)params->player)->grid.at(x, y)++;
        int ref = *params->frameIdx;
        for (i = 0; i < count; i++) {
            LosLine* line = table->GetLosLine(i);
            short num = line->GetLosLineStepCount();
            // Declared in this order: bestIdx, j1, bestDiff, j; dx and dy stay int locals.
            int bestIdx = 0;
            int j1;
            int bestDiff = -1;
            short j = 0;
            if ((short)num > 0) {
                j1 = 1;
                do {
                    int dx;
                    int dy;
                    line->GetLosLineStep(j, &dx, &dy);
                    dx += x;
                    dy += y;
                    if ((unsigned)(short)dx >= grid->width)
                        continue;
                    if ((unsigned)(short)dy >= grid->height)
                        continue;
                    unsigned char* cell =
                        grid->cells + ((short)dy * grid->width + (short)dx) * 2;
                    int d1 = cell[1] - ref;
                    int d0 = cell[0] - ref;
                    if (d0 * bestIdx > bestDiff * j1) {
                        ((Player_482270*)params->player)
                            ->grid.at((short)dx, (short)dy)++;
                        if (d1 * bestIdx > bestDiff * j1) {
                            bestIdx = j1;
                            bestDiff = d1;
                        }
                    }
                } while (j++, j1++, (short)j < (short)num);
            }
        }
    } else {
        int ref = *params->frameIdx;
        GafFrame* frame =
            GetGafFrame((unsigned short*)(void *)g_game->losTable, ref);
        // limitX and limitY are if/else statements, not ?:.
        int limitX;
        if (x + frame->width >= halfW)
            limitX = halfW - x;
        else
            limitX = frame->width;
        int limitY;
        if (y + frame->height >= halfH)
            limitY = halfH - y;
        else
            limitY = frame->height;
        int nx = x < 0 ? -x : 0;
        int ny = y < 0 ? -y : 0;
        if (ny >= limitY)
            return;
        if (nx >= limitX)
            return;
        for (int i = ny; i < limitY; i++) {
            // Named ex local: shapes the destination address.
            ByteMap_482270* ex = &((Player_482270*)params->player)->grid;
            unsigned char* dst = ex->data + (y + i) * ex->size.width + nx + x;
            unsigned char* src = frame->pixelsOrLayers + i * frame->width + nx;
            // Counted j loop: lets the compiler turn it into dec/jne.
            for (int j = nx; j < limitX; j++) {
                if (*src != frame->transparency)
                    (*dst)++;
                dst++;
                src++;
            }
        }
    }
}
