#include "defines.h"
#include "types.h"

template <size_t vbits>
struct lane {
    static constexpr size_t count = (32 / vbits);
    static constexpr uint32_t mask = ((1U << vbits) - 1);
};

template <size_t vbits, bool vsigned>
constexpr int32_t get_limit(bool max) {
    if constexpr (vsigned) {
        return max ? ((1 << (vbits - 1)) - 1) : -(1 << (vbits - 1));
    } else {
        return max ? ((1 << vbits) - 1) : 0;
    }
}

template <size_t out_bits, size_t n>
inline reg_pair pack_wide(const int32_t (&results)[n]) {
    constexpr size_t half_n = n / 2;
    constexpr uint32_t mask = static_cast<uint32_t>((1ULL << out_bits) - 1);
    uint32_t words[2] = {0, 0};
    for (size_t i = 0; i < n; i++) {
        words[i / half_n] |= (
            (TO_U32(results[i]) & mask) << ((i % half_n) * out_bits)
        );
    }
    return {words[0], words[1]};
}

// handle the "sub-byte" casting logic generically
// 'if' optimized away since 'vbits' and 'vsigned' are compile-time constants
template <int vbits, bool vsigned>
inline int32_t extract_val(uint32_t val) {
    // handle standard types (native casting)
    if constexpr (vbits == 8) {
        return vsigned ? TO_I8(val) : TO_U8(val);
    } else if constexpr (vbits == 16) {
        return vsigned ? TO_I16(val) : TO_U16(val);
    } else if constexpr (vbits == 32) {
        return vsigned ? TO_I32(val) : TO_U32(val);
    } else {
        // handle sub-byte types (manual bit-twiddling)
        constexpr uint32_t mask = ((1U << vbits) - 1);
        uint32_t masked = (val & mask);
        if (vsigned) {
            constexpr uint32_t sign_bit = (1U << (vbits - 1)); // generic s ext
            // if sign bit is set, OR with all upper bits set (using ~mask)
            if (masked & sign_bit) return TO_U32(masked | ~mask);
            return TO_U32(masked);
        } else {
            return TO_U32(masked); // unsigned is simple masking
        }
    }
}
