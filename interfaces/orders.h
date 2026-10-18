#ifndef INTERFACES_ORDERS_H
#define INTERFACES_ORDERS_H

class AiSearchGoal;
class OrderFx;
struct Unit;
struct Order;
struct Point16;

// Major version of the orders interface, changed when a slot is removed or its signature changes.
#define ORDERS_VERSION_MAJOR 1
// Minor version of the orders interface, changed when a slot is added.
#define ORDERS_VERSION_MINOR 2

// Unit orders and command queues.
struct OrdersSeam {
    // Gives a path search goal the waypoints a search found (thiscall in 3.1c, this first). Hooks: none.
    void (__stdcall* AiSearchGoalSetWaypoints)(AiSearchGoal* self, Point16* points, int count);
    // Sets bits in the flags of an order's target (thiscall in 3.1c, this first). Hooks: none.
    void (__stdcall* OrderFxAddFlags)(OrderFx* self, int flags);
    // Adds an order to the end of a unit's order list. Hooks: none.
    void (__stdcall* AppendOrder)(Unit* owner, Order* node);
};

#endif
