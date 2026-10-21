// Decompiled by space-bunny-free, finished by deepseek-v4.1-flash, finished by GPT-6.1-sol, edited by deepseek-v4.1, finished by deepseek-v4.1-flash, finished by GPT-6.1-sol, finished by Space Bunny Free, verified by GPT-6., retried by Claude Opus 5.5, matched by claude-opus-5-5. Names are provisional.
// Keep <windows.h> and fewer than 58 symbols after it: the symbol count sets
// the register choice.
#include "win_types.h"

struct Vertex_0045a610 {
    int x;
    int y;
    int z;
};

#include "gaf_frame.h"
#include "model_types.h"
#include "memory_cache.h"

#pragma pack(push, 1)
struct Map_00458fa0 {
    char unknown_0[0x241];
    unsigned int unknown_241 : 30;
    unsigned int brightFaces : 1;    // +0x241 bit 30, the wider field
    unsigned int unknown_242 : 1;
};
#pragma pack(pop)

void __stdcall DrawPolygonEdges(GafFrame* view, Vertex_0045a610* points, int count, int color);

// FUNCTION: 0x458fa0
void CMemoryCache::DrawPieceEdges(GafFrame* view, Model_459200* model, int color)
{
    Vertex_0045a610 verts[2000];
    Vertex_0045a610 tmp[25];
    int faceno = 0;
    int i = model->count - 1;
    if (i >= 0) {
        Piece_459c70* piece = &model->pieces[i];
        while (i >= 0) {
            // Byte flags local plus the empty do-while set the reload order after the copy loop.
            unsigned char flags = piece->flags.word;
            do {} while (0);  // emits no code; the match needs it (header note 2)
            if (flags & 1) {
                Object3do* info = piece->object;
                Vertex_0045a610* v = (Vertex_0045a610*)piece->points;
                for (int j = 0; j < info->vertexCount; j++, v++) {
                    int bright = ((Map_00458fa0*)model->owner->def)->brightFaces ? 125 : 50;
                    int x = (short)(v->x >> 16);
                    int y = (short)(v->y >> 16);
                    int z = (short)(-v->z >> 16);
                    verts[j].x = x;
                    verts[j].y = z - (y >> 1);
                    verts[j].z = y + bright;
                    verts[j].x += view->xOffset;
                    verts[j].y += view->yOffset;
                }
                // f is assigned in both arms, not set before the if.
                Face_459c70* f;
                if (info->firstFace != -1) {
                    f = (Face_459c70*)info->faces + 1;
                    faceno = 1;
                } else {
                    f = (Face_459c70*)info->faces;
                    faceno = 0;
                }
                for (; faceno < info->faceCount; faceno++, f++) {
                    unsigned short* ip = f->indices;
                    int k;
                    for (k = 0; k < f->count; k++, ip++)
                        tmp[k] = verts[*ip];
                    tmp[k] = tmp[0];
                    DrawPolygonEdges(view, tmp, f->count + 1, color);
                }
            }
            piece--;
            i--;
        }
    }
}
