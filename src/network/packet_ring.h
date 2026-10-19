#ifndef PACKET_RING_H
#define PACKET_RING_H

struct NetPacketPendingEntry;

class PacketRing {
public:
    int count;
    int readIdx;
    int writeIdx;
    NetPacketPendingEntry* buf[0x400];

    PacketRing();
    int PushPacket(NetPacketPendingEntry* value);
    NetPacketPendingEntry* PopPacket();
    NetPacketPendingEntry* Peek()
    {
        if (count > 0)
            return buf[readIdx];
        return 0;
    }
    NetPacketPendingEntry* Pop()
    {
        if (count > 0) {
            count--;
            NetPacketPendingEntry* value = buf[readIdx];
            readIdx++;
            if (readIdx >= 0x400)
                readIdx = 0;
            return value;
        }
        return 0;
    }
    int Push(NetPacketPendingEntry* value)
    {
        if (count < 0x400) {
            writeIdx = writeIdx + 1;
            if (writeIdx >= 0x400) {
                writeIdx = 0;
            }
            buf[writeIdx] = value;
            count = count + 1;
            return 1;
        }
        return 0;
    }
};

#endif
