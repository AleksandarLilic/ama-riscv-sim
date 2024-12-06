#pragma once

#include "bp.h"

class bp_static : public bp {
    public:
        bp_static(std::string type_name, bp_cfg_t cfg) : bp(type_name, cfg) {
            size = 0;
        }

        // backward taken, forward not taken
        virtual uint32_t predict(uint32_t target_pc, uint32_t pc) override {
            find_b_dir(target_pc, pc);
            if (b_dir_last == b_dir_t::forward) predicted_pc = pc + 4;
            else predicted_pc = target_pc;
            return predicted_pc;
        }

        virtual bool eval_and_update(bool /*taken*/, uint32_t next_pc)override{
            return (next_pc == predicted_pc);
        }

        virtual void dump() override { }
};
