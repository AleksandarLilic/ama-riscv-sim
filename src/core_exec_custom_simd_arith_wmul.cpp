#include "defines.h"
#include "core.h"

reg_pair core::alu_c_wmul16(uint32_t a, uint32_t b) {
    // multiply 2 halfword chunks into 2 32-bit results
    int32_t words[2];
    #ifdef DASM_EN
    simd_ss_init("[ ", "[ ", "[ ");
    #endif

    for (auto &word : words) {
        word = TO_I32(TO_I16(a & 0xffff)) * TO_I32(TO_I16(b & 0xffff));
        #ifdef DASM_EN
        simd_ss_append(TO_I32(TO_I16(a & 0xffff)), TO_I32(TO_I16(b & 0xffff)));
        #endif
        a >>= 16;
        b >>= 16;
    }

    #ifdef DASM_EN
    dasm.simd_c << TO_I32(words[0]) << " ], [ " << TO_I32(words[1]);
    simd_ss_finish(" ]", "]", "]");
    #endif

    return {TO_U32(words[0]), TO_U32(words[1])};
}

reg_pair core::alu_c_wmul16u(uint32_t a, uint32_t b) {
    // multiply 2 halfword chunks into 2 32-bit results
    uint32_t words[2];
    #ifdef DASM_EN
    simd_ss_init("[ ", "[ ", "[ ");
    #endif

    for (auto &word : words) {
        word = TO_U32(TO_U16(a & 0xffff)) * TO_U32(TO_U16(b & 0xffff));
        #ifdef DASM_EN
        simd_ss_append(TO_U32(TO_U16(a & 0xffff)), TO_U32(TO_U16(b & 0xffff)));
        #endif
        a >>= 16;
        b >>= 16;
    }

    #ifdef DASM_EN
    dasm.simd_c << TO_U32(words[0]) << " ], [ " << TO_U32(words[1]);
    simd_ss_finish(" ]", "]", "]");
    #endif

    return {words[0], words[1]};
}

reg_pair core::alu_c_wmul8(uint32_t a, uint32_t b) {
    // multiply 4 byte chunks into 2 32-bit results
    int16_t halves[4];
    #ifdef DASM_EN
    simd_ss_init("", "[ ", "[ ");
    #endif

    for (auto &half : halves) {
        half = TO_I16(TO_I8(a & 0xff)) * TO_I16(TO_I8(b & 0xff));
        #ifdef DASM_EN
        simd_ss_append(TO_I16(TO_I8(a & 0xff)), TO_I16(TO_I8(b & 0xff)));
        #endif
        a >>= 8;
        b >>= 8;
    }

    #ifdef DASM_EN
    dasm.simd_c << "[ " << TO_I32(TO_I16(halves[0]) & 0xFFFF) << " "
                << TO_I32(TO_I16(halves[1]) & 0xFFFF) << " ], "
                << "[ " << TO_I32(TO_I16(halves[2]) & 0xFFFF) << " "
                << TO_I32(TO_I16(halves[3]) & 0xFFFF) << " ]";
    simd_ss_finish("", "]", "]");
    #endif

    int32_t words[2] = {0, 0};
    words[0] = (TO_I32(halves[0]) & 0xFFFF) |
               ((TO_I32(halves[1]) & 0xFFFF) << 16);
    words[1] = (TO_I32(halves[2]) & 0xFFFF) |
               ((TO_I32(halves[3]) & 0xFFFF) << 16);
    return {TO_U32(words[0]), TO_U32(words[1])};
}

reg_pair core::alu_c_wmul8u(uint32_t a, uint32_t b) {
    // multiply 4 byte chunks into 2 32-bit results
    uint16_t halves[4];
    #ifdef DASM_EN
    simd_ss_init("", "[ ", "[ ");
    #endif

    for (auto &half : halves) {
        half = TO_U16(TO_U8(a & 0xff)) * TO_U16(TO_U8(b & 0xff));
        #ifdef DASM_EN
        simd_ss_append(TO_U16(TO_U8(a & 0xff)), TO_U16(TO_U8(b & 0xff)));
        #endif
        a >>= 8;
        b >>= 8;
    }

    #ifdef DASM_EN
    dasm.simd_c << "[ " << TO_U32(TO_U16(halves[0]) & 0xFFFF) << " "
                << TO_U32(TO_U16(halves[1]) & 0xFFFF) << " ], "
                << "[ " << TO_U32(TO_U16(halves[2]) & 0xFFFF) << " "
                << TO_U32(TO_U16(halves[3]) & 0xFFFF) << " ]";
    simd_ss_finish("", "]", "]");
    #endif

    uint32_t words[2] = {0, 0};
    words[0] = (TO_U32(halves[0]) & 0xFFFF) |
               ((TO_U32(halves[1]) & 0xFFFF) << 16);
    words[1] = (TO_U32(halves[2]) & 0xFFFF) |
               ((TO_U32(halves[3]) & 0xFFFF) << 16);
    return {words[0], words[1]};
}
