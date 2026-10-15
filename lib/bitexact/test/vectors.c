#include <stdio.h>

#include "vectors.h"

static FILE *vec_file;
static int vec_writing;
static int vec_short;

int vec_open(const char *path)
{
#ifdef BX_REPLAY
    vec_file = fopen(path, "rb");
    vec_writing = 0;
#else
    vec_file = fopen(path, "wb");
    vec_writing = 1;
#endif
    if (vec_file == NULL) {
        printf("cannot open vectors %s\n", path);
        return 0;
    }
    setvbuf(vec_file, NULL, _IOFBF, 1 << 20);
    return 1;
}

bx_u32 vec_put(bx_u32 value)
{
    if (vec_writing)
        fwrite(&value, sizeof(value), 1, vec_file);
    return value;
}

bx_u32 vec_get(void)
{
    bx_u32 value = 0;
    if (fread(&value, sizeof(value), 1, vec_file) != 1)
        vec_short = 1;
    return value;
}

void vec_put_double(double value)
{
    union {
        double d;
        bx_u32 w[2];
    } b;
    b.d = value;
    vec_put(b.w[0]);
    vec_put(b.w[1]);
}

double vec_get_double(void)
{
    union {
        double d;
        bx_u32 w[2];
    } b;
    b.w[0] = vec_get();
    b.w[1] = vec_get();
    return b.d;
}

void vec_put_bytes(const void *data, size_t size)
{
    if (vec_writing)
        fwrite(data, 1, size, vec_file);
}

void vec_get_bytes(void *data, size_t size)
{
    if (fread(data, 1, size, vec_file) != size)
        vec_short = 1;
}

int vec_close(void)
{
    int bad = 0;
    if (vec_file == NULL)
        return 1;
    if (!vec_writing) {
        if (vec_short) {
            printf("vectors: ran out before the tests did\n");
            bad = 1;
        } else if (fgetc(vec_file) != EOF) {
            printf("vectors: values left over after the tests\n");
            bad = 1;
        }
    }
    fclose(vec_file);
    vec_file = NULL;
    return bad;
}
