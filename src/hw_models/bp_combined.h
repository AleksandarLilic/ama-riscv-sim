#pragma once

#include "bp.h"
#include "bp_cnt.h"

class bp_combined : public bp {
    public:
        const std::string type_name;

    private:
        bp_cnt cnt;
        std::array<bp_t, 2> bps;
        uint32_t idx_last;

    public:
        bp_combined(
            std::string type_name,
            bp_cfg_t cfg,
            std::array<bp_t, 2> bps) :
                bp(type_name, cfg),
                type_name(type_name),
                cnt({cfg.pc_bits, cfg.cnt_bits, type_name}),
                bps(bps)
        {
            size = (cnt.get_bit_size() + 8) >> 3; // to bytes, round up
            // plus the size of the two predictors from their own instances
            // added when selected in the bp_if class
        }

        virtual ~bp_combined() = default;

        void add_size(uint32_t size) { this->size += size; }

        uint32_t get_idx(uint32_t pc) { return (pc >> 2) & cnt.get_mask(); }

        bp_t select(uint32_t pc) {
            idx_last = get_idx(pc);
            if (cnt.thr_check(idx_last)) return bps[0];
            else return bps[1];
        }

        void store_prediction(uint32_t predicted_pc) {
            this->predicted_pc = predicted_pc;
        }

        void store_direction(uint32_t target_pc, uint32_t pc) {
            find_b_dir(target_pc, pc);
        }

        void update(bool p0c, bool p1c) {
            cnt.update(p0c, p1c, idx_last);
        }

        virtual uint32_t predict(uint32_t /* target_pc */,
                                 uint32_t /* pc */) override { return 0; }

        virtual bool eval_and_update(
            bool /* taken */,
            uint32_t /* next_pc */) override { return false; }

        virtual void dump() override {
            std::cout << "    " << type_name << ": " << std::endl;
            cnt.dump();
            std::cout << std::dec << std::endl;
        }
};
