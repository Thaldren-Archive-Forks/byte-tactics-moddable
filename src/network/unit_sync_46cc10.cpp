// Decompiled by space-bunny-free. Names are provisional.
#include <vector>

#pragma pack(push, 1)
struct Elem_0046faf0 {
	unsigned char type;
	unsigned char arg;
	int id;
	int key;
	int value;
};
#pragma pack(pop)

extern "C" int __cdecl GetLocalHumanDpid();
extern "C" void __stdcall SendPacketToPlayer(int a, unsigned int b, void* c, int d);

class PacketSequencer {
public:
	int lastSent;
	char unknown_4[8];
	std::vector<Elem_0046faf0> sentQueue;
	void SendSequenced(Elem_0046faf0* to, Elem_0046faf0* packet);
};

// FUNCTION: 0x46cc10
// The local reference below is load bearing: writing sentQueue.insert(sentQueue.end(), ...)
// directly makes the front end keep the end() load rooted at ecx+0x14, which
// blocks the load CSE with the insert's own [esi + 8] read of _Last and costs
// the original's single `mov edi, ecx` in the capacity block.
void PacketSequencer::SendSequenced(Elem_0046faf0* to, Elem_0046faf0* packet)
{
	packet->id = ++lastSent;
	std::vector<Elem_0046faf0>& _v = sentQueue;
	_v.insert(_v.end(), 1, *packet);
	SendPacketToPlayer(GetLocalHumanDpid(), (unsigned int)to, packet, 0xe);
}
