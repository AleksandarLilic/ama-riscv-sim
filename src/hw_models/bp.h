#pragma once

#include "defines.h"
#include "bp_stats.h"

class bp {
    protected:
        uint32_t predicted_pc;
        b_dir_t b_dir_last;
        uint32_t size;

    public:
        const std::string type_name;
        bp_stats_t stats;

    public:
        bp(std::string type_name) : type_name(type_name), stats(type_name) {}
        virtual ~bp() = default;
        virtual void find_b_dir(uint32_t target_pc, uint32_t pc) {
            b_dir_last = (target_pc > pc) ? b_dir_t::forward :
                                            b_dir_t::backward;
        }
        virtual uint32_t predict(uint32_t target_pc, uint32_t pc) = 0;
        virtual void update(bool taken) = 0;
        virtual void update_stats(uint32_t pc, uint32_t next_pc) {
            bool correct = (next_pc == predicted_pc);
            stats.eval(pc, correct, b_dir_last);
        }
        virtual void dump() = 0;
};
