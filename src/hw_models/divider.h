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
        // a, b, div, rem, op_uns, valid
        static constexpr uint32_t div_result_cache_entry_bits =
            32 + 32 + 32 + 32 + 1 + 1;
        uint32_t div_cache_entries;
        uint32_t div_cache_wr_ptr;
        std::vector<div_result_cache_t> div_cache;
        div_size_t size;
        div_stats_t stats;

    public:
        divider(uint32_t div_cache_entries = 1) :
            div_cache_entries(div_cache_entries),
            div_cache_wr_ptr(0),
            div_cache(div_cache_entries),
            size(div_cache_entries, div_result_cache_entry_bits)
        {
            if (div_cache_entries == 0) {
                std::cerr << "ERROR: divider result cache: number of entries "
                          << "cannot be 0" << std::endl;
                throw std::invalid_argument("");
            }
        }

        void profiling(bool enable) { stats.profiling(enable); }

        void eval(uint32_t a, uint32_t b, bool op_uns) {
            if (cache_hit(a, b, op_uns)) {
                stats.hit();
                return;
            }

            div_special_t special = classify_special(a, b, op_uns);
            if (special != div_special_t::none) {
                stats.special(special);
            } else {
                stats.common(count_common_bits(a, b, op_uns));
                div_cache.at(div_cache_wr_ptr).update(a, b, op_uns);
                div_cache_wr_ptr = ((div_cache_wr_ptr + 1) % div_cache_entries);
            }
        }

        void log_stats(std::string name, std::ofstream& file) {
            file << "\n\"" << name << "\"" << ": {";
            stats.log(file);
            file << ",";
            size.log(file);
            file << "\n},";
        }

        void finish(bool show) const {
            if (!show) return;
            size.show();
            std::cout << "\n" << INDENT;
            stats.show();
            std::cout << "\n";
        }

    private:
        bool cache_hit(uint32_t a, uint32_t b, bool op_uns) const {
            for (const auto& cache_entry : div_cache) {
                if (cache_entry.hit(a, b, op_uns)) return true;
            }
            return false;
        }

        static uint32_t abs_val(uint32_t x, bool op_uns) {
            if (op_uns || (TO_I32(x) >= 0)) return x;
            return (~x + 1u);
        }

        static uint8_t clz32(uint32_t x) {
            if (x == 0) return 32;
            return TO_U8(__builtin_clz(x));
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

        static uint8_t count_common_bits(uint32_t a, uint32_t b, bool op_uns) {
            uint8_t bits_a = TO_U8(32 - clz32(abs_val(a, op_uns)));
            uint8_t bits_b = TO_U8(32 - clz32(abs_val(b, op_uns)));
            return TO_U8(bits_a - (bits_b - 1));
        }
};
