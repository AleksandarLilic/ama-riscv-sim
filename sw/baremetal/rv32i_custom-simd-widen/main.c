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
        // using only widening to stay within the same group of instructions
        // but getting vector element into scalar register should be done with
        // 'vector extract' (vext*) instructions

        { // int16
            int16_t in[] = {-15, 19};
            const int16x2_t b = v_load_int16x2(in);
            int32x2_t result = _widen16(b);
            CHECK(result.w.lo, in[0], 101);
            CHECK(result.w.hi, in[1], 102);
        }

        { // uint16
            uint16_t in[] = {1, 16153};
            uint32x2_t result = _widen16u(v_load_uint16x2(in));
            CHECK(result.w.lo, in[0], 111);
            CHECK(result.w.hi, in[1], 112);
        }

        { // int8
            int8_t in[] = {-5, 99, 127, -33};
            int16x4_t result = _widen8(v_load_int8x4(in));
            int32x2_t result_32;
            result_32 = _widen16(result.w.lo);
            CHECK(result_32.w.lo, in[0], 201);
            CHECK(result_32.w.hi, in[1], 202);
            result_32 = _widen16(result.w.hi);
            CHECK(result_32.w.lo, in[2], 203);
            CHECK(result_32.w.hi, in[3], 204);
        }

        { // uint8
            uint8_t in[] = {188, 4, 127, 255};
            uint16x4_t result = _widen8u(v_load_uint8x4(in));
            uint32x2_t result_32;
            result_32 = _widen16u(result.w.lo);
            CHECK(result_32.w.lo, in[0], 211);
            CHECK(result_32.w.hi, in[1], 212);
            result_32 = _widen16u(result.w.hi);
            CHECK(result_32.w.lo, in[2], 213);
            CHECK(result_32.w.hi, in[3], 214);
        }

        { // int4
            int8_t in[] = {7, 2, -2, -110}; // (7, 0), (2, 0), (-2, -1), (2, -7)
            int8x8_t result = _widen4(v_load_int4x8(in));
            int16x4_t result_16;
            int32x2_t result_32;

            result_16 = _widen8(result.w.lo);
            result_32 = _widen16(result_16.w.lo);
            CHECK(result_32.w.lo, 7, 301);
            CHECK(result_32.w.hi, 0, 302);
            result_32 = _widen16(result_16.w.hi);
            CHECK(result_32.w.lo, 2, 303);
            CHECK(result_32.w.hi, 0, 304);

            result_16 = _widen8(result.w.hi);
            result_32 = _widen16(result_16.w.lo);
            CHECK(result_32.w.lo, -2, 305);
            CHECK(result_32.w.hi, -1, 306);
            result_32 = _widen16(result_16.w.hi);
            CHECK(result_32.w.lo, 2, 307);
            CHECK(result_32.w.hi, -7, 308);
        }

        { // uint4
            uint8_t in[] = {116, 1, 8, 14}; // (4, 7), (1, 0), (8, 0), (14, 0)
            uint8x8_t result = _widen4u(v_load_uint4x8(in));
            uint16x4_t result_16;
            uint32x2_t result_32;

            result_16 = _widen8u(result.w.lo);
            result_32 = _widen16u(result_16.w.lo);
            CHECK(result_32.w.lo, 4, 311);
            CHECK(result_32.w.hi, 7, 312);
            result_32 = _widen16u(result_16.w.hi);
            CHECK(result_32.w.lo, 1, 313);
            CHECK(result_32.w.hi, 0, 314);

            result_16 = _widen8u(result.w.hi);
            result_32 = _widen16u(result_16.w.lo);
            CHECK(result_32.w.lo, 8, 315);
            CHECK(result_32.w.hi, 0, 316);
            result_32 = _widen16u(result_16.w.hi);
            CHECK(result_32.w.lo, 14, 317);
            CHECK(result_32.w.hi, 0, 318);
        }

        { // int2
            // (0, 1, 1, 0), (1, 1, 1, 1), (-2, -1, -1, -1), (0, -2, -1, -2)
            int8_t in[] = {20, 85, -2, -72};
            int4x16_t result = _widen2(v_load_int2x16(in));
            int8x8_t result_8;
            int16x4_t result_16;
            int32x2_t result_32;

            result_8 = _widen4(result.w.lo);
            result_16 = _widen8(result_8.w.lo);
            result_32 = _widen16(result_16.w.lo);
            CHECK(result_32.w.lo, 0, 401);
            CHECK(result_32.w.hi, 1, 402);
            result_32 = _widen16(result_16.w.hi);
            CHECK(result_32.w.lo, 1, 403);
            CHECK(result_32.w.hi, 0, 404);

            result_16 = _widen8(result_8.w.hi);
            result_32 = _widen16(result_16.w.lo);
            CHECK(result_32.w.lo, 1, 405);
            CHECK(result_32.w.hi, 1, 406);
            result_32 = _widen16(result_16.w.hi);
            CHECK(result_32.w.lo, 1, 407);
            CHECK(result_32.w.hi, 1, 408);

            result_8 = _widen4(result.w.hi);
            result_16 = _widen8(result_8.w.lo);
            result_32 = _widen16(result_16.w.lo);
            CHECK(result_32.w.lo, -2, 409);
            CHECK(result_32.w.hi, -1, 410);
            result_32 = _widen16(result_16.w.hi);
            CHECK(result_32.w.lo, -1, 411);
            CHECK(result_32.w.hi, -1, 412);

            result_16 = _widen8(result_8.w.hi);
            result_32 = _widen16(result_16.w.lo);
            CHECK(result_32.w.lo, 0, 413);
            CHECK(result_32.w.hi, -2, 414);
            result_32 = _widen16(result_16.w.hi);
            CHECK(result_32.w.lo, -1, 415);
            CHECK(result_32.w.hi, -2, 416);

        }

        { // uint2
            // (0, 1, 0, 1), (1, 1, 1, 1), (1, 3, 2, 1), (0, 2, 3, 2)
            uint8_t in[] = {68, 85, 109, 184};
            uint4x16_t result = _widen2u(v_load_uint2x16(in));
            uint8x8_t result_8;
            uint16x4_t result_16;
            uint32x2_t result_32;

            result_8 = _widen4u(result.w.lo);
            result_16 = _widen8u(result_8.w.lo);
            result_32 = _widen16u(result_16.w.lo);
            CHECK(result_32.w.lo, 0, 421);
            CHECK(result_32.w.hi, 1, 422);
            result_32 = _widen16u(result_16.w.hi);
            CHECK(result_32.w.lo, 0, 423);
            CHECK(result_32.w.hi, 1, 424);

            result_16 = _widen8u(result_8.w.hi);
            result_32 = _widen16u(result_16.w.lo);
            CHECK(result_32.w.lo, 1, 425);
            CHECK(result_32.w.hi, 1, 426);
            result_32 = _widen16u(result_16.w.hi);
            CHECK(result_32.w.lo, 1, 427);
            CHECK(result_32.w.hi, 1, 428);

            result_8 = _widen4u(result.w.hi);
            result_16 = _widen8u(result_8.w.lo);
            result_32 = _widen16u(result_16.w.lo);
            CHECK(result_32.w.lo, 1, 429);
            CHECK(result_32.w.hi, 3, 430);
            result_32 = _widen16u(result_16.w.hi);
            CHECK(result_32.w.lo, 2, 431);
            CHECK(result_32.w.hi, 1, 432);

            result_16 = _widen8u(result_8.w.hi);
            result_32 = _widen16u(result_16.w.lo);
            CHECK(result_32.w.lo, 0, 433);
            CHECK(result_32.w.hi, 2, 434);
            result_32 = _widen16u(result_16.w.hi);
            CHECK(result_32.w.lo, 3, 435);
            CHECK(result_32.w.hi, 2, 436);
        }

    }
    pass();
}
