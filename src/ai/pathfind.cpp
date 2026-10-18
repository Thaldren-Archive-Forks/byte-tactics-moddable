// Decompiled by DeepSeek V4.1 Flash, Claude Opus 5.5, Opus, space-bunny-free, deepseek-v4.1-flash, Haiku, Sonnet and GPT-6. Names are provisional.
// The AI's path search (AISearch): an A* over the map's cells with an open
// heap of nodes (the OpenHeap base, whose sift-up and sift-down are defined
// here too, at their place in the original file), a per-tick scheduler that
// shares the search steps among the players, and the trace back from the goal.
// Needed: without it GetCellState's first argument lands in eax, not edx.
#include <stdlib.h>
#include "../state/block.h"
#include <string.h>
#include <windows.h>
#include <ddraw.h>
#include <vector>

void __cdecl operator delete(void* p);
void __cdecl GameFreeThunk(void* p);

// A min-heap record for MakeHeap and friends (the STL heap the search uses).
struct Entry_40d670 {
    Entry_40d670() {}
    Entry_40d670(const Entry_40d670& o) : value(o.value), key(o.key) {}
    int value;                       // +0x0
    float key;                       // +0x4
};

static inline bool operator<(const Entry_40d670& a, const Entry_40d670& b)
{
    return a.key < b.key;
}

void __stdcall AdjustHeap(Entry_40d670* first, int hole, int len, Entry_40d670 val);
void __stdcall PushHeapSiftUp(Entry_40d670* first, int hole, int top, Entry_40d670 val);

// Placement copy of a pair of ints (an STL construct helper), like 0x40d600.
// FUNCTION: 0x40d5e0
void __stdcall CopyDwordPair(int* dest, int* src) {
    if (dest != 0) {
        dest[0] = src[0];
        dest[1] = src[1];
    }
}

// Placement copy of one char (an STL construct helper), like 0x40d600.
// FUNCTION: 0x40d600
void __stdcall CopyOneByte(char* dest, char* src)
{
    if (dest != 0) {
        *dest = *src;
    }
}

// make_heap over [first, last): the STL _Make_heap shape.
// FUNCTION: 0x40d620
void __stdcall MakeHeap(Entry_40d670* first, Entry_40d670* last, int*, Entry_40d670*)
{
    int n = last - first;
    for (int h = n / 2; 0 < h; ) {
        --h;
        AdjustHeap(first, h, n, Entry_40d670(first[h]));
    }
}

// FUNCTION: 0x40d670
void __stdcall AdjustHeap(Entry_40d670* first, int hole, int len, Entry_40d670 val)
{
    int top = hole;
    int k = 2 * hole + 2;

    while (k < len) {
        if (first[k] < first[k - 1])
            k--;
        first[hole] = first[k];
        hole = k;
        k = 2 * k + 2;
    }
    if (k == len) {
        first[hole] = first[k - 1];
        hole = k - 1;
    }
    PushHeapSiftUp(first, hole, top, val);
}

// FUNCTION: 0x40d700
void __stdcall PopHeapFirst(Entry_40d670* first, Entry_40d670* last, Entry_40d670* dest,
                             Entry_40d670 val, void* unused)
{
    *dest = *first;
    AdjustHeap(first, 0, last - first, val);
}

// push_heap's sift-up (the STL _Push_heap shape) for the heap of
// AdjustHeap: moves parents down while they are less than val.
// FUNCTION: 0x40d740
void __stdcall PushHeapSiftUp(Entry_40d670* first, int hole, int top, Entry_40d670 val)
{
    for (int idx = (hole - 1) / 2; top < hole && first[idx] < val; idx = (hole - 1) / 2) {
        first[hole] = first[idx];
        hole = idx;
    }
    first[hole] = val;
}

#include "pathfinder.h"

extern signed char g_dirDeltaX[];      // dx per direction
extern signed char g_dirDeltaZ[];      // dy per direction

class Planner_0040eb70;

#pragma pack(push, 1)
struct Owner_0040eb70 {
    Planner_0040eb70* planner;         // +0x0
    void* owner;                       // +0x4, the movement class
};

#include "../units/unit.h"

#include "../network/player.h"

