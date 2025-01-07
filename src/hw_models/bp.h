#pragma once

#include "defines.h"
#include "bp_stats.h"

struct bp_cfg_t {
    const uint8_t pc_bits;
    const uint8_t cnt_bits;
    const uint8_t hist_bits;
    const uint8_t gr_bits;
    //const std::string type_name;
    const char* type_name;
};

class bp {
    public:
        const std::string type_name;

    protected:
        uint32_t predicted_pc;
        b_dir_t b_dir_last;
        uint32_t size;
        bp_stats_t stats;

    public:
        bp(bp_cfg_t cfg) : type_name(cfg.type_name), stats(cfg.type_name) {}

        virtual ~bp() = default;

        virtual uint32_t get_size() { return size; }

        virtual uint32_t predict(uint32_t target_pc, uint32_t pc) = 0;

        virtual bool eval_and_update(bool taken, uint32_t next_pc) = 0;

        virtual void update_stats(uint32_t pc, uint32_t next_pc) {
            bool correct = (next_pc == predicted_pc);
            stats.eval(pc, correct, b_dir_last);
        }

        // for ideal predictor only
        virtual void goto_future(uint32_t /* correct_pc */) {};

        virtual void dump() = 0;

        virtual void show_stats() {
            std::cout << INDENT << std::left << std::setw(14) << type_name;
            std::cout << " (" << std::right << std::setw(5) << size << " B): ";
            stats.show();
            std::cout << std::endl;
        }

        virtual void log_stats(std::string name, std::ofstream &file) {
            file << "\"" << name << "\"" << ": {";
            stats.log(file);
            file << ", \"size\": " << size << "},";
        }

        virtual void summarize_stats() {
            stats.summarize();
        }

        virtual uint32_t get_predicted_stats(uint32_t pc) {
            return stats.get_predicted(pc);
        }

    protected:
        virtual void find_b_dir(uint32_t target_pc, uint32_t pc) {
            b_dir_last = (target_pc > pc) ? b_dir_t::forward :
                                            b_dir_t::backward;
        }

        uint32_t predict_common(uint32_t target_pc, uint32_t pc, bool taken) {
            find_b_dir(target_pc, pc);
            if (taken) predicted_pc = target_pc;
            else predicted_pc = pc + 4;
            return predicted_pc;
        }
};
