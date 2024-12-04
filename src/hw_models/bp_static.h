#pragma once

#include "bp.h"

class bp_static : public bp {
    public:
        bp_static(std::string type_name) : bp(type_name) {
            size = 0;
        }

        // backward taken, forward not taken
        virtual uint32_t predict(uint32_t target_pc, uint32_t pc) override {
            find_b_dir(target_pc, pc);
            if (b_dir_last == b_dir_t::forward) predicted_pc = pc + 4;
            else predicted_pc = target_pc;
            return predicted_pc;
        }

        // no update, static prediction, three ways to handle it (for ref.)
        virtual void update(bool /* taken */) override { }
        //virtual void update([[maybe_unused]] bool taken) override { }
        //virtual void update(bool taken) override { (void)taken; }
        virtual void dump() override { }
};
