#pragma once

#include "bp_gselect.h"

class bp_gshare : public bp_gselect {
    public:
        bp_gshare(std::string type_name, bp_gselect_cfg_t cfg)
        : bp_gselect(type_name, cfg)
        {
            // workaround to reuse the gselect's constructor,
            // and then resize the table
            // the longer one determines the size since it's XORing for index
            uint32_t max_bits = std::max(cfg.gr_bits, cfg.pc_bits);
            uint32_t cnt_table_size = 1 << max_bits;
            cnt.resize_table(cnt_table_size);
            size = cnt.get_bit_size() + cfg.gr_bits;
            size = (size + 8) >> 3; // to bytes, round up
            idx_mask = max_bits - 1;
        }

        virtual uint32_t get_idx(uint32_t pc, uint32_t gr) override {
            uint32_t pc_part = (pc >> 2) & pc_mask;
            uint32_t gr_part = gr & gr_mask;
            // xor, fit within the mask length
            // TODO: XORs from the rightmost bits, add a switch for left?
            // applicable only if unequal lengths of pc and gr slices
            return (pc_part ^ gr_part) & idx_mask;
        }
};
