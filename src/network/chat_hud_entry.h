#ifndef CHAT_HUD_ENTRY_H
#define CHAT_HUD_ENTRY_H

#pragma pack(push, 1)
struct ChatHudEntry {
    char text[0x40];
    union { unsigned int time; };
    unsigned short unit;
    union {
        char player;
        unsigned char unit_46;
    };
    unsigned char flags;
};
#pragma pack(pop)

#if !defined(_MSC_VER) || _MSC_VER >= 1600
static_assert(sizeof(ChatHudEntry) == 72, "ChatHudEntry");
#endif

#endif
