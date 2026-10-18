#ifndef INTERFACES_ORDERS_H
#define INTERFACES_ORDERS_H

class AiSearchGoal;
class OrderFx;
class HapiBank;
class UnitMotion;
struct Unit;
struct Order;
struct Point16;
struct Vec3;
class MissionType;

// Major version of the orders interface, changed when a slot is removed or its signature changes.
#define ORDERS_VERSION_MAJOR 1
// Minor version of the orders interface, changed when a slot is added.
#define ORDERS_VERSION_MINOR 5

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
    // Makes a unit that was just read from a saved game take the effects of its first order back (thiscall in 3.1c: this in ecx). Hooks: none.
    void (__fastcall* ReattachFxToUnit)(Order* order);
    // Writes an order of a unit to a saved game bank under a name and returns 1 when it was written (thiscall in 3.1c: this in ecx). Hooks: none.
    int (__fastcall* SerializeOrderToSave)(Order* order, int unusedEdx, Unit* unit, HapiBank* file, char* name);
    // Adds the order that makes a unit wait to be carried. Hooks: none.
    void (__stdcall* AddBeCarriedOrder)(Unit* unit);
    // Runs the first order of a unit for one tick. Hooks: none.
    void (__stdcall* RunOrders)(Unit* unit);
    // Runs the secondary orders of a unit for one tick. Hooks: none.
    void (__stdcall* RunSecondaryOrders)(Unit* unit);
    // Frees what the movement of a unit holds (thiscall in 3.1c: this in ecx). Hooks: none.
    void (__fastcall* DestroyMotionObject)(UnitMotion* motion);
    // Reads the movement of a unit from a saved game bank (thiscall in 3.1c: this in ecx). Hooks: none.
    void (__fastcall* LoadMotion)(UnitMotion* motion, int unusedEdx, Unit* unit, HapiBank* file);
    // Writes the movement of a unit to a saved game bank (thiscall in 3.1c: this in ecx). Hooks: none.
    void (__fastcall* SaveMotion)(UnitMotion* motion, int unusedEdx, Unit* unit, HapiBank* file);
    // Steers a unit, moves it and updates its move rate for one tick (thiscall in 3.1c: this in ecx). Hooks: none.
    void (__fastcall* UpdateMotion)(UnitMotion* motion, int unusedEdx, Unit* unit);
    // Registers the order types of every module in the order type table. Hooks: none.
    void (__stdcall* RegisterAllOrderTypes)(void);
    // Empties the order type table. Hooks: none.
    void (__stdcall* ClearOrderTypeTable)(void);
    // Creates the pathfinder of a game and stores it in the game state. Hooks: none.
    void (__stdcall* CreatePathfinder)(void);
    // Destroys the pathfinder of a game and clears it in the game state. Hooks: none.
    void (__stdcall* DestroyPathfinder)(void);
};

#endif
