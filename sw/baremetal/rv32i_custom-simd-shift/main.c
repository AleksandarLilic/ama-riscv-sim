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
#define PK(lo, hi) \
    (int32_t)( (((hi) & 0xFFFF) << 16) | ((lo) & 0xFFFF) )

#define PK2(l0, l1, l2, l3) \
    (int32_t)( \
        (((l3) & 0xFF) << 24) | (((l2) & 0xFF) << 16) | \
        (((l1) & 0xFF) << 8) | ((l0) & 0xFF) \
    )

void main() {
    for (uint32_t i = 0; i < LOOPS; i++) {

        // Shift Left Logical (slli)
        { // slli16
            {
                GLOBAL_SYMBOL("op_slli16");
                int16_t a[] = {10, -10};
                int32_t imm = 4;
                int16x2_t c = _slli16(v_load_int16x2(a), imm);
                // 10 << 4 = 160
                // -10 (0xFFF6) << 4 = 0xFF60 (-160)
                CHECK(c.v, PK(160, -160), 101);
            }
            {
                GLOBAL_SYMBOL("op_slli16_max_shift");
                // Shift 15 places. Only the LSB survives to become the MSB (Sign bit).
                int16_t a[] = {1, 0};
                int32_t imm = 15;
                int16x2_t c = _slli16(v_load_int16x2(a), imm);
                // 1 << 15 = 0x8000 (-32768)
                CHECK(c.v, PK((int16_t)0x8000, 0), 102);
            }
            {
                GLOBAL_SYMBOL("op_slli16_overflow_discard");
                // Verify bits shifted out are lost, not wrapped
                int16_t a[] = {0x4000, 0}; // Binary 0100...
                int32_t imm = 2;
                int16x2_t c = _slli16(v_load_int16x2(a), imm);
                // 0x4000 << 2 = 0x10000 -> Truncates to 0 in 16-bit
                CHECK(c.v, PK(0, 0), 103);
            }
        }

        { // slli8
            {
                GLOBAL_SYMBOL("op_slli8");
                int8_t a[] = {10, -10, 2, -2};
                int32_t imm = 2;
                int8x4_t c = _slli8(v_load_int8x4(a), imm);
                CHECK(c.v, PK2(40, -40, 8, -8), 111);
            }
            {
                GLOBAL_SYMBOL("op_slli8_max_shift");
                int8_t a[] = {1, -1, 0, 0};
                int32_t imm = 7;
                int8x4_t c = _slli8(v_load_int8x4(a), imm);
                // 1 << 7 = 0x80 (-128)
                // -1 (0xFF) << 7 = 0x80 (-128)
                CHECK(c.v, PK2((int8_t)0x80, (int8_t)0x80, 0, 0), 112);
            }
            {
                GLOBAL_SYMBOL("op_slli8_wipe");
                // Shift 8 places (effectively 0 for 8-bit, but usually handled as 0 or clear)
                // Assuming ISA supports < width. Let's test standard truncation with high shift
                // Testing shift 4 on value 16 (0x10) -> 0x100 -> 0
                int8_t a[] = {16, 0, 0, 0};
                int32_t imm = 4;
                int8x4_t c = _slli8(v_load_int8x4(a), imm);
                CHECK(c.v, PK2(0, 0, 0, 0), 113);
            }
        }

        // Shift Right Logical (srli) - Zero Extension
        { // srli16
            {
                GLOBAL_SYMBOL("op_srli16");
                // 0x8000 (Min) shifted right logically must lose the sign bit
                int16_t a[] = {160, -32768};
                int32_t imm = 4;
                int16x2_t c = _srli16(v_load_int16x2(a), imm);
                // 160 >> 4 = 10
                // 0x8000 >> 4 = 0x0800 (2048) -> MUST BE POSITIVE
                CHECK(c.v, PK(10, 2048), 121);
            }
            {
                GLOBAL_SYMBOL("op_srli16_neg_to_pos");
                // -1 (0xFFFF) >> 1 = 0x7FFF (32767)
                int16_t a[] = {-1, 0};
                int32_t imm = 1;
                int16x2_t c = _srli16(v_load_int16x2(a), imm);
                CHECK(c.v, PK(32767, 0), 122);
            }
            {
                GLOBAL_SYMBOL("op_srli16_isolate_lsb");
                int16_t a[] = {-1, 0}; // 0xFFFF
                int32_t imm = 15;
                int16x2_t c = _srli16(v_load_int16x2(a), imm);
                // 0xFFFF >> 15 = 0x0001
                CHECK(c.v, PK(1, 0), 123);
            }
        }

        { // srli8
            {
                GLOBAL_SYMBOL("op_srli8");
                // 0x80 (-128) >> 4 = 0x08 (8)
                int8_t a[] = {32, -128, 0, 0};
                int32_t imm = 4;
                int8x4_t c = _srli8(v_load_int8x4(a), imm);
                CHECK(c.v, PK2(2, 8, 0, 0), 131);
            }
            {
                GLOBAL_SYMBOL("op_srli8_neg_to_pos");
                // -1 (0xFF) >> 1 = 0x7F (127)
                int8_t a[] = {-1, 0, 0, 0};
                int32_t imm = 1;
                int8x4_t c = _srli8(v_load_int8x4(a), imm);
                CHECK(c.v, PK2(127, 0, 0, 0), 132);
            }
            {
                GLOBAL_SYMBOL("op_srli8_max_shift");
                int8_t a[] = {-1, 0, 0, 0};
                int32_t imm = 7;
                int8x4_t c = _srli8(v_load_int8x4(a), imm);
                CHECK(c.v, PK2(1, 0, 0, 0), 133);
            }
        }

        // Shift Right Arithmetic (srai) - Sign Extension
        { // srai16
            {
                GLOBAL_SYMBOL("op_srai16");
                // Positive numbers behave same as Logical
                int16_t a[] = {160, 32};
                int32_t imm = 4;
                int16x2_t c = _srai16(v_load_int16x2(a), imm);
                CHECK(c.v, PK(10, 2), 141);
            }
            {
                GLOBAL_SYMBOL("op_srai16_sign_preserve");
                // Negative numbers must stay negative
                // 0x8000 (-32768) >> 4 = 0xF800 (-2048)
                int16_t a[] = {-32768, 0};
                int32_t imm = 4;
                int16x2_t c = _srai16(v_load_int16x2(a), imm);
                CHECK(c.v, PK(-2048, 0), 142);
            }
            {
                GLOBAL_SYMBOL("op_srai16_sticky_minus1");
                // -1 shifted right logically becomes positive.
                // -1 shifted right arithmetically STAYS -1.
                int16_t a[] = {-1, -1};
                int32_t imm = 8;
                int16x2_t c = _srai16(v_load_int16x2(a), imm);
                CHECK(c.v, PK(-1, -1), 143);
            }
        }

        { // srai8
            {
                GLOBAL_SYMBOL("op_srai8");
                int8_t a[] = {32, 64, 0, 0};
                int32_t imm = 4;
                int8x4_t c = _srai8(v_load_int8x4(a), imm);
                CHECK(c.v, PK2(2, 4, 0, 0), 151);
            }
            {
                GLOBAL_SYMBOL("op_srai8_sign_preserve");
                // 0x80 (-128) >> 4 = 0xF8 (-8)
                int8_t a[] = {-128, 0, 0, 0};
                int32_t imm = 4;
                int8x4_t c = _srai8(v_load_int8x4(a), imm);
                CHECK(c.v, PK2(-8, 0, 0, 0), 152);
            }
            {
                GLOBAL_SYMBOL("op_srai8_sticky_minus1");
                // -1 >> 7 should still be -1 (0xFF)
                int8_t a[] = {-1, -1, -1, -1};
                int32_t imm = 7;
                int8x4_t c = _srai8(v_load_int8x4(a), imm);
                CHECK(c.v, PK2(-1, -1, -1, -1), 153);
            }
        }

    }
    pass();
}
