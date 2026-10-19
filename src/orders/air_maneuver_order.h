#ifndef AIR_MANEUVER_ORDER_H
#define AIR_MANEUVER_ORDER_H

#include "../util/vec3.h"
#include "order_fx.h"

class BitReader;
struct Owner_0044e9c0;

#pragma pack(push, 2)
class __declspec(novtable) AirManeuverOrder : public OrderFx {
public:
    union {
        unsigned short flags;          // +0x8, bit 0: heading set
        struct {
            unsigned short headingSet : 1;
            unsigned short unknown_rest : 15;
        };
    };
    Vec3 target;                       // +0xa
    Vec3 other;                        // +0x16
    short saveOnly22;                  // +0x22
    unsigned short heading;            // +0x24
    unsigned short value_26;           // +0x26
    Unit* self;                        // +0x28

    AirManeuverOrder(Order* order, const Vec3& a, const Vec3& b);
    AirManeuverOrder(int owner, HapiBank* file, char* name);
    AirManeuverOrder(Owner_0044e9c0* owner, BitReader* reader);
    void* Destroy(int flag);
    int SerializeToSave(Order* order, HapiBank* file, char* name);
    int GetDesiredHeading(unsigned short* out);
    void SetAltitude(int altitude);
    int FillWorldPos(Vec3* out);
    void SerializeToBits(BitWriter* stream);
    int IsComplete(Unit* unit);
    int GetType();
    int IsFxStyle();
    int KeepAfterComplete();
    void SetHeading(short v);
};
#pragma pack(pop)

#endif
