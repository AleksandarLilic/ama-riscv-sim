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

template <typename T, size_t N>
constexpr T sum(const std::array<T, N>& a) {
    T acc = 0;
    for (auto v : a) acc += v;
    return acc;
}

template <bool B>
constexpr uint32_t branch_select(uint32_t val) {
    if constexpr (B) {
        return (val + 1);
    } else {
        static_assert(B, "dead branch instantiated");
        return 0;
    }
}

struct tda {
    tda_cnt_t counters = {}; // optionally private if not accessed from outside
    tda()  { init_tda_counters(); }
    ~tda() { save_tda_counters(&counters); print_tda_counters_json(&counters); }
};

namespace res {
    // acc(305419914) + lambda(+123) + branch_select(+1)
    constexpr uint32_t expected = 305420038;
}

int main() {
    {
        tda t;
        constexpr std::array<int, 4> vals = {1, 2, 3, 4};
        static_assert(sum(vals) == 10);

        constexpr accumulator<0x12345678u> acc{0x10u}; // 305419880
        volatile uint32_t in = 0x22u;
        volatile uint32_t result = acc.apply(in, op::_add); // 305419914

        const uint32_t delta = 123u;
        auto inc = [result](uint32_t x) { return x + result; };
        result = inc(delta); // +123 = 305420037

        result = branch_select<true>(result); // +1 = 305420038

        if (res::expected != result) {
            write_mismatch(result, res::expected, 1);
            fail();
        }
    }

    pass();
    return 0;
}
