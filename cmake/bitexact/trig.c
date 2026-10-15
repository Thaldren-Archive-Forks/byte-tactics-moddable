#include "bitexact.h"

extern bx_i32 g_rotationMatrix[9];

// FUNCTION: 0x4b70a0
bx_i32 __cdecl FixedSine(int angle)
{
    return bx_FixedSine((bx_u16)angle);
}

bx_i32 __cdecl FUN_004b70c0(int angle)
{
    return bx_fixed_cos((bx_u16)angle);
}

bx_i32 __cdecl FUN_004b70e0(bx_i32 a, bx_i32 b)
{
    return bx_mul_hi32(a, b);
}

bx_i32 __cdecl FUN_004b70ef(int angle, bx_i32 scale)
{
    return bx_FUN_004b70ef((bx_i16)angle, scale);
}

bx_i32 __cdecl FUN_004b7123(int angle, bx_i32 scale)
{
    return bx_FUN_004b7123((bx_i16)angle, scale);
}

bx_i32 __cdecl FUN_004b715a(bx_i32 x, bx_i32 z)
{
    return bx_FUN_004b715a(x, z);
}

void __cdecl FUN_004b7173(int angle, bx_i32 *xz)
{
    bx_FUN_004b7173((bx_i16)angle, xz);
}

void __cdecl FUN_004b71a7(const char *object)
{
    bx_build_rotation_matrix((const bx_i16 *)(object + 0xc), g_rotationMatrix);
}

void __cdecl FUN_004b72e2(const bx_i32 *in, bx_i32 *out)
{
    bx_apply_rotation_matrix(g_rotationMatrix, in, out);
}

bx_i32 __cdecl FUN_004b7381(bx_i32 a, bx_i32 b, bx_i32 c)
{
    return bx_FUN_004b7381(a, b, c);
}
