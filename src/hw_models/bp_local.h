#pragma once

#include "bp.h"
#include "bp_cnt.h"

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
        const uint8_t pc_bits;
        const uint32_t hist_entries_num;
        const uint32_t pc_mask;
        std::vector<bp_local_entry_t> hist_table;
        const uint8_t hist_bits;
        const uint32_t cnt_entries_num;
        const uint32_t hist_mask;
        bp_cnt cnt; // counter per history entry
        uint32_t idx_last;
        uint32_t hist_last;

    private:
        uint8_t validate_pc_bits(uint8_t pc_bits) {
            bool error = false;
            if (pc_bits == 0) {
                CNT_ERR("pc_bits", "cannot be 0");
                error = true;
            }
            if (pc_bits > 30) {
                CNT_ERR("pc_bits", "cannot be greater than 30");
                error = true;
            }
            if (error) {
                throw std::runtime_error(
                    "Invalid history table inputs encountered");
            }
            return pc_bits;
        }

    public:
        bp_local(std::string type_name, bp_cfg_t cfg) :
            bp(type_name, cfg),
            pc_bits(validate_pc_bits(cfg.pc_bits)),
            hist_entries_num(1 << pc_bits),
            pc_mask(hist_entries_num - 1),
            hist_table(hist_entries_num),
            hist_bits(cfg.hist_bits),
            cnt_entries_num(1 << hist_bits),
            hist_mask(cnt_entries_num - 1),
            cnt({hist_bits, cfg.cnt_bits, type_name})
        {
            for (auto& e : hist_table) e.hist_pattern = 0;
            size = hist_table.size() * hist_bits;
            size += cnt.get_bit_size();
            size = (size + 8) >> 3; // to bytes, round up
        }

        uint32_t get_idx(uint32_t pc) { return (pc >> 2) & pc_mask; }

        uint32_t predict(uint32_t target_pc, uint32_t pc) {
            idx_last = get_idx(pc);
            hist_last = hist_table[idx_last].hist_pattern & hist_mask;
            return predict_common(target_pc, pc, cnt.thr_check(hist_last));
        }

        virtual bool eval_and_update(bool taken, uint32_t next_pc) override {
            uint32_t& hist_entry = hist_table[idx_last].hist_pattern;
            hist_entry = ((hist_entry << 1) | taken) & hist_mask;
            cnt.update(taken, hist_last);
            return (next_pc == predicted_pc);
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
