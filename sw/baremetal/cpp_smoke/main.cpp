#include <array>
#include "common.h"

enum class op : uint32_t {
    _add,
    _xor,
};

template<uint32_t seed>
class accumulator {
public:
    constexpr explicit accumulator(uint32_t initial)
        : value(initial ^ seed) {}

    constexpr uint32_t apply(uint32_t x, op op) const {
        switch (op) {
            case op::_add: return (value + x);
            case op::_xor: return (value ^ x);
        }
        return value;
    }
private:
    uint32_t value;
};

namespace res {
    constexpr uint32_t expected = 305419914;
}

template <typename T, size_t N>
constexpr T sum(const std::array<T, N>& a) {
    T acc = 0;
    for (auto v : a) acc += v;
    return acc;
}

int main() {
    constexpr std::array<int, 4> vals = {1, 2, 3, 4};
    static_assert(sum(vals) == 10);

    constexpr accumulator<0x12345678u> acc{0x10u}; // 305419880
    volatile uint32_t in = 0x22u;
    volatile uint32_t result = acc.apply(in, op::_add);

    if (res::expected != result) {
        write_mismatch(result, res::expected, 1);
        fail();
    }

    pass();
    return 0;
}
