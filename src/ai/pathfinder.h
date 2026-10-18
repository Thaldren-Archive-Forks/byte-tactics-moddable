#ifndef PATHFINDER_H
#define PATHFINDER_H

#include <vector>

#include "../../interfaces/seams.h"
#include "../units/movement_class.h"
#include "../util/vec3.h"

struct Unit;
class AiSearchGoal;

void* __cdecl operator new(unsigned int size);
void __cdecl operator delete(void* p);

// What the search is looking for.
class Target {
public:
    virtual void unused0();
    virtual void unused1();
    virtual void unused2();
    virtual void unused3();
    virtual void unused4();
    virtual int IsGoal(int x, int y);
    virtual void GetGoals(std::vector<Point16>& goals);
    virtual int Cost(int x, int y);
};

struct NodeData {
    Point16 pos;                       // +0x0
    int g;                             // +0x4
    int f;                             // +0x8, the heap key
    short penalty;                     // +0xc
    short steps;                       // +0xe, steps since the last turn
    NodeData() {}
    NodeData(short x, short y, int g_, int f_, short steps_)
    {
        pos.x = x;
        pos.y = y;
        g = g_;
        f = f_;
        steps = steps_;
    }
};

struct Node {
    int index;                         // +0x0 heap slot, or next free node
    NodeData data;                     // +0x4
};

struct PathCell {
    unsigned char flags;               // +0 bit 0 open, bit 2 goal, bit 3 visited
    unsigned char dir;                 // +1 the step taken into this cell
    unsigned short node;               // +2 index into the node pool
};

struct Grid {
    PathCell* cells;                    // +0x0
    unsigned int width;                // +0x4
    unsigned int height;               // +0x8
    int count;                         // +0xc
    unsigned int* dirty;               // +0x10, one bit per 8 cells
    int InBounds(unsigned int x, unsigned int y)
    {
        return x < width && y < height;
    }
    void Set(unsigned int x, unsigned int y, unsigned char kind)
    {
        unsigned int i = width * y + x;
        cells[i].flags = kind;
        dirty[i >> 8] |= 1 << ((i >> 3) & 0x1f);
    }
    unsigned int Index(unsigned int x, unsigned int y)
    {
        return width * y + x;
    }
    PathCell* At(int x, int y)
    {
        return &cells[width * y + x];
    }
    void ClearBlock(int i, int last)
    {
        if (dirty[i]) {
            unsigned int bits = dirty[i];
            dirty[i] = 0;
            PathCell* p = &cells[i * 256];
            while (bits) {
                if (bits & 1) {
                    int c = i << 8;
                    PathCell* q = p;
                    for (int k = 8; k; k--) {
                        if (!last || c < count)
                            q->flags = 0;
                        c++;
                        q++;
                    }
                }
                bits >>= 1;
                p += 8;
            }
        }
    }
    // The dead `t` and its folded branch emit nothing, but the store keeps
    // MSVC from propagating the `cells` load into the pointer add, so the load
    // lands after the `dirty[i] = 0` store as in the original.
    void ClearLast(int i)
    {
        if (dirty[i]) {
            unsigned int bits = dirty[i];
            dirty[i] = 0;
            PathCell* p = cells;
            int t = 0;
            if (t)
                bits = 0;
            p += i * 256;
            while (bits) {
                if (bits & 1) {
                    int c = i << 8;
                    PathCell* q = p;
                    for (int k = 8; k; k--) {
                        if (c < count)
                            q->flags = 0;
                        c++;
                        q++;
                    }
                }
                bits >>= 1;
                p += 8;
            }
        }
    }
};

struct Table_0040d880 {
    unsigned char values[8];
};

struct Pair_0040d880 {
    unsigned char a;
    unsigned char b;
};

extern const Table_0040d880 g_turnCosts;

static int FixMul(int a, int b)
{
    return (int)(((__int64)a * b) >> 0x10);
}


// The open list: a binary min-heap of node pointers over a pool of nodes,
// each node holding its heap slot (or, when free, the next free node).
class OpenHeap {
public:
    Node* pool;                        // +0x0
    Node** items;                      // +0x4
    int freeHead;                      // +0x8
    int used;                          // +0xc
    int capacity;                      // +0x10
    int count;                         // +0x14
    int topPopped;                     // +0x18

    // FUNCTION: 0x40f000 ?SiftUp@OpenHeap@@QAEXH@Z
    void SiftUp(int i)
    {
        if (i != 0) {
            int parent = (i - 1) >> 1;
            Node* node = items[i];
            Node* p = items[parent];
            if (node->data.f < p->data.f) {
                items[i] = p;
                p->index = i;
                i = parent;
                while (i != 0) {
                    parent = (i - 1) >> 1;
                    p = items[parent];
                    if (node->data.f >= p->data.f) break;
                    items[i] = p;
                    p->index = i;
                    i = parent;
                }
                items[i] = node;
                node->index = i;
            }
        }
    }
    void SiftDown(int i);
    // RemoveNode inlines this copy; the out-of-line 0x40f060 below is what the
    // other methods call.
    void SiftDownInline(int i)
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
    void Clear()
    {
        count = 0;
        freeHead = -1;
        used = 0;
        topPopped = 0;
    }
    int Size()
    {
        return count - topPopped;
    }
    void Free(int k)
    {
        pool[k].index = freeHead;
        freeHead = k;
    }
    void Remove(int k)
    {
        int idx = pool[k].index;
        Free(k);
        count--;
        if (idx < count) {
            items[idx] = items[count];
            items[idx]->index = idx;
            SiftDown(idx);
        }
    }
    void Pop()
    {
        int idx = items[0] - pool;
        Node* n = pool + idx;
        int pos = n->index;
        n->index = freeHead;
        freeHead = idx;
        int last = --count;
        if (pos < last) {
            items[pos] = items[last];
            items[pos]->index = pos;
            SiftDown(pos);
        }
    }
    void Update(int k)
    {
        if (topPopped) {
            Node* top = items[0];
            SiftUp(pool[k].index);
            if (top->index != 0) {
                topPopped = 0;
                Remove(top - pool);
            }
        } else {
            SiftUp(pool[k].index);
        }
    }
};

