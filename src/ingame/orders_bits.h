#ifndef ORDERS_BITS_H
#define ORDERS_BITS_H

#pragma pack(push, 1)
struct OrdersBits {
    unsigned short unknown_0 : 1;
    unsigned short refresh : 1;
    unsigned short unknown_2 : 2;
    unsigned short flag_4 : 1;
    unsigned short unknown_5 : 2;
    unsigned short next : 1;
    unsigned short prev : 1;
    unsigned short orders : 1;
    unsigned short build : 1;
    unsigned short unknown_11 : 1;
    unsigned short fireOrder : 3;
    unsigned short unknown_15 : 1;
    unsigned short moveOrder : 3;
    unsigned short cloak : 2;
    unsigned short onOff : 2;
    unsigned short canMove : 1;
    unsigned short canStop : 1;
    unsigned short canAttack : 1;
    unsigned short canDefend : 1;
    unsigned short canPatrol : 1;
    unsigned short canLoad : 1;
    unsigned short canReclaim : 1;
    unsigned short canCapture : 1;
    unsigned short canRepair : 1;
    unsigned short canBlast : 1;
    unsigned short unknown_1 : 15;
};
#pragma pack(pop)

#if defined(__clang__) || !defined(_MSC_VER) || _MSC_VER >= 1600
static_assert(sizeof(OrdersBits) == 6, "OrdersBits");
#endif

#endif
