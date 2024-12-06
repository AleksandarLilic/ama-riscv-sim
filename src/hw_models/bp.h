#pragma once

#include "defines.h"
#include "bp_stats.h"

struct bp_cfg_t {
    uint32_t cnt_entries;
    uint8_t cnt_bits;
    uint32_t hist_entries;
    uint8_t hist_bits;
    uint32_t gr_bits;
    uint32_t pc_bits;
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
        bp(std::string type_name, bp_cfg_t /* cfg */)
        : type_name(type_name), stats(type_name) {}

        virtual ~bp() = default;

        virtual uint32_t get_size() { return size; }

        virtual void find_b_dir(uint32_t target_pc, uint32_t pc) {
            b_dir_last = (target_pc > pc) ? b_dir_t::forward :
                                            b_dir_t::backward;
        }

        virtual uint32_t predict(uint32_t target_pc, uint32_t pc) = 0;

        virtual bool eval_and_update(bool taken, uint32_t next_pc) = 0;

        virtual void update_stats(uint32_t pc, uint32_t next_pc) {
            bool correct = (next_pc == predicted_pc);
            stats.eval(pc, correct, b_dir_last);
        }

        virtual void dump() = 0;

        virtual void show_stats() {
            std::cout << "  " << std::left << std::setw(8) << type_name;
            std::cout << " (" << std::right << std::setw(5) << size << " B): ";
            stats.show();
            std::cout << std::endl;
        }

        virtual void log_stats(std::string name, std::ofstream &file) {
            stats.log(name, file);
        }

        virtual void summarize_stats(uint64_t all_insts) {
            stats.summarize(all_insts);
        }

        virtual uint32_t get_predicted_stats(uint32_t pc) {
            return stats.get_predicted(pc);
        }
};
