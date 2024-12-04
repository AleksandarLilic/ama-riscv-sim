#pragma once

#include "bp.h"
#include "bp_cnt.h"

struct bp_global_cfg_t {
    uint8_t cnt_bits;
    uint8_t gr_bits;
};

class bp_global : public bp {
    private:
        bp_cnt cnt;
        uint8_t idx_last;
        uint64_t gr; // global register
        const uint64_t gr_mask;

    public:
        bp_global(std::string type_name, bp_global_cfg_t cfg)
        : bp(type_name),
          cnt({TO_U32(1 << cfg.gr_bits), cfg.cnt_bits}),
          gr(0),
          gr_mask((1 << cfg.gr_bits) - 1)
        {
            size = cnt.get_size();
            size += (cfg.gr_bits >> 3);
        }

        uint8_t get_idx(uint64_t gr) { return gr & gr_mask; }

        virtual uint32_t predict(uint32_t target_pc, uint32_t pc) override {
            find_b_dir(target_pc, pc);
            idx_last = get_idx(gr);
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
