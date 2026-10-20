#ifndef MEMORY_CACHE_H
#define MEMORY_CACHE_H

struct GafFrame;
struct Chunk_00437a30;
struct Vec3;
union Vec3_459200;
struct Model_459200;
struct Object3do;
struct Pos_4589c0;

class CMemoryCache {
public:
    int cap;                           // +0x00, the arena's length
    union {
        int base;                      // +0x04, the arena
        int* basePointer;              // +0x04
    };
    Chunk_00437a30* cur;               // +0x08, the next chunk to hand out
    void* handle;                      // +0x0c
    union {
        GafFrame* image;               // +0x10, the scratch image
        GafFrame* bitmap;              // +0x10
        GafFrame* shadow;              // +0x10
        void* buffer;                  // +0x10
        void* ptr;                     // +0x10
        int field_10;                  // +0x10
    };

    CMemoryCache* ClearPointers();
    int InitCache(unsigned int size);
    int InitSimCache(unsigned int size);
    void FreeCache();
    void FreeBuffer();
    int AllocHandle(void** handle, int size);
    int AllocBitmap(GafFrame** handle, int w, int h);
    int AllocTwoPlaneBitmap(GafFrame** handle, int w, int h);
    void FlushCache();
    void ReleaseHandle(int handle);
    void DrawObjectState(Model_459200* list, void* context);
    void CopyPicture(GafFrame* source);
    void BuildShadow(Model_459200* model, GafFrame* dest);
    void DrawPiece(Model_459200* model, void* surface, Vec3* camera,
        Object3do* info, Vec3* vertices, unsigned char palette, int useColor);
    void DrawObjectPicture(int param_2, Model_459200* model, Vec3_459200 v, int useColor);
    void DrawObjectPieces(int param_1, Model_459200* list, Vec3 v, int param_6);
    void MergeIntoComposite(GafFrame* src, Model_459200* model);
    void AddModelBounds(int* minX, int* maxX, int* minY, int* maxY,
        Model_459200* model, Pos_4589c0 pos);
    void RecolorByShade(GafFrame* img, unsigned char level, int above, int below, int between);
    int ShadeByIntensity(GafFrame* image, Model_459200* model);
    void DrawPieceEdges(GafFrame* view, Model_459200* model, int color);
    GafFrame* MakeSilhouette(GafFrame* src);
    void MeasureShadow(int* width, int* height, int* originX, int* originY, Model_459200* model);
    void DrawShadowShape(GafFrame* view, Model_459200* model);
};

class UnitTable : public CMemoryCache {
public:
    UnitTable* Construct(void);
    int Initialize(unsigned int size);
    void Destroy();
    void MeasureModel(int* width, int* height, int* originX, int* originY, Model_459200* model, Vec3* offset);
    int BuildObjectPicture(Model_459200* list, int param_2, int param_3);
    void DrawPieces(GafFrame* bitmap, Model_459200* list, int kind, int useColor);
    void DrawLitPieces(GafFrame* bitmap, Model_459200* list, int kind, int useColor);
};

#endif
