#ifndef C_TEST_COMMON_H
#define C_TEST_COMMON_H

// result, expected, index
#define CHECK(r, e, i) \
    if ((r) != (e)){ write_mismatch((r), (e), (i)); fail(); }

// packing macro helpers
// 32-bit
#define PK_I16(lo, hi) \
    (int32_t)( (((hi) & 0xFFFF) << 16) | ((lo) & 0xFFFF) )

#define PK_U16(lo, hi) \
    (uint32_t)( (((hi) & 0xFFFF) << 16) | ((lo) & 0xFFFF) )

#define PK_I8(l0, l1, l2, l3) \
    (int32_t)( \
        (((l3) & 0xFF) << 24) | (((l2) & 0xFF) << 16) | \
        (((l1) & 0xFF) << 8) | ((l0) & 0xFF) \
    )

#define PK_U8(l0, l1, l2, l3) \
    (uint32_t)( \
        (((l3) & 0xFF) << 24) | (((l2) & 0xFF) << 16) | \
        (((l1) & 0xFF) << 8) | ((l0) & 0xFF) \
    )

// sub-byte
#define PK_SB_I4(lo, hi) \
    (int8_t)( (((hi) & 0xF) << 4) | ((lo) & 0xF) )

#define PK_SB_U4(lo, hi) \
    (uint8_t)( (((hi) & 0xF) << 4) | ((lo) & 0xF) )

#define PK_SB_I2(l3, l2, l1, l0) \
    (int8_t)( (((l3)&3)<<6) | (((l2)&3)<<4) | (((l1)&3)<<2) | ((l0)&3) )

#define PK_SB_U2(l3, l2, l1, l0) \
    (uint8_t)( (((l3)&3)<<6) | (((l2)&3)<<4) | (((l1)&3)<<2) | ((l0)&3) )

#endif
