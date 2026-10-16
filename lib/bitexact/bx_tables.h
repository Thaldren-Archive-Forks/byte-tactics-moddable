#ifndef BX_TABLES_H
#define BX_TABLES_H

#include "bitexact.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BX_SIN_TABLE_LEN 640
extern const bx_i16 bx_sin_table[BX_SIN_TABLE_LEN];

extern const bx_u32 bx_rad_to_angle16_bits[2];
extern const bx_u32 bx_angle16_to_rad_bits[2];

extern const bx_u32 bx_atan_sixteenths[17 * 4];

extern const bx_u32 bx_atan_series[12 * 4];

extern const bx_u32 bx_pi_pi_half[2 * 4];

extern const bx_u32 bx_sincos_table[65536 * 5];

extern const bx_u32 bx_pow10_pos[29 * 3];
extern const bx_u32 bx_pow10_neg[29 * 3];

#ifdef __cplusplus
}
#endif

#endif
