// Decompiled by DeepSeek V4.1 Flash, finished by Claude Opus 5.5. Names are provisional.
// Stays in a file of its own: this initialiser matches only with the views of
// PacketChannel and PacketRing below, which packets.cpp cannot share.
// The compiler-generated dynamic initialiser (_$E4) of the global
// g_packetManager (vtable 0x4fd514): it runs the constructor (0x4611e0) and
// registers the atexit destructor _$E2 (0x460f60).
//
// Layout: m_defaultSendPacingMs at +0x04 (the name comes from 0x461020's
// debug string), eleven 0x1044-byte per-player channels from +0x08, a
// {buffer, used, capacity} triple at +0xb2f4 and a PacketReceiver member at
// +0xb300 that is handed the owner. The destructors 0x460f60, 0x461340 and
// 0x461420 walk "entries at +0x10": that is each channel's own +0x08 (the
// items/count pair), not a second array.

#include "packet_ring.h"

inline PacketRing::PacketRing() { count = 0; readIdx = 0; writeIdx = -1; }

#include "packet_receiver.h"

// capacity stays in the initialiser list: the vtable store must come after it.
inline PacketReceiver::PacketReceiver(void* o)
    : unused(0), owner(o), fromId(-1), toId(-1), savedFrameEntry(0), buffer(0), spare(0),
      capacity(0), length(0), spareLength(0), spareFromId(-1), spareToId(-1)
{
}

inline PacketReceiver::~PacketReceiver()
{
    void* p = spare;
    if (!p)
        p = buffer;
    operator delete(p);
}

#include "packet_channel.h"

inline PacketChannel::PacketChannel()
    : bufferIndex(-1), sendPacingTicks(0), buffers(0), bufferCount(0), frameNumber(-2), dpid(-1), timeoutTicks(0),
      packetIndex(-1), queuedBytes(0), nextSendTick(0), packets(0), count(0), firstPacket(0), lastPacket(0)
{
    SetMinRetainMs(4000);
    SetSendPacingMs(200);
}

inline PacketChannel::~PacketChannel()
{
    if (buffers) {
        for (unsigned i = 0; i < bufferCount; i++)
            operator delete(buffers[i]);
        operator delete(buffers);
    }
    operator delete(packets);
}

class PacketManager {
public:
    virtual ~PacketManager() { }
    int m_defaultSendPacingMs;         // +0x04
    PacketChannel channels[11];        // +0x08
    char* buffer;                      // +0xb2f4
    int used;                          // +0xb2f8
    int capacity;                      // +0xb2fc
    PacketReceiver member;             // +0xb300

    PacketManager();
};

// FUNCTION: 0x460e20 _$E4
PacketManager g_packetManager;

PacketManager::PacketManager() : m_defaultSendPacingMs(200), buffer(0), used(0), capacity(0), member(this) { }
