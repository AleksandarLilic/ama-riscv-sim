#include "defines.h"

// handle the "sub-byte" casting logic generically
// 'if' optimized away since 'vbits' and 'vsigned' are compile-time constants
template <int vbits, bool vsigned>
inline int32_t extract_val(uint32_t val) {
    // handle standard types (native casting)
    if constexpr (vbits == 8) {
        return vsigned ? static_cast<int8_t>(val) : static_cast<uint8_t>(val);
    } else if constexpr (vbits == 16) {
        return vsigned ? static_cast<int16_t>(val) : static_cast<uint16_t>(val);
    } else {
        // handle sub-byte types (manual bit-twiddling)
        constexpr uint32_t mask = ((1U << vbits) - 1);
        uint32_t masked = (val & mask);
        if (vsigned) {
            constexpr uint32_t sign_bit = (1U << (vbits - 1)); // generic s ext
            // if sign bit is set, OR with all upper bits set (using ~mask)
            if (masked & sign_bit) return static_cast<uint32_t>(masked | ~mask);
            return static_cast<uint32_t>(masked);
        } else {
            return static_cast<uint32_t>(masked); // unsigned is simple masking
        }
    }
}
