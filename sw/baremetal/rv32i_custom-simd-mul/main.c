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

        { // mul16: low 16 bits of signed 16-bit product
            {
                GLOBAL_SYMBOL("op_mul16");
                int16_t a[] = {10, -10};
                int16_t b[] = {20, -20};
                int16x2_t c = _mul16(v_load_int16x2(a), v_load_int16x2(b));
                int32x2_t r = _widen16(c);
                CHECK(r.w.lo, 200, 101);
                CHECK(r.w.hi, 200, 102);
            }
            {
                GLOBAL_SYMBOL("op_mul16_signs");
                // negative * positive and positive * negative
                int16_t a[] = {-1, 1};
                int16_t b[] = {100, -100};
                int16x2_t c = _mul16(v_load_int16x2(a), v_load_int16x2(b));
                int32x2_t r = _widen16(c);
                CHECK(r.w.lo, -100, 103);
                CHECK(r.w.hi, -100, 104);
            }
            {
                GLOBAL_SYMBOL("op_mul16_wrap");
                // 32767*2=65534 wraps to -2 as int16; 32767*3=98301 -> low16=32765
                int16_t a[] = {32767, 32767};
                int16_t b[] = {2, 3};
                int16x2_t c = _mul16(v_load_int16x2(a), v_load_int16x2(b));
                int32x2_t r = _widen16(c);
                CHECK(r.w.lo, -2, 105);
                CHECK(r.w.hi, 32765, 106);
            }
        }

        { // mul8: low 8 bits of signed 8-bit product
            {
                GLOBAL_SYMBOL("op_mul8");
                int8_t a[] = {10, -10, 2, -2};
                int8_t b[] = {5, 5, 3, 3};
                int8x4_t c = _mul8(v_load_int8x4(a), v_load_int8x4(b));
                int16x4_t w = _widen8(c);
                int32x2_t r_lo = _widen16(w.w.lo);
                int32x2_t r_hi = _widen16(w.w.hi);
                CHECK(r_lo.w.lo, 50, 201);
                CHECK(r_lo.w.hi, -50, 202);
                CHECK(r_hi.w.lo, 6, 203);
                CHECK(r_hi.w.hi, -6, 204);
            }
            {
                GLOBAL_SYMBOL("op_mul8_wrap");
                // 127*2=254=0xFE=-2 as int8; 127*3=381->low8=0x7D=125
                int8_t a[] = {127, 127, 0, 0};
                int8_t b[] = {2, 3, 0, 0};
                int8x4_t c = _mul8(v_load_int8x4(a), v_load_int8x4(b));
                int16x4_t w = _widen8(c);
                int32x2_t r_lo = _widen16(w.w.lo);
                int32x2_t r_hi = _widen16(w.w.hi);
                CHECK(r_lo.w.lo, -2, 211);
                CHECK(r_lo.w.hi, 125, 212);
                CHECK(r_hi.w.lo, 0, 213);
                CHECK(r_hi.w.hi, 0, 214);
            }
        }

        { // mulh16: high 16 bits of signed 16-bit product
            {
                GLOBAL_SYMBOL("op_mulh16");
                // small values: high 16 is 0
                int16_t a[] = {10, -10};
                int16_t b[] = {20, -20};
                int16x2_t c = _mulh16(v_load_int16x2(a), v_load_int16x2(b));
                int32x2_t r = _widen16(c);
                CHECK(r.w.lo, 0, 301);
                CHECK(r.w.hi, 0, 302);
            }
            {
                GLOBAL_SYMBOL("op_mulh16_max_mag");
                // -32768*-32768=0x40000000 -> high16=0x4000=16384
                // 32767*32767=0x3FFF0001  -> high16=0x3FFF=16383
                int16_t a[] = {-32768, 32767};
                int16_t b[] = {-32768, 32767};
                int16x2_t c = _mulh16(v_load_int16x2(a), v_load_int16x2(b));
                int32x2_t r = _widen16(c);
                CHECK(r.w.lo, 16384, 303);
                CHECK(r.w.hi, 16383, 304);
            }
            {
                GLOBAL_SYMBOL("op_mulh16_neg");
                // -32768*32767=0xC0008000 -> high16=0xC000=-16384
                // -1*1=-1=0xFFFFFFFF     -> high16=0xFFFF=-1
                int16_t a[] = {-32768, -1};
                int16_t b[] = {32767, 1};
                int16x2_t c = _mulh16(v_load_int16x2(a), v_load_int16x2(b));
                int32x2_t r = _widen16(c);
                CHECK(r.w.lo, -16384, 305);
                CHECK(r.w.hi, -1, 306);
            }
        }

        { // mulh16u: high 16 bits of unsigned 16-bit product
            {
                GLOBAL_SYMBOL("op_mulh16u");
                // small values: high 16 is 0
                uint16_t a[] = {100, 200};
                uint16_t b[] = {100, 200};
                uint16x2_t c = _mulh16u(v_load_uint16x2(a), v_load_uint16x2(b));
                uint32x2_t r = _widen16u(c);
                CHECK(r.w.lo, 0, 401);
                CHECK(r.w.hi, 0, 402);
            }
            {
                GLOBAL_SYMBOL("op_mulh16u_high_bit");
                // 0xFFFF*1=65535 -> high16=0 (vs signed: -1*1=-1 -> high16=-1)
                uint16_t a[] = {0xFFFF, 0x8000};
                uint16_t b[] = {1, 1};
                uint16x2_t c = _mulh16u(v_load_uint16x2(a), v_load_uint16x2(b));
                uint32x2_t r = _widen16u(c);
                CHECK(r.w.lo, 0, 403);
                CHECK(r.w.hi, 0, 404);
            }
            {
                GLOBAL_SYMBOL("op_mulh16u_max");
                // 65535*65535=0xFFFE0001 -> high16=0xFFFE=65534
                uint16_t a[] = {0xFFFF, 0xFFFF};
                uint16_t b[] = {0xFFFF, 0xFFFF};
                uint16x2_t c = _mulh16u(v_load_uint16x2(a), v_load_uint16x2(b));
                uint32x2_t r = _widen16u(c);
                CHECK(r.w.lo, 65534, 405);
                CHECK(r.w.hi, 65534, 406);
            }
        }

        { // mulh8: high 8 bits of signed 8-bit product
            {
                GLOBAL_SYMBOL("op_mulh8");
                // 10*5=50=0x0032   -> high8=0
                // -10*5=-50=0xFFCE -> high8=0xFF=-1
                // 5*3=15=0x000F    -> high8=0
                // -5*3=-15=0xFFF1  -> high8=0xFF=-1 (wait: -15>>8=-1, yes)
                int8_t a[] = {10, -10, 5, -5};
                int8_t b[] = {5, 5, 3, 3};
                int8x4_t c = _mulh8(v_load_int8x4(a), v_load_int8x4(b));
                int16x4_t w = _widen8(c);
                int32x2_t r_lo = _widen16(w.w.lo);
                int32x2_t r_hi = _widen16(w.w.hi);
                CHECK(r_lo.w.lo, 0, 501);
                CHECK(r_lo.w.hi, -1, 502);
                CHECK(r_hi.w.lo, 0, 503);
                CHECK(r_hi.w.hi, -1, 504);
            }
            {
                GLOBAL_SYMBOL("op_mulh8_max_mag");
                // -128*-128=16384=0x4000 -> high8=0x40=64
                // -128*127=-16256=0xFFFFC080 -> high8=0xC0=-64
                // 0*0=0 -> high8=0
                int8_t a[] = {-128, -128, 0, 0};
                int8_t b[] = {-128, 127, 0, 0};
                int8x4_t c = _mulh8(v_load_int8x4(a), v_load_int8x4(b));
                int16x4_t w = _widen8(c);
                int32x2_t r_lo = _widen16(w.w.lo);
                int32x2_t r_hi = _widen16(w.w.hi);
                CHECK(r_lo.w.lo, 64, 511);
                CHECK(r_lo.w.hi, -64, 512);
                CHECK(r_hi.w.lo, 0, 513);
                CHECK(r_hi.w.hi, 0, 514);
            }
        }

        { // mulh8u: high 8 bits of unsigned 8-bit product
            {
                GLOBAL_SYMBOL("op_mulh8u");
                // 10*10=100=0x64   -> high8=0
                // 20*20=400=0x190  -> high8=1
                // 30*30=900=0x384  -> high8=3
                // 40*40=1600=0x640 -> high8=6
                uint8_t a[] = {10, 20, 30, 40};
                uint8_t b[] = {10, 20, 30, 40};
                uint8x4_t c = _mulh8u(v_load_uint8x4(a), v_load_uint8x4(b));
                uint16x4_t w = _widen8u(c);
                uint32x2_t r_lo = _widen16u(w.w.lo);
                uint32x2_t r_hi = _widen16u(w.w.hi);
                CHECK(r_lo.w.lo, 0, 601);
                CHECK(r_lo.w.hi, 1, 602);
                CHECK(r_hi.w.lo, 3, 603);
                CHECK(r_hi.w.hi, 6, 604);
            }
            {
                GLOBAL_SYMBOL("op_mulh8u_high_bit");
                // 255*255=65025=0xFE01 -> high8=0xFE=254
                // 200*200=40000=0x9C40 -> high8=0x9C=156 (vs signed: -56*-56=3136=0xC40 -> high8=12)
                uint8_t a[] = {255, 200, 0, 0};
                uint8_t b[] = {255, 200, 0, 0};
                uint8x4_t c = _mulh8u(v_load_uint8x4(a), v_load_uint8x4(b));
                uint16x4_t w = _widen8u(c);
                uint32x2_t r_lo = _widen16u(w.w.lo);
                uint32x2_t r_hi = _widen16u(w.w.hi);
                CHECK(r_lo.w.lo, 254, 611);
                CHECK(r_lo.w.hi, 156, 612);
                CHECK(r_hi.w.lo, 0, 613);
                CHECK(r_hi.w.hi, 0, 614);
            }
        }

    }
    pass();
}
