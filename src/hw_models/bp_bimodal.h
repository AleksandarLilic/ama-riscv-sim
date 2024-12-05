#pragma once

#include "bp.h"
#include "bp_cnt.h"

/*
struct bp_bimodal_entry_t {
    // tag table, can be used to improve prediction
    //uint32_t tag;
    // BTB, needed if uarch can't resolve branch target in the same cycle
    //uint32_t target;
};
*/

class bp_bimodal : public bp {
    private:
        bp_cnt cnt;
        uint32_t idx_last;

    public:
        bp_bimodal(std::string type_name, bp_cnt_cfg_t cfg)
        : bp(type_name), cnt(cfg) {
            size = (cnt.get_bit_size() + 8) >> 3; // to bytes, round up
        }

        uint32_t get_idx(uint32_t pc) { return (pc >> 2) & cnt.get_mask(); }

        virtual uint32_t predict(uint32_t target_pc, uint32_t pc) override {
            find_b_dir(target_pc, pc);
            idx_last = get_idx(pc);
            if (cnt.thr_check(idx_last)) predicted_pc = target_pc;
            else predicted_pc = pc + 4;
            return predicted_pc;
        }

        virtual void update(bool taken) override {cnt.update(taken, idx_last);}

        virtual void dump() override {
            std::cout << "    " << type_name << ": " << std::endl;
            cnt.dump();
            std::cout << std::dec << std::endl;
        }
};
