#include <stdint.h>
#include "common.h"
#include "common_math.h"

#ifndef LOOPS
#define LOOPS 1
#endif

#ifndef __riscv_xsimd
_Static_assert(0, "SIMD isa required for the test");
#endif

// result, expected, index
#define CHECK(r, e, i) \
    if ((r) != (e)){ write_mismatch((r), (e), (i)); fail(); }

// packing macro helpers
// 32-bit
#define PK_I16(lo, hi) \
    (int32_t)( (((hi) & 0xFFFF) << 16) | ((lo) & 0xFFFF) )

#define PK_U16(lo, hi) \
    (uint32_t)( (((hi) & 0xFFFF) << 16) | ((lo) & 0xFFFF) )

#define PK_U8(l0, l1, l2, l3) \
    (uint32_t)( \
        (((l3) & 0xFF) << 24) | (((l2) & 0xFF) << 16) | \
        (((l1) & 0xFF) << 8) | ((l0) & 0xFF) \
    )

#define PK_I8(l0, l1, l2, l3) \
    (int32_t)( \
        (((l3) & 0xFF) << 24) | (((l2) & 0xFF) << 16) | \
        (((l1) & 0xFF) << 8) | ((l0) & 0xFF) \
    )

// sub-byte
#define PK_SB_I4(lo, hi) \
    (int8_t)( (((hi) & 0xF) << 4) | ((lo) & 0xF) )

#define PK_SB_U4(lo, hi) \
    (uint8_t)( (((hi) & 0xF) << 4) | ((lo) & 0xF) )

#define PK_SB_I2(l3, l2, l1, l0) \
    (int8_t)( (((l3)&3)<<6) | (((l2)&3)<<4) | (((l1)&3)<<2) | ((l0)&3) )

#define PK_SB_U2(l3, l2, l1, l0) \
    (uint8_t)( (((l3)&3)<<6) | (((l2)&3)<<4) | (((l1)&3)<<2) | ((l0)&3) )

