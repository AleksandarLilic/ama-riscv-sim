#pragma once

#include "defines.h"
#include "divider_stats.h"

struct div_result_cache_t {
    bool valid = false;
    uint32_t a = 0;
    uint32_t b = 0;
    bool op_uns = false;

    bool hit(uint32_t in_a, uint32_t in_b, bool in_op_uns) const {
        return (valid && (a == in_a) && (b == in_b) && (op_uns == in_op_uns));
    }

    void update(uint32_t in_a, uint32_t in_b, bool in_op_uns) {
        valid = true;
        a = in_a;
        b = in_b;
        op_uns = in_op_uns;
    }
};

class divider {
    private:
        div_result_cache_t div_cache;
        div_stats_t stats;

    public:
        void profiling(bool enable) { stats.profiling(enable); }

        void eval(uint32_t a, uint32_t b, bool op_uns) {
            if (div_cache.hit(a, b, op_uns)) {
                stats.hit();
                return;
            }

            div_special_t special = classify_special(a, b, op_uns);
            if (special != div_special_t::none) {
                stats.special(special);
            } else {
                stats.common(count_common_bits(a, op_uns));
                div_cache.update(a, b, op_uns);
            }
        }

        void log_stats(std::string name, std::ofstream& file) {
            file << "\n\"" << name << "\"" << ": {";
            stats.log(file);
            file << "\n},";
        }

    private:
        static uint32_t abs_val(uint32_t x, bool op_uns) {
            if (op_uns || (TO_I32(x) >= 0)) return x;
            return (~x + 1u);
        }

        static uint32_t clz32(uint32_t x) {
            if (x == 0) return 32;
            return TO_U32(__builtin_clz(x));
        }

        static bool is_pow2(uint32_t x) {
            return (x != 0) && ((x & (x - 1u)) == 0);
        }

        static div_special_t classify_special(
            uint32_t a, uint32_t b,bool op_uns)
        {
            if (b == 0) return div_special_t::div_by_zero;

            if (!op_uns && (a == 0x80000000u) && (b == 0xffffffffu)) {
                return div_special_t::overflow;
            }

            uint32_t abs_a = abs_val(a, op_uns);
            uint32_t abs_b = abs_val(b, op_uns);

            if ((abs_a == 0) || (abs_a < abs_b)) return div_special_t::abs_lt;
            if (is_pow2(abs_b)) return div_special_t::divisor_pow2;
            return div_special_t::none;
        }

        static uint32_t count_common_bits(uint32_t a, bool op_uns) {
            return 32 - clz32(abs_val(a, op_uns));
        }
};