#pragma pack(push, 1)
class Pathfinder : public OpenHeap {
public:
    Grid grid;                         // +0x1c
    Point16 start;                     // +0x30
    Point16 found;                     // +0x34, the goal cell reached
    int goalX;                         // +0x38
    int goalY;                         // +0x3c
    int probe;                         // +0x40, the straight-line probe's cost
    int range;                         // +0x44, the turns tried per node
    int stepsPerTick;                  // +0x48
    int steps;                         // +0x4c
    int costScale;                     // +0x50
    int baseScale;                     // +0x54
    Unit* object;                      // +0x58
    AiSearchGoal* path;                // +0x5c
    Target* target;                    // +0x60
    MovementClass* owner;              // +0x64
    union {
        Table_0040d880 table;          // +0x68
        unsigned char turnCost[8];
    };
    union {
        Pair_0040d880 pairs[4];        // +0x70
        unsigned char stepCost[8];
    };
    unsigned char player;              // +0x78
    Unit* cursor[10];                  // +0x79
    int budget[10];                    // +0xa1

    // The heuristic estimate: caches the scale before the virtual call, then
    // truncates the 64-bit product. The original inlines it here; the
    // out-of-line 0x40da40 below is the same arithmetic returned 64 bits wide.
    int EstimateCost(int x, int y)
    {
        int factor = costScale;
        int v = target->Cost(x, y);
        return (int)(((__int64)v * factor) >> 16);
    }
    int Cost(int x, int y)
    {
        int s = costScale;
        return (int)(((__int64)s * target->Cost(x, y)) >> 0x10);
    }
    void ResetTable()
    {
        table = g_turnCosts;
        pairs[0].a = pairs[1].a = pairs[2].a = pairs[3].a = 0x10;
        pairs[0].b = pairs[1].b = pairs[2].b = pairs[3].b = 0x16;
    }
    void MarkGoal(unsigned int x, unsigned int y)
    {
        if (grid.InBounds(x, y))
            grid.Set(x, y, 4);
    }
    void Release()
    {
        g_seams.units->RefreshUnitIfStale(owner, object);
        object = 0;
        owner = 0;
    }
    void Finish()
    {
        g_seams.orders->AiSearchGoalSetWaypoints(path, 0, 0);
        Release();
    }
    int CostFix(int x, int y)
    {
        int s = costScale;
        return FixMul(s, target->Cost(x, y));
    }
    unsigned int Passable(int x, int y)
    {
        return GetCellState(x, y);
    }
    unsigned char Visit(unsigned int x, unsigned int y, char dir)
    {
        unsigned int i = grid.width * y + x;
        grid.dirty[i >> 8] |= 1 << ((i >> 3) & 0x1f);
        PathCell* c = &grid.cells[i];
        c->dir = dir;
        return c->flags |= 8;
    }
    int OnLine(int x, int y, int nx, int ny)
    {
        int gdx = goalX - x;
        int gdy = goalY - y;
        int px = nx - x;
        int py = ny - y;
        if (gdx < 0) {
            gdx = -gdx;
            px = -px;
        }
        if (gdy < 0) {
            gdy = -gdy;
            py = -py;
        }
        if (py == 0 && px > 0 && px <= gdx)
            return 1;
        if (px == gdx && py > 0 && py <= gdy)
            return 1;
        return 0;
    }
    int Dir(int x, int y)
    {
        int d = goalX - x;
        if (d < 0)
            return 2;
        if (d > 0)
            return 6;
        return goalY - y > 0 ? 4 : 0;
    }
    short Steps(NodeData* from, int turn)
    {
        return turn ? 1 : from->steps + 1;
    }
    int Push(const NodeData& d);

    int GetCellState(int x, int y);
    void InitCostTables();
    void MarkGoalCell(unsigned int x, unsigned int y);
    void ClearDirtyCells();
    __int64 Estimate(int param1, int param2);
    // Inlines the node pool's growth (0x40f110) that StartSearch calls out of
    // line.
    void ExpandNeighbour(NodeData* from, PathCell* fromCell, int turn);
    int ExpandBestNode();
    void TracePath();
    int ProbeStraightPath();
    void StartSearch(Target* t);
    void ReleaseHeldPathLock();
    void AbortIfGoalMatch(int goal);
    Pathfinder();
    ~Pathfinder();
    void RunSearches();
    // Inlines the sift-down (0x40f060) the other methods call out of line.
    void RemoveNode(int k);
    // FUNCTION: 0x40f110 ?GrowNodes@Pathfinder@@QAEXH@Z
    void GrowNodes(int n)
    {
        int cap = capacity;
        if (n < cap)
            n = cap + (cap >> 1) + 0x10;
        Node* newe = (Node*)operator new(n * 0x14);
        int i;
        for (i = 0; i < used; i++)
            *(newe + i) = pool[i];
        operator delete(pool);
        Node** newp = (Node**)operator new(n * 4);
        for (i = 0; i < count; i++)
            newp[i] = newe + (items[i] - pool);
        operator delete(items);
        pool = newe;
        items = newp;
        capacity = n;
    }
    void FreeNode(int index);
};
#pragma pack(pop)

#endif
