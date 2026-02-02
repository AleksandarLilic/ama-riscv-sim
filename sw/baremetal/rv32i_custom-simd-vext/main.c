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
        int32_t res_s;
        uint32_t res_u;

        { // vext16 / vext16u - 16-bit elements, 2 lanes
            // vec = [0x1234, 0x5678] = [4660, 22136]
            uint16_t arr16[] = {0x1234, 0x5678};
            uint16x2_t v16 = v_load_uint16x2(arr16);

            // vext16 lane 0: 0x1234 = 4660 (positive, sign extend = same)
            res_s = _vext16(v16, 0);
            CHECK(res_s, 0x00001234, 101);

            // vext16 lane 1: 0x5678 = 22136 (positive, sign extend = same)
            res_s = _vext16(v16, 1);
            CHECK(res_s, 0x00005678, 102);

            // vext16u lane 0: zero extend
            res_u = _vext16u(v16, 0);
            CHECK(res_u, 0x00001234, 103);

            // vext16u lane 1: zero extend
            res_u = _vext16u(v16, 1);
            CHECK(res_u, 0x00005678, 104);

            // Test negative value (sign bit set)
            // vec = [0x8000, 0xFFFF] = [-32768, -1] as signed
            uint16_t arr16_neg[] = {0x8000, 0xFFFF};
            uint16x2_t v16_neg = v_load_uint16x2(arr16_neg);

            // vext16 lane 0: 0x8000 -> sign extend -> 0xFFFF8000 = -32768
            res_s = _vext16(v16_neg, 0);
            CHECK(res_s, (int32_t)0xFFFF8000, 105);

            // vext16 lane 1: 0xFFFF -> sign extend -> 0xFFFFFFFF = -1
            res_s = _vext16(v16_neg, 1);
            CHECK(res_s, -1, 106);

            // vext16u lane 0: 0x8000 -> zero extend -> 0x00008000 = 32768
            res_u = _vext16u(v16_neg, 0);
            CHECK(res_u, 0x00008000, 107);

            // vext16u lane 1: 0xFFFF -> zero extend -> 0x0000FFFF = 65535
            res_u = _vext16u(v16_neg, 1);
            CHECK(res_u, 0x0000FFFF, 108);
        }

        { // vext8 / vext8u - 8-bit elements, 4 lanes
            // vec = [0x12, 0x34, 0x56, 0x78] = [18, 52, 86, 120]
            uint8_t arr8[] = {0x12, 0x34, 0x56, 0x78};
            uint8x4_t v8 = v_load_uint8x4(arr8);

            res_s = _vext8(v8, 0);
            CHECK(res_s, 0x12, 201);

            res_s = _vext8(v8, 2);
            CHECK(res_s, 0x56, 202);

            res_u = _vext8u(v8, 1);
            CHECK(res_u, 0x34, 203);

            res_u = _vext8u(v8, 3);
            CHECK(res_u, 0x78, 204);

            // Test negative values (sign bit set)
            // vec = [0x80, 0xFF, 0x7F, 0x00] = [-128, -1, 127, 0] as signed
            uint8_t arr8_neg[] = {0x80, 0xFF, 0x7F, 0x00};
            uint8x4_t v8_neg = v_load_uint8x4(arr8_neg);

            // vext8 lane 0: 0x80 -> sign extend -> 0xFFFFFF80 = -128
            res_s = _vext8(v8_neg, 0);
            CHECK(res_s, (int32_t)0xFFFFFF80, 205);

            // vext8 lane 1: 0xFF -> sign extend -> 0xFFFFFFFF = -1
            res_s = _vext8(v8_neg, 1);
            CHECK(res_s, -1, 206);

            // vext8u lane 0: 0x80 -> zero extend -> 0x00000080 = 128
            res_u = _vext8u(v8_neg, 0);
            CHECK(res_u, 0x80, 207);

            // vext8u lane 1: 0xFF -> zero extend -> 0x000000FF = 255
            res_u = _vext8u(v8_neg, 1);
            CHECK(res_u, 0xFF, 208);
        }

        { // vext4 / vext4u - 4-bit elements, 8 lanes
            // vec packed: 0x12345678
            // lanes (4-bit each, lsb first): 8,7,6,5,4,3,2,1
            // lane 0 = 0x8 = -8 signed, 8 unsigned
            // lane 7 = 0x1
            uint8_t arr4[] = {0x78, 0x56, 0x34, 0x12}; // little endian
            uint4x8_t v4 = v_load_uint4x8(arr4);

            // vext4 lane 0: 0x8 -> sign extend -> 0xFFFFFFF8 = -8
            res_s = _vext4(v4, 0);
            CHECK(res_s, (int32_t)0xFFFFFFF8, 301);

            // vext4 lane 1: 0x7 -> sign extend -> 0x00000007 = 7
            res_s = _vext4(v4, 1);
            CHECK(res_s, 7, 302);

            // vext4 lane 7: 0x1 -> sign extend -> 0x00000001 = 1
            res_s = _vext4(v4, 7);
            CHECK(res_s, 1, 303);

            // vext4u lane 0: 0x8 -> zero extend -> 0x00000008 = 8
            res_u = _vext4u(v4, 0);
            CHECK(res_u, 8, 304);

            // vext4u lane 5: 0x3 -> zero extend -> 3
            res_u = _vext4u(v4, 5);
            CHECK(res_u, 3, 305);

            // Test all negative (0xF = -1 signed, 15 unsigned)
            uint8_t arr4_neg[] = {0xFF, 0xFF, 0xFF, 0xFF};
            uint4x8_t v4_neg = v_load_uint4x8(arr4_neg);

            res_s = _vext4(v4_neg, 3);
            CHECK(res_s, -1, 306);

            res_u = _vext4u(v4_neg, 3);
            CHECK(res_u, 0xF, 307);
        }

        { // vext2 / vext2u - 2-bit elements, 16 lanes
            // vec = 0xAAAAAAAA = binary 10 10 10 10 ... (all lanes = 2)
            // 0xAA = 10 10 10 10
            uint8_t arr2[] = {0xAA, 0xAA, 0xAA, 0xAA};
            uint2x16_t v2 = v_load_uint2x16(arr2);

            // vext2 lane 0: 0b10 = -2 signed
            res_s = _vext2(v2, 0);
            CHECK(res_s, (int32_t)0xFFFFFFFE, 401);

            // vext2 lane 5: 0b10 = -2 signed
            res_s = _vext2(v2, 5);
            CHECK(res_s, (int32_t)0xFFFFFFFE, 402);

            // vext2u lane 0: 0b10 = 2 unsigned
            res_u = _vext2u(v2, 0);
            CHECK(res_u, 2, 403);

            // vext2u lane 15: 0b10 = 2 unsigned
            res_u = _vext2u(v2, 15);
            CHECK(res_u, 2, 404);

            // Test mixed values: 0x1B = 00 01 10 11 (lanes 0-3: 3,2,1,0)
            uint8_t arr2_mix[] = {0x1B, 0x1B, 0x1B, 0x1B};
            uint2x16_t v2_mix = v_load_uint2x16(arr2_mix);

            // lane 0: 0b11 = -1 signed, 3 unsigned
            res_s = _vext2(v2_mix, 0);
            CHECK(res_s, -1, 405);

            res_u = _vext2u(v2_mix, 0);
            CHECK(res_u, 3, 406);

            // lane 1: 0b10 = -2 signed, 2 unsigned
            res_s = _vext2(v2_mix, 1);
            CHECK(res_s, (int32_t)0xFFFFFFFE, 407);

            res_u = _vext2u(v2_mix, 1);
            CHECK(res_u, 2, 408);

            // lane 2: 0b01 = 1 signed, 1 unsigned
            res_s = _vext2(v2_mix, 2);
            CHECK(res_s, 1, 409);

            // lane 3: 0b00 = 0
            res_s = _vext2(v2_mix, 3);
            CHECK(res_s, 0, 410);
        }
    }
    pass();
}