struct Game {
    char unknown_0[0x1b63];
    Player players[10];                // +0x1b63
    char unknown_2851[0x2a3c - 0x2851];
    unsigned short numPlayers;         // +0x2a3c, the number of players
    char unknown_2a3e[0x14233 - 0x2a3e];
    int mapWidthTiles;                 // +0x14233
    int mapHeightTiles;                // +0x14237
    char unknown_1423b[0x14273 - 0x1423b];
    unsigned short* visibilityMask;    // +0x14273, one bit per player
    char unknown_14277[0x1434f - 0x14277];
    unsigned short unitsPerPlayer;     // +0x1434f
};
#pragma pack(pop)

extern Game* g_game;
extern int g_budgetRefreshCounter;
extern int g_playerTickLoad[10];
extern int g_playerBudgetCap[10];

#include "../units/movement_class.h"

#include "../../interfaces/seams.h"

#include "ai_search_goal.h"

class Planner_0040eb70 {
public:
    virtual void unused0();
    virtual void unused1();
    virtual void unused2();
    virtual void unused3();
    virtual void unused4();
    virtual void unused5();
    virtual AiSearchGoal* GetPath();
};

inline int Pathfinder::Push(const NodeData& d)
{
    if (topPopped) {
        Node* n = items[0];
        n->data = d;
        SiftDown(0);
        topPopped = 0;
        return n - pool;
    }
    if (count == capacity)
        GrowNodes(-1);
    int i = count++;
    int k = freeHead;
    if (k == -1)
        k = used++;
    else
        freeHead = pool[k].index;
    pool[k].data = d;
    pool[k].index = i;
    items[i] = &pool[k];
    SiftUp(i);
    return k;
}

static int IsPlaying(unsigned char i)
{
    if (i < 10) {
        Player* p = &g_game->players[i];
        if (p->active != 0 && (p->type == 1 || p->type == 2 || p->type == 3)
            && p->index != 10)
            return 1;
    }
    return 0;
}

// The original calls every method here out of line; only the helpers defined
// in the classes above are inlined.
#pragma auto_inline(off)

// Looks up the 2-bit state of map cell (x, y): 0 when the cell is off the map
// or its visibility cell is off the game grid, 2 when the player's bit is not
// set in that visibility cell, else the cell's stored value.
// FUNCTION: 0x40d7b0
int Pathfinder::GetCellState(int x, int y)
{
    if (!g_seams.units->MovementClassInBounds(owner, x, y))
        return 0;
    int cx = (x >> 1) + (owner->footprintX >> 2);
    int cy = (y >> 1) + (owner->footprintZ >> 2);
    // `g_game->mapWidthTiles >> 1` stays written twice, not held in a local.
    if (cx >= (g_game->mapWidthTiles >> 1) || cy >= (g_game->mapHeightTiles >> 1))
        return 0;
    if (!((1 << player) & g_game->visibilityMask[cy * (g_game->mapWidthTiles >> 1) + cx]))
        return 2;
    return g_seams.units->MovementClassGet(owner, x, y);
}

// Resets a table of eight bytes {0, 40, 60, 80, 100, 80, 60, 40} copied from
// a constant, and four byte pairs to {16, 22}.
// FUNCTION: 0x40d880
void Pathfinder::InitCostTables()
{
    table = g_turnCosts;
    pairs[0].a = pairs[1].a = pairs[2].a = pairs[3].a = 0x10;
    pairs[0].b = pairs[1].b = pairs[2].b = pairs[3].b = 0x16;
}

// Marks the cell at (x, y) with kind 4 and sets the dirty
// bit of the block of eight cells that holds it.
// FUNCTION: 0x40d8b0
void Pathfinder::MarkGoalCell(unsigned int x, unsigned int y)
{
    // Call the grid methods on the member; writing their bodies here breaks the match.
    if (grid.InBounds(x, y))
        grid.Set(x, y, 4);
}

// Clears the kind of every cell in the dirty blocks of the grid, and the dirty
// bits.
// FUNCTION: 0x40d900
void Pathfinder::ClearDirtyCells()
{
    int n = ((grid.count + 0xff) >> 8) - 1;
    int i;
    for (i = 0; i < n; i++)
        grid.ClearBlock(i, 0);
    grid.ClearLast(i);
}

// Maps a (dx, dy) step to one of eight directions, or -1.
// FUNCTION: 0x40d9c0
int __stdcall DirectionFromStep(int dx, int dy)
{
    if (dx > 0) {
        if (dy == dx)
            return 5;
        if (dy == -dx)
            return 7;
        if (dy == 0)
            return 6;
    } else if (dx < 0) {
        if (dy == dx)
            return 1;
        if (dy == -dx)
            return 3;
        if (dy == 0)
            return 2;
    } else {
        if (dy > 0)
            return 4;
        if (dy < 0)
            return 0;
    }
    return -1;
}

