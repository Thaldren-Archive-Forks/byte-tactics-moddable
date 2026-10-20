// Decompiled by deepseek-v4.1, edited by deepseek-v4.1, finished by deepseek-v4.1-flash, edited by claude-sonnet-5-5, finished by Space Bunny Free, finished by DeepSeek V4.1 Flash, finished by Claude Opus 5.5. Names are provisional.
// FLAGS: /Gi
// Kept its own file: it matches only under /Gi, which the other functions do
// not use.
// Draws the selected unit / feature info panel (and the PFSTATE debug overlay).
#include <windows.h>
#include <stdio.h>

struct Game;
extern Game* g_game;

int GetScreenHeight();
int __stdcall GetGafFrame(unsigned short* param_1, int param_2);
void __stdcall DrawFrame(void* dst, void* bmp, int x, int y);
void __stdcall SetTextColors(int param_1, int param_2);
int GetTextKeyColor();
void __stdcall SetFont(int param_1);
int GetFontHeight();
int __stdcall GetTextWidth(void* font, unsigned char* text);
void __stdcall DrawString(void* dst, unsigned char* text, int x, int y, int maxWidth);
char* __stdcall Translate(char* key);
int __stdcall GetBuildWeaponPercent(void* owner);
int __stdcall GetOrderTarget(void* unit);
int __stdcall GetOrderName(void* obj);
int __stdcall IsUnitVisibleToPlayer(void* map, void* u);
void __stdcall BlitSideLogoToRect(void* surf, void* player, void* rect, int dy);
unsigned short __stdcall FindUnitTypeId(const char* name);
int __stdcall FillRectangle(void* surface, void* rect, int color);

#include "../map/mission.h"


#include "../network/player_info.h"
#include "../network/player.h"

#include "../graphics/rect.h"

struct WeaponDef_0046a860;
struct Unit_0046a860;

#include "../units/unit_def.h"

#pragma pack(push, 1)
// A weapon type (Thaldren: WeaponDef); only the reload time is read here.
struct WeaponDef_0046a860 {
    char unknown_0[0xe4];
    unsigned short reloadTime;        // +0xe4
};

// A unit, 0x118 bytes, the elements of the array at g_game+0x14357. The two
// signednesses of +0x108 both appear in this file, so the field is a union.
struct Unit_0046a860 {
    char unknown_0[0x92];
    UnitDef* def;                     // +0x92
    Player* player;                   // +0x96
    char unknown_9a[0xa6 - 0x9a];
    unsigned short unitDefIndex;      // +0xa6
    unsigned short id;                // +0xa8
    char unknown_aa[0xb8 - 0xaa];
    unsigned short killCount;         // +0xb8
    char unknown_ba[0xcc - 0xba];
    float energyMakePrev;             // +0xcc
    float energyUsePrev;              // +0xd0
    char unknown_d4[0xe4 - 0xd4];
    float metalMakePrev;              // +0xe4
    float metalUsePrev;               // +0xe8
    char unknown_ec[0xff - 0xec];
    unsigned char playerIndex;        // +0xff
    char unknown_100[0x108 - 0x100];
    union {
        short health;                 // +0x108
        unsigned short healthRaw;     // +0x108, the unsigned reads
    };
    char unknown_10a[0x110 - 0x10a];
    unsigned int flags;               // +0x110
    char unknown_114[0x118 - 0x114];
};

// A feature's 0x100-byte record, the elements of the table at g_game+0x1426f
// (Thaldren: FeatureDef).
struct Feature_0046a860 {
    char name[0x80];                  // +0x000
    char description[0x14];           // +0x080
    char unknown_94[0xec - 0x94];
    float energy;                     // +0x0ec
    float metal;                      // +0x0f0
    char unknown_f4[0xff - 0xf4];
    unsigned char flags;              // +0x0ff
};

// One map cell, 13 bytes, the elements of the table at g_game+0x14287.
struct Cell_0046a860 {
    char unknown_0[4];
    unsigned char shade;              // +0x04
    char unknown_5[0x0d - 0x5];
};

#include "../graphics/gaf_frame.h"

// The display context at g_game+0xc; only two fields are read here.
struct Display_0046a860 {
    char unknown_0[0x9c];
    int haveBackBuffer;               // +0x09c
    char unknown_a0[0xf0 - 0xa0];
    unsigned char flags;              // +0x0f0, low byte of the state flags word
};

