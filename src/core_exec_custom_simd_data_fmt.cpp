#include "defines.h"
#include "core.h"

reg_pair core::data_fmt_c_widen16(uint32_t a) {
    // unpack 2 16-bit values to 2 32-bit values
    constexpr size_t e = 2;
    int16_t halves[e];
    #ifdef DASM_EN
    simd_ss_init("[ ");
    #endif

    for (size_t i = 0; i < e; i++) {
        halves[i] = TO_I16(a & 0xffff);
        #ifdef DASM_EN
        simd_ss_append(TO_I32(TO_I16(a & 0xffff)));
        #endif
        a >>= 16;
    }

    #ifdef DASM_EN
    dasm.simd_a << "] ";
    dasm.simd_c << "[ " << TO_I32(halves[0]) << " ], [ "
                << TO_I32(halves[1]) << " ]";
    simd_ss_finish("");
    #endif

    return {TO_U32(halves[0]), TO_U32(halves[1])};
}

reg_pair core::data_fmt_c_widen16u(uint32_t a) {
    // unpack 2 16-bit values to 2 32-bit values unsigned
    constexpr size_t e = 2;
    uint16_t halves[e];
    #ifdef DASM_EN
    simd_ss_init("[ ");
    #endif

    for (size_t i = 0; i < e; i++) {
        halves[i] = TO_U16(a & 0xffff);
        #ifdef DASM_EN
        simd_ss_append(TO_U32(TO_U16(a & 0xffff)));
        #endif
        a >>= 16;
    }

    #ifdef DASM_EN
    dasm.simd_a << "] ";
    dasm.simd_c << "[ " << TO_U32(TO_U16(halves[0])) << " ], [ "
                << TO_U32(TO_U16(halves[1])) << " ]";
    simd_ss_finish("");
    #endif

    return {TO_U32(halves[0]), TO_U32(halves[1])};
}

reg_pair core::data_fmt_c_widen8(uint32_t a) {
    // unpack 4 8-bit values to 4 16-bit values (as 2 32-bit values)
    constexpr size_t e = 4;
    int8_t bytes[e];
    #ifdef DASM_EN
    simd_ss_init("[ ");
    #endif

    for (size_t i = 0; i < e; i++) {
        bytes[i] = TO_I8(a & 0xff);
        #ifdef DASM_EN
        simd_ss_append(TO_I32(TO_I8(a & 0xff)));
        #endif
        a >>= 8;
    }

    #ifdef DASM_EN
    dasm.simd_a << "] ";
    dasm.simd_c << "[ " << TO_I32(TO_I16(bytes[0])) << " "
                << TO_I32(TO_I16(bytes[1])) << " ], "
                << "[ " << TO_I32(TO_I16(bytes[2])) << " "
                << TO_I32(TO_I16(bytes[3])) << " ]";
    simd_ss_finish("");
    #endif

    int32_t words[2];
    words[0] = (TO_I32(TO_I16(bytes[0])) & 0xFFFF) |
               ((TO_I32(TO_I16(bytes[1])) & 0xFFFF) << 16);
    words[1] = (TO_I32(TO_I16(bytes[2])) & 0xFFFF) |
               ((TO_I32(TO_I16(bytes[3])) & 0xFFFF) << 16);
    return {TO_U32(words[0]), TO_U32(words[1])};
}

reg_pair core::data_fmt_c_widen8u(uint32_t a) {
    // unpack 4 8-bit values to 4 16-bit values (as 2 32-bit values) unsigned
    constexpr size_t e = 4;
    uint8_t bytes[e];
    #ifdef DASM_EN
    simd_ss_init("[ ");
    #endif

    for (size_t i = 0; i < e; i++) {
        bytes[i] = TO_U8(a & 0xff);
        #ifdef DASM_EN
        simd_ss_append(TO_U32(TO_U8(a & 0xff)));
        #endif
        a >>= 8;
    }

    #ifdef DASM_EN
    dasm.simd_a << "] ";
    dasm.simd_c << "[ " << TO_U32(TO_U16(bytes[0])) << " "
                << TO_U32(TO_U16(bytes[1])) << " ], "
                << "[ " << TO_U32(TO_U16(bytes[2])) << " "
                << TO_U32(TO_U16(bytes[3])) << " ]";
    simd_ss_finish("");
    #endif

    uint32_t words[2];
    words[0] = (TO_U32(TO_U16(bytes[0])) & 0xFFFF) |
               ((TO_U32(TO_U16(bytes[1])) & 0xFFFF) << 16);
    words[1] = (TO_U32(TO_U16(bytes[2])) & 0xFFFF) |
               ((TO_U32(TO_U16(bytes[3])) & 0xFFFF) << 16);
    return {words[0], words[1]};
}

