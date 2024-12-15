#pragma once

#include "bp.h"

class bp_none : public bp {
    public:
        bp_none(std::string type_name, bp_cfg_t cfg) : bp(type_name, cfg) {
            size = 0;
        }

        // always miss
        virtual uint32_t predict(uint32_t target_pc, uint32_t pc) override {
            find_b_dir(target_pc, pc);
            return 0;
        }

        virtual bool eval_and_update(
            bool /* taken */, uint32_t /* next_pc */) override {
            return false;
        }

        virtual void dump() override { }
};
