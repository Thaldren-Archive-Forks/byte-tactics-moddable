// Decompiled by DeepSeek V4.1 Flash. Names are provisional.
// Stays in a file of its own: this destructor matches only with the inline
// PacketReceiver, PlayerFrameInfo and FrameQueue destructors below, where
// packets.cpp needs them out of line. The compiler-generated static
// destructor (_$E2) of the global object g_packetManager, whose dynamic
// initialiser is 0x460e20 and whose out-of-line destructor is 0x461420.

void __cdecl operator delete(void*);

#include "packet_manager.h"

inline FrameQueue::~FrameQueue()
{
    operator delete(buffer);
    operator delete(recvBuffer);
}

inline PlayerFrameInfo::~PlayerFrameInfo()
{
    operator delete(frame);
}

inline PacketReceiver::~PacketReceiver()
{
    void* p = spare;
    if (!p)
        p = buffer;
    operator delete(p);
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

inline PacketManager::~PacketManager() { }

// FUNCTION: 0x460f60 _$E2
PacketManager g_packetManager;
