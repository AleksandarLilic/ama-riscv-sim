#pragma once

#include "defines.h"
#include "bp_stats.h"

struct bp_entry_t {
    // tag table, not used atm
    // can be used to improve prediction
    uint32_t tag;
    // BHT
    uint8_t hist;
    // BTB, not used atm
    // needed if uarch can't resolve branch target in the same cycle
    uint32_t target;
};

class bp_bimodal {
    private:
        const uint8_t cnt_bits;
        std::vector<bp_entry_t> table;
        const uint8_t cnt_max;
        const uint8_t thr_taken;
        uint8_t idx;
        uint32_t predicted_pc;
        b_dir_t dir;
        const uint32_t size;

    public:
        const std::string type_name;
        bp_stats_t stats;

    public:
        bp_bimodal(std::string type_name, uint8_t cnt_bits)
        : cnt_bits(cnt_bits),
          table(BP_BIMODAL_ENTRIES),
          cnt_max((1 << cnt_bits) - 1),
          thr_taken(cnt_max == 1 ? cnt_max : cnt_max >> 1),
          size((BP_BIMODAL_ENTRIES * cnt_bits) >> 3),
          type_name(type_name),
          stats(type_name)
        {
            table.resize(BP_BIMODAL_ENTRIES);
            for (int i = 0; i < BP_BIMODAL_ENTRIES; i++) {
                table[i] = {0, thr_taken, 0};
            }
        }
        uint32_t predict(uint32_t target_pc, uint32_t pc) {
            dir = (target_pc > pc) ? b_dir_t::forward : b_dir_t::backward;
            idx = get_idx(pc);
            if (table[idx].hist >= thr_taken) predicted_pc = target_pc;
            else predicted_pc = pc + 4;
            return predicted_pc;
        }
        uint8_t get_idx(uint32_t pc) {
            return (pc >> 2) & (BP_BIMODAL_ENTRIES - 1);
        }
        void update(uint32_t target, bool taken) {
            uint8_t& entry = table[idx].hist;
            if (taken) {
                if (entry < cnt_max) entry++;
            } else {
                if (entry > 0) entry--;
            }
            table[idx].target = target;
        }
        void update_stats(uint32_t pc, uint32_t next_pc) {
            bool correct = (next_pc == predicted_pc);
            stats.eval(pc, correct, dir);
        }
        void dump() {
            std::cout << "bimodal table: ";
            for (auto& entry : table) {
                std::cout << std::hex << TO_U32(entry.hist) << " ";
            }
            std::cout << std::dec << std::endl;
        }
};
