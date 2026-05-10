#pragma once

#include "defines.h"

enum class div_special_t {
    none,
    div_by_zero,
    overflow,
    abs_lt,
    divisor_pow2
};

struct div_special_stats_t {
    private:
        uint64_t div_by_zero = 0;
        uint64_t overflow = 0;
        uint64_t abs_lt = 0;
        uint64_t divisor_pow2 = 0;

    public:
        void collect(div_special_t type) {
            switch (type) {
                case div_special_t::div_by_zero: div_by_zero++; break;
                case div_special_t::overflow: overflow++; break;
                case div_special_t::abs_lt: abs_lt++; break;
                case div_special_t::divisor_pow2: divisor_pow2++; break;
                default: break;
            }
        }

        void log(std::ofstream& log_file) const {
            log_file << JSON_N << "\"special_cases_class\": {"
                     << "\"div_by_zero\": " << div_by_zero << ", "
                     << "\"overflow\": " << overflow << ", "
                     << "\"abs_lt\": " << abs_lt << ", "
                     << "\"divisor_pow2\": " << divisor_pow2
                     << "}";
        }
};

struct div_common_stats_t {
    private:
        uint64_t total = 0;
        static constexpr uint8_t na_min = 255; // never seen a divide
        static constexpr uint8_t na_max = 0; // never seen a divide
        uint8_t min = na_min;
        uint8_t max = na_max;

    public:
        void collect(uint8_t bits) {
            total += bits;
            min = std::min(min, bits);
            max = std::max(max, bits);
        }

        void log(std::ofstream& log_file) const {
            log_file << JSON_N << "\"common_cases_info\": {"
                     << "\"total\": " << total << ", "
                     << "\"min\": ";
            if (min == na_min) log_file << "null";
            else log_file << min;
            log_file << ", " << "\"max\": ";
            if (max == na_max) log_file << "null";
            else log_file << max;
            log_file << "}";
        }
};

struct div_stats_t {
    private:
        uint64_t total = 0;
        uint64_t cache_hits = 0;
        uint64_t special_cases = 0;
        uint64_t common_cases = 0;
        div_special_stats_t special_classes;
        div_common_stats_t common_cases_info;
        bool prof_active = false;

    public:
        void profiling(bool enable) { prof_active = enable; }

        void hit() {
            if (!prof_active) return;
            total++;
            cache_hits++;
        }

        void special(div_special_t type) {
            if (!prof_active) return;
            total++;
            special_cases++;
            special_classes.collect(type);
        }

        void common(uint8_t bits) {
            if (!prof_active) return;
            total++;
            common_cases++;
            common_cases_info.collect(bits);
        }

        void log(std::ofstream& log_file) const {
            log_file << JSON_N << "\"total\": " << total << ","
                     << JSON_N << "\"cache_hits\": " << cache_hits << ","
                     << JSON_N << "\"special_cases\": " << special_cases << ",";
            special_classes.log(log_file);
            log_file << ","
                     << JSON_N << "\"common_cases\": " << common_cases << ",";
            common_cases_info.log(log_file);
        }
};
