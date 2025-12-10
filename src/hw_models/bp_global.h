#pragma once

#include "bp.h"

class bp_global : public bp {
    private:
        const uint8_t idx_bits;
        const uint32_t idx_mask;
        bp_pht pht;
        uint32_t ghr; // global register
        uint32_t idx_last;

    public:
        bp_global(bp_cfg_t cfg) :
            bp(cfg),
            idx_bits(cfg.ghr_bits),
            idx_mask((1 << idx_bits) - 1),
            pht({idx_bits, cfg.cnt_bits, cfg.type_name}),
            ghr(0)
        {
            size = pht.get_bit_size() + idx_bits;
            size = (size + 4) >> 3;
            pht_ptr = &pht;
        }

        uint32_t get_idx(uint32_t ghr) { return ghr & idx_mask; }

        virtual uint32_t predict(uint32_t target_pc, uint32_t pc) override {
            idx_last = get_idx(ghr);
            return predict_common(target_pc, pc, pht.thr_check(idx_last));
        }

        virtual bool eval_and_update(bool taken, uint32_t next_pc) override {
            pht.update(taken, idx_last);
            ghr = ((ghr << 1) | taken);
            return (next_pc == predicted_pc);
        }
};
