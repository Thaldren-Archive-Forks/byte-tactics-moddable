#ifndef COB_TYPES_H
#define COB_TYPES_H

struct ScriptTable {
    int unknown_0;
    int count;
    int pieceCount;
    int unknown_c;
    int staticCount;
    char unknown_14[0x18 - 0x14];
    int* entries;
    char** names;
    char unknown_20[0x24 - 0x20];
    int* code;
};

struct Callback
{
    virtual void Complete(int) = 0;
};

struct Piece
{
    int active;
    union {
        struct {
            int move[3];
            int moveSpeed[3];
            int turn[3];
            int turnSpeed[3];
            int spin[3];
            int acceleration[3];
        };
        int e[6][3];
    };
};

#endif
