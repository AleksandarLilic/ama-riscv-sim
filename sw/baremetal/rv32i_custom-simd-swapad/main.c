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

        { // swapad16: rd = [rs1.H[0], rs2.H[0]], rdp = [rs1.H[1], rs2.H[1]]
            {
                GLOBAL_SYMBOL("op_swapad16");
                uint16_t a[] = {0x2222, 0x1111};
                uint16_t b[] = {0x4444, 0x3333};
                uint16x4_t c = _swapad16(v_load_uint16x2(a), v_load_uint16x2(b));
                CHECK(c.w.lo.v, 0x44442222U, 101);
                CHECK(c.w.hi.v, 0x33331111U, 102);
            }
            {
                GLOBAL_SYMBOL("op_swapad16_zeros_ones");
                uint16_t a[] = {0xFFFF, 0x0000};
                uint16_t b[] = {0x0000, 0xFFFF};
                uint16x4_t c = _swapad16(v_load_uint16x2(a), v_load_uint16x2(b));
                CHECK(c.w.lo.v, 0x0000FFFFU, 103);
                CHECK(c.w.hi.v, 0xFFFF0000U, 104);
            }
            {
                GLOBAL_SYMBOL("op_swapad16_alternating");
                uint16_t a[] = {0xBBBB, 0xAAAA};
                uint16_t b[] = {0xDDDD, 0xCCCC};
                uint16x4_t c = _swapad16(v_load_uint16x2(a), v_load_uint16x2(b));
                CHECK(c.w.lo.v, 0xDDDDBBBBU, 105);
                CHECK(c.w.hi.v, 0xCCCCAAAAU, 106);
            }
        }

        { // swapad8: rd = [rs1.B[0], rs2.B[0], rs1.B[2], rs2.B[2]]
            {
                GLOBAL_SYMBOL("op_swapad8");
                uint8_t a[] = {0x04, 0x03, 0x02, 0x01};
                uint8_t b[] = {0x08, 0x07, 0x06, 0x05};
                uint8x8_t c = _swapad8(v_load_uint8x4(a), v_load_uint8x4(b));
                CHECK(c.w.lo.v, 0x06020804U, 201);
                CHECK(c.w.hi.v, 0x05010703U, 202);
            }
            {
                GLOBAL_SYMBOL("op_swapad8_edge");
                uint8_t a[] = {0x00, 0xFF, 0x00, 0xFF};
                uint8_t b[] = {0xFF, 0x00, 0xFF, 0x00};
                uint8x8_t c = _swapad8(v_load_uint8x4(a), v_load_uint8x4(b));
                CHECK(c.w.lo.v, 0xFF00FF00U, 203);
                CHECK(c.w.hi.v, 0x00FF00FFU, 204);
            }
        }

        { // swapad4: even nibbles -> rd, odd -> rdp
            {
                GLOBAL_SYMBOL("op_swapad4");
                // lane mapping for 0x10: lo=0, hi=1.
                // lane mapping for 0x54: lo=4, hi=5.
                // rd (evens): 0, 4, 2, 6... -> bytes 0x40, 0x62...
                uint8_t a[] = {0x10, 0x32, 0x10, 0x32};
                uint8_t b[] = {0x54, 0x76, 0x54, 0x76};
                uint4x16_t c = _swapad4(v_load_uint4x8(a), v_load_uint4x8(b));
                CHECK(c.w.lo.v, 0x62406240U, 301);
                CHECK(c.w.hi.v, 0x73517351U, 302);
            }
            {
                GLOBAL_SYMBOL("op_swapad4_edge");
                uint8_t a[] = {0x00, 0x00, 0x00, 0x00};
                uint8_t b[] = {0xFF, 0xFF, 0xFF, 0xFF};
                uint4x16_t c = _swapad4(v_load_uint4x8(a), v_load_uint4x8(b));
                CHECK(c.w.lo.v, 0xF0F0F0F0U, 303);
                CHECK(c.w.hi.v, 0xF0F0F0F0U, 304);
            }
        }

        { // swapad2: even 2-bit lanes -> rd, odd -> rdp
            {
                GLOBAL_SYMBOL("op_swapad2");
                // check both even and odd lanes have data, verifying rd and rdp
                uint8_t a[] = {0x55, 0x55, 0x55, 0x55}; // lanes: 1, 1, 1, 1...
                uint8_t b[] = {0xAA, 0xAA, 0xAA, 0xAA}; // lanes: 2, 2, 2, 2...
                uint2x32_t c = _swapad2(v_load_uint2x16(a), v_load_uint2x16(b));

                // rd: 1, 2, 1, 2... -> 0x99
                // rdp: 1, 2, 1, 2... -> 0x99
                CHECK(c.w.lo.v, 0x99999999U, 401);
                CHECK(c.w.hi.v, 0x99999999U, 402);
            }
            {
                GLOBAL_SYMBOL("op_swapad2_edge");
                // a (rs1) provides the EVEN lanes (0, 2...) -> LSBs -> 0
                // b (rs2) provides the ODD lanes (0, 2...)  -> MSBs -> 3
                uint8_t a[] = {0x00, 0x00, 0x00, 0x00};
                uint8_t b[] = {0xFF, 0xFF, 0xFF, 0xFF};
                uint2x32_t c = _swapad2(v_load_uint2x16(a), v_load_uint2x16(b));

                // binary per nibble: 11 (from b) | 00 (from a) -> 1100 -> 0xc
                CHECK(c.w.lo.v, 0xCCCCCCCCU, 403);
                CHECK(c.w.hi.v, 0xCCCCCCCCU, 404);
            }
        }
    }
    pass();
}
