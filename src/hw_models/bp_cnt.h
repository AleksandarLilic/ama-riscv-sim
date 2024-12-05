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
        const uint8_t cnt_max;
        const uint8_t thr_taken;
        std::vector<uint8_t> cnt_table;
        uint32_t bit_size;

    public:
        bp_cnt(bp_cnt_cfg_t cfg)
        : cnt_bits(cfg.cnt_bits),
          cnt_entries(cfg.cnt_entries),
          cnt_entries_mask(cnt_entries - 1),
          cnt_max((1 << cnt_bits) - 1),
          thr_taken(cnt_max == 1 ? cnt_max : cnt_max >> 1)
        {
            resize_table(cnt_entries);
        }

        void resize_table(uint32_t new_size) {
            cnt_table.resize(new_size);
            for (size_t i = 0; i < cnt_entries; i++) cnt_table[i] = {thr_taken};
            bit_size = cnt_table.size() * cnt_bits;
        }

        uint32_t get_bit_size() { return bit_size; }
        uint32_t get_mask() { return cnt_entries_mask; }

        bool thr_check(uint32_t idx) { return cnt_table[idx] >= thr_taken; }

        void update(bool taken, uint32_t idx) {
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
