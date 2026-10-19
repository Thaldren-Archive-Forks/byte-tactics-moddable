#ifndef PACKET_MANAGER_H
#define PACKET_MANAGER_H

#include "packet_channel.h"
#include "packet_receiver.h"

struct Msg_00461900;

class PacketManager {
public:
    virtual ~PacketManager();
    unsigned long m_defaultSendPacingMs;
    PacketChannel channels[11];
    unsigned char* buffer;
    unsigned int size;
    unsigned int capacity;
    PacketReceiver member;

    PacketManager();
    int AppendToSendBuffer(unsigned char* data, unsigned int len);
    void ClearSendBuffer();
    void NopRet_D();
    PacketChannel* FindChannel(int dpid, int create);
    int InitChannels(int arg1, int arg2);
    void ReleaseChannel(int id);
    void ReleaseAllChannels();
    int SendAllQueued(int force);
    int SendFrameState(int from, Msg_00461900* msg);
    int QueueOnChannel(int frame, PacketChannel* channel, int data, int len);
    int QueuePacket(int frame, int dpid, void* data, unsigned int len);
    void SetDefaultSendPacing(int rate);
    void HandleIntegrityNop(int, int, int);
};

#endif
