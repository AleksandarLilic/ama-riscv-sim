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

        { // vins16: insert scalar into lane 0 or 1
            {
                GLOBAL_SYMBOL("op_vins16_zero_l0");
                uint16x2_t v = { .v = 0u };
                uint16x2_t c = _vins16(v, 0xABCDu, 0);
                CHECK(c.v, 0x0000ABCDu, 101);
            }
            {
                GLOBAL_SYMBOL("op_vins16_zero_l1");
                uint16x2_t v = { .v = 0u };
                uint16x2_t c = _vins16(v, 0xABCDu, 1);
                CHECK(c.v, 0xABCD0000u, 102);
            }
            {
                GLOBAL_SYMBOL("op_vins16_ones_l0");
                uint16x2_t v = { .v = 0xFFFFFFFFu };
                uint16x2_t c = _vins16(v, 0x1234u, 0);
                CHECK(c.v, 0xFFFF1234u, 103);
            }
        }

        { // vins8: insert scalar into lane 0..3
            {
                GLOBAL_SYMBOL("op_vins8_zero_l0");
                uint8x4_t v = { .v = 0u };
                uint8x4_t c = _vins8(v, 0xABu, 0);
                CHECK(c.v, 0x000000ABu, 201);
            }
            {
                GLOBAL_SYMBOL("op_vins8_zero_l2");
                uint8x4_t v = { .v = 0u };
                uint8x4_t c = _vins8(v, 0xABu, 2);
                CHECK(c.v, 0x00AB0000u, 202);
            }
            {
                GLOBAL_SYMBOL("op_vins8_ones_l0");
                uint8x4_t v = { .v = 0xFFFFFFFFu };
                uint8x4_t c = _vins8(v, 0x12u, 0);
                CHECK(c.v, 0xFFFFFF12u, 203);
            }
        }

        { // vins4: insert scalar into lane 0..7
            {
                GLOBAL_SYMBOL("op_vins4_zero_l0");
                uint4x8_t v = { .v = 0u };
                uint4x8_t c = _vins4(v, 0x5u, 0);
                CHECK(c.v, 0x00000005u, 301);
            }
            {
                GLOBAL_SYMBOL("op_vins4_zero_l5");
                uint4x8_t v = { .v = 0u };
                uint4x8_t c = _vins4(v, 0x5u, 5);
                CHECK(c.v, 0x00500000u, 302);
            }
            {
                GLOBAL_SYMBOL("op_vins4_ones_l0");
                uint4x8_t v = { .v = 0xFFFFFFFFu };
                uint4x8_t c = _vins4(v, 0x5u, 0);
                CHECK(c.v, 0xFFFFFFF5u, 303);
            }
        }

        { // vins2: insert scalar into lane 0..15
            {
                GLOBAL_SYMBOL("op_vins2_zero_l0");
                uint2x16_t v = { .v = 0u };
                uint2x16_t c = _vins2(v, 1u, 0);
                CHECK(c.v, 0x00000001u, 401);
            }
            {
                GLOBAL_SYMBOL("op_vins2_zero_l1");
                uint2x16_t v = { .v = 0u };
                uint2x16_t c = _vins2(v, 1u, 1);
                CHECK(c.v, 0x00000004u, 402);
            }
            {
                GLOBAL_SYMBOL("op_vins2_ones_l0");
                uint2x16_t v = { .v = 0xFFFFFFFFu };
                uint2x16_t c = _vins2(v, 0u, 0);
                CHECK(c.v, 0xFFFFFFFCu, 403);
            }
        }
    }
    pass();
}