// FUNCTION: 0x40da40
__int64 Pathfinder::Estimate(int param1, int param2)
{
    int a = costScale;
    return ((__int64)target->Cost(param1, param2) * (__int64)a) >> 0x10;
}

// Expands one open-list node onto the navigation grid: for each of the
// directions in [fromCell->dir - range, fromCell->dir + range], update the
// neighbour cell's cost and push it or sift it up in the open heap.
// FUNCTION: 0x40da70
void Pathfinder::ExpandNeighbour(NodeData* from, PathCell* fromCell, int turn)
{
    int dir = (fromCell->dir + turn) & 7;
    unsigned int x = from->pos.x + g_dirDeltaX[dir];
    unsigned int y = from->pos.y + g_dirDeltaZ[dir];
    if (!grid.InBounds(x, y))
        return;
    unsigned int i = grid.width * y + x;
    PathCell* cell = &grid.cells[i];
    switch (cell->flags & 3) {
    case 0: {
        grid.dirty[i >> 8] |= 1 << ((i >> 3) & 0x1f);
        unsigned int r = GetCellState(x, y);
        if (r < 1 && !(cell->flags & 8)) {
            cell->flags |= 3;
            return;
        }
        int h = EstimateCost(x, y);
        if (h <= probe)
            cell->flags |= 5;
        else
            cell->flags |= 1;
        cell->dir = dir;
        NodeData d;
        d.pos.x = x;
        d.pos.y = y;
        if (turn)
            d.steps = 1;
        else
            d.steps = from->steps + 1;
        d.penalty = r > 1 ? 0 : 30;
        d.g = turnCost[turn] + stepCost[dir] + from->g + d.penalty;
        if (turn && from->steps < 5)
            d.g += 75;
        d.f = d.g + h;
        cell->node = Push(d);
        break;
    }
    case 1: {
        NodeData* n = &pool[cell->node].data;
        int g = turnCost[turn] + stepCost[dir] + from->g + n->penalty;
        if (turn && from->steps < 5)
            g += 75;
        if (g < n->g) {
            cell->dir = dir;
            n->f += g - n->g;
            n->g = g;
            n->steps = Steps(from, turn);
            Update(cell->node);
        }
        break;
    }
    }
}

// Pops the best node from an open list and expands it on the navigation grid.
// FUNCTION: 0x40df00
int Pathfinder::ExpandBestNode()
{
    if (topPopped != 0) {
        topPopped = 0;
        Pop();
    }
    Node* top = items[0];
    NodeData local = top->data;
    if (topPopped == 0)
        topPopped = 1;
    else
        Pop();
    int y = local.pos.y;
    int i = grid.width * y + local.pos.x;
    PathCell* c = &grid.cells[i];
    if (c->flags & 4) {
        found = local.pos;
        return 1;
    }
    c->flags = 2;
    for (int d = -range; d <= range; d++)
        ExpandNeighbour(&local, c, d & 7);
    return 0;
}

// Traces the path found by the AI search back from the goal cell it reached
// (+0x34) to the start (+0x30) by following each cell's direction byte, keeps
// the points where the direction changes (a ring of 64), and hands them to the
// path object in world coordinates, start first. Called from 0x40eb70.
// FUNCTION: 0x40e050
void Pathfinder::TracePath()
{
    Point16 cur = found;
    int dir = (char)grid.At(found.x, found.y)->dir;
    Point16 pts[64];
    Point16 out[64];
    pts[0] = cur;
    int n = 1;

    // Point::operator!= inline, not a plain `||`: sets the cur.x/cur.y load order.
    while (cur != start) {
        PathCell* c = grid.At(cur.x, cur.y);
        // dir is read twice on purpose: keeps the cell address lea in the loop.
        if ((char)c->dir != dir) {
            dir = (char)c->dir;
            pts[n & 0x3f] = cur;
            n++;
        }
        cur.x -= g_dirDeltaX[dir];
        cur.y -= g_dirDeltaZ[dir];
    }
    pts[n & 0x3f] = start;
    n++;
    int count = n;
    if (count >= 0x40)
        count = 0x40;
    for (int i = 0; i < count; i++) {
        Point16 pt = pts[(n - 1 - i) & 0x3f];
        out[i].x = (pt.x * 2 + owner->footprintX) * 8;
        out[i].y = (pt.y * 2 + owner->footprintZ) * 8;
    }
    g_seams.orders->AiSearchGoalSetWaypoints(path, out, count);
}

