#pragma once

#include "defines.h"

struct bp_cnt_cfg_t {
    uint32_t cnt_entries;
    uint8_t cnt_bits;
};

class bp_cnt {
    private:
        const uint8_t cnt_bits;
        const uint32_t cnt_entries;
        const uint32_t cnt_entries_mask;
        std::vector<uint8_t> cnt_table;
        const uint8_t cnt_max;
        const uint8_t thr_taken;
        uint32_t size;

    public:
        bp_cnt(bp_cnt_cfg_t cfg)
        : cnt_bits(cfg.cnt_bits),
          cnt_entries(cfg.cnt_entries),
          cnt_entries_mask(cnt_entries - 1),
          cnt_table(cnt_entries),
          cnt_max((1 << cnt_bits) - 1),
          thr_taken(cnt_max == 1 ? cnt_max : cnt_max >> 1)
        {
            for (size_t i = 0; i < cnt_entries; i++) cnt_table[i] = {thr_taken};
            size = (cnt_table.size() * cnt_bits) >> 3;
        }

        uint32_t get_size() { return size; }
        uint32_t get_mask() { return cnt_entries_mask; }

        bool thr_check(uint8_t idx) { return cnt_table[idx] >= thr_taken; }

        void update(bool taken, uint8_t idx) {
            uint8_t& cnt_entry = cnt_table[idx];
            if (taken) {
                if (cnt_entry < cnt_max) cnt_entry++;
            } else {
                if (cnt_entry > 0) cnt_entry--;
            }
        }

        void dump() {
            std::cout << "      counters: ";
            for (size_t i = 0; i < cnt_entries; i++) {
                std::cout << "[" << i << "]=" << TO_U32(cnt_table[i]) << " ";
            }
        }
};
