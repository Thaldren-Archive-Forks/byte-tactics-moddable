#ifndef DEF_0049D270_H
#define DEF_0049D270_H

class Map_0042e440;

#pragma pack(push, 1)
struct Def_0049d270 {
    unsigned char used;
    char unknown_1[31];
    char name2[64];
    char unknown_60[4];
    Map_0042e440* sub;
    int weaponvelocity;
    int startvelocity;
    int weaponacceleration;
    void* text;
    void* anim1;
    void* anim2;
    char model[64];
    float energypershot;
    float metalpershot;
    float minbarrelangle;
    int shakemagnitude;
    int shakeduration;
    unsigned short damage;
    short areaofeffect;
    float edgeeffectiveness;
    int range;
    int coverage;
    short reloadtime;
    short weapontimer;
    short turnrate;
    short burst;
    short burstrate;
    short sprayangle;
    short duration;
    short randomdecay;
    unsigned short soundstart;
    unsigned short soundhit;
    unsigned short soundwater;
    short smokedelay;
    short flighttime;
    short holdtime;
    char unknown_100[4];
    short accuracy;
    short tolerance;
    short pitchtolerance;
    unsigned char id;
    unsigned char firestarter;
    unsigned char rendertype;
    unsigned char color;
    unsigned char color2;
    char unknown_10f[2];
    unsigned int lineofsight : 1;
    unsigned int ballistic : 1;
    unsigned int shellweapon : 1;
    unsigned int beamweapon : 1;
    unsigned int vlaunch : 1;
    unsigned int meteor : 1;
    unsigned int noradar : 1;
    unsigned int paralyzer : 1;
    unsigned int dropped : 1;
    unsigned int startsmoke : 1;
    unsigned int endsmoke : 1;
    unsigned int soundtrigger : 1;
    unsigned int guidance : 1;
    unsigned int tracks : 1;
    unsigned int unitsonly : 1;
    unsigned int groundbounce : 1;
    unsigned int waterweapon : 1;
    unsigned int toairweapon : 1;
    unsigned int smoketrail : 1;
    unsigned int turret : 1;
    unsigned int selfprop : 1;
    unsigned int propeller : 1;
    unsigned int noexplode : 1;
    unsigned int burnblow : 1;
    unsigned int twophase : 1;
    unsigned int cruise : 1;
    unsigned int commandfire : 1;
    unsigned int noautorange : 1;
    unsigned int stockpile : 1;
    unsigned int targetable : 1;
    unsigned int interceptor : 1;
    unsigned int : 1;
};
#pragma pack(pop)

#if !defined(_MSC_VER) || _MSC_VER >= 1600
static_assert(sizeof(Def_0049d270) == 277, "Def_0049d270");
#endif

#endif
