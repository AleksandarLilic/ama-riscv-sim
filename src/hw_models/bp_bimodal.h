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
        bp_bimodal(bp_cfg_t cfg) :
            bp(cfg),
            cnt({cfg.pc_bits, cfg.cnt_bits, cfg.type_name})
        {
            size = (cnt.get_bit_size() + 8) >> 3; // to bytes, round up
            cnt_ptr = &cnt;
        }

        uint32_t get_idx(uint32_t pc) {
            return (pc >> BP_PC_CUTOFF_BITS) & cnt.get_mask();
        }

        virtual uint32_t predict(uint32_t target_pc, uint32_t pc) override {
            idx_last = get_idx(pc);
            return predict_common(target_pc, pc, cnt.thr_check(idx_last));
        }

        virtual bool eval_and_update(bool taken, uint32_t next_pc) override {
            cnt.update(taken, idx_last);
            return (next_pc == predicted_pc);
        }
};