// Bug-style path probe: walks straight towards the goal (x first, then y)
// until blocked, then follows the obstacle's outline both ways at once until
// one side reaches the x-then-y line to the goal again. Returns the lowest
// cost seen, or 0 when the walk reaches a goal cell or a zero-cost cell.
// FUNCTION: 0x40e160
int Pathfinder::ProbeStraightPath()
{
    int x = start.x;
    int y = start.y;
    int best = CostFix(x, y);
    if (Passable(start.x, start.y) < 1)
        return best;
    char dir;
    int nx;
    int ny;
    // The return to the straight walk is a goto; CostFix and Cost must stay two helpers.
greedy:
        for (;;) {
            steps++;
            if (best == 0)
                return 0;
            dir = Dir(x, y);
            nx = g_dirDeltaX[dir] + x;
            ny = g_dirDeltaZ[dir] + y;
            if (Passable(nx, ny) < 1)
                break;
            x = nx;
            y = ny;
            if (Visit(nx, ny, dir) & 4)
                return 0;
            int c = CostFix(nx, ny);
            if (c < best)
                best = c;
        }

        int ax = x;
        int ay = y;
        int bx = x;
        int by = y;
        char dirA = (dir + 2) & 7;
        char dirB = dirA;
        int started = 0;
        for (;;) {
            char stop = (dirA - 3) & 7;
            dirA = (dirA - 2) & 7;
            steps++;
            nx = g_dirDeltaX[dirA] + ax;
            ny = g_dirDeltaZ[dirA] + ay;
            while (Passable(nx, ny) < 1) {
                if (dirA == stop)
                    return best;
                dirA = (dirA + 1) & 7;
                nx = g_dirDeltaX[dirA] + ax;
                ny = g_dirDeltaZ[dirA] + ay;
            }
            if (ax == bx && ay == by && dirA == dirB && started)
                return best;
            ax = nx;
            ay = ny;
            started = 1;
            if (Visit(ax, ay, dirA) & 4)
                return 0;
            if (OnLine(x, y, nx, ny)) {
                x = nx;
                y = ny;
                goto greedy;
            }
            int c = Cost(nx, ny);
            if (c < best)
                best = c;

            stop = (dirB + 3) & 7;
            dirB = (dirB + 2) & 7;
            nx = bx - g_dirDeltaX[dirB];
            ny = by - g_dirDeltaZ[dirB];
            while (Passable(nx, ny) < 1) {
                if (dirB == stop)
                    return best;
                dirB = (dirB - 1) & 7;
                nx = bx - g_dirDeltaX[dirB];
                ny = by - g_dirDeltaZ[dirB];
            }
            if (ax == bx && ay == by && dirA == dirB)
                return best;
            bx = nx;
            by = ny;
            if (Visit(bx, by, dirB) & 4)
                return 0;
            if (OnLine(x, y, nx, ny)) {
                x = nx;
                y = ny;
                goto greedy;
            }
            c = Cost(nx, ny);
            if (c < best)
                best = c;
        }
}

// Starts a path search for the object at +0x58: marks every goal cell the
// target reports, picks the goal nearest to the start as the probe's aim,
// runs the straight-line probe (0x40e160) and, when that did not reach a
// goal, seeds the open heap with the start cell.
// FUNCTION: 0x40e630
void Pathfinder::StartSearch(Target* t)
{
    owner = (MovementClass*)((Owner_0040eb70 *)object->motion)->owner;
    target = t;
    start = object->cell;
    g_seams.units->RefreshMovedUnits(owner, object);
    ResetTable();
    ClearDirtyCells();

    std::vector<Point16> goals;
    target->GetGoals(goals);
    int bestDist = 0x7fffffff;
    for (Point16* p = goals.begin(); p != goals.end(); p++) {
        MarkGoal(p->x, p->y);
        int dy = start.y - p->y;
        int dx = start.x - p->x;
        int d = dx * dx + dy * dy;
        if (d < bestDist) {
            goalX = p->x;
            bestDist = d;
            goalY = p->y;
        }
    }

    // Each early exit is its own `Finish(); return;`, not a goto to a shared block.
    if (target->IsGoal(start.x, start.y)) {
        g_seams.orders->OrderFxAddFlags((OrderFx*)target, 0x100);
        Finish();
        return;
    }
    int cost = Cost(start.x, start.y);
    if (!grid.InBounds(start.x, start.y)) {
        g_seams.orders->OrderFxAddFlags((OrderFx*)target, 0x200);
        Finish();
        return;
    }
    probe = ProbeStraightPath();
    if (probe == 0) {
        g_seams.orders->OrderFxAddFlags((OrderFx*)target, 0x100);
    } else {
        g_seams.orders->OrderFxAddFlags((OrderFx*)target, 0x200);
        if (probe >= cost) {
            Finish();
            return;
        }
    }
    Clear();
    NodeData d(start.x, start.y, 0, cost, 100);
    unsigned int i = grid.Index(start.x, start.y);
    grid.dirty[i >> 8] |= 1 << ((i >> 3) & 0x1f);
    grid.cells[i].flags |= 1;
    grid.cells[i].dir = ((((unsigned short)object->heading) + 0x1000) >> 13) & 7;
    grid.cells[i].node = Push(d);
    range = 4;
}