reg_pair core::data_fmt_c_widen4(uint32_t a) {
    // unpack 8 4-bit values to 8 8-bit values (as 2 32-bit values)
    constexpr size_t e = 8;
    int8_t nibbles[e];
    #ifdef DASM_EN
    simd_ss_init("[ ");
    #endif

    for (size_t i = 0; i < e; i++) {
        nibbles[i] = TO_I4(a & 0xf);
        #ifdef DASM_EN
        simd_ss_append(TO_I32(TO_I4(a & 0xf)));
        #endif
        a >>= 4;
    }

    #ifdef DASM_EN
    dasm.simd_a << "] ";
    dasm.simd_c << "[ ";
    for (size_t i = 0; i < e; i++) {
        dasm.simd_c << TO_I32(TO_I8(nibbles[i])) << " ";
        if (i==3) dasm.simd_c << "], [ ";
        if (i==7) dasm.simd_c << "]";
    }
    simd_ss_finish("");
    #endif

    uint32_t words[2] = {0, 0};
    for (size_t i = 0; i < (e>>1); i++) {
        words[0] |= (TO_I32(TO_I8(nibbles[i])) & 0xFF) << (i * 8);
        words[1] |= (TO_I32(TO_I8(nibbles[i + 4])) & 0xFF) << (i * 8);
    }
    return {TO_U32(words[0]), TO_U32(words[1])};
}

reg_pair core::data_fmt_c_widen4u(uint32_t a) {
    // unpack 8 4-bit values to 8 8-bit values (as 2 32-bit values) unsigned
    constexpr size_t e = 8;
    uint8_t nibbles[e];
    #ifdef DASM_EN
    simd_ss_init("[ ");
    #endif

    for (size_t i = 0; i < e; i++) {
        nibbles[i] = TO_U4(a & 0xf);
        #ifdef DASM_EN
        simd_ss_append(TO_I32(TO_U4(a & 0xf)));
        #endif
        a >>= 4;
    }

    #ifdef DASM_EN
    dasm.simd_a << "] ";
    dasm.simd_c << "[ ";
    for (size_t i = 0; i < e; i++) {
        dasm.simd_c << TO_U32(TO_U8(nibbles[i])) << " ";
        if (i==3) dasm.simd_c << "], [ ";
        if (i==7) dasm.simd_c << "]";
    }
    simd_ss_finish("");
    #endif

    uint32_t words[2] = {0, 0};
    for (size_t i = 0; i < (e>>1); i++) {
        words[0] |= (TO_U32(TO_U8(nibbles[i])) & 0xFF) << (i * 8);
        words[1] |= (TO_U32(TO_U8(nibbles[i + 4])) & 0xFF) << (i * 8);
    }
    return {words[0], words[1]};
}

reg_pair core::data_fmt_c_widen2(uint32_t a) {
    // unpack 16 2-bit values to 16 4-bit values (as 2 32-bit values)
    constexpr size_t e = 16;
    int8_t crumbs[e];
    #ifdef DASM_EN
    simd_ss_init("[ ");
    #endif

    for (size_t i = 0; i < e; i++) {
        crumbs[i] = TO_I2(a & 0x3);
        #ifdef DASM_EN
        simd_ss_append(TO_I32(TO_I2(a & 0x3)));
        #endif
        a >>= 2;
    }

    #ifdef DASM_EN
    dasm.simd_a << "] ";
    dasm.simd_c << "[ ";
    for (size_t i = 0; i < e; i++) {
        dasm.simd_c << TO_I32(TO_I4(crumbs[i])) << " ";
        if (i==7) dasm.simd_c << "], [ ";
        if (i==15) dasm.simd_c << "]";
    }
    simd_ss_finish("");
    #endif

    int32_t words[2] = {0, 0};
    for (size_t i = 0; i < (e>>1); i++) {
        words[0] |= (TO_I32(TO_I8(crumbs[i])) & 0xF) << (i * 4);
        words[1] |= (TO_I32(TO_I8(crumbs[i + 8])) & 0xF) << (i * 4);
    }
    return {TO_U32(words[0]), TO_U32(words[1])};
}

reg_pair core::data_fmt_c_widen2u(uint32_t a) {
    // unpack 16 2-bit values to 16 4-bit values (as 2 32-bit values) unsigned
    constexpr size_t e = 16;
    uint8_t crumbs[e];
    #ifdef DASM_EN
    simd_ss_init("[ ");
    #endif

    for (size_t i = 0; i < e; i++) {
        crumbs[i] = TO_U2(a & 0x3);
        #ifdef DASM_EN
        simd_ss_append(TO_U32(TO_U2(a & 0x3)));
        #endif
        a >>= 2;
    }

    #ifdef DASM_EN
    dasm.simd_a << "] ";
    dasm.simd_c << "[ ";
    for (size_t i = 0; i < e; i++) {
        dasm.simd_c << TO_U32(TO_U4(crumbs[i])) << " ";
        if (i==7) dasm.simd_c << "], [ ";
        if (i==15) dasm.simd_c << "]";
    }
    simd_ss_finish("");
    #endif

    uint32_t words[2] = {0, 0};
    for (size_t i = 0; i < (e>>1); i++) {
        words[0] |= (TO_U32(TO_U8(crumbs[i])) & 0xF) << (i * 4);
        words[1] |= (TO_U32(TO_U8(crumbs[i+8])) & 0xF) << (i * 4);
    }
    return {words[0], words[1]};
}
