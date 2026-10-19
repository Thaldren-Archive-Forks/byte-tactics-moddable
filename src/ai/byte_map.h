#ifndef BYTE_MAP_H
#define BYTE_MAP_H

struct MapSize_00408090 {
    unsigned int width;
    unsigned int height;

    int Contains(unsigned int tx, unsigned int ty) { return tx < width && ty < height; }
};

struct ByteMap_00408090 {
    unsigned char* data;
    MapSize_00408090 size;

    unsigned char Get(int x, int y) { return data[size.width * y + x]; }
};

#endif
