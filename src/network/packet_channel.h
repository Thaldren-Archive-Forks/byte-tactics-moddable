#ifndef PACKET_CHANNEL_H
#define PACKET_CHANNEL_H

#include "packet_ring.h"

class PacketBuffer;
struct NetPacketPendingEntry;

class PacketChannel {
public:
    int bufferIndex;
    unsigned int sendPacingTicks;
    PacketBuffer** buffers;
    unsigned int bufferCount;
    int frameNumber;
    int dpid;
    unsigned int timeoutTicks;
    int packetIndex;
    unsigned int queuedBytes;
    unsigned int nextSendTick;
    NetPacketPendingEntry* packets;
    unsigned int count;
    NetPacketPendingEntry* firstPacket;
    NetPacketPendingEntry* lastPacket;
    PacketRing queue;

    PacketChannel();
    ~PacketChannel();
    PacketBuffer* AllocBuffer();
    NetPacketPendingEntry* AllocPacket(int frame);
    int GetMinRetainMs();
    int GetPacketEntry(int index);
    int InitPools(int a1, unsigned int a2, int a3, int a4);
    void EnqueuePacket(NetPacketPendingEntry* item);
    int GrowPools(int growbufs, int growpackets);
    void DequeuePacket(NetPacketPendingEntry* item);
    void ResetChannel();
    int SendQueued(int force);
    int AddPacket(int frame, void* data, unsigned int size);
    void SetMinRetainMs(unsigned int ms);
    void SetSendPacingMs(int ms);
};

#endif
