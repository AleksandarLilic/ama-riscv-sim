#pragma once

#include "bp.h"

class bp_combined : public bp {
    public:
        const std::string type_name;

    private:
        bp_pht pht;
        std::array<std::unique_ptr<bp>, 2> bpsn;
        uint32_t idx_last;

    public:
        bp_combined(
            bp_cfg_t cfg,
            std::array<std::unique_ptr<bp>, 2> bps_in) :
                bp(cfg),
                type_name(cfg.type_name),
                pht({cfg.pc_bits, cfg.cnt_bits, cfg.type_name})
        {
            bpsn = std::move(bps_in);
            size = bpsn[0]->get_size() + bpsn[1]->get_size();
            size += (pht.get_bit_size() + 4) >> 3;
        }

        virtual ~bp_combined() = default;

        void add_size(uint32_t size) { this->size += size; }

        uint32_t get_idx(uint32_t pc) { return get_pc(pc, pht.get_idx_mask()); }

        uint32_t select(uint32_t pc) {
            idx_last = get_idx(pc);
            if (pht.thr_check(idx_last)) return 0;
            else return 1;
        }

        virtual uint32_t predict(uint32_t target_pc, uint32_t pc) override {
            find_b_dir(target_pc, pc);
            std::array<uint32_t, 2> bp_preds;
            bp_preds[0] = bpsn[0]->predict(target_pc, pc);
            bp_preds[1] = bpsn[1]->predict(target_pc, pc);
            predicted_pc = bp_preds[select(pc)];
            return predicted_pc;
        }

        virtual bool eval_and_update(bool taken, uint32_t next_pc) override {
            bool p0c = bpsn[0]->eval_and_update(taken, next_pc);
            bool p1c = bpsn[1]->eval_and_update(taken, next_pc);
            pht.update(p0c, p1c, idx_last);
            return (next_pc == predicted_pc);
        }

        virtual void dump() override {
            std::cout << INDENT << type_name << ": " << std::endl;
            pht.dump();
            std::cout << INDENT << "bp 1: ";
            bpsn[0]->dump();
            std::cout << INDENT << "bp 2: ";
            bpsn[1]->dump();
            std::cout << std::dec << std::endl;
        }
};
