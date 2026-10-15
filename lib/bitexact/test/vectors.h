#ifndef BX_VECTORS_H
#define BX_VECTORS_H

#include "../bitexact.h"

#ifdef __cplusplus
extern "C" {
#endif

int vec_open(const char *path);
bx_u32 vec_put(bx_u32 value);
bx_u32 vec_get(void);
void vec_put_double(double value);
double vec_get_double(void);
void vec_put_bytes(const void *data, size_t size);
void vec_get_bytes(void *data, size_t size);
int vec_close(void);

#ifdef __cplusplus
}
#endif

#endif
