#pragma once

#include "defines.h"
#include "bp_common.h"
#include "bp_stats.h"

/*
struct bp_bimodal_entry_t {
    // tag table, can be used to improve prediction
    //uint32_t tag;
    // BTB, needed if uarch can't resolve branch target in the same cycle
    //uint32_t target;
};
*/

class bp_bimodal {
    private:
        const uint8_t cnt_bits;
        std::vector<bp_cnt_entry_t> cnt_table;
        const uint8_t cnt_max;
        const uint8_t thr_taken;
        uint8_t idx_last;
        uint32_t predicted_pc;
        b_dir_t dir_last;
        const uint32_t size;

    public:
        const std::string type_name;
        bp_stats_t stats;

    public:
        bp_bimodal(std::string type_name, uint8_t cnt_bits)
        : cnt_bits(cnt_bits),
          cnt_table(BP_BIMODAL_ENTRIES),
          cnt_max((1 << cnt_bits) - 1),
          thr_taken(cnt_max == 1 ? cnt_max : cnt_max >> 1),
          size((cnt_table.size() * cnt_bits) >> 3),
          type_name(type_name),
          stats(type_name)
        {
            for (uint32_t i = 0; i < BP_BIMODAL_ENTRIES; i++) {
                cnt_table[i] = {thr_taken};
            }
        }

        uint32_t predict(uint32_t target_pc, uint32_t pc) {
            dir_last = (target_pc > pc) ? b_dir_t::forward : b_dir_t::backward;
            idx_last = get_idx(pc);
            if (cnt_table[idx_last].cnt >= thr_taken) predicted_pc = target_pc;
            else predicted_pc = pc + 4;
            return predicted_pc;
        }

        uint8_t get_idx(uint32_t pc) {
            return (pc >> 2) & (BP_BIMODAL_ENTRIES - 1);
        }

        void update(bool taken) {
            uint8_t& cnt_entry = cnt_table[idx_last].cnt;
            if (taken) {
                if (cnt_entry < cnt_max) cnt_entry++;
            } else {
                if (cnt_entry > 0) cnt_entry--;
            }
        }

        void update_stats(uint32_t pc, uint32_t next_pc) {
            bool correct = (next_pc == predicted_pc);
            stats.eval(pc, correct, dir_last);
        }

        void dump() {
            std::cout << "    " << type_name << ": " << std::endl;
            std::cout << "      counters: ";
            for (auto& cnt_entry : cnt_table) {
                std::cout << std::hex << TO_U32(cnt_entry.cnt) << " ";
            }
            std::cout << std::dec << std::endl;
        }
};
