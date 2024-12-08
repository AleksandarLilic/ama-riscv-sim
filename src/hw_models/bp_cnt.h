#pragma once

#include "defines.h"

struct bp_cnt_cfg_t {
    uint32_t cnt_idx_bits;
    uint8_t cnt_bits;
};

class bp_cnt {
    private:
        uint8_t cnt_bits;
        uint32_t cnt_entries;
        uint32_t cnt_entries_mask;
        uint8_t cnt_max;
        uint8_t thr_taken;
        std::vector<uint8_t> cnt_table;
        uint32_t bit_size;

    public:
        bp_cnt(bp_cnt_cfg_t cfg) {
            setup(cfg);
        }

        void setup(bp_cnt_cfg_t cfg) {
            cnt_bits = cfg.cnt_bits;
            cnt_entries = TO_U32(1 << cfg.cnt_idx_bits);
            cnt_entries_mask = std::max(1u, cnt_entries) - 1;
            cnt_max = (1 << cnt_bits) - 1;
            thr_taken = cnt_max == 1 ? cnt_max : cnt_max >> 1;
            cnt_table.resize(cnt_entries);
            for (auto& e : cnt_table) e = thr_taken;
            bit_size = cnt_table.size() * cnt_bits;
        }

        uint32_t get_bit_size() { return bit_size; }
        uint32_t get_mask() { return cnt_entries_mask; }

        bool thr_check(uint32_t idx) { return cnt_table[idx] >= thr_taken; }

        // for single predictor
        void update(bool taken, uint32_t idx) {
            uint8_t& cnt_entry = cnt_table[idx];
            if (taken) {
                if (cnt_entry < cnt_max) cnt_entry++;
            } else {
                if (cnt_entry > 0) cnt_entry--;
            }
        }

        // for combined predictor
        void update(bool p0c, bool p1c, uint32_t idx) {
            uint8_t& cnt_entry = cnt_table[idx];
            if (p0c != p1c) {
                if (p0c) {
                    if (cnt_entry < cnt_max) cnt_entry++;
                } else { // p1c, decrement
                    if (cnt_entry > 0) cnt_entry--;
                }
            } // no change if they match
        }

        void dump() {
            std::cout << "      counters: ";
            for (size_t i = 0; i < cnt_entries; i++) {
                std::cout << "[" << i << "]=" << TO_U32(cnt_table[i]) << " ";
            }
        }
};
