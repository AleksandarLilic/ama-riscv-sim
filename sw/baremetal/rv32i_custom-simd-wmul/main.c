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

        { // wmul16 (signed)
            {
                GLOBAL_SYMBOL("op_wmul16");
                int16_t a[] = {10, -10};
                int16_t b[] = {20, -20};
                int32x2_t c = _wmul16(v_load_int16x2(a), v_load_int16x2(b));
                CHECK(c.w.lo, 200, 101);
                CHECK(c.w.hi, 200, 102);
            }
            {
                GLOBAL_SYMBOL("op_wmul16_signs");
                // verify negative result handling
                int16_t a[] = {-1, 1};
                int16_t b[] = {1000, -1000};
                int32x2_t c = _wmul16(v_load_int16x2(a), v_load_int16x2(b));
                CHECK(c.w.lo, -1000, 103);
                CHECK(c.w.hi, -1000, 104);
            }
            {
                GLOBAL_SYMBOL("op_wmul16_max_mag");
                // min * min = positive max power (2^30)
                // -32768 * -32768 = 1,073,741,824
                int16_t a[] = {-32768, -32768};
                int16_t b[] = {-32768, 32767};
                int32x2_t c = _wmul16(v_load_int16x2(a), v_load_int16x2(b));
                CHECK(c.w.lo, 1073741824, 105);
                CHECK(c.w.hi, -1073709056, 106);
            }
        }

        { // wmul16u (unsigned)
            {
                GLOBAL_SYMBOL("op_wmul16u");
                uint16_t a[] = {100, 200};
                uint16_t b[] = {100, 200};
                uint32x2_t c = _wmul16u(v_load_uint16x2(a), v_load_uint16x2(b));
                CHECK(c.w.lo, 10000, 111);
                CHECK(c.w.hi, 40000, 112);
            }
            {
                GLOBAL_SYMBOL("op_wmul16u_high_bit");
                // signed: -1 * 1 = -1
                // unsigned: 65535 * 1 = 65535
                uint16_t a[] = {0xFFFF, 0};
                uint16_t b[] = {1, 0};
                uint32x2_t c = _wmul16u(v_load_uint16x2(a), v_load_uint16x2(b));
                CHECK(c.w.lo, 65535, 113);
                CHECK(c.w.hi, 0, 114);
            }
            {
                GLOBAL_SYMBOL("op_wmul16u_max");
                // 65535 * 65535 = 4,294,836,225 (0xFFFE0001)
                uint16_t a[] = {65535, 65535};
                uint16_t b[] = {65535, 65535};
                uint32x2_t c = _wmul16u(v_load_uint16x2(a), v_load_uint16x2(b));
                CHECK(c.w.lo, 0xFFFE0001, 115);
                CHECK(c.w.hi, 0xFFFE0001, 116);
            }
        }

        { // wmul8 (signed)
            {
                GLOBAL_SYMBOL("op_wmul8");
                int8_t a[] = {10, -10, 2, -2};
                int8_t b[] = {5, 5, 3, 3};
                int16x4_t c = _wmul8(v_load_int8x4(a), v_load_int8x4(b));
                int32x2_t r_lo = _widen16(c.w.lo);
                int32x2_t r_hi = _widen16(c.w.hi);
                CHECK(r_lo.w.lo, 50, 201);
                CHECK(r_lo.w.hi, -50, 202);
                CHECK(r_hi.w.lo, 6, 203);
                CHECK(r_hi.w.hi, -6, 204);
            }
            {
                GLOBAL_SYMBOL("op_wmul8_signs");
                // -1 * 100 = -100.
                // if treated as unsigned, 255 * 100 = 25500, a distinct diff
                int8_t a[] = {-1, -1, -1, -1};
                int8_t b[] = {100, 100, 100, 100};
                int16x4_t c = _wmul8(v_load_int8x4(a), v_load_int8x4(b));
                int32x2_t r_lo = _widen16(c.w.lo);
                int32x2_t r_hi = _widen16(c.w.hi);
                CHECK(r_lo.w.lo, -100, 211);
                CHECK(r_lo.w.hi, -100, 212);
                CHECK(r_hi.w.lo, -100, 213);
                CHECK(r_hi.w.hi, -100, 214);
            }
            {
                GLOBAL_SYMBOL("op_wmul8_max_mag");
                // -128 * -128 = 16384
                int8_t a[] = {-128, -128, 0, 0};
                int8_t b[] = {-128, -128, 0, 0};
                int16x4_t c = _wmul8(v_load_int8x4(a), v_load_int8x4(b));
                int32x2_t r_lo = _widen16(c.w.lo);
                int32x2_t r_hi = _widen16(c.w.hi);
                CHECK(r_lo.w.lo, 16384, 221);
                CHECK(r_lo.w.hi, 16384, 222);
                CHECK(r_hi.w.lo, 0, 223);
                CHECK(r_hi.w.hi, 0, 224);
            }
        }

        { // wmul8u (unsigned)
            {
                GLOBAL_SYMBOL("op_wmul8u");
                uint8_t a[] = {10, 20, 30, 40};
                uint8_t b[] = {10, 20, 30, 40};
                uint16x4_t c = _wmul8u(v_load_uint8x4(a), v_load_uint8x4(b));
                uint32x2_t r_lo = _widen16u(c.w.lo);
                uint32x2_t r_hi = _widen16u(c.w.hi);
                CHECK(r_lo.w.lo, 100, 241);
                CHECK(r_lo.w.hi, 400, 242);
                CHECK(r_hi.w.lo, 900, 243);
                CHECK(r_hi.w.hi, 1600, 244);
            }
            {
                GLOBAL_SYMBOL("op_wmul8u_high_bit");
                // signed: -1 * 1 = -1
                // unsigned: 255 * 1 = 255
                uint8_t a[] = {255, 0, 0, 0};
                uint8_t b[] = {1, 0, 0, 0};
                uint16x4_t c = _wmul8u(v_load_uint8x4(a), v_load_uint8x4(b));
                uint32x2_t r_lo = _widen16u(c.w.lo);
                uint32x2_t r_hi = _widen16u(c.w.hi);
                CHECK(r_lo.w.lo, 255, 251);
                CHECK(r_lo.w.hi, 0, 252);
                CHECK(r_hi.w.lo, 0, 253);
                CHECK(r_hi.w.hi, 0, 254);
            }
            {
                GLOBAL_SYMBOL("op_wmul8u_max");
                // 255 * 255 = 65025
                uint8_t a[] = {255, 255, 0, 0};
                uint8_t b[] = {255, 255, 0, 0};
                uint16x4_t c = _wmul8u(v_load_uint8x4(a), v_load_uint8x4(b));
                uint32x2_t r_lo = _widen16u(c.w.lo);
                uint32x2_t r_hi = _widen16u(c.w.hi);
                CHECK(r_lo.w.lo, 65025, 261);
                CHECK(r_lo.w.hi, 65025, 262);
                CHECK(r_hi.w.lo, 0, 263);
                CHECK(r_hi.w.hi, 0, 264);
            }
        }

    }
    pass();
}
