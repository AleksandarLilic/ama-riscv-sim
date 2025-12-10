#pragma once

#include "bp.h"

class bp_gselect : public bp {
    protected:
        const uint8_t idx_bits;
        const uint8_t ghr_bits;
        const uint8_t pc_bits;
        const uint32_t idx_mask;
        const uint32_t ghr_mask;
        const uint32_t pc_mask;
        bp_pht pht;
        uint32_t ghr; // global register
        uint32_t idx_last;

    public:
        bp_gselect(bp_cfg_t cfg) :
            bp(cfg),
            idx_bits(cfg.ghr_bits + cfg.pc_bits),
            ghr_bits(cfg.ghr_bits),
            pc_bits(cfg.pc_bits),
            idx_mask((1 << idx_bits) - 1),
            ghr_mask((1 << ghr_bits) - 1),
            pc_mask((1 << pc_bits) - 1),
            pht({idx_bits, cfg.cnt_bits, cfg.type_name}),
            ghr(0)
        {
            size = pht.get_bit_size() + cfg.ghr_bits;
            size = (size + 4) >> 3;
            pht_ptr = &pht;
        }

        virtual uint32_t get_idx(uint32_t pc, uint32_t ghr) {
            uint32_t pc_part = get_pc(pc, pc_mask);
            uint32_t ghr_part = ghr & ghr_mask;
            // concat with pc top, ghr bottom
            return ((pc_part << ghr_bits) | ghr_part) & idx_mask;
        }

        virtual uint32_t predict(uint32_t target_pc, uint32_t pc) override {
            idx_last = get_idx(pc, ghr);
            return predict_common(target_pc, pc, pht.thr_check(idx_last));
        }

        virtual bool eval_and_update(bool taken, uint32_t next_pc) override {
            pht.update(taken, idx_last);
            ghr = ((ghr << 1) | taken);
            return (next_pc == predicted_pc);
        }
};
