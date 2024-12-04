#pragma once

#include "bp.h"
#include "bp_cnt.h"

struct bp_local_cfg_t {
    uint32_t hist_entries;
    uint8_t hist_bits;
    uint8_t cnt_bits;
};

struct bp_local_entry_t {
    // tag table, can be used to improve prediction
    //uint32_t tag;
    // BTB, needed if uarch can't resolve branch target in the same cycle
    //uint32_t target;
    // local history (max 32 last branches)
    uint32_t hist_pattern;
};

class bp_local : public bp {
    private:
        bp_cnt cnt;
        uint8_t idx_last;
        b_dir_t dir_last;
        uint32_t hist_last;
        uint32_t size;

        const uint32_t hist_bits;
        const uint32_t hist_entries;
        std::vector<bp_local_entry_t> hist_table;
        const uint32_t hist_mask;

    public:
        bp_local(std::string type_name, bp_local_cfg_t cfg)
        :  bp(type_name),
           // counter per history entry
           cnt({TO_U32((1 << cfg.hist_bits)), cfg.cnt_bits}),
           hist_bits(cfg.hist_bits),
           hist_entries(cfg.hist_entries),
           hist_table(cfg.hist_entries),
           hist_mask((1 << hist_bits) - 1)
        {
            for (size_t i = 0; i < hist_entries; i++) hist_table[i] = {0};
            size = (hist_table.size() * hist_bits) >> 3;
            size += cnt.get_size();
        }

        uint8_t get_idx(uint32_t pc) { return (pc >> 2) & (hist_entries - 1); }

        uint32_t predict(uint32_t target_pc, uint32_t pc) {
            dir_last = (target_pc > pc) ? b_dir_t::forward : b_dir_t::backward;
            idx_last = get_idx(pc);
            hist_last = hist_table[idx_last].hist_pattern & hist_mask;
            if (cnt.thr_check(hist_last)) predicted_pc = target_pc;
            else predicted_pc = pc + 4;
            return predicted_pc;
        }

        virtual void update(bool taken) override {
            uint32_t& hist_entry = hist_table[idx_last].hist_pattern;
            hist_entry = ((hist_entry << 1) | taken) & hist_mask;
            cnt.update(taken, hist_last);
        }

        virtual void dump() override {
            std::cout << "    " << type_name << ": " << std::endl;
            std::cout << "      history: ";
            for (auto& local_entry : hist_table) {
                std::cout << std::hex << TO_U32(local_entry.hist_pattern) <<" ";
            }
            std::cout << std::endl;
            cnt.dump();
            std::cout << std::dec << std::endl;
        }
};
