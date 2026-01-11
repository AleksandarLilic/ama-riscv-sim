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
    (int8_t)( (((hi) & 0xF) << 4) | ((lo) & 0xF) )

#define PK_U(lo, hi) \
    (uint8_t)( (((hi) & 0xF) << 4) | ((lo) & 0xF) )

#define PK2(l3, l2, l1, l0) \
    (int8_t)( (((l3)&3)<<6) | (((l2)&3)<<4) | (((l1)&3)<<2) | ((l0)&3) )

#define PK2_U(l3, l2, l1, l0) \
    (uint8_t)( (((l3)&3)<<6) | (((l2)&3)<<4) | (((l1)&3)<<2) | ((l0)&3) )

void main() {
    for (uint32_t i = 0; i < LOOPS; i++) {

        { // dot16
            {
                GLOBAL_SYMBOL("op_dot16");
                int16_t a[] = {-15, 19};
                int16_t b[] = {4, -43};
                int32_t c = 831;
                _dot16(v_load_int16x2(a), v_load_int16x2(b), &c);
                CHECK(c, -46, 101);
            }
            {
                GLOBAL_SYMBOL("op_dot16_zero");
                int16_t a[] = {0, 0};
                int16_t b[] = {0, 0};
                int32_t c = 0;
                _dot16(v_load_int16x2(a), v_load_int16x2(b), &c);
                CHECK(c, 0, 102);
            }
            {
                GLOBAL_SYMBOL("op_dot16_single_lane_pos");
                int16_t a[] = {9, 0};
                int16_t b[] = {8, 0};
                int32_t c = 20;
                _dot16(v_load_int16x2(a), v_load_int16x2(b), &c);
                CHECK(c, (9 * 8 + 20), 103);
            }
            {
                GLOBAL_SYMBOL("op_dot16_single_lane_neg");
                int16_t a[] = {0, -5};
                int16_t b[] = {0, -1193};
                int32_t c = 982;
                _dot16(v_load_int16x2(a), v_load_int16x2(b), &c);
                CHECK(c, (-5 * -1193 + 982), 104);
            }
            {
                GLOBAL_SYMBOL("op_dot16_alt_mixed_neg");
                int16_t a[] = {-256, 255};
                int16_t b[] = {255, -256};
                int32_t c = -1;
                _dot16(v_load_int16x2(a), v_load_int16x2(b), &c);
                CHECK(c, ((-256 * 255) * 2 - 1), 105);
            }
            {
                GLOBAL_SYMBOL("op_dot16_all_minus1_pos");
                int16_t a[] = {-1, -1};
                int16_t b[] = {-1, -1};
                int32_t c = -1;
                _dot16(v_load_int16x2(a), v_load_int16x2(b), &c);
                CHECK(c, ((-1 * -1) * 2 - 1), 106);
            }
            {
                GLOBAL_SYMBOL("op_dot16_max_pos_pos");
                int16_t a[] = {32767, 32767};
                int16_t b[] = {32767, 32767};
                int32_t c = 0;
                _dot16(v_load_int16x2(a), v_load_int16x2(b), &c);
                CHECK(c, ((32767 * 32767) * 2), 107);
            }
            {
                GLOBAL_SYMBOL("op_dot16_max_neg_pos");
                int16_t a[] = {-32768, -32768};
                int16_t b[] = {32767, 32767};
                int32_t c = 0;
                _dot16(v_load_int16x2(a), v_load_int16x2(b), &c);
                CHECK(c, ((-32768 * 32767) * 2), 108);
            }
            {
                GLOBAL_SYMBOL("op_dot16_overflow_wrap");
                // -32768 * -32768 = 0x40000000 (1,073,741,824)
                // sum = 0x80000000 (2,147,483,648) -> Wraps to INT32_MIN
                int16_t a[] = {-32768, -32768};
                int16_t b[] = {-32768, -32768};
                int32_t c = 0;
                _dot16(v_load_int16x2(a), v_load_int16x2(b), &c);
                // note: cast expected value to int32_t to mimic the wrap
                CHECK(c, (int32_t)0x80000000, 109);
            }
            {
                GLOBAL_SYMBOL("op_dot16_acc_overflow");
                int16_t a[] = {1, 0};
                int16_t b[] = {1, 0};
                // start at MAX, adding 1 should wrap to MIN
                int32_t c = 2147483647;
                _dot16(v_load_int16x2(a), v_load_int16x2(b), &c);
                CHECK(c, (int32_t)0x80000000, 110);
            }
            {
                GLOBAL_SYMBOL("op_dot16_acc_underflow");
                int16_t a[] = {1, 0};
                int16_t b[] = {-1, 0};
                // start at MIN, subtracting 1 should wrap to MAX
                int32_t c = (int32_t)0x80000000;
                _dot16(v_load_int16x2(a), v_load_int16x2(b), &c);
                CHECK(c, 2147483647, 111);
            }
            {
                GLOBAL_SYMBOL("op_dot16_large_cancel");
                int16_t a[] = {32767, 32767};
                int16_t b[] = {32767, -32767};
                int32_t c = 5;
                _dot16(v_load_int16x2(a), v_load_int16x2(b), &c);
                CHECK(c, 5, 112);
            }
            {
                GLOBAL_SYMBOL("op_dot16_extreme_mix_to_minus1");
                int16_t a[] = {-32768, 32767};
                int16_t b[] = {1, 1};
                int32_t c = 0;
                _dot16(v_load_int16x2(a), v_load_int16x2(b), &c);
                // -32768 + 32767 = -1
                CHECK(c, -1, 113);
            }
        }

        { // dot16u
            {
                GLOBAL_SYMBOL("op_uint16");
                uint16_t a[] = {1, 16153};
                uint16_t b[] = {33561, 2};
                int32_t c = 5412;
                _dot16u(v_load_uint16x2(a), v_load_uint16x2(b), &c);
                CHECK(c, 71279, 201);
            }
            {
                GLOBAL_SYMBOL("op_dot16u_zero");
                uint16_t a[] = {0, 0};
                uint16_t b[] = {0, 0};
                int32_t c = 0;
                _dot16u(v_load_uint16x2(a), v_load_uint16x2(b), &c);
                CHECK(c, 0, 202);
            }
            {
                GLOBAL_SYMBOL("op_dot16u_single_lane_pos");
                uint16_t a[] = {9, 0};
                uint16_t b[] = {8, 0};
                int32_t c = 20;
                _dot16u(v_load_uint16x2(a), v_load_uint16x2(b), &c);
                CHECK(c, (9 * 8 + 20), 203);
            }
            {
                GLOBAL_SYMBOL("op_dot16u_single_lane_hi_bit");
                // If rs1/rs2 are treated as signed by mistake:
                // 0x8000 would be -32768, giving -65536 instead of +65536.
                uint16_t a[] = {0x8000u, 0};
                uint16_t b[] = {2, 0};
                int32_t c = 0;
                _dot16u(v_load_uint16x2(a), v_load_uint16x2(b), &c);
                CHECK(c, 65536, 204);
            }
            {
                GLOBAL_SYMBOL("op_dot16u_alt_mixed");
                // same bit-patterns as (-256, 255) and (255, -256)
                // but unsigned lanes
                uint16_t a[] = {(uint16_t)-256, 255};
                uint16_t b[] = {255, (uint16_t)-256};
                int32_t c = -1;
                _dot16u(v_load_uint16x2(a), v_load_uint16x2(b), &c);
                CHECK(c, 33292799, 205);
            }
            {
                GLOBAL_SYMBOL("op_dot16u_all_ffff_wrap");
                // Each lane: 0xFFFF * 0xFFFF = 0xFFFE0001
                // Sum: 2 * 0xFFFE0001 = 0xFFFC0002 (mod 2^32)
                // + c (-1) => 0xFFFC0001
                uint16_t a[] = {(uint16_t)-1, (uint16_t)-1};
                uint16_t b[] = {(uint16_t)-1, (uint16_t)-1};
                int32_t c = -1;
                _dot16u(v_load_uint16x2(a), v_load_uint16x2(b), &c);
                CHECK(c, (int32_t)0xFFFC0001u, 206);
            }
            {
                GLOBAL_SYMBOL("op_dot16u_max_pos_pos");
                uint16_t a[] = {32767, 32767};
                uint16_t b[] = {32767, 32767};
                int32_t c = 0;
                _dot16u(v_load_uint16x2(a), v_load_uint16x2(b), &c);
                CHECK(c, ((32767 * 32767) * 2), 207);
            }
            {
                GLOBAL_SYMBOL("op_dot16u_hi_max_pos");
                // same bit-patterns as (-32768, -32768)
                // but unsigned => (32768, 32768)
                uint16_t a[] = {(uint16_t)-32768, (uint16_t)-32768};
                uint16_t b[] = {32767, 32767};
                int32_t c = 0;
                _dot16u(v_load_uint16x2(a), v_load_uint16x2(b), &c);
                CHECK(c, 2147418112, 208);
            }
            {
                GLOBAL_SYMBOL("op_dot16u_overflow_wrap");
                // 0x8000 * 0x8000 = 0x40000000
                // sum = 0x80000000 -> wraps to INT32_MIN
                uint16_t a[] = {(uint16_t)-32768, (uint16_t)-32768};
                uint16_t b[] = {(uint16_t)-32768, (uint16_t)-32768};
                int32_t c = 0;
                _dot16u(v_load_uint16x2(a), v_load_uint16x2(b), &c);
                CHECK(c, (int32_t)0x80000000u, 209);
            }
            {
                GLOBAL_SYMBOL("op_dot16u_acc_overflow");
                uint16_t a[] = {1, 0};
                uint16_t b[] = {1, 0};
                int32_t c = 2147483647; // INT32_MAX
                _dot16u(v_load_uint16x2(a), v_load_uint16x2(b), &c);
                CHECK(c, (int32_t)0x80000000u, 210);
            }
            {
                GLOBAL_SYMBOL("op_dot16u_acc_underflow");
                // Need dot == 0xFFFFFFFF (-1) using unsigned lanes:
                // 0xFFFF*0xFFFF = 0xFFFE0001
                // 2*0xFFFF       = 0x0001FFFE
                // sum            = 0xFFFFFFFF
                uint16_t a[] = {(uint16_t)-1, 2};
                uint16_t b[] = {(uint16_t)-1, (uint16_t)-1};
                int32_t c = (int32_t)0x80000000u; // INT32_MIN
                _dot16u(v_load_uint16x2(a), v_load_uint16x2(b), &c);
                CHECK(c, 2147483647, 211); // wraps to INT32_MAX
            }
            {
                GLOBAL_SYMBOL("op_dot16u_large_cancel_wrap");
                // dot = 0x80000000, start c = 0x80000000 => result 0x00000000
                uint16_t a[] = {(uint16_t)-32768, (uint16_t)-32768};
                uint16_t b[] = {(uint16_t)-32768, (uint16_t)-32768};
                int32_t c = (int32_t)0x80000000u;
                _dot16u(v_load_uint16x2(a), v_load_uint16x2(b), &c);
                CHECK(c, 0, 212);
            }
            {
                GLOBAL_SYMBOL("op_dot16u_extreme_mix_to_65535");
                // Unsigned: 0x8000 + 0x7FFF = 0xFFFF (65535)
                // If treated signed by mistake, you'd get -1.
                uint16_t a[] = {(uint16_t)-32768, 32767};
                uint16_t b[] = {1, 1};
                int32_t c = 0;
                _dot16u(v_load_uint16x2(a), v_load_uint16x2(b), &c);
                CHECK(c, 65535, 213);
            }
        }

        { // dot8
            {
                GLOBAL_SYMBOL("op_dot8");
                int8_t a[] = {-5, 99, 127, -33};
                int8_t b[] = {44, 32, -2, -99};
                int32_t c = -1089;
                _dot8(v_load_int8x4(a), v_load_int8x4(b), &c);
                CHECK(c, 4872, 301);
            }
            {
                GLOBAL_SYMBOL("op_dot8_zero");
                int8_t a[] = {0, 0, 0, 0};
                int8_t b[] = {0, 0, 0, 0};
                int32_t c = 0;
                _dot8(v_load_int8x4(a), v_load_int8x4(b), &c);
                CHECK(c, 0, 302);
            }
            {
                GLOBAL_SYMBOL("op_dot8_single_lane_pos");
                int8_t a[] = {9, 0, 0, 0};
                int8_t b[] = {8, 0, 0, 0};
                int32_t c = 20;
                _dot8(v_load_int8x4(a), v_load_int8x4(b), &c);
                CHECK(c, (9 * 8 + 20), 303);
            }
            {
                GLOBAL_SYMBOL("op_dot8_single_lane_neg");
                int8_t a[] = {0, 0, 0, -5};
                int8_t b[] = {0, 0, 0, -100};
                int32_t c = 982;
                _dot8(v_load_int8x4(a), v_load_int8x4(b), &c);
                CHECK(c, (-5 * -100 + 982), 304);
            }
            {
                GLOBAL_SYMBOL("op_dot8_alt_mixed_neg");
                int8_t a[] = {-128, 127, -128, 127};
                int8_t b[] = {127, -128, 127, -128};
                int32_t c = -1;
                _dot8(v_load_int8x4(a), v_load_int8x4(b), &c);
                // ((-128 * 127) * 4) - 1
                CHECK(c, -65025, 305);
            }
            {
                GLOBAL_SYMBOL("op_dot8_all_minus1_pos");
                int8_t a[] = {-1, -1, -1, -1};
                int8_t b[] = {-1, -1, -1, -1};
                int32_t c = -1;
                _dot8(v_load_int8x4(a), v_load_int8x4(b), &c);
                // (-1 * -1) * 4 - 1
                CHECK(c, 3, 306);
            }
            {
                GLOBAL_SYMBOL("op_dot8_max_pos_pos");
                int8_t a[] = {127, 127, 127, 127};
                int8_t b[] = {127, 127, 127, 127};
                int32_t c = 0;
                _dot8(v_load_int8x4(a), v_load_int8x4(b), &c);
                CHECK(c, ((127 * 127) * 4), 307);
            }
            {
                GLOBAL_SYMBOL("op_dot8_max_neg_pos");
                int8_t a[] = {-128, -128, -128, -128};
                int8_t b[] = {127, 127, 127, 127};
                int32_t c = 0;
                _dot8(v_load_int8x4(a), v_load_int8x4(b), &c);
                CHECK(c, ((-128 * 127) * 4), 308);
            }
            {
                GLOBAL_SYMBOL("op_dot8_max_magnitude_neg_neg");
                // NOTE: int8x4 mul cannot overflow int32 on its own
                // max val: (-128*-128)*4 = 65536, fits easily in int32
                // this replaces the overflow_wrap test from dot16
                int8_t a[] = {-128, -128, -128, -128};
                int8_t b[] = {-128, -128, -128, -128};
                int32_t c = 0;
                _dot8(v_load_int8x4(a), v_load_int8x4(b), &c);
                CHECK(c, 65536, 309);
            }
            {
                GLOBAL_SYMBOL("op_dot8_acc_overflow");
                int8_t a[] = {1, 0, 0, 0};
                int8_t b[] = {1, 0, 0, 0};
                // start at MAX, adding 1 should wrap to MIN
                int32_t c = 2147483647;
                _dot8(v_load_int8x4(a), v_load_int8x4(b), &c);
                CHECK(c, (int32_t)0x80000000, 310);
            }
            {
                GLOBAL_SYMBOL("op_dot8_acc_underflow");
                int8_t a[] = {1, 0, 0, 0};
                int8_t b[] = {-1, 0, 0, 0};
                // start at MIN, subtracting 1 should wrap to MAX
                int32_t c = (int32_t)0x80000000;
                _dot8(v_load_int8x4(a), v_load_int8x4(b), &c);
                CHECK(c, 2147483647, 311);
            }
            {
                GLOBAL_SYMBOL("op_dot8_large_cancel");
                // 2 lanes positive max, 2 lanes negative max = 0
                int8_t a[] = {127, 127, 127, 127};
                int8_t b[] = {127, 127, -127, -127};
                int32_t c = 5;
                _dot8(v_load_int8x4(a), v_load_int8x4(b), &c);
                CHECK(c, 5, 312);
            }
            {
                GLOBAL_SYMBOL("op_dot8_extreme_mix_to_minus1");
                int8_t a[] = {-128, 127, 0, 0};
                int8_t b[] = {1, 1, 0, 0};
                int32_t c = 0;
                _dot8(v_load_int8x4(a), v_load_int8x4(b), &c);
                // -128 + 127 = -1
                CHECK(c, -1, 313);
            }
        }

        { // dot8u
            {
                GLOBAL_SYMBOL("op_dot8u");
                uint8_t a[] = {188, 4, 127, 255};
                uint8_t b[] = {7, 64, 1, 16};
                int32_t c = 451;
                _dot8u(v_load_uint8x4(a), v_load_uint8x4(b), &c);
                CHECK(c, 6230, 401);
            }
            {
                GLOBAL_SYMBOL("op_dot8u_zero");
                uint8_t a[] = {0, 0, 0, 0};
                uint8_t b[] = {0, 0, 0, 0};
                int32_t c = 0;
                _dot8u(v_load_uint8x4(a), v_load_uint8x4(b), &c);
                CHECK(c, 0, 402);
            }
            {
                GLOBAL_SYMBOL("op_dot8u_lane_order");
                uint8_t a[] = {1, 2, 3, 4};
                uint8_t b[] = {5, 6, 7, 8};
                int32_t c = 9;
                _dot8u(v_load_uint8x4(a), v_load_uint8x4(b), &c);
                CHECK(c, 79, 403);
            }
            {
                GLOBAL_SYMBOL("op_dot8u_single_lane_lo");
                uint8_t a[] = {9, 0, 0, 0};
                uint8_t b[] = {8, 0, 0, 0};
                int32_t c = 20;
                _dot8u(v_load_uint8x4(a), v_load_uint8x4(b), &c);
                CHECK(c, (9 * 8 + 20), 404);
            }
            {
                GLOBAL_SYMBOL("op_dot8u_single_lane_hi");
                // Unsignedness trap: 0x80 must be 128 (not -128)
                uint8_t a[] = {0x80u, 0, 0, 0};
                uint8_t b[] = {2, 0, 0, 0};
                int32_t c = 0;
                _dot8u(v_load_uint8x4(a), v_load_uint8x4(b), &c);
                CHECK(c, 256, 405);
            }
            {
                GLOBAL_SYMBOL("op_dot8u_last_lane_only");
                // Ensures lane3 is included (not only first 2 lanes, etc.)
                uint8_t a[] = {0, 0, 0, 5};
                uint8_t b[] = {0, 0, 0, 7};
                int32_t c = 10;
                _dot8u(v_load_uint8x4(a), v_load_uint8x4(b), &c);
                CHECK(c, 45, 406);
            }
            {
                GLOBAL_SYMBOL("op_dot8u_all_ff");
                // max dot = 4 * 255*255 = 260100
                uint8_t a[] = {(uint8_t)-1, (uint8_t)-1, (uint8_t)-1, (uint8_t)-1};
                uint8_t b[] = {(uint8_t)-1, (uint8_t)-1, (uint8_t)-1, (uint8_t)-1};
                int32_t c = -1;
                _dot8u(v_load_uint8x4(a), v_load_uint8x4(b), &c);
                CHECK(c, 260099, 407);
            }
            {
                GLOBAL_SYMBOL("op_dot8u_extreme_mix_to_255");
                // unsigned: 128 + 127 = 255
                // if treated signed, you'd get -1.
                uint8_t a[] = {0x80u, 0x7Fu, 0, 0};
                uint8_t b[] = {1, 1, 0, 0};
                int32_t c = 0;
                _dot8u(v_load_uint8x4(a), v_load_uint8x4(b), &c);
                CHECK(c, 255, 408);
            }
            {
                GLOBAL_SYMBOL("op_dot8u_acc_overflow");
                // start at MAX, adding 1 should wrap to MIN
                uint8_t a[] = {1, 0, 0, 0};
                uint8_t b[] = {1, 0, 0, 0};
                int32_t c = 2147483647;
                _dot8u(v_load_uint8x4(a), v_load_uint8x4(b), &c);
                CHECK(c, (int32_t)0x80000000u, 409);
            }
            {
                GLOBAL_SYMBOL("op_dot8u_acc_overflow_large_dot");
                // Force overflow with a larger dot (260100):
                // pick c = INT32_MAX - 260100 + 1 so result wraps to INT32_MIN.
                uint8_t a[] = {(uint8_t)-1, (uint8_t)-1, (uint8_t)-1, (uint8_t)-1};
                uint8_t b[] = {(uint8_t)-1, (uint8_t)-1, (uint8_t)-1, (uint8_t)-1};
                int32_t c = 2147223548; // 2147483647 - 260100 + 1
                _dot8u(v_load_uint8x4(a), v_load_uint8x4(b), &c);
                CHECK(c, (int32_t)0x80000000u, 410);
            }
            {
                GLOBAL_SYMBOL("op_dot8u_large_cancel");
                // dot = 4 * 255*255 = 260100
                // c = -260100 so result becomes 0 (tests wrap/cancel logic)
                uint8_t a[] = {(uint8_t)-1, (uint8_t)-1, (uint8_t)-1, (uint8_t)-1};
                uint8_t b[] = {(uint8_t)-1, (uint8_t)-1, (uint8_t)-1, (uint8_t)-1};
                int32_t c = -260100;
                _dot8u(v_load_uint8x4(a), v_load_uint8x4(b), &c);
                CHECK(c, 0, 411);
            }
            {
                GLOBAL_SYMBOL("op_dot8u_acc_min_plus_maxdot");
                // Underflow can't happen for dot8u,
                // but this hits the "MIN + something" boundary region.
                // INT32_MIN + 260100 = -2147223548 (no wrap expected)
                uint8_t a[] = {(uint8_t)-1, (uint8_t)-1, (uint8_t)-1, (uint8_t)-1};
                uint8_t b[] = {(uint8_t)-1, (uint8_t)-1, (uint8_t)-1, (uint8_t)-1};
                int32_t c = (int32_t)0x80000000u; // INT32_MIN
                _dot8u(v_load_uint8x4(a), v_load_uint8x4(b), &c);
                CHECK(c, -2147223548, 412);
            }
            {
                GLOBAL_SYMBOL("op_dot8u_acc_hit_max_exact");
                // Make the result hit INT32_MAX exactly (no wrap):
                // c = INT32_MAX - 260100 = 2147223547; result should be 2147483647.
                uint8_t a[] = {(uint8_t)-1, (uint8_t)-1, (uint8_t)-1, (uint8_t)-1};
                uint8_t b[] = {(uint8_t)-1, (uint8_t)-1, (uint8_t)-1, (uint8_t)-1};
                int32_t c = 2147223547;
                _dot8u(v_load_uint8x4(a), v_load_uint8x4(b), &c);
                CHECK(c, 2147483647, 413);
            }
        }

        {
            { // int4
                GLOBAL_SYMBOL("op_dot4");
                // (7, 0), (2, 0), (-2, -1), (2, -7)
                int8_t a[] = {7, 2, -2, -110};
                 // (0, 4), (2, -7), (-2, -1), (-1, 6)
                int8_t b[] = {64, -110, -2, 111};
                int32_t c = -321;
                _dot4(v_load_int4x8(a), v_load_int4x8(b), &c);
                CHECK(c, -356, 501);
            }
            {
                GLOBAL_SYMBOL("op_dot4_zero");
                int8_t a[] = {0, 0, 0, 0};
                int8_t b[] = {0, 0, 0, 0};
                int32_t c = 0;
                _dot4(v_load_int4x8(a), v_load_int4x8(b), &c);
                CHECK(c, 0, 502);
            }
            {
                GLOBAL_SYMBOL("op_dot4_single_lane_pos");
                // Lane 0 = 7, Lane 1..7 = 0
                int8_t a[] = {PK(7, 0), 0, 0, 0};
                int8_t b[] = {PK(2, 0), 0, 0, 0};
                int32_t c = 20;
                _dot4(v_load_int4x8(a), v_load_int4x8(b), &c);
                CHECK(c, (7 * 2 + 20), 503);
            }
            {
                GLOBAL_SYMBOL("op_dot4_single_lane_neg");
                // Lane 0 = -8, Lane 1..7 = 0
                int8_t a[] = {PK(-8, 0), 0, 0, 0};
                int8_t b[] = {PK(7, 0), 0, 0, 0};
                int32_t c = 100;
                _dot4(v_load_int4x8(a), v_load_int4x8(b), &c);
                CHECK(c, (-8 * 7 + 100), 504);
            }
            {
                GLOBAL_SYMBOL("op_dot4_all_max_pos");
                // All lanes = 7
                // 0x77 = PK(7, 7)
                int8_t a[] = {0x77, 0x77, 0x77, 0x77};
                int8_t b[] = {0x77, 0x77, 0x77, 0x77};
                int32_t c = 0;
                _dot4(v_load_int4x8(a), v_load_int4x8(b), &c);
                CHECK(c, ((7 * 7) * 8), 505);
            }
            {
                GLOBAL_SYMBOL("op_dot4_all_max_neg");
                // All lanes = -8
                // 0x88 = PK(-8, -8)
                int8_t a[] = {(int8_t)0x88, (int8_t)0x88, (int8_t)0x88, (int8_t)0x88};
                int8_t b[] = {(int8_t)0x88, (int8_t)0x88, (int8_t)0x88, (int8_t)0x88};
                int32_t c = 0;
                _dot4(v_load_int4x8(a), v_load_int4x8(b), &c);
                CHECK(c, ((-8 * -8) * 8), 506);
            }
            {
                GLOBAL_SYMBOL("op_dot4_max_neg_vs_pos");
                // All lanes: a=-8, b=7
                int8_t a[] = {(int8_t)0x88, (int8_t)0x88, (int8_t)0x88, (int8_t)0x88};
                int8_t b[] = {0x77, 0x77, 0x77, 0x77};
                int32_t c = 0;
                _dot4(v_load_int4x8(a), v_load_int4x8(b), &c);
                CHECK(c, ((-8 * 7) * 8), 507);
            }
            {
                GLOBAL_SYMBOL("op_dot4_acc_overflow");
                // 8 lanes of 1*1 = 8.
                int8_t a[] = {PK(1,1), PK(1,1), PK(1,1), PK(1,1)};
                int8_t b[] = {PK(1,1), PK(1,1), PK(1,1), PK(1,1)};
                // Start at MAX - 7. Adding 8 should wrap to MIN.
                int32_t c = 2147483640;
                _dot4(v_load_int4x8(a), v_load_int4x8(b), &c);
                CHECK(c, (int32_t)0x80000000, 508);
            }
            {
                GLOBAL_SYMBOL("op_dot4_large_cancel");
                // First 4 lanes max pos product (7*7 = 49)
                // Last 4 lanes max neg product (-8*7 = -56)
                // Sum = (49*4) + (-56*4) = 196 - 224 = -28
                int8_t a[] = {PK(7,7), PK(7,7), PK(-8,-8), PK(-8,-8)};
                int8_t b[] = {PK(7,7), PK(7,7), PK(7,7),   PK(7,7)};
                int32_t c = 28;
                _dot4(v_load_int4x8(a), v_load_int4x8(b), &c);
                CHECK(c, 0, 509);
            }
            {
                GLOBAL_SYMBOL("op_dot4_high_nibble_isolation");
                // ensure high nibble of 'a' doesn't bleed into low nibble res
                // a: (0, -8), b: (0, 1) -> Low: 0*0=0, High: -8*1=-8. Sum=-8
                int8_t a[] = {PK(0, -8), 0, 0, 0};
                int8_t b[] = {PK(0, 1), 0, 0, 0};
                int32_t c = 0;
                _dot4(v_load_int4x8(a), v_load_int4x8(b), &c);
                CHECK(c, -8, 510);
            }
        }

        {
            {
                GLOBAL_SYMBOL("op_dot4u");
                uint8_t a[] = {116, 1, 8, 14}; // (4, 7), (1, 0), (8, 0), (14, 0)
                uint8_t b[] = {1, 64, 5, 187}; // (1, 0), (0, 4), (5, 0), (11, 11)
                int32_t c = 4509;
                _dot4u(v_load_uint4x8(a), v_load_uint4x8(b), &c);
                CHECK(c, 4707, 601);
            }
            {
                GLOBAL_SYMBOL("op_dot4u_zero");
                uint8_t a[] = {0, 0, 0, 0};
                uint8_t b[] = {0, 0, 0, 0};
                int32_t c = 0;
                _dot4u(v_load_uint4x8(a), v_load_uint4x8(b), &c);
                CHECK(c, 0, 602);
            }
            {
                GLOBAL_SYMBOL("op_dot4u_single_lane");
                // Lane 0 = 15 (0xF), others 0
                uint8_t a[] = {PK_U(15, 0), 0, 0, 0};
                uint8_t b[] = {PK_U(1, 0), 0, 0, 0};
                int32_t c = 10;
                _dot4u(v_load_uint4x8(a), v_load_uint4x8(b), &c);
                CHECK(c, (15 * 1 + 10), 603);
            }
            {
                GLOBAL_SYMBOL("op_dot4u_all_max");
                // All lanes = 15 (0xF)
                // 0xFF = PK_U(15, 15)
                uint8_t a[] = {0xFF, 0xFF, 0xFF, 0xFF};
                uint8_t b[] = {0xFF, 0xFF, 0xFF, 0xFF};
                int32_t c = 0;
                _dot4u(v_load_uint4x8(a), v_load_uint4x8(b), &c);
                // (15 * 15) * 8 = 225 * 8 = 1800
                CHECK(c, 1800, 604);
            }
            {
                GLOBAL_SYMBOL("op_dot4u_acc_overflow");
                // All lanes = 1. Dot product = 8.
                uint8_t a[] = {PK_U(1,1), PK_U(1,1), PK_U(1,1), PK_U(1,1)};
                uint8_t b[] = {PK_U(1,1), PK_U(1,1), PK_U(1,1), PK_U(1,1)};
                // Start at MAX - 7. Adding 8 should wrap to MIN.
                int32_t c = 2147483640;
                _dot4u(v_load_uint4x8(a), v_load_uint4x8(b), &c);
                CHECK(c, (int32_t)0x80000000, 605);
            }
            {
                GLOBAL_SYMBOL("op_dot4u_negative_acc_recovery");
                // Unsigned dot products are always positive, but they can
                // pull a negative accumulator back towards zero.
                uint8_t a[] = {0xFF, 0xFF, 0xFF, 0xFF}; // Max (15)
                uint8_t b[] = {0xFF, 0xFF, 0xFF, 0xFF}; // Max (15)
                // Sum = 1800. Start accumulator at -1800. Result should be 0.
                int32_t c = -1800;
                _dot4u(v_load_uint4x8(a), v_load_uint4x8(b), &c);
                CHECK(c, 0, 606);
            }
            {
                GLOBAL_SYMBOL("op_dot4u_nibble_isolation");
                // Ensure high nibble of A doesn't interact with low nibble of B
                // Lane 0: a=0, b=15. Lane 1: a=15, b=0.
                // Result should be 0.
                uint8_t a[] = {PK_U(0, 15), 0, 0, 0};
                uint8_t b[] = {PK_U(15, 0), 0, 0, 0};
                int32_t c = 100;
                _dot4u(v_load_uint4x8(a), v_load_uint4x8(b), &c);
                CHECK(c, 100, 607);
            }
        }

        {
            {
                GLOBAL_SYMBOL("op_dot2");
                // (0, 1, 1, 0), (1, 1, 1, 1), (-2, -1, -1, -1), (0, -2, -1, -2)
                int8_t a[] = {20, 85, -2, -72};
                // (-2, -2, 0, -1), (0, 1, 1, 0), (-2, -1, -1, -1), (-1, -2, -1, -2)
                int8_t b[] = {-54, 20, -2, -69};
                int32_t c = -1001;
                _dot2(v_load_int2x16(a), v_load_int2x16(b), &c);
                CHECK(c, -985, 701);
            }
            {
                GLOBAL_SYMBOL("op_dot2_zero");
                int8_t a[] = {0, 0, 0, 0};
                int8_t b[] = {0, 0, 0, 0};
                int32_t c = 0;
                _dot2(v_load_int2x16(a), v_load_int2x16(b), &c);
                CHECK(c, 0, 702);
            }
            {
                GLOBAL_SYMBOL("op_dot2_single_lane_pos");
                // Lane 0 = 1, others 0
                int8_t a[] = {PK2(0,0,0,1), 0, 0, 0};
                int8_t b[] = {PK2(0,0,0,1), 0, 0, 0};
                int32_t c = 50;
                _dot2(v_load_int2x16(a), v_load_int2x16(b), &c);
                CHECK(c, (1 * 1 + 50), 703);
            }
            {
                GLOBAL_SYMBOL("op_dot2_single_lane_neg");
                // Lane 0 = -2, others 0
                int8_t a[] = {PK2(0,0,0,-2), 0, 0, 0};
                int8_t b[] = {PK2(0,0,0,1), 0, 0, 0};
                int32_t c = 100;
                _dot2(v_load_int2x16(a), v_load_int2x16(b), &c);
                CHECK(c, (-2 * 1 + 100), 704);
            }
            {
                GLOBAL_SYMBOL("op_dot2_all_max_pos");
                // All lanes = 1 (binary 01)
                // 0x55 = 01010101
                int8_t a[] = {0x55, 0x55, 0x55, 0x55};
                int8_t b[] = {0x55, 0x55, 0x55, 0x55};
                int32_t c = 0;
                _dot2(v_load_int2x16(a), v_load_int2x16(b), &c);
                CHECK(c, ((1 * 1) * 16), 705);
            }
            {
                GLOBAL_SYMBOL("op_dot2_all_max_neg");
                // All lanes = -2 (binary 10)
                // 0xAA = 10101010 (cast to int8_t to suppress compiler warnings)
                int8_t a[] = {(int8_t)0xAA, (int8_t)0xAA, (int8_t)0xAA, (int8_t)0xAA};
                int8_t b[] = {(int8_t)0xAA, (int8_t)0xAA, (int8_t)0xAA, (int8_t)0xAA};
                int32_t c = 0;
                _dot2(v_load_int2x16(a), v_load_int2x16(b), &c);
                // (-2 * -2) * 16 = 4 * 16 = 64
                CHECK(c, 64, 706);
            }
            {
                GLOBAL_SYMBOL("op_dot2_max_neg_vs_pos");
                // A = -2, B = 1 everywhere
                int8_t a[] = {(int8_t)0xAA, (int8_t)0xAA, (int8_t)0xAA, (int8_t)0xAA};
                int8_t b[] = {0x55, 0x55, 0x55, 0x55};
                int32_t c = 0;
                _dot2(v_load_int2x16(a), v_load_int2x16(b), &c);
                // (-2 * 1) * 16 = -32
                CHECK(c, -32, 707);
            }
            {
                GLOBAL_SYMBOL("op_dot2_acc_overflow");
                // 16 lanes of 1*1 = 16.
                int8_t a[] = {0x55, 0x55, 0x55, 0x55};
                int8_t b[] = {0x55, 0x55, 0x55, 0x55};
                // Start at MAX - 15. Adding 16 should wrap to MIN.
                int32_t c = 2147483632;
                _dot2(v_load_int2x16(a), v_load_int2x16(b), &c);
                CHECK(c, (int32_t)0x80000000, 708);
            }
            {
                GLOBAL_SYMBOL("op_dot2_bit_packing_isolation");
                // Crucial for sub-byte SIMD. Ensure bit fields don't bleed.
                // Lane 0 of 'a' is -2 (10), others 0.
                // Lane 1 of 'b' is 1 (01), others 0.
                // Result should be 0. If bits bleed/shift wrong, you get nonzero.
                int8_t a[] = {PK2(0,0,0,-2), 0, 0, 0};
                int8_t b[] = {PK2(0,0,1, 0), 0, 0, 0};
                int32_t c = 7;
                _dot2(v_load_int2x16(a), v_load_int2x16(b), &c);
                CHECK(c, 7, 709);
            }
        }

        {
            {
                GLOBAL_SYMBOL("op_dot2u");
                // (0, 1, 0, 1), (1, 1, 1, 1), (1, 3, 2, 1), (0, 2, 3, 2)
                uint8_t a[] = {68, 85, 109, 184};
                // (3, 0, 2, 2), (0, 1, 1, 0), (0, 3, 1, 2), (2, 3, 2, 3)
                uint8_t b[] = {202, 20, 54, 187};
                int32_t c = 99;
                _dot2u(v_load_uint2x16(a), v_load_uint2x16(b), &c);
                CHECK(c, 134, 801);
            }
            {
                GLOBAL_SYMBOL("op_dot2u_zero");
                uint8_t a[] = {0, 0, 0, 0};
                uint8_t b[] = {0, 0, 0, 0};
                int32_t c = 0;
                _dot2u(v_load_uint2x16(a), v_load_uint2x16(b), &c);
                CHECK(c, 0, 802);
            }
            {
                GLOBAL_SYMBOL("op_dot2u_single_lane");
                // Lane 0 = 3, others 0
                uint8_t a[] = {PK2_U(0,0,0,3), 0, 0, 0};
                uint8_t b[] = {PK2_U(0,0,0,2), 0, 0, 0};
                int32_t c = 10;
                _dot2u(v_load_uint2x16(a), v_load_uint2x16(b), &c);
                CHECK(c, (3 * 2 + 10), 803);
            }
            {
                GLOBAL_SYMBOL("op_dot2u_all_max");
                // All lanes = 3 (binary 11)
                // 0xFF = 11111111
                uint8_t a[] = {0xFF, 0xFF, 0xFF, 0xFF};
                uint8_t b[] = {0xFF, 0xFF, 0xFF, 0xFF};
                int32_t c = 0;
                _dot2u(v_load_uint2x16(a), v_load_uint2x16(b), &c);
                // (3 * 3) * 16 = 9 * 16 = 144
                CHECK(c, 144, 804);
            }
            {
                GLOBAL_SYMBOL("op_dot2u_acc_overflow");
                // 16 lanes of 1*1 = 16.
                // 0x55 = 01010101
                uint8_t a[] = {0x55, 0x55, 0x55, 0x55};
                uint8_t b[] = {0x55, 0x55, 0x55, 0x55};
                // Start at MAX - 15. Adding 16 should wrap to MIN.
                int32_t c = 2147483632;
                _dot2u(v_load_uint2x16(a), v_load_uint2x16(b), &c);
                CHECK(c, (int32_t)0x80000000, 805);
            }
            {
                GLOBAL_SYMBOL("op_dot2u_negative_acc_recovery");
                // All lanes max (3). Total dot product = 144.
                uint8_t a[] = {0xFF, 0xFF, 0xFF, 0xFF};
                uint8_t b[] = {0xFF, 0xFF, 0xFF, 0xFF};
                // Start at -144. Result should be 0.
                int32_t c = -144;
                _dot2u(v_load_uint2x16(a), v_load_uint2x16(b), &c);
                CHECK(c, 0, 806);
            }
            {
                GLOBAL_SYMBOL("op_dot2u_bit_packing_isolation");
                // Ensure adjacent bits don't bleed.
                // A: Lane 0 = 3 (11), Lane 1 = 0
                // B: Lane 0 = 0, Lane 1 = 3 (11)
                // Product should be 0.
                uint8_t a[] = {PK2_U(0,0,0,3), 0, 0, 0};
                uint8_t b[] = {PK2_U(0,0,3,0), 0, 0, 0};
                int32_t c = 77;
                _dot2u(v_load_uint2x16(a), v_load_uint2x16(b), &c);
                CHECK(c, 77, 807);
            }
        }

    }
    pass();
}
