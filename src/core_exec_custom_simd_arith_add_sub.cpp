#include "defines.h"
#include "core.h"

uint32_t core::alu_c_add16(uint32_t a, uint32_t b) {
    // parallel add 2 halfword chunks
    constexpr size_t e = 2;
    int32_t res = 0;
    #ifdef DASM_EN
    simd_ss_init("[ ", "[ ", "[ ");
    #endif

    for (size_t i = 0; i < e; i++) {
        int32_t sum = TO_I32(TO_I16(a & 0xffff)) + TO_I32(TO_I16(b & 0xffff));
        #ifdef DASM_EN
        simd_ss_append(
            TO_I32(TO_I16(sum & 0xFFFF)),
            TO_I32(TO_I16(a & 0xffff)),
            TO_I32(TO_I16(b & 0xffff))
        );
        #endif
        a >>= 16;
        b >>= 16;
        res |= (sum & 0xFFFF) << (i * 16);
    }

    #ifdef DASM_EN
    simd_ss_finish("]", "]", "]");
    #endif

    return res;
}

uint32_t core::alu_c_add8(uint32_t a, uint32_t b) {
    // parallel add 4 byte chunks
    constexpr size_t e = 4;
    int32_t res = 0;
    #ifdef DASM_EN
    simd_ss_init("[ ", "[ ", "[ ");
    #endif

    for (size_t i = 0; i < e; i++) {
        int32_t sum = TO_I32(TO_I8(a & 0xff)) + TO_I32(TO_I8(b & 0xff));
        #ifdef DASM_EN
        simd_ss_append(
            TO_I32(TO_I8(sum & 0xff)),
            TO_I32(TO_I8(a & 0xff)),
            TO_I32(TO_I8(b & 0xff))
        );
        #endif
        a >>= 8;
        b >>= 8;
        res |= (sum & 0xFF) << (i * 8);
    }

    #ifdef DASM_EN
    simd_ss_finish("]", "]", "]");
    #endif

    return res;
}

uint32_t core::alu_c_sub16(uint32_t a, uint32_t b) {
    // parallel sub 2 halfword chunks
    constexpr size_t e = 2;
    int32_t res = 0;
    #ifdef DASM_EN
    simd_ss_init("[ ", "[ ", "[ ");
    #endif

    for (size_t i = 0; i < e; i++) {
        int32_t sum = TO_I32(TO_I16(a & 0xffff)) - TO_I32(TO_I16(b & 0xffff));
        #ifdef DASM_EN
        simd_ss_append(
            TO_I32(TO_I16(sum & 0xFFFF)),
            TO_I32(TO_I16(a & 0xffff)),
            TO_I32(TO_I16(b & 0xffff))
        );
        #endif
        a >>= 16;
        b >>= 16;
        res |= (sum & 0xFFFF) << (i * 16);
    }

    #ifdef DASM_EN
    simd_ss_finish("]", "]", "]");
    #endif

    return res;
}

uint32_t core::alu_c_sub8(uint32_t a, uint32_t b) {
    // parallel sub 4 byte chunks
    constexpr size_t e = 4;
    int32_t res = 0;
    #ifdef DASM_EN
    simd_ss_init("[ ", "[ ", "[ ");
    #endif

    for (size_t i = 0; i < e; i++) {
        int32_t sum = TO_I32(TO_I8(a & 0xff)) - TO_I32(TO_I8(b & 0xff));
        #ifdef DASM_EN
        simd_ss_append(
            TO_I32(TO_I8(sum & 0xff)),
            TO_I32(TO_I8(a & 0xff)),
            TO_I32(TO_I8(b & 0xff))
        );
        #endif
        a >>= 8;
        b >>= 8;
        res |= (sum & 0xFF) << (i * 8);
    }

    #ifdef DASM_EN
    simd_ss_finish("]", "]", "]");
    #endif

    return res;
}
