#pragma once

#include "defines.h"
#include "bp_stats.h"

class bp_static {
    private:
        uint32_t predicted_pc;
        b_dir_t dir;
        const uint32_t size = 0;

    public:
        const std::string type_name;
        bp_stats_t stats;

    public:
        bp_static(std::string type_name)
        : type_name(type_name),
          stats(type_name) { }

        // backward taken, forward not taken
        uint32_t predict(uint32_t target_pc, uint32_t pc) {
            dir = (target_pc > pc) ? b_dir_t::forward : b_dir_t::backward;
            if (dir == b_dir_t::forward) predicted_pc = pc + 4;
            else predicted_pc = target_pc;
            return predicted_pc;
        }

        void update_stats(uint32_t pc, uint32_t next_pc) {
            bool correct = (next_pc == predicted_pc);
            stats.eval(pc, correct, dir);
        }
};
