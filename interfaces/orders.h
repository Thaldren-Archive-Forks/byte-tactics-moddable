#ifndef INTERFACES_ORDERS_H
#define INTERFACES_ORDERS_H

class AiSearchGoal;
class OrderFx;
struct Unit;
struct Order;
struct Point16;
struct Vec3;
class MissionType;

// Major version of the orders interface, changed when a slot is removed or its signature changes.
#define ORDERS_VERSION_MAJOR 1
// Minor version of the orders interface, changed when a slot is added.
#define ORDERS_VERSION_MINOR 3

// Unit orders and command queues.
struct OrdersSeam {
    // Gives a path search goal the waypoints a search found (thiscall in 3.1c, this first). Hooks: none.
    void (__stdcall* AiSearchGoalSetWaypoints)(AiSearchGoal* self, Point16* points, int count);
    // Sets bits in the flags of an order's target (thiscall in 3.1c, this first). Hooks: none.
    void (__stdcall* OrderFxAddFlags)(OrderFx* self, int flags);
    // Adds an order to the end of a unit's order list. Hooks: none.
    void (__stdcall* AppendOrder)(Unit* owner, Order* node);
    // Gives a unit an order of a kind against a target unit or a position (the order is queued behind the others unless remove is set). Hooks: none.
    void (__stdcall* AddOrder)(MissionType kind, int remove, Unit* owner, Unit* target, Vec3* pos, int param_6, int param_7);
    // Deletes the orders of a unit, except the ones marked to stay when all is 0. Hooks: none.
    void (__stdcall* DeleteOrders)(Unit* owner, int all);
    // Returns the flags of the first order of a unit, or 0 when it has none. Hooks: none.
    int (__stdcall* GetOrderFlags)(Unit* unit);
    // Returns the kind of order a unit gets against a target unit or position for a relation mode. Hooks: none.
    MissionType (__stdcall* GetOrderType)(unsigned char mode, Unit* unit, Unit* target, Vec3* pos);
    // Orders a unit to attack a target and returns 1 when it did. Hooks: none.
    int (__stdcall* IssueAttackOrder)(Unit* unit, Unit* target, int param_3);
};

#endif