void main() {
    for (uint32_t i = 0; i < LOOPS; i++) {

        // ---------------------------------------------------------------------
        { // narrow32: 2x uint32 -> 1x uint16x2
            {
                GLOBAL_SYMBOL("op_narrow32");
                uint32_t a = 0x111100AA; // expect 0x00AA
                uint32_t b = 0x222200BB; // expect 0x00BB
                uint16x2_t c = _narrow32(a, b);
                CHECK(c.v, PK_U16(0x00AA, 0x00BB), 101);
            }
            {
                GLOBAL_SYMBOL("op_narrow32_truncation");
                // verify upper 16 bits are strictly ignored
                // a: 0xFFFF0000 -> 0x0000
                // b: 0xFFFF1234 -> 0x1234
                uint32_t a = 0xFFFF0000;
                uint32_t b = 0xFFFF1234;
                uint16x2_t c = _narrow32(a, b);
                CHECK(c.v, PK_U16(0x0000, 0x1234), 102);
            }
            {
                GLOBAL_SYMBOL("op_narrow32_max_val");
                // verify full 16-bit range is preserved
                uint32_t a = 0xFFFFFFFF; // -> 0xFFFF
                uint32_t b = 0x0000FFFF; // -> 0xFFFF
                uint16x2_t c = _narrow32(a, b);
                CHECK(c.v, PK_U16(0xFFFF, 0xFFFF), 103);
            }
        }

        { // narrow16: 2x uint16x2 -> 1x uint8x4
            {
                GLOBAL_SYMBOL("op_narrow16");
                // a: (0x01, 0x02) -> (0x01, 0x02)
                // b: (0x03, 0x04) -> (0x03, 0x04)
                uint16_t a[] = {0x1101, 0x2202};
                uint16_t b[] = {0x3303, 0x4404};
                uint8x4_t c = _narrow16(v_load_uint16x2(a), v_load_uint16x2(b));
                CHECK(c.v, PK_U8(0x01, 0x02, 0x03, 0x04), 111);
            }
            {
                GLOBAL_SYMBOL("op_narrow16_truncation");
                // ensure high byte of uint16 doesn't bleed
                uint16_t a[] = {0xFF00, 0x00FF}; // -> 00, FF
                uint16_t b[] = {0x1234, 0xFFFF}; // -> 34, FF
                uint8x4_t c = _narrow16(v_load_uint16x2(a), v_load_uint16x2(b));
                CHECK(c.v, PK_U8(0x00, 0xFF, 0x34, 0xFF), 112);
            }
        }

        { // narrow8: 2x uint8x4 -> 1x uint4x8 (Packed into uint32)
            {
                GLOBAL_SYMBOL("op_narrow8");
                // a: (0x01, 0x02, 0x03, 0x04) -> Low nibbles kept
                // b: (0x05, 0x06, 0x07, 0x08)
                // result lanes: 1, 2, 3, 4, 5, 6, 7, 8
                uint8_t a[] = {0xF1, 0xE2, 0xD3, 0xC4};
                uint8_t b[] = {0xB5, 0xA6, 0x97, 0x88};
                uint4x8_t c = _narrow8(v_load_uint8x4(a), v_load_uint8x4(b));
                // pack logic:
                // byte 0: lanes 1,0 -> (2<<4)|1 = 0x21
                // byte 1: lanes 3,2 -> (4<<4)|3 = 0x43
                // byte 2: lanes 5,4 -> (6<<4)|5 = 0x65
                // byte 3: lanes 7,6 -> (8<<4)|7 = 0x87
                CHECK(c.v, PK_U8(0x21, 0x43, 0x65, 0x87), 121);
            }
            {
                GLOBAL_SYMBOL("op_narrow8_truncation");
                // verify upper nibble of input uint8 is strictly discarded
                // a: (0xf0, 0xf0, 0xf0, 0xf0) -> (0, 0, 0, 0)
                // b: (0xff, 0xff, 0xff, 0xff) -> (f, f, f, f)
                uint8_t a[] = {0xF0, 0xF0, 0xF0, 0xF0};
                uint8_t b[] = {0xFF, 0xFF, 0xFF, 0xFF};
                uint4x8_t c = _narrow8(v_load_uint8x4(a), v_load_uint8x4(b));
                // byte 0: 0,0 -> 0x00
                // byte 1: 0,0 -> 0x00
                // byte 2: f,f -> 0xff
                // byte 3: f,f -> 0xff
                CHECK(c.v, PK_U8(0x00, 0x00, 0xFF, 0xFF), 122);
            }
        }

        { // narrow4: 2x uint4x8 -> 1x uint2x16 (Packed into uint32)
            {
                 GLOBAL_SYMBOL("op_narrow4");
                 // 0, 1, 2, 3, 0, 1, 2, 3.
                 // 0x3210 3210 -> packed bytes: 0x32, 0x10, 0x32, 0x10
                 uint8_t a[] = {0x10, 0x32, 0x10, 0x32};
                 uint8_t b[] = {0x10, 0x32, 0x10, 0x32};

                 // after narrow4 (truncating 4 bits to 2 bits):
                 // 0->0, 1->1, 2->2, 3->3 (values fit in 2 bits, no loss)
                 // output has 16 lanes of: 0,1,2,3, 0,1,2,3, 0,1,2,3, 0,1,2,3
                 // packing 4 lanes (3,2,1,0) into 1 byte:
                 // ((3<<6) | (2<<4) | (1<<2) | 0) = 11 10 01 00 = 0xE4
                 uint2x16_t c = _narrow4(v_load_uint4x8(a), v_load_uint4x8(b));
                 CHECK(c.v, PK_U8(0xE4, 0xE4, 0xE4, 0xE4), 131);
            }
            {
                GLOBAL_SYMBOL("op_narrow4_truncation");
                // a (8 lanes): 0xCC, 0xCC, 0xCC, 0xCC
                //    a contributes all zeros (0xc = 0b1100)
                // b (8 lanes): 0x77, 0x77, 0x77, 0x77
                //    B contributes all threes (0x7 = 0b0011)
                uint8_t a[] = {0xCC, 0xCC, 0xCC, 0xCC};
                uint8_t b[] = {0x77, 0x77, 0x77, 0x77};
                uint2x16_t c = _narrow4(v_load_uint4x8(a), v_load_uint4x8(b));

                // lanes 0-7 (from a): 0
                // lanes 8-15 (from b): 3 (binary 11)
                // lower 2 bytes (lanes 0-7): 0x00, 0x00
                // upper 2 bytes (lanes 8-15): each byte is 4 lanes of '3' -> 0xff
                CHECK(c.v, PK_U8(0x00, 0x00, 0xFF, 0xFF), 132);
            }
        }

        // ---------------------------------------------------------------------
        { // qnarrow32
            {
                GLOBAL_SYMBOL("op_qnarrow32");
                // fits in int16
                int32_t a = 32767;
                int32_t b = -32768;
                int16x2_t c = _qnarrow32(a, b);
                CHECK(c.v, PK_I16(32767, -32768), 101);
            }
            {
                GLOBAL_SYMBOL("op_qnarrow32_sat");
                // 32768 -> 32767 (max)
                // -32769 -> -32768 (min)
                int32_t a = 32768;
                int32_t b = -32769;
                int16x2_t c = _qnarrow32(a, b);
                CHECK(c.v, PK_I16(32767, -32768), 102);
            }
            {
                GLOBAL_SYMBOL("op_qnarrow32_extreme");
                // max int32 -> max int16
                // min int32 -> min int16
                int32_t a = 2147483647;
                int32_t b = (int32_t)0x80000000;
                int16x2_t c = _qnarrow32(a, b);
                CHECK(c.v, PK_I16(32767, -32768), 103);
            }
        }

        { // qnarrow32u
            {
                GLOBAL_SYMBOL("op_qnarrow32u");
                // fits in uint16
                uint32_t a = 65535;
                uint32_t b = 0;
                uint16x2_t c = _qnarrow32u(a, b);
                CHECK(c.v, PK_U16(65535, 0), 201);
            }
            {
                GLOBAL_SYMBOL("op_qnarrow32u_sat");
                // 65536 -> 65535 (max)
                uint32_t a = 65536;
                uint32_t b = 0xFFFFFFFF;
                uint16x2_t c = _qnarrow32u(a, b);
                CHECK(c.v, PK_U16(65535, 65535), 202);
            }
        }

        { // qnarrow16
            {
                GLOBAL_SYMBOL("op_qnarrow16");
                // fits in int8
                int16_t a[] = {127, -128};
                int16_t b[] = {0, -1};
                int8x4_t c = _qnarrow16(v_load_int16x2(a), v_load_int16x2(b));
                CHECK(c.v, PK_I8(127, -128, 0, -1), 301);
            }
            {
                GLOBAL_SYMBOL("op_qnarrow16_sat");
                // 128 -> 127
                // -129 -> -128
                int16_t a[] = {128, -129};
                int16_t b[] = {32767, -32768};
                int8x4_t c = _qnarrow16(v_load_int16x2(a), v_load_int16x2(b));
                CHECK(c.v, PK_I8(127, -128, 127, -128), 302);
            }
        }

        { // qnarrow16u
            {
                GLOBAL_SYMBOL("op_qnarrow16u");
                // fits in uint8
                uint16_t a[] = {255, 0};
                uint16_t b[] = {10, 20};
                uint8x4_t c = _qnarrow16u(v_load_uint16x2(a), v_load_uint16x2(b));
                CHECK(c.v, PK_U8(255, 0, 10, 20), 401);
            }
            {
                GLOBAL_SYMBOL("op_qnarrow16u_sat");
                // 256 -> 255
                uint16_t a[] = {256, 65535};
                uint16_t b[] = {0, 0};
                uint8x4_t c = _qnarrow16u(v_load_uint16x2(a), v_load_uint16x2(b));
                CHECK(c.v, PK_U8(255, 255, 0, 0), 402);
            }
        }

        { // qnarrow8 (int8 -> int4)
            {
                GLOBAL_SYMBOL("op_qnarrow8");
                // fits in int4 (-8 to 7)
                int8_t a[] = {7, -8, 0, -1};
                int8_t b[] = {3, -5, 2, -2};
                int4x8_t c = _qnarrow8(v_load_int8x4(a), v_load_int8x4(b));
                // low lanes (a), high lanes (b)
                CHECK(c.v, PK_U8(
                    PK_SB_I4(7, -8), PK_SB_I4(0, -1), // a
                    PK_SB_I4(3, -5), PK_SB_I4(2, -2)  // b
                ), 501);
            }
            {
                GLOBAL_SYMBOL("op_qnarrow8_sat");
                // sat max: > 7 -> 7
                // sat min: < -8 -> -8
                int8_t a[] = {8, 127, -9, -128};
                int8_t b[] = {10, -20, 100, -100};
                int4x8_t c = _qnarrow8(v_load_int8x4(a), v_load_int8x4(b));
                CHECK(c.v, PK_U8(
                    PK_SB_I4(7, 7), PK_SB_I4(-8, -8), // a
                    PK_SB_I4(7, -8), PK_SB_I4(7, -8)  // b
                ), 502);
            }
        }

        { // qnarrow8u (uint8 -> uint4)
            {
                GLOBAL_SYMBOL("op_qnarrow8u");
                // fits in uint4 (0 to 15)
                uint8_t a[] = {15, 0, 10, 5};
                uint8_t b[] = {1, 2, 3, 4};
                uint4x8_t c = _qnarrow8u(v_load_uint8x4(a), v_load_uint8x4(b));
                CHECK(c.v, PK_U8(
                    PK_SB_U4(15, 0), PK_SB_U4(10, 5), // a
                    PK_SB_U4(1, 2), PK_SB_U4(3, 4)    // b
                ), 601);
            }
            {
                GLOBAL_SYMBOL("op_qnarrow8u_sat");
                // sat max: > 15 -> 15
                uint8_t a[] = {16, 255, 100, 20};
                uint8_t b[] = {0, 0, 0, 0};
                uint4x8_t c = _qnarrow8u(v_load_uint8x4(a), v_load_uint8x4(b));
                CHECK(c.v, PK_U8(
                    PK_SB_U4(15, 15), PK_SB_U4(15, 15), // a
                    PK_SB_U4(0, 0), PK_SB_U4(0, 0)      // b
                ), 602);
            }
        }

        { // qnarrow4 (int4 -> int2)
            {
                GLOBAL_SYMBOL("op_qnarrow4");
                // input: int4 (packed in int8 containers)
                // target: int2 (-2 to 1)
                // safe values: 0, 1, -1, -2
                int8_t a[] = { PK_SB_I4(0, 1), PK_SB_I4(-1, -2), 0, 0 };
                int8_t b[] = { PK_SB_I4(1, -2), 0, 0, 0 };
                int2x16_t c = _qnarrow4(v_load_int4x8(a), v_load_int4x8(b));

                // output packing: PK_SB_I2(l3, l2, l1, l0)
                // lanes 0-3 come from first byte of a
                CHECK(c.v, PK_U8(
                    PK_SB_I2( -2, -1, 1, 0 ), 0, // a
                    PK_SB_I2( 0, 0, -2, 1 ), 0   // b
                ), 701);
            }
            {
                GLOBAL_SYMBOL("op_qnarrow4_sat");
                // sat max: > 1 -> 1 (e.g., 2, 7)
                // sat min: < -2 -> -2 (e.g., -3, -8)
                int8_t a[] = { PK_SB_I4(2, 7), PK_SB_I4(-3, -8), 0, 0 };
                int8_t b[] = { 0, 0, 0, 0 };
                int2x16_t c = _qnarrow4(v_load_int4x8(a), v_load_int4x8(b));

                CHECK(c.v, PK_U8(
                    PK_SB_I2( -2, -2, 1, 1 ), 0, // a (saturated)
                    0, 0 // b
                ), 702);
            }
        }

        { // qnarrow4u (uint4 -> uint2)
            {
                GLOBAL_SYMBOL("op_qnarrow4u");
                // input: uint4
                // target: uint2 (0 to 3)
                uint8_t a[] = { PK_SB_U4(0, 1), PK_SB_U4(2, 3), 0, 0 };
                uint8_t b[] = { PK_SB_U4(3, 0), 0, 0, 0 };
                uint2x16_t c = _qnarrow4u(v_load_uint4x8(a), v_load_uint4x8(b));

                CHECK(c.v, PK_U8(
                    PK_SB_U2( 3, 2, 1, 0 ), 0, // a
                    PK_SB_U2( 0, 0, 0, 3 ), 0  // b
                ), 801);
            }
            {
                GLOBAL_SYMBOL("op_qnarrow4u_sat");
                // sat max: > 3 -> 3 (e.g., 4, 15)
                uint8_t a[] = { PK_SB_U4(4, 15), PK_SB_U4(8, 0), 0, 0 };
                uint8_t b[] = { 0, 0, 0, 0 };
                uint2x16_t c = _qnarrow4u(v_load_uint4x8(a), v_load_uint4x8(b));

                CHECK(c.v, PK_U8(
                    PK_SB_U2( 0, 3, 3, 3 ), 0, // a (4->3, 15->3, 8->3)
                    0, 0 // b
                ), 802);
            }
        }

    }
    pass();
}
