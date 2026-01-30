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

        { // min16
            {
                GLOBAL_SYMBOL("op_min16");
                int16_t a[] = {100, -49};
                int16_t b[] = {200, -50};
                int16x2_t c = _min16(v_load_int16x2(a), v_load_int16x2(b));
                CHECK(c.v, PK_I16(100, -50), 101);
            }
            {
                GLOBAL_SYMBOL("op_min16_sign_diff");
                // critical: -1 is smaller than 1
                // if treated as unsigned, 1 is smaller
                int16_t a[] = {-1, 0};
                int16_t b[] = {1, 0};
                int16x2_t c = _min16(v_load_int16x2(a), v_load_int16x2(b));
                CHECK(c.v, PK_I16(-1, 0), 102);
            }
            {
                GLOBAL_SYMBOL("op_min16_equal");
                int16_t a[] = {33, -33};
                int16_t b[] = {33, -33};
                int16x2_t c = _min16(v_load_int16x2(a), v_load_int16x2(b));
                CHECK(c.v, PK_I16(33, -33), 103);
            }
        }

        { // min16u
            {
                GLOBAL_SYMBOL("op_min16u");
                uint16_t a[] = {100, 50000};
                uint16_t b[] = {200, 10000};
                uint16x2_t c = _min16u(v_load_uint16x2(a), v_load_uint16x2(b));
                CHECK(c.v, PK_I16(100, 10000), 111);
            }
            {
                GLOBAL_SYMBOL("op_min16u_high_bit");
                // critical: 1 is smaller than 65535
                // if treated as signed, 65535 (-1) is smaller
                uint16_t a[] = {0xFFFF, 0};
                uint16_t b[] = {1, 0};
                uint16x2_t c = _min16u(v_load_uint16x2(a), v_load_uint16x2(b));
                CHECK(c.v, PK_I16(1, 0), 112);
            }
            {
                GLOBAL_SYMBOL("op_min16u_zero");
                uint16_t a[] = {0, 0xFFFF};
                uint16_t b[] = {1, 0xFFFF};
                uint16x2_t c = _min16u(v_load_uint16x2(a), v_load_uint16x2(b));
                CHECK(c.v, PK_I16(0, 0xFFFF), 113);
            }
        }

        { // min8
            {
                GLOBAL_SYMBOL("op_min8");
                int8_t a[] = {10, -20, 30, -40};
                int8_t b[] = {5, 5, 5, 5};
                int8x4_t c = _min8(v_load_int8x4(a), v_load_int8x4(b));
                CHECK(c.v, PK_I8(5, -20, 5, -40), 121);
            }
            {
                GLOBAL_SYMBOL("op_min8_sign_diff");
                // critical: -128 is smaller than 127
                int8_t a[] = {-128, 0, 0, 0};
                int8_t b[] = {127, 0, 0, 0};
                int8x4_t c = _min8(v_load_int8x4(a), v_load_int8x4(b));
                CHECK(c.v, PK_I8(-128, 0, 0, 0), 122);
            }
            {
                GLOBAL_SYMBOL("op_min8_equal");
                int8_t a[] = {7, 7, 7, 7};
                int8_t b[] = {7, 7, 7, 7};
                int8x4_t c = _min8(v_load_int8x4(a), v_load_int8x4(b));
                CHECK(c.v, PK_I8(7, 7, 7, 7), 123);
            }
        }

        { // min8u
            {
                GLOBAL_SYMBOL("op_min8u");
                uint8_t a[] = {10, 200, 30, 150};
                uint8_t b[] = {5, 5, 5, 5};
                uint8x4_t c = _min8u(v_load_uint8x4(a), v_load_uint8x4(b));
                CHECK(c.v, PK_I8(5, 5, 5, 5), 131);
            }
            {
                GLOBAL_SYMBOL("op_min8u_high_bit");
                // critical: 1 is smaller than 255
                uint8_t a[] = {255, 0, 0, 0};
                uint8_t b[] = {1, 0, 0, 0};
                uint8x4_t c = _min8u(v_load_uint8x4(a), v_load_uint8x4(b));
                CHECK(c.v, PK_I8(1, 0, 0, 0), 132);
            }
            {
                GLOBAL_SYMBOL("op_min8u_zero");
                uint8_t a[] = {0, 255, 0, 255};
                uint8_t b[] = {1, 255, 1, 255};
                uint8x4_t c = _min8u(v_load_uint8x4(a), v_load_uint8x4(b));
                CHECK(c.v, PK_I8(0, 255, 0, 255), 133);
            }
        }

        { // max16
            {
                GLOBAL_SYMBOL("op_max16");
                int16_t a[] = {100, -49};
                int16_t b[] = {200, -50};
                int16x2_t c = _max16(v_load_int16x2(a), v_load_int16x2(b));
                CHECK(c.v, PK_I16(200, -49), 201);
            }
            {
                GLOBAL_SYMBOL("op_max16_sign_diff");
                // critical: 1 is larger than -1
                int16_t a[] = {-1, 0};
                int16_t b[] = {1, 0};
                int16x2_t c = _max16(v_load_int16x2(a), v_load_int16x2(b));
                CHECK(c.v, PK_I16(1, 0), 202);
            }
            {
                GLOBAL_SYMBOL("op_max16_equal");
                int16_t a[] = {33, -33};
                int16_t b[] = {33, -33};
                int16x2_t c = _max16(v_load_int16x2(a), v_load_int16x2(b));
                CHECK(c.v, PK_I16(33, -33), 203);
            }
        }

        { // max16u
            {
                GLOBAL_SYMBOL("op_max16u");
                uint16_t a[] = {100, 50000};
                uint16_t b[] = {200, 10000};
                uint16x2_t c = _max16u(v_load_uint16x2(a), v_load_uint16x2(b));
                CHECK(c.v, PK_I16(200, 50000), 211);
            }
            {
                GLOBAL_SYMBOL("op_max16u_high_bit");
                // critical: 65535 is larger than 1
                // if treated as signed, 1 is larger
                uint16_t a[] = {0xFFFF, 0};
                uint16_t b[] = {1, 0};
                uint16x2_t c = _max16u(v_load_uint16x2(a), v_load_uint16x2(b));
                CHECK(c.v, PK_I16(0xFFFF, 0), 212);
            }
            {
                GLOBAL_SYMBOL("op_max16u_equal");
                uint16_t a[] = {0xFFFF, 0};
                uint16_t b[] = {0xFFFF, 0};
                uint16x2_t c = _max16u(v_load_uint16x2(a), v_load_uint16x2(b));
                CHECK(c.v, PK_I16(0xFFFF, 0), 213);
            }
        }

        { // max8
            {
                GLOBAL_SYMBOL("op_max8");
                int8_t a[] = {10, -20, 30, -40};
                int8_t b[] = {5, 5, 5, 5};
                int8x4_t c = _max8(v_load_int8x4(a), v_load_int8x4(b));
                CHECK(c.v, PK_I8(10, 5, 30, 5), 221);
            }
            {
                GLOBAL_SYMBOL("op_max8_sign_diff");
                // critical: 127 is larger than -128
                int8_t a[] = {-128, 0, 0, 0};
                int8_t b[] = {127, 0, 0, 0};
                int8x4_t c = _max8(v_load_int8x4(a), v_load_int8x4(b));
                CHECK(c.v, PK_I8(127, 0, 0, 0), 222);
            }
            {
                GLOBAL_SYMBOL("op_max8_equal");
                int8_t a[] = {7, 7, 7, 7};
                int8_t b[] = {7, 7, 7, 7};
                int8x4_t c = _max8(v_load_int8x4(a), v_load_int8x4(b));
                CHECK(c.v, PK_I8(7, 7, 7, 7), 223);
            }
        }

        { // max8u
            {
                GLOBAL_SYMBOL("op_max8u");
                uint8_t a[] = {10, 200, 30, 150};
                uint8_t b[] = {5, 5, 5, 5};
                uint8x4_t c = _max8u(v_load_uint8x4(a), v_load_uint8x4(b));
                CHECK(c.v, PK_I8(10, 200, 30, 150), 231);
            }
            {
                GLOBAL_SYMBOL("op_max8u_high_bit");
                // critical: 255 is larger than 1
                uint8_t a[] = {255, 0, 0, 0};
                uint8_t b[] = {1, 0, 0, 0};
                uint8x4_t c = _max8u(v_load_uint8x4(a), v_load_uint8x4(b));
                CHECK(c.v, PK_I8(255, 0, 0, 0), 232);
            }
            {
                GLOBAL_SYMBOL("op_max8u_equal");
                uint8_t a[] = {255, 255, 0, 0};
                uint8_t b[] = {255, 255, 0, 0};
                uint8x4_t c = _max8u(v_load_uint8x4(a), v_load_uint8x4(b));
                CHECK(c.v, PK_I8(255, 255, 0, 0), 233);
            }
        }
    }
    pass();
}
