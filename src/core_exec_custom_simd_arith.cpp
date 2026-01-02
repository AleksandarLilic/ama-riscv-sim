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
    dasm.simd_c << TO_I32(words[0]) << ", " << TO_I32(words[1]);
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
    dasm.simd_c << TO_U32(words[0]) << ", " << TO_U32(words[1]);
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

uint32_t core::alu_c_dot16(uint32_t a, uint32_t b, uint32_t c) {
    // multiply 2 halfword chunks and sum the results
    constexpr size_t e = 2;
    int32_t res = 0;
    #ifdef DASM_EN
    simd_ss_init("[ ", "[ ");
    #endif

    for (size_t i = 0; i < e; i++) {
        #ifdef DASM_EN
        simd_ss_append(TO_I32(TO_I16(a & 0xffff)), TO_I32(TO_I16(b & 0xffff)));
        #endif
        res += TO_I32(TO_I16(a & 0xffff)) * TO_I32(TO_I16(b & 0xffff));
        a >>= 16;
        b >>= 16;
    }
    res += c;

    #ifdef DASM_EN
    simd_ss_finish("]", "]", res, c);
    #endif
    return res;
}

uint32_t core::alu_c_dot8(uint32_t a, uint32_t b, uint32_t c) {
    // multiply 4 byte chunks and sum the results
    constexpr size_t e = 4;
    int32_t res = 0;
    #ifdef DASM_EN
    simd_ss_init("[ ", "[ ");
    #endif

    for (size_t i = 0; i < e; i++) {
        #ifdef DASM_EN
        simd_ss_append(TO_I32(TO_I8(a & 0xff)), TO_I32(TO_I8(b & 0xff)));
        #endif
        res += TO_I32(TO_I8(a & 0xff)) * TO_I32(TO_I8(b & 0xff));
        a >>= 8;
        b >>= 8;
    }
    res += c;

    #ifdef DASM_EN
    simd_ss_finish("]", "]", res, c);
    #endif
    return res;
}

uint32_t core::alu_c_dot4(uint32_t a, uint32_t b, uint32_t c) {
    // multiply 8 nibble chunks and sum the results
    constexpr size_t e = 8;
    int32_t res = 0;
    #ifdef DASM_EN
    simd_ss_init("[ ", "[ ");
    #endif

    for (size_t i = 0; i < e; i++) {
        #ifdef DASM_EN
        simd_ss_append(TO_I32(TO_I4(a & 0xf)), TO_I32(TO_I4(b & 0xf)));
        #endif
        res += TO_I32(TO_I4(a & 0xf)) * TO_I32(TO_I4(b & 0xf));
        a >>= 4;
        b >>= 4;
    }
    res += c;

    #ifdef DASM_EN
    simd_ss_finish("]", "]", res, c);
    #endif
    return res;
}