// The object g_game+0x531 points at (0x495860's shape).
struct Struct_0046a860 {
    int unknown_0;
    int value;                        // +0x4
};
#pragma pack(pop)

static void DrawBar_0046a860(void* surface, Rect* bounds, int value, int maximum, int dy,
                             unsigned char* palette) {
    Rect rect = *bounds;
    rect.top = rect.top + dy;
    rect.bottom += dy;
    if (value < 0)
        value = 0;
    if (value > maximum)
        value = maximum;
    rect.right = rect.left + (bounds->right - bounds->left) * value / maximum;
    FillRectangle(surface, &rect, palette[10]);
    if (rect.right != bounds->right) {
        rect.left = rect.right + 1;
        rect.right = bounds->right;
        FillRectangle(surface, &rect, palette[4]);
    }
}

static float Positive_0046a860(float value) {
    float result = value > 0.0f ? value : 0.0f;
    return result;
}

struct Flags46b_3923b { unsigned short b0:1; unsigned short b1:1; };

// The game state this file reads, with the member names the other Game views
// use. The bit word shares a union with this view's bitfield type.
#pragma pack(push, 1)
// A side's panel layout at g_game+0x37f3d, stride 0x232 (Thaldren: SideDef).
struct SideDef_0046a860 {
    char aName[0x1e];                   // +0x000
    char aNamePrefix[4];                // +0x01e
    char aCommander[0x20];              // +0x022
    Rect rcLogo;               // +0x042
    Rect rcEnergyBar;          // +0x052
    Rect rcEnergyNum;          // +0x062
    Rect rcMetalBar;           // +0x072
    Rect rcMetalNum;           // +0x082
    Rect rcTotalUnits;         // +0x092
    Rect rcTotalTime;          // +0x0a2
    Rect rcEnergyMax;          // +0x0b2
    Rect rcMetalMax;           // +0x0c2
    Rect rcEnergy0;            // +0x0d2
    Rect rcMetal0;             // +0x0e2
    Rect rcEnergyProduced;     // +0x0f2
    Rect rcEnergyConsumed;     // +0x102
    Rect rcMetalProduced;      // +0x112
    Rect rcMetalConsumed;      // +0x122
    Rect rcLogo2;              // +0x132
    Rect rcUnitName;           // +0x142
    Rect rcDamageBar;          // +0x152
    Rect rcUnitEnergyMake;     // +0x162
    Rect rcUnitEnergyUse;      // +0x172
    Rect rcUnitMetalMake;      // +0x182
    Rect rcUnitMetalUse;       // +0x192
    Rect rcMissionText;        // +0x1a2
    Rect rcUnitName2;          // +0x1b2
    Rect rcDamageBar2;         // +0x1c2
    Rect rcName;               // +0x1d2
    Rect rcDescription;        // +0x1e2
    Rect aReloadRects[3];      // +0x1f2
    int nEnergyColor;                   // +0x222
    int nMetalColor;                    // +0x226
    int nSideIndex;                     // +0x22a
    char* pFontData;                    // +0x22e
};

#include "../game/game.h"
#pragma pack(pop)

