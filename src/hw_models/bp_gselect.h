#pragma once

#include "bp.h"
#include "bp_cnt.h"

struct bp_gselect_cfg_t {
    uint8_t cnt_bits;
    uint8_t gr_bits;
    uint8_t pc_bits;
};

class bp_gselect : public bp {
    protected:
        bp_cnt cnt;
        uint32_t idx_last;
        uint32_t gr; // global register
        uint32_t idx_mask;
        const uint32_t gr_bits;
        //const uint32_t pc_bits;
        const uint32_t gr_mask;
        const uint32_t pc_mask;

    public:
        bp_gselect(std::string type_name, bp_gselect_cfg_t cfg)
        : bp(type_name),
          cnt({TO_U32(1 << (cfg.gr_bits + cfg.pc_bits)), cfg.cnt_bits}),
          gr(0),
          idx_mask((1 << (cfg.gr_bits + cfg.pc_bits)) - 1),
          gr_bits(cfg.gr_bits),
          //pc_bits(cfg.pc_bits),
          gr_mask((1 << cfg.gr_bits) - 1),
          pc_mask((1 << cfg.pc_bits) - 1)
        {
            size = cnt.get_bit_size() + cfg.gr_bits;
            size = (size + 8) >> 3; // to bytes, round up
        }

        virtual uint32_t get_idx(uint32_t pc, uint32_t gr) {
            uint32_t pc_part = (pc >> 2) & pc_mask;
            uint32_t gr_part = gr & gr_mask;
            // concat with pc top, gr bottom
            return ((pc_part << gr_bits) | gr_part) & idx_mask;
        }

        virtual uint32_t predict(uint32_t target_pc, uint32_t pc) override {
            find_b_dir(target_pc, pc);
            idx_last = get_idx(pc, gr);
            if (cnt.thr_check(idx_last)) predicted_pc = target_pc;
            else predicted_pc = pc + 4;
            return predicted_pc;
        }

        virtual void update(bool taken) override {
            cnt.update(taken, idx_last);
            gr = ((gr << 1) | taken) & gr_mask;
        }

        virtual void dump() override {
            std::cout << "    " << type_name << ": " << std::endl;
            cnt.dump();
            std::cout << std::dec << std::endl;
        }
};
