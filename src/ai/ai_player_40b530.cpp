// Decompiled by GPT-6. Names are provisional.
struct Unit;
void __stdcall CopyDwordIfNonNull(Unit**, Unit* const*);
namespace std {
inline void _Construct(Unit** dest, Unit* const& src) { CopyDwordIfNonNull(dest, &src); }
}
#include <vector>
#include "../state/sim_allocator.h"
#include "vec.h"
#pragma pack(push,1)
struct Def { char pad[0x241]; unsigned flags; };
#include "../units/unit.h"
#include "../state/sim_globals.h"
struct Owner { char pad[0x25]; SimVector<Unit*> factories; };
#pragma pack(pop)
typedef Owner* SimView_g_playerAI[];
#undef g_playerAI
#define g_playerAI (*reinterpret_cast<SimView_g_playerAI*>(&Sim().g_playerAI))
// FUNCTION: 0x40b530
void __stdcall GetFactoriesInRadius(int player,const Vec* pos,int radius,std::vector<Unit*>* out)
{
    SimVector<Unit*>& list=g_playerAI[player]->factories;
    radius*=radius;
    for(SimVector<Unit*>::iterator it=list.begin();it!=list.end();++it) {
        Unit* unit=*it;
        if((((Def*)unit->def)->flags&0x40) && (((Def*)unit->def)->flags&0x200) && (unit->activateFlags&1)) {
            int dz=pos->z-unit->pos.z;
            int dx=pos->x-unit->pos.x;
            int d=(int)(((__int64)dx*dx)>>32)+(int)(((__int64)dz*dz)>>32);
            if(d<=radius) out->push_back(*it);
        }
    }
}