// FUNCTION: 0x46a860
void __stdcall DrawUnitInfoPanel(void* surface) {
    int yOffset = g_game->field_37e23 - g_game->baseHeight;
    Snapshot_0046a860 snapshot;
    memset(&snapshot, 0, sizeof(snapshot));
    char text[256];

    unsigned short flags = g_game->endGameFlags;
    if (flags & 1 && flags & 2) {
        int y = 0x81;
        unsigned char player = g_game->playerIndex;
        PlayerInfo* table = g_game->players[player].info;
        int idx = table->side;
        do {
            int dy = GetScreenHeight() - 0x20;
            unsigned short* icon = g_game->sidePanelBotSeq[idx];
            GafFrame* bmp = (GafFrame*)GetGafFrame(icon, 0);
            DrawFrame(surface, (void*)bmp, (short)bmp->xOffset + y,
                         (short)bmp->yOffset + dy);
            y += bmp->width;
        } while (y < g_game->width_37e1f);

        SetFont(g_game->fontComix);
        SetTextColors(0x53, GetTextKeyColor());
        // No pfable or field_c locals: the PFSTATE sprintf re-reads g_game->displayContext per argument.
        int pfstate = g_game->field_37e23 - GetFontHeight() - 1;
        sprintf(text, "PFSTATE %d, PFABLE %d\n", ((Display_0046a860 *)g_game->unknown_c)->flags & 1,
                ((Display_0046a860 *)g_game->unknown_c)->haveBackBuffer);
        DrawString(surface, (unsigned char*)text, 0x82, pfstate, -1);

        if (g_game->hoverUnitId != 0) {
            int idx = g_game->hoverUnitId & 0xffff;
            Unit_0046a860* unit = (Unit_0046a860 *)g_game->units + idx;
            sprintf(text, "MOVEORD: %d FIREORD: %d\n", (unit->flags >> 0x12) & 3,
                    (unit->flags >> 0x14) & 3);
            DrawString(surface, (unsigned char*)text, 0x108, pfstate, -1);
        }

        sprintf(text, "DELTATIME: %d\n", g_game->frames_38a3b);
        DrawString(surface, (unsigned char*)text, 0x190, pfstate, -1);

        sprintf(text, "GAMETIME: %d\n", g_game->ticks);
        DrawString(surface, (unsigned char*)text, 0x208, pfstate, -1);

        pfstate -= 0x10;
        sprintf(text, "X: %d  Y: %d\n", g_game->scrollX, g_game->scrollY);
        DrawString(surface, (unsigned char*)text, 0x82, pfstate, -1);

        sprintf(text, "UNITS %d\\%d\n", g_game->f14353, g_game->count_14367);
        DrawString(surface, (unsigned char*)text, 0x108, pfstate, -1);

        sprintf(text, "PACKETS: %d %d %d\n", g_game->players[1].messages,
                g_game->players[2].messages, g_game->players[3].messages);
        DrawString(surface, (unsigned char*)text, 0x190, pfstate, -1);

        int v = g_game->width;
        v *= (short)(unsigned short)g_game->y;
        v += (short)(unsigned short)g_game->x;
        unsigned char c = ((Cell_0046a860 *)g_game->cells)[v].shade;
        sprintf(text, "XYH: %d %d %d\n", (short)(unsigned short)g_game->x,
                (short)(unsigned short)g_game->y, c);
        DrawString(surface, (unsigned char*)text, 0x208, pfstate, -1);
        return;
    }

    snapshot.button = g_game->selected;
    snapshot.selected = g_game->hoverUnitId;
    snapshot.feature = g_game->cellFeature;
    snapshot.height = g_game->selectionInfoCache.height;
    snapshot.width = g_game->selectionInfoCache.width;

    if (snapshot.selected != 0) {
        Unit_0046a860* unit = (Unit_0046a860 *)g_game->units + snapshot.selected;
        snapshot.health = unit->healthRaw;
        snapshot.build = unit->killCount;
        snapshot.orderName = GetOrderName(unit);
        // Copied through float temporaries; direct assignment gives integer moves.
        float f0 = unit->energyUsePrev;
        float f1 = unit->energyMakePrev;
        float f2 = unit->metalUsePrev;
        float f3 = unit->metalMakePrev;
        snapshot.values[0] = f0;
        snapshot.values[1] = f1;
        snapshot.values[2] = f2;
        snapshot.values[3] = f3;
        char* slot = (char*)unit + 0x1f;
        int* out = snapshot.weapons;
        // Keep both `*out = -1` stores (nested ifs): the extra use sets the register of out.
        for (int i = 0; i < 3; i++, out++, slot += 0x1c) {
            WeaponDef_0046a860* weapon = *(WeaponDef_0046a860**)(slot - 0xf);
            if (weapon->reloadTime > 0x1e) {
                if (*(unsigned char*)slot & 2)
                    *out = *(unsigned short*)(slot - 7);
                else
                    *out = -1;
            } else
                *out = -1;
        }

        snapshot.targetType = 0;
        if (unit->playerIndex == g_game->playerIndex) {
            Unit_0046a860* result = (Unit_0046a860*)GetOrderTarget(unit);
            if (result != 0) {
                snapshot.targetType = result->id;
                snapshot.targetHealth = result->healthRaw;
            }
        }
    }

    char* saved = (char*)&g_game->selectionInfoCache;
    if (memcmp(saved, &snapshot, sizeof(snapshot)) == 0)
        return;
    memcpy(saved, &snapshot, sizeof(snapshot));

    char* palette = (char*)&g_game->colors[0];
    SetTextColors(0x53, GetTextKeyColor());

    int player = g_game->playerIndex;
    Player* playerInfo = &g_game->players[player];
    PlayerInfo* info = playerInfo->info;
    int side = info->side;
    SideDef_0046a860* panel = &(*(SideDef_0046a860 (*)[5])&g_game->sideDefs)[side];
    SetFont((int)panel->pFontData);
    int y = 0x81;
    do {
        int dy = GetScreenHeight() - 0x20;
        PlayerInfo* loopInfo = playerInfo->info;
        int loopSide = loopInfo->side;
        unsigned short* icon = g_game->sidePanelBotSeq[loopSide];
        GafFrame* bmp = (GafFrame*)GetGafFrame(icon, 0);
        DrawFrame(surface, (void*)bmp, (short)bmp->xOffset + y,
                     (short)bmp->yOffset + dy);
        y += bmp->width;
    } while (y < g_game->width_37e1f);

    if (snapshot.button != -1) {
        strncpy(text, (char*)(snapshot.button * 0x15b + ((Struct_0046a860 *)g_game->layer)->value + 2), 0x10);
        text[0x10] = 0;
        unsigned short type = FindUnitTypeId(text);
        if (type != 0) {
            UnitDef* entry = g_game->defs + type;
            if (_strcmpi(text, "CORBUILD") != 0) {
                sprintf(text, "%s  M:%d E:%d", entry, (int)entry->metalCost,
                        (int)entry->energyCost);
                DrawString(surface, (unsigned char*)text, panel->rcName.left,
                             panel->rcName.top + yOffset, -1);
                DrawString(surface, (unsigned char*)entry->description, panel->rcDescription.left,
                             panel->rcDescription.top + yOffset, -1);
                return;
            }
        }
    } else if (snapshot.selected != 0) {
        Unit_0046a860* unit = (Unit_0046a860 *)g_game->units + snapshot.selected;
        if (unit->unitDefIndex != 0) {
            char* playerMap = (char*)&g_game->players[g_game->playerIndex];
            if (IsUnitVisibleToPlayer(playerMap, unit)) {
                UnitDef* definition = unit->def;
                unsigned int unitFlags = definition->flags2;
                int flagsOk = ((unitFlags & 0x20000) | ((unitFlags >> 1) & 0x20000)) >> 0x11;
                int gameMode = g_game->net->GetGameType();
                if (gameMode == 3 && flagsOk)
                    strcpy(text, unit->player->name);
                else
                    strcpy(text, (char*)unit->def);
                int titleX = panel->rcUnitName.left -
                             GetTextWidth((void*)panel->pFontData, (unsigned char*)text) / 2;
                SetTextColors(0x53, GetTextKeyColor());
                DrawString(surface, (unsigned char*)text, titleX, yOffset + panel->rcUnitName.top, -1);
                if (unit->player->index == g_game->playerIndex ||
                    !(unit->def->flags1 & 0x4000)) {
                    int maximum = unit->def->maxHealth;
                    DrawBar_0046a860(surface, &panel->rcDamageBar, unit->health,
                                     maximum, yOffset, (unsigned char*)palette);
                }
                BlitSideLogoToRect(surface, unit->player, &panel->rcLogo2, yOffset);
                if (unit->playerIndex == g_game->playerIndex ||
                    (g_game->flagsByte_3923b & 2)) {
                    // Declared in this block, not at function scope: affects load scheduling.
                    char amount[100];
                    char killsText[100];
                    SetTextColors((unsigned char)palette[10], GetTextKeyColor());
                    sprintf(amount, "+%.1f", Positive_0046a860(snapshot.values[3]));
                    DrawString(surface, (unsigned char*)amount, panel->rcUnitMetalMake.left,
                                 panel->rcUnitMetalMake.top + yOffset, -1);
                    sprintf(amount, "+%.0f", Positive_0046a860(snapshot.values[1]));
                    DrawString(surface, (unsigned char*)amount, panel->rcUnitEnergyMake.left,
                                 panel->rcUnitEnergyMake.top + yOffset, -1);
                    SetTextColors((unsigned char)palette[12], GetTextKeyColor());
                    sprintf(amount, "-%.1f", Positive_0046a860(snapshot.values[2]));
                    DrawString(surface, (unsigned char*)amount, panel->rcUnitMetalUse.left,
                                 panel->rcUnitMetalUse.top + yOffset, -1);
                    sprintf(amount, "-%.0f", Positive_0046a860(snapshot.values[0]));
                    DrawString(surface, (unsigned char*)amount, panel->rcUnitEnergyUse.left,
                                 panel->rcUnitEnergyUse.top + yOffset, -1);
                    if ((unit->flags & 0x80000000) && unit->killCount) {
                        int killsY = panel->rcDamageBar.bottom + yOffset + 2, killsX = panel->rcDamageBar.left;
                        char* plural = Translate("kills");
                        char* singular = Translate("kill");
                        if (unit->killCount > 4)
                            sprintf(killsText, "%d %s - %s", unit->killCount,
                                    unit->killCount == 1 ? singular : plural,
                                    Translate("Veteran"));
                        else
                            sprintf(killsText, "%d %s", unit->killCount,
                                    unit->killCount == 1 ? singular : plural);
                        SetTextColors(g_game->colors[15], GetTextKeyColor());
                        DrawString(surface, (unsigned char*)killsText, killsX, killsY, -1);
                    }
                    if (snapshot.orderName) {
                        strcpy(amount, Translate((char*)snapshot.orderName));
                        int orderX = panel->rcMissionText.left -
                                     GetTextWidth((void*)panel->pFontData, (unsigned char*)amount) / 2;
                        SetTextColors(0x53, GetTextKeyColor());
                        DrawString(surface, (unsigned char*)amount, orderX, panel->rcMissionText.top + yOffset, -1);
                    }
                }
                int progress = GetBuildWeaponPercent(unit);
                if (progress) {
                    if (unit->player->index != g_game->playerIndex)
                        return;
                    char* weaponText = Translate("Weapon");
                    int progressX = panel->rcUnitName2.left -
                                    GetTextWidth((void*)panel->pFontData, (unsigned char*)weaponText) / 2;
                    SetTextColors(0x53, GetTextKeyColor());
                    DrawString(surface, (unsigned char*)weaponText, progressX, panel->rcUnitName2.top + yOffset, -1);
                    DrawBar_0046a860(surface, &panel->rcDamageBar2, progress, 100, yOffset,
                                     (unsigned char*)palette);
                    return;
                }
                if (snapshot.targetType) {
                    Unit_0046a860* target = (Unit_0046a860 *)g_game->units + snapshot.targetType;
                    if (!target->unitDefIndex || !IsUnitVisibleToPlayer(playerMap, target))
                        return;
                    int targetX = panel->rcUnitName2.left -
                                  GetTextWidth((void*)panel->pFontData, (unsigned char*)target->def) / 2;
                    SetTextColors(0x53, GetTextKeyColor());
                    DrawString(surface, (unsigned char*)target->def, targetX,
                                 panel->rcUnitName2.top + yOffset, -1);
                    if (target->player->index == g_game->playerIndex ||
                        !(target->def->flags1 & 0x4000)) {
                        int maximum = target->def->maxHealth;
                        DrawBar_0046a860(surface, &panel->rcDamageBar2, target->health,
                                         maximum, yOffset, (unsigned char*)palette);
                    }
                }
            } else {
                char* prefix = (char*)(((unsigned char)(unit->flags >> 9) & 1) != 0 ? "S: " : "R: ");
                char* s = Translate("Unidentified object");
                sprintf(text, "%s%s", prefix, s);
                int w = GetTextWidth((void*)panel->pFontData, (unsigned char*)text);
                int x = panel->rcUnitName.left - w / 2;
                SetTextColors(0x53, GetTextKeyColor());
                DrawString(surface, (unsigned char*)text, x, panel->rcUnitName.top + yOffset, -1);
                return;
            }
        }
    } else if (snapshot.feature != 0xffff) {
        Feature_0046a860* feature = (Feature_0046a860 *)g_game->features + snapshot.feature;
        if ((feature->flags & 4) == 0 || (g_game->flagsByte_3923b & 2) != 0) {
            char mText[16];
            if (feature->metal != 0.0f)
                sprintf(mText, " M:%d", (int)feature->metal);
            else
                mText[0] = 0;
            char eText[16];
            if (feature->energy != 0.0f)
                sprintf(eText, " E:%d", (int)feature->energy);
            else
                eText[0] = 0;
            char* name = (*(Flags46b_3923b*)&g_game->endGameFlags_3923b).b1 ? feature->name : feature->description;
            if ((feature->flags & 2) == 0)
                sprintf(text, "%s %s%s", Translate(name), mText, eText);
            else
                strcpy(text, Translate(name));
            SetTextColors(0x53, GetTextKeyColor());
            DrawString(surface, (unsigned char*)text, panel->rcName.left, panel->rcName.top + yOffset, -1);
        }
    }
}
