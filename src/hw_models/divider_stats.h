#pragma once

#include "defines.h"

#define DIV_SPECIAL_STATS_JSON_ENTRY(stat_struct) \
    JSON_N << "\"special_cases_class\": {" \
    << "\"div_by_zero\": " << stat_struct->div_by_zero << ", " \
    << "\"overflow\": " << stat_struct->overflow << ", " \
    << "\"abs_lt\": " << stat_struct->abs_lt << ", " \
    << "\"divisor_pow2\": " << stat_struct->divisor_pow2 \
    << "}"

#define DIV_SIZE_JSON_ENTRY(size_struct) \
    JSON_N << "\"size\": {" \
    << "\"total\": " << size_struct->total \
    << ", \"entries\": " << size_struct->entries \
    << ", \"entry_size\": " << size_struct->entry_size \
    << "}"

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
            log_file << DIV_SPECIAL_STATS_JSON_ENTRY(this);
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

        void log(std::ofstream& log_file, uint64_t count) const {
            float_t avg = -1.0;
            if (count != 0) avg = TO_F32(total) / TO_F32(count);
            log_file << JSON_N << "\"common_cases_info\": {"
                     << "\"total\": " << total << ", "
                     << "\"min\": ";
            if (min == na_min) log_file << "null";
            else log_file << TO_U32(min);
            log_file << ", " << "\"max\": ";
            if (max == na_max) log_file << "null";
            else log_file << TO_U32(max);
            log_file << ", " << "\"avg\": ";
            if (min == na_min) log_file << "null";
            else log_file << std::fixed << std::setprecision(2) << avg;
            log_file << "}";
        }

        uint64_t get_total() const { return total; }
};

struct div_size_t {
    private:
        uint32_t total = 0;
        uint32_t entries = 0;
        uint32_t entry_size = 0;

    public:
        div_size_t() = default;
        div_size_t(uint32_t entries, uint32_t entry_bits) :
            total((entries * entry_bits) >> 3),
            entries(entries),
            entry_size((entry_bits) >> 3)
        {}

        void log(std::ofstream& log_file) const {
            log_file << DIV_SIZE_JSON_ENTRY(this);
        }

        void show() const {
            std::cout << "divider (E: " << entries
                      << ", S/ES: " << total << "/" << entry_size << " B):";
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
        static float_t fraction(uint64_t count, uint64_t total) {
            if (total == 0) return -1.0;
            return (TO_F32(count) / TO_F32(total) * 100.0);
        }

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
            float_t cache_fraction = fraction(cache_hits, total);
            float_t special_fraction = fraction(special_cases, total);
            float_t common_fraction = fraction(common_cases, total);

            log_file << std::fixed << std::setprecision(2);
            log_file << JSON_N << "\"total\": " << total << ","
                     << JSON_N << "\"cache\": {"
                     << "\"count\": " << cache_hits
                     << ", \"fraction\": " << cache_fraction
                     << "},"
                     << JSON_N << "\"special_cases\": {"
                     << "\"count\": " << special_cases
                     << ", \"fraction\": " << special_fraction
                     << "},";
            special_classes.log(log_file);
            log_file << ","
                     << JSON_N << "\"common_cases\": {"
                     << "\"count\": " << common_cases
                     << ", \"fraction\": " << common_fraction
                     << "},";
            common_cases_info.log(log_file, common_cases);
        }

        void show() const {
            float_t cache_fraction = fraction(cache_hits, total);
            float_t special_fraction = fraction(special_cases, total);
            float_t common_fraction = fraction(common_cases, total);
            float_t avg_common_bits = -1.0;
            if (common_cases != 0) {
                avg_common_bits =
                    TO_F32(common_cases_info.get_total()) / TO_F32(common_cases);
            }

            std::cout << std::fixed << std::setprecision(2);
            std::cout << "Div: " << total
                      << ", Cache: " << cache_hits
                      << " (" << cache_fraction << "%)"
                      << ", Special: " << special_cases
                      << " (" << special_fraction << "%)"
                      << ", Common: " << common_cases
                      << "(" << common_fraction << "%), "
                      << common_cases_info.get_total() << " b, "
                      << avg_common_bits << " b/d";
        }
};
