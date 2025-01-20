#pragma once

#include "defines.h"

#define CNT_ERR(param, msg) \
    std::cerr << "ERROR: " << param << " for " << type_name << " predictor " \
              << msg << std::endl;

struct bp_cnt_cfg_t {
    public:
        uint8_t cnt_idx_bits;
        uint8_t cnt_bits;
    public:
        bp_cnt_cfg_t(
            uint8_t cnt_idx_bits,
            uint8_t cnt_bits,
            std::string type_name) :
                cnt_idx_bits(cnt_idx_bits),
                cnt_bits(cnt_bits)
        {
            validate_inputs(type_name);
        }
    private:
        void validate_inputs(std::string type_name) {
            bool error = false;
            if (cnt_bits == 0) {
                CNT_ERR("cnt_bits", "cannot be 0");
                error = true;
            }
            if (cnt_idx_bits == 0) {
                CNT_ERR("cnt_idx_bits", "cannot be 0");
                error = true;
            }
            if (cnt_bits > 8) {
                CNT_ERR("cnt_bits", "cannot be greater than 8");
                std::cerr << "Specified: " << TO_U32(cnt_bits) << std::endl;
                error = true;
            }
            if (cnt_idx_bits > 30) {
                CNT_ERR("cnt_idx_bits", "cannot be greater than 30");
                std::cerr << "Specified: " << TO_U32(cnt_idx_bits) << std::endl;
                error = true;
            }
            if (error) {
                throw std::runtime_error(
                    "Invalid counter table inputs encountered");
            }
        };
};

class bp_cnt {
    private:
        const uint8_t cnt_bits;
        const uint32_t cnt_entries_num;
        const uint32_t cnt_entries_mask;
        const uint8_t cnt_max;
        const uint8_t thr_taken;
        const uint32_t bit_size;
        std::vector<uint8_t> cnt_table;
        std::vector<uint32_t> cnt_table_accesses;

    public:
        bp_cnt(bp_cnt_cfg_t cfg) :
            cnt_bits(cfg.cnt_bits),
            cnt_entries_num(TO_U32(1 << cfg.cnt_idx_bits)),
            cnt_entries_mask(cnt_entries_num - 1),
            cnt_max((1 << cnt_bits) - 1),
            thr_taken(cnt_max == 1 ? cnt_max : cnt_max >> 1),
            bit_size(cnt_entries_num * cnt_bits),
            cnt_table(cnt_entries_num, thr_taken),
            cnt_table_accesses(cnt_entries_num, 0)
        {
        }

        uint32_t get_bit_size() { return bit_size; }
        uint32_t get_mask() { return cnt_entries_mask; }

        bool thr_check(uint32_t idx) { return cnt_table[idx] >= thr_taken; }

        // for single predictor
        void update(bool taken, uint32_t idx) {
            uint8_t& cnt_entry = cnt_table[idx];
            cnt_table_accesses[idx]++;
            if (taken) {
                if (cnt_entry < cnt_max) cnt_entry++;
            } else {
                if (cnt_entry > 0) cnt_entry--;
            }
        }

        // for combined predictor
        void update(bool p0c, bool p1c, uint32_t idx) {
            uint8_t& cnt_entry = cnt_table[idx];
            cnt_table_accesses[idx]++;
            if (p0c != p1c) {
                if (p0c) {
                    if (cnt_entry < cnt_max) cnt_entry++;
                } else { // p1c, decrement
                    if (cnt_entry > 0) cnt_entry--;
                }
            } // no change if they match
        }

        void dump() {
            // TODO: should be stored as csv or similar
            std::cout << INDENT << INDENT << "counter accesses:\n";
            for (size_t i = 0; i < cnt_entries_num; i++) {
                std::cout << INDENT << INDENT << INDENT << "[0x"
                          << std::hex << std::setw(3) << std::setfill('0')
                          << i << "] = " << std::dec << cnt_table_accesses[i]
                          << "\n";
            }
        }
};
