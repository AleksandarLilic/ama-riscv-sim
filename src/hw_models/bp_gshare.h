#pragma once

#include "bp.h"
#include "bp_cnt.h"

class bp_gshare : public bp {
    protected:
        const uint8_t idx_bits;
        const uint8_t gr_bits;
        const uint8_t pc_bits;
        const uint32_t idx_mask;
        const uint32_t gr_mask;
        const uint32_t pc_mask;
        const uint32_t gr_offset;
        bp_cnt cnt;
        uint32_t gr; // global register
        uint32_t idx_last;

    public:
        bp_gshare(bp_cfg_t cfg) :
            bp(cfg),
            idx_bits(std::max(cfg.gr_bits, cfg.pc_bits)),
            gr_bits(cfg.gr_bits),
            pc_bits(cfg.pc_bits),
            idx_mask((1 << idx_bits) - 1),
            gr_mask((1 << gr_bits) - 1),
            pc_mask((1 << pc_bits) - 1),
            gr_offset(pc_bits > gr_bits ? pc_bits - gr_bits : 0),
            cnt({idx_bits, cfg.cnt_bits, cfg.type_name}),
            gr(0)
        {
            size = cnt.get_bit_size() + cfg.gr_bits;
            size = (size + 8) >> 3; // to bytes, round up
            cnt_ptr = &cnt;
        }

        virtual uint32_t get_idx(uint32_t pc, uint32_t gr) {
            uint32_t pc_part = (pc >> BP_PC_CUTOFF_BITS) & pc_mask;
            uint32_t gr_part = gr & gr_mask;
            // xor with top PC bits, fit within the mask length
            // TODO: slide XORs bits L/R?
            // applicable only if unequal number of pc and gr bits
            return (pc_part ^ (gr_part << gr_offset)) & idx_mask;
        }

        virtual uint32_t predict(uint32_t target_pc, uint32_t pc) override {
            idx_last = get_idx(pc, gr);
            return predict_common(target_pc, pc, cnt.thr_check(idx_last));
        }

        virtual bool eval_and_update(bool taken, uint32_t next_pc) override {
            cnt.update(taken, idx_last);
            gr = ((gr << 1) | taken);
            return (next_pc == predicted_pc);
        }
};
