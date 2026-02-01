#include <stdint.h>
#include "common.h"
#include "common_math.h"

#include "c_test_common.h"

#ifndef LOOPS
#define LOOPS 1
#endif

#ifndef __riscv_xsimd
_Static_assert(0, "SIMD isa required for the test");
#endif

void main() {
    for (uint32_t i = 0; i < LOOPS; i++) {

        { // dup16: broadcast low 16 bits (sign-extended) into 2 lanes
            {
                GLOBAL_SYMBOL("op_dup16");
                uint16x2_t c = _dup16(0xABCDU);
                CHECK(c.v, 0xABCDABCDU, 101);
            }
            {
                GLOBAL_SYMBOL("op_dup16_zero");
                uint16x2_t c = _dup16(0);
                CHECK(c.v, 0x00000000U, 102);
            }
            {
                GLOBAL_SYMBOL("op_dup16_sign");
                uint16x2_t c = _dup16(0x8000U);  // sign-extended -> 0x80008000
                CHECK(c.v, 0x80008000U, 103);
            }
        }

        { // dup8: broadcast low 8 bits (sign-extended) into 4 lanes
            {
                GLOBAL_SYMBOL("op_dup8");
                uint8x4_t c = _dup8(0x12U);
                CHECK(c.v, 0x12121212U, 201);
            }
            {
                GLOBAL_SYMBOL("op_dup8_zero");
                uint8x4_t c = _dup8(0);
                CHECK(c.v, 0x00000000U, 202);
            }
            {
                GLOBAL_SYMBOL("op_dup8_sign");
                uint8x4_t c = _dup8(0x80U);  // sign-extended -> 0x80808080
                CHECK(c.v, 0x80808080U, 203);
            }
        }

        { // dup4: broadcast low 4 bits (sign-extended) into 8 lanes
            {
                GLOBAL_SYMBOL("op_dup4");
                uint4x8_t c = _dup4(0x5U);
                CHECK(c.v, 0x55555555U, 301);
            }
            {
                GLOBAL_SYMBOL("op_dup4_zero");
                uint4x8_t c = _dup4(0);
                CHECK(c.v, 0x00000000U, 302);
            }
            {
                GLOBAL_SYMBOL("op_dup4_sign");
                uint4x8_t c = _dup4(0x8U);  // sign-extended (0x8 = -8 in 4-bit) -> 0x88888888
                CHECK(c.v, 0x88888888U, 303);
            }
        }

        { // dup2: broadcast low 2 bits (sign-extended) into 16 lanes
            {
                GLOBAL_SYMBOL("op_dup2");
                uint2x16_t c = _dup2(0x3U);  // all lanes = 3 -> 0xFFFFFFFF
                CHECK(c.v, 0xFFFFFFFFU, 401);
            }
            {
                GLOBAL_SYMBOL("op_dup2_zero");
                uint2x16_t c = _dup2(0);
                CHECK(c.v, 0x00000000U, 402);
            }
            {
                GLOBAL_SYMBOL("op_dup2_ones");
                uint2x16_t c = _dup2(0x1U);  // all lanes = 1 -> 0x55555555
                CHECK(c.v, 0x55555555U, 403);
            }
        }
    }
    pass();
}