// FUNCTION: 0x40e9a0
void Pathfinder::ReleaseHeldPathLock()
{
    g_seams.units->RefreshUnitIfStale(owner, object);
    object = 0;
    owner = 0;
}

// FUNCTION: 0x40e9c0
void Pathfinder::AbortIfGoalMatch(int goal)
{
    if (goal == (int)path) {
        object = 0;
        path = 0;
        target = 0;
        owner = 0;
    }
}

// AISearch constructor (201-byte object): seeds the per-player search cost
// table, sizes the map cell array from the game's map dimensions, and builds
// the "touched" bitmap.
// FUNCTION: 0x40e9e0
Pathfinder::Pathfinder()
{
    pool = 0;
    items = 0;
    capacity = 0;
    count = 0;
    freeHead = -1;
    used = 0;
    topPopped = 0;
    grid.width = 0;
    grid.height = 0;
    grid.count = 0;
    // memset, not `grid.cells = 0`: an assignment changes the later operator delete call.
    memset(&grid.cells, 0, sizeof(grid.cells));

    int w, h;
    h = g_game->mapHeightTiles;
    w = g_game->mapWidthTiles;
    grid.width = w;
    grid.height = h;
    operator delete(grid.cells);
    grid.count = (h * w + 7) & ~7;
    grid.cells = grid.count ? new (kSim) PathCell[grid.count] : 0;

    unsigned int m = (grid.count + 0xff) >> 8;
    unsigned int n = m * 4;
    grid.dirty = (unsigned int*)SimAllocTagged("AISearch touched mapentries", n);
    // Only n - 1 bytes are filled; the last dword is zeroed separately.
    memset(grid.dirty, 0xff, n - 1);
    *(int*)((char*)grid.dirty + n - 4) = 0;

    for (unsigned int i = grid.count - 0x100; i < (unsigned int)grid.count; i++)
        grid.dirty[i >> 8] |= 1 << ((i >> 3) & 0x1f);

    ClearDirtyCells();

    object = 0;
    stepsPerTick = 0x535;
    steps = 0;
    player = 0;
    costScale = 0;
    baseScale = 0x18000;

    for (unsigned int k = 0; k < 10; k++) {
        g_playerBudgetCap[k] = baseScale;
        budget[k] = 0;
        cursor[k] = *(Unit**)((char*)g_game + 0x1a7f + 0x14b * (k + 1));
    }
}

// FUNCTION: 0x40eb30
Pathfinder::~Pathfinder()
{
    GameFreeThunk(grid.dirty);
    operator delete(grid.cells);
    operator delete(pool);
    operator delete(items);
}

