#pragma once

#include "bp.h"

class bp_ideal : public bp {
    public:
        bp_ideal(std::string type_name, bp_cfg_t cfg) : bp(type_name, cfg) {
            size = 0;
        }

        void goto_future(uint32_t correct_pc) { predicted_pc = correct_pc; }

        virtual uint32_t predict(uint32_t target_pc, uint32_t pc) override {
            find_b_dir(target_pc, pc);
            return predicted_pc;
        }

        virtual bool eval_and_update(bool /*taken*/, uint32_t next_pc)override{
            return (next_pc == predicted_pc);
        }

        virtual void dump() override { }
};