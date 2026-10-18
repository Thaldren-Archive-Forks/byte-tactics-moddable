#ifndef PATHFINDER_H
#define PATHFINDER_H

#include <vector>

#include "../../interfaces/seams.h"
#include "../state/block.h"
#include "../units/movement_class.h"
#include "../util/vec3.h"

struct Unit;
class AiSearchGoal;

void __cdecl operator delete(void* p);

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
    Point16 pos;
    int g;
    int f;
    short penalty;
    short steps;
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
    int index;
    NodeData data;
};

struct PathCell {
    unsigned char flags;
    unsigned char dir;
    unsigned short node;
};

struct Grid {
    PathCell* cells;
    unsigned int width;
    unsigned int height;
    int count;
    unsigned int* dirty;
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


class OpenHeap {
public:
    Node* pool;
    Node** items;
    int freeHead;
    int used;
    int capacity;
    int count;
    int topPopped;

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
    Grid grid;
    Point16 start;
    Point16 found;
    int goalX;
    int goalY;
    int probe;
    int range;
    int stepsPerTick;
    int steps;
    int costScale;
    int baseScale;
    Unit* object;
    AiSearchGoal* path;
    Target* target;
    MovementClass* owner;
    union {
        Table_0040d880 table;
        unsigned char turnCost[8];
    };
    union {
        Pair_0040d880 pairs[4];
        unsigned char stepCost[8];
    };
    unsigned char player;
    Unit* cursor[10];
    int budget[10];

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
    void RemoveNode(int k);
    // FUNCTION: 0x40f110 ?GrowNodes@Pathfinder@@QAEXH@Z
    void GrowNodes(int n)
    {
        int cap = capacity;
        if (n < cap)
            n = cap + (cap >> 1) + 0x10;
        Node* newe = (Node*)SimAlloc(n * 0x14);
        int i;
        for (i = 0; i < used; i++)
            *(newe + i) = pool[i];
        operator delete(pool);
        Node** newp = (Node**)SimAlloc(n * 4);
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
