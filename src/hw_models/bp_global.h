#pragma once

#include "bp.h"
#include "bp_cnt.h"

class bp_global : public bp {
    private:
        const uint8_t idx_bits;
        const uint32_t idx_mask;
        bp_cnt cnt;
        uint32_t gr; // global register
        uint32_t idx_last;

    public:
        bp_global(bp_cfg_t cfg) :
            bp(cfg),
            idx_bits(cfg.gr_bits),
            idx_mask((1 << idx_bits) - 1),
            cnt({idx_bits, cfg.cnt_bits, cfg.type_name}),
            gr(0)
        {
            size = cnt.get_bit_size() + idx_bits;
            size = (size + 8) >> 3; // to bytes, round up
            cnt_ptr = &cnt;
        }

        uint32_t get_idx(uint32_t gr) { return gr & idx_mask; }

        virtual uint32_t predict(uint32_t target_pc, uint32_t pc) override {
            idx_last = get_idx(gr);
            return predict_common(target_pc, pc, cnt.thr_check(idx_last));
        }

        virtual bool eval_and_update(bool taken, uint32_t next_pc) override {
            cnt.update(taken, idx_last);
            gr = ((gr << 1) | taken);
            return (next_pc == predicted_pc);
        }
};