// Per-tick path search scheduler: shares this tick's step budget among the
// active players, then spends it on their path requests in turn, starting a
// new search (0x40e630) for the next unit of the current player or expanding
// the open heap of the running one until it reaches a goal or runs dry.
// FUNCTION: 0x40eb70
void Pathfinder::RunSearches()
{
    unsigned short players = g_game->numPlayers;
    if (players == 0)
        return;
    int share = stepsPerTick / players;
    int total = 0;
    if (++g_budgetRefreshCounter >= 150) {
        g_budgetRefreshCounter = 0;
        for (int i = 0; i < 10; i++) {
            int r = g_playerTickLoad[i] / g_game->unitsPerPlayer;
            if (r < 1)
                g_playerBudgetCap[i] = baseScale * 6;
            else if (r < 2)
                g_playerBudgetCap[i] = baseScale * 3;
            else if (r < 3)
                g_playerBudgetCap[i] = baseScale;
            else
                g_playerBudgetCap[i] = baseScale;
            g_playerTickLoad[i] = 0;
        }
    }
    for (int i = 0; i < 10; i++) {
        if (IsPlaying(i)) {
            budget[i] += share;
            total += budget[i];
        }
    }
    while (total > 0) {
        steps = 0;
        if (object == 0) {
            steps = 1;
            while (budget[player] <= 0) {
                if (++player >= 10)
                    player = 0;
            }
            Player* pl = &g_game->players[player];
            g_playerTickLoad[player]++;
            Unit** c = &cursor[player];
            if (*c == pl->unitsEnd)
                *c = pl->unitsBegin;
            else
                (*c)++;
            Unit* u = cursor[player];
            if (((short)u->unitDefIndex) != 0 && ((Owner_0040eb70 *)u->motion) != 0 && ((Owner_0040eb70 *)u->motion)->owner != 0) {
                path = ((Owner_0040eb70 *)u->motion)->planner->GetPath();
                if (path != 0) {
                    object = u;
                    steps += 100;
                    costScale = g_playerBudgetCap[player];
                    // The redundant ((Pathfinder*)this)-> casts stay; removing them changes ProbeStraightPath.
                    ((Pathfinder*)this)->StartSearch((Target*)path->target);
                }
            }
        // Loop is `while (1)` with the empty test inside, under this else-if.
        } else if (Size() != 0) {
            while (1) {
                if (Size() == 0)
                    break;
                steps++;
                if (topPopped) {
                    topPopped = 0;
                    int k = items[0] - pool;
                    int idx = pool[k].index;
                    ((Pathfinder*)this)->FreeNode(k);
                    count--;
                    if (idx < count) {
                        items[idx] = items[count];
                        items[idx]->index = idx;
                        ((OpenHeap*)this)->SiftDown(idx);
                    }
                }
                NodeData d = items[0]->data;
                if (!topPopped)
                    topPopped = 1;
                else
                    ((Pathfinder*)this)->RemoveNode(items[0] - pool);
                PathCell* cell = &grid.cells[grid.width * d.pos.y + d.pos.x];
                if (cell->flags & 4) {
                    found = d.pos;
                    ((Pathfinder*)this)->TracePath();
                    Release();
                    break;
                }
                cell->flags = 2;
                for (int k = -range; k <= range; k++)
                    ((Pathfinder*)this)->ExpandNeighbour(&d, cell, k & 7);
                range = 2;
                if (steps >= 100)
                    break;
            }
        } else {
            g_seams.orders->AiSearchGoalSetWaypoints(path, 0, 0);
            Release();
        }
        total -= steps;
        budget[player] -= steps;
    }
}

// Removes node `k` from the open heap: puts it back on the free list, shrinks
// the heap by one, moves the last heap element into the freed slot and sifts it
// down.
// FUNCTION: 0x40ef20
void Pathfinder::RemoveNode(int k)
{
    int idx = pool[k].index;
    Free(k);
    count--;
    if (idx < count) {
        items[idx] = items[count];
        items[idx]->index = idx;
        SiftDownInline(idx);
    }
}

// Sift-down of a binary min-heap of node pointers; each node stores its heap index.
// FUNCTION: 0x40f060
void OpenHeap::SiftDown(int i)
{
    Node* node = items[i];
    while (true) {
        int left = i * 2 + 1;
        int right = i * 2 + 2;
        if (right < count) {
            Node* l = items[left];
            Node* r = items[right];
            if (r->data.f < l->data.f) {
                if (r->data.f >= node->data.f)
                    break;
                items[i] = r;
                r->index = i;
                i = right;
            } else {
                if (l->data.f >= node->data.f)
                    break;
                items[i] = l;
                l->index = i;
                i = left;
            }
        } else {
            if (left >= count)
                break;
            Node* l = items[left];
            if (l->data.f >= node->data.f)
                break;
            items[i] = l;
            items[left]->index = i;
            i = left;
        }
    }
    items[i] = node;
    node->index = i;
}

// Pushes entry `index` onto the free list threaded through the entries.
// FUNCTION: 0x40f1e0
void Pathfinder::FreeNode(int index)
{
    pool[index].index = freeHead;
    freeHead = index;
}

#pragma auto_inline(on)
