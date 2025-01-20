#pragma once

#include "bp.h"



class bp_static : public bp {
    private:
        bp_sttc_t method;

    public:
        bp_static(bp_cfg_t cfg) : bp(cfg) {
            size = 0;
            cnt_ptr = nullptr;
            uint8_t m = cfg.cnt_bits;

            if (m >= TO_U8(bp_sttc_t::_count)) {
                std::cerr << "ERROR: " << m << " for " << cfg.type_name
                          << " predictor is not a valid method" << std::endl;
                throw std::runtime_error("Invalid static predictor method");
            }
            if (m == TO_U8(bp_sttc_t::btfn)) method = bp_sttc_t::btfn;
            else if (m == TO_U8(bp_sttc_t::at)) method = bp_sttc_t::at;
            else if (m == TO_U8(bp_sttc_t::ant)) method = bp_sttc_t::ant;
        }

        virtual uint32_t predict(uint32_t target_pc, uint32_t pc) override {
            find_b_dir(target_pc, pc);
            if (method == bp_sttc_t::btfn) predict_btfn(target_pc, pc);
            else if (method == bp_sttc_t::at) predict_taken(target_pc);
            else if (method == bp_sttc_t::ant) predict_not_taken(pc);

            return predicted_pc;
        }

        // backward taken forward not taken
        void predict_btfn(uint32_t target_pc, uint32_t pc) {
            if (b_dir_last == b_dir_t::backward) predicted_pc = target_pc;
            else predicted_pc = pc + 4;
        }

        // always taken
        void predict_taken(uint32_t target_pc) {
            predicted_pc = target_pc;
        }

        // always not taken
        void predict_not_taken(uint32_t pc) {
            predicted_pc = pc + 4;
        }

        virtual bool eval_and_update(
            bool /* taken */, uint32_t next_pc) override {
            return (next_pc == predicted_pc);
        }

        virtual void dump() override {
            std::cout << INDENT << type_name << ": static\n";
         }
};
