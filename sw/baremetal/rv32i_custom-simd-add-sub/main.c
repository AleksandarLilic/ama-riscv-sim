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

#define PK_U(lo, hi) \
    (uint8_t)( (((hi) & 0xF) << 4) | ((lo) & 0xF) )

#define PK2(l0, l1, l2, l3) \
    (int32_t)( \
        (((l3) & 0xFF) << 24) | (((l2) & 0xFF) << 16) | \
        (((l1) & 0xFF) << 8) | ((l0) & 0xFF) \
    )

#define PK2_U(l3, l2, l1, l0) \
    (uint8_t)( (((l3)&3)<<6) | (((l2)&3)<<4) | (((l1)&3)<<2) | ((l0)&3) )

void main() {
    for (uint32_t i = 0; i < LOOPS; i++) {

        { // add16
            {
                GLOBAL_SYMBOL("op_add16");
                int16_t a[] = {100, -50};
                int16_t b[] = {200, -50};
                int16x2_t c = _add16(v_load_int16x2(a), v_load_int16x2(b));
                CHECK(c.v, PK(300, -100), 1001);
            }
            {
                GLOBAL_SYMBOL("op_add16_overflow");
                // lane 0: 32767 + 1 = -32768 (wrap to min)
                // lane 1: safe
                int16_t a[] = {32767, 0};
                int16_t b[] = {1, 0};
                int16x2_t c = _add16(v_load_int16x2(a), v_load_int16x2(b));
                CHECK(c.v, PK((int16_t)0x8000, 0), 1002);
            }
            {
                GLOBAL_SYMBOL("op_add16_underflow");
                // lane 0: -32768 + (-1) = 32767 (wrap to max)
                // lane 1: safe
                int16_t a[] = {-32768, 0};
                int16_t b[] = {-1, 0};
                int16x2_t c = _add16(v_load_int16x2(a), v_load_int16x2(b));
                CHECK(c.v, PK(32767, 0), 1003);
            }
        }

        { // add8
            {
                GLOBAL_SYMBOL("op_add8");
                int8_t a[] = {10, -20, 30, -40};
                int8_t b[] = {5, 5, 5, 5};
                int8x4_t c = _add8(v_load_int8x4(a), v_load_int8x4(b));
                CHECK(c.v, PK2(15, -15, 35, -35), 1004);
            }
            {
                GLOBAL_SYMBOL("op_add8_overflow");
                // lane 0: 127 + 1 = -128 (wrap)
                int8_t a[] = {127, 0, 0, 0};
                int8_t b[] = {1, 0, 0, 0};
                int8x4_t c = _add8(v_load_int8x4(a), v_load_int8x4(b));
                CHECK(c.v, PK2((int8_t)0x80, 0, 0, 0), 1005);
            }
            {
                GLOBAL_SYMBOL("op_add8_underflow");
                // lane 0: -128 + (-1) = 127 (wrap)
                int8_t a[] = {-128, 0, 0, 0};
                int8_t b[] = {-1, 0, 0, 0};
                int8x4_t c = _add8(v_load_int8x4(a), v_load_int8x4(b));
                CHECK(c.v, PK2(127, 0, 0, 0), 1006);
            }
        }

        { // sub16
            {
                GLOBAL_SYMBOL("op_sub16");
                int16_t a[] = {300, -100};
                int16_t b[] = {200, -50};
                // 300 - 200 = 100
                // -100 - (-50) = -50
                int16x2_t c = _sub16(v_load_int16x2(a), v_load_int16x2(b));
                CHECK(c.v, PK(100, -50), 2001);
            }
            {
                GLOBAL_SYMBOL("op_sub16_overflow_to_neg");
                // lane 0: 32767 - (-1) = 32768 -> wraps to -32768 (min)
                int16_t a[] = {32767, 0};
                int16_t b[] = {-1, 0};
                int16x2_t c = _sub16(v_load_int16x2(a), v_load_int16x2(b));
                CHECK(c.v, PK((int16_t)0x8000, 0), 2002);
            }
            {
                GLOBAL_SYMBOL("op_sub16_underflow_to_pos");
                // lane 0: -32768 - 1 = -32769 -> wraps to 32767 (max)
                int16_t a[] = {-32768, 0};
                int16_t b[] = {1, 0};
                int16x2_t c = _sub16(v_load_int16x2(a), v_load_int16x2(b));
                CHECK(c.v, PK(32767, 0), 2003);
            }
        }

        { // sub8
            {
                GLOBAL_SYMBOL("op_sub8");
                int8_t a[] = {50, -50, 10, -10};
                int8_t b[] = {20, -20, 10, -10};
                int8x4_t c = _sub8(v_load_int8x4(a), v_load_int8x4(b));
                CHECK(c.v, PK2(30, -30, 0, 0), 2004);
            }
            {
                GLOBAL_SYMBOL("op_sub8_overflow_to_neg");
                // lane 0: 127 - (-1) = 128 -> wraps to -128
                int8_t a[] = {127, 0, 0, 0};
                int8_t b[] = {-1, 0, 0, 0};
                int8x4_t c = _sub8(v_load_int8x4(a), v_load_int8x4(b));
                CHECK(c.v, PK2((int8_t)0x80, 0, 0, 0), 2005);
            }
            {
                GLOBAL_SYMBOL("op_sub8_underflow_to_pos");
                // lane 0: -128 - 1 = -129 -> wraps to 127
                int8_t a[] = {-128, 0, 0, 0};
                int8_t b[] = {1, 0, 0, 0};
                int8x4_t c = _sub8(v_load_int8x4(a), v_load_int8x4(b));
                CHECK(c.v, PK2(127, 0, 0, 0), 2006);
            }
        }

        { // qadd16 (saturating signed)
            {
                GLOBAL_SYMBOL("op_qadd16");
                int16_t a[] = {1000, -1000};
                int16_t b[] = {1000, -1000};
                int16x2_t c = _qadd16(v_load_int16x2(a), v_load_int16x2(b));
                CHECK(c.v, PK(2000, -2000), 1007);
            }
            {
                GLOBAL_SYMBOL("op_qadd16_sat_max");
                // lane 0: 32767 + 10 = 32767 (clamp max)
                int16_t a[] = {32767, 0};
                int16_t b[] = {10, 0};
                int16x2_t c = _qadd16(v_load_int16x2(a), v_load_int16x2(b));
                CHECK(c.v, PK(32767, 0), 1008);
            }
            {
                GLOBAL_SYMBOL("op_qadd16_sat_min");
                // lane 0: -32768 + (-10) = -32768 (clamp min)
                int16_t a[] = {-32768, 0};
                int16_t b[] = {-10, 0};
                int16x2_t c = _qadd16(v_load_int16x2(a), v_load_int16x2(b));
                CHECK(c.v, PK((int16_t)0x8000, 0), 1009);
            }
        }

        { // qadd16u (saturating unsigned)
            {
                GLOBAL_SYMBOL("op_qadd16u");
                uint16_t a[] = {1000, 50000};
                uint16_t b[] = {2000, 10000};
                uint16x2_t c = _qadd16u(v_load_uint16x2(a), v_load_uint16x2(b));
                CHECK(c.v, PK(3000, 60000), 1013);
            }
            {
                GLOBAL_SYMBOL("op_qadd16u_sat_max_boundary");
                // lane 0: 65535 + 1 = 65535 (clamp max)
                uint16_t a[] = {65535, 0};
                uint16_t b[] = {1, 0};
                uint16x2_t c = _qadd16u(v_load_uint16x2(a), v_load_uint16x2(b));
                CHECK(c.v, PK(65535, 0), 1014);
            }
            {
                GLOBAL_SYMBOL("op_qadd16u_sat_max_heavy");
                // lane 0: 40000 + 40000 = 65535 (clamp max)
                uint16_t a[] = {40000, 0};
                uint16_t b[] = {40000, 0};
                uint16x2_t c = _qadd16u(v_load_uint16x2(a), v_load_uint16x2(b));
                CHECK(c.v, PK(65535, 0), 1015);
            }
        }

        { // qadd8 (saturating signed)
            {
                GLOBAL_SYMBOL("op_qadd8");
                int8_t a[] = {50, -50, 0, 0};
                int8_t b[] = {50, -50, 0, 0};
                int8x4_t c = _qadd8(v_load_int8x4(a), v_load_int8x4(b));
                CHECK(c.v, PK2(100, -100, 0, 0), 1010);
            }
            {
                GLOBAL_SYMBOL("op_qadd8_sat_max");
                // lane 0: 127 + 5 = 127 (clamp max)
                int8_t a[] = {127, 0, 0, 0};
                int8_t b[] = {5, 0, 0, 0};
                int8x4_t c = _qadd8(v_load_int8x4(a), v_load_int8x4(b));
                CHECK(c.v, PK2(127, 0, 0, 0), 1011);
            }
            {
                GLOBAL_SYMBOL("op_qadd8_sat_min");
                // lane 0: -128 + (-5) = -128 (clamp min)
                int8_t a[] = {-128, 0, 0, 0};
                int8_t b[] = {-5, 0, 0, 0};
                int8x4_t c = _qadd8(v_load_int8x4(a), v_load_int8x4(b));
                CHECK(c.v, PK2((int8_t)0x80, 0, 0, 0), 1012);
            }
        }

        { // qadd8u (saturating unsigned)
            {
                GLOBAL_SYMBOL("op_qadd8u");
                uint8_t a[] = {10, 200, 50, 0};
                uint8_t b[] = {10, 10, 50, 0};
                uint8x4_t c = _qadd8u(v_load_uint8x4(a), v_load_uint8x4(b));
                CHECK(c.v, PK2(20, 210, 100, 0), 1016);
            }
            {
                GLOBAL_SYMBOL("op_qadd8u_sat_max_boundary");
                // lane 0: 255 + 1 = 255 (clamp max)
                uint8_t a[] = {255, 0, 0, 0};
                uint8_t b[] = {1, 0, 0, 0};
                uint8x4_t c = _qadd8u(v_load_uint8x4(a), v_load_uint8x4(b));
                CHECK(c.v, PK2(255, 0, 0, 0), 1017);
            }
            {
                GLOBAL_SYMBOL("op_qadd8u_sat_max_heavy");
                // lane 0: 150 + 150 = 255 (clamp max)
                uint8_t a[] = {150, 0, 0, 0};
                uint8_t b[] = {150, 0, 0, 0};
                uint8x4_t c = _qadd8u(v_load_uint8x4(a), v_load_uint8x4(b));
                CHECK(c.v, PK2(255, 0, 0, 0), 1018);
            }
        }

        { // qsub16 (saturating signed)
            {
                GLOBAL_SYMBOL("op_qsub16");
                int16_t a[] = {1000, -1000};
                int16_t b[] = {1, -1};
                int16x2_t c = _qsub16(v_load_int16x2(a), v_load_int16x2(b));
                CHECK(c.v, PK(999, -999), 2007);
            }
            {
                GLOBAL_SYMBOL("op_qsub16_sat_max");
                // lane 0: 32767 - (-10) = 32777 -> clamps to 32767
                int16_t a[] = {32767, 0};
                int16_t b[] = {-10, 0};
                int16x2_t c = _qsub16(v_load_int16x2(a), v_load_int16x2(b));
                CHECK(c.v, PK(32767, 0), 2008);
            }
            {
                GLOBAL_SYMBOL("op_qsub16_sat_min");
                // lane 0: -32768 - 10 = -32778 -> clamps to -32768
                int16_t a[] = {-32768, 0};
                int16_t b[] = {10, 0};
                int16x2_t c = _qsub16(v_load_int16x2(a), v_load_int16x2(b));
                CHECK(c.v, PK((int16_t)0x8000, 0), 2009);
            }
        }

        { // qsub16u (saturating unsigned)
            {
                GLOBAL_SYMBOL("op_qsub16u");
                uint16_t a[] = {5000, 100};
                uint16_t b[] = {2000, 100};
                uint16x2_t c = _qsub16u(v_load_uint16x2(a), v_load_uint16x2(b));
                CHECK(c.v, PK(3000, 0), 2013);
            }
            {
                GLOBAL_SYMBOL("op_qsub16u_sat_floor_boundary");
                // lane 0: 10 - 11 = -1 -> clamps to 0
                uint16_t a[] = {10, 0};
                uint16_t b[] = {11, 0};
                uint16x2_t c = _qsub16u(v_load_uint16x2(a), v_load_uint16x2(b));
                CHECK(c.v, PK(0, 0), 2014);
            }
            {
                GLOBAL_SYMBOL("op_qsub16u_sat_floor_heavy");
                // lane 0: 0 - 65535 = very negative -> clamps to 0
                uint16_t a[] = {0, 0};
                uint16_t b[] = {65535, 0};
                uint16x2_t c = _qsub16u(v_load_uint16x2(a), v_load_uint16x2(b));
                CHECK(c.v, PK(0, 0), 2015);
            }
        }

        { // qsub8 (saturating signed)
            {
                GLOBAL_SYMBOL("op_qsub8");
                int8_t a[] = {100, -100, 0, 0};
                int8_t b[] = {50, -20, 0, 0};
                int8x4_t c = _qsub8(v_load_int8x4(a), v_load_int8x4(b));
                CHECK(c.v, PK2(50, -80, 0, 0), 2010);
            }
            {
                GLOBAL_SYMBOL("op_qsub8_sat_max");
                // lane 0: 127 - (-5) = 132 -> clamps to 127
                int8_t a[] = {127, 0, 0, 0};
                int8_t b[] = {-5, 0, 0, 0};
                int8x4_t c = _qsub8(v_load_int8x4(a), v_load_int8x4(b));
                CHECK(c.v, PK2(127, 0, 0, 0), 2011);
            }
            {
                GLOBAL_SYMBOL("op_qsub8_sat_min");
                // lane 0: -128 - 5 = -133 -> clamps to -128
                int8_t a[] = {-128, 0, 0, 0};
                int8_t b[] = {5, 0, 0, 0};
                int8x4_t c = _qsub8(v_load_int8x4(a), v_load_int8x4(b));
                CHECK(c.v, PK2((int8_t)0x80, 0, 0, 0), 2012);
            }
        }

        { // qsub8u (saturating unsigned)
            {
                GLOBAL_SYMBOL("op_qsub8u");
                uint8_t a[] = {255, 10, 5, 50};
                uint8_t b[] = {5, 5, 5, 0};
                uint8x4_t c = _qsub8u(v_load_uint8x4(a), v_load_uint8x4(b));
                CHECK(c.v, PK2(250, 5, 0, 50), 2016);
            }
            {
                GLOBAL_SYMBOL("op_qsub8u_sat_floor_boundary");
                // lane 0: 5 - 6 = -1 -> clamps to 0
                uint8_t a[] = {5, 0, 0, 0};
                uint8_t b[] = {6, 0, 0, 0};
                uint8x4_t c = _qsub8u(v_load_uint8x4(a), v_load_uint8x4(b));
                CHECK(c.v, PK2(0, 0, 0, 0), 2017);
            }
            {
                GLOBAL_SYMBOL("op_qsub8u_sat_floor_heavy");
                // lane 0: 0 - 255 = -255 -> clamps to 0
                uint8_t a[] = {0, 0, 0, 0};
                uint8_t b[] = {255, 0, 0, 0};
                uint8x4_t c = _qsub8u(v_load_uint8x4(a), v_load_uint8x4(b));
                CHECK(c.v, PK2(0, 0, 0, 0), 2018);
            }
        }

    }
    pass();
}
