#ifndef BX_TABLES_H
#define BX_TABLES_H

#include "bitexact.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The exe's bytes 0x509f00..0x50a400: the sine table, and the cosine table a quarter turn (128 entries) on. */
#define BX_SIN_TABLE_LEN 640
extern const bx_i16 bx_sin_table[BX_SIN_TABLE_LEN];

/* The doubles at 0x509ef0 and 0x509ef8 as {high word, low word}. */
extern const bx_u32 bx_rad_to_angle16_bits[2];
extern const bx_u32 bx_angle16_to_rad_bits[2];

/* atan(k / 16) for k = 0..16 as double-double {hi high, hi low, lo high, lo low}. */
extern const bx_u32 bx_atan_sixteenths[17 * 4];

/* pi, then pi / 2, as double-double in the same layout. */
extern const bx_u32 bx_pi_pi_half[2 * 4];

/* Per 16-bit angle (index: the angle as unsigned), the 80-bit fsincos results as {sin mantissa high, sin low, cos mantissa high, cos low, sin sign/exponent in the high half and cos in the low half}. */
extern const bx_u32 bx_sincos_table[65536 * 5];

#ifdef __cplusplus
}
#endif

#endif
