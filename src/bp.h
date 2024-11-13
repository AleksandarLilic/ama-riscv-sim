#pragma once

#include "defines.h"

#define BP_JSON_ENTRY(name, stat_struct) \
    "\"" << name << "\"" << ": {" \
    << "\"branches\": " << stat_struct->total \
    << ", \"predicted\": " << stat_struct->predicted \
    << ", \"predicted_fwd\": " << stat_struct->predicted_fwd \
    << ", \"predicted_bwd\": " << stat_struct->predicted_bwd \
    << ", \"mispredicted\": " << stat_struct->mispredicted \
    << ", \"mispredicted_fwd\": " << stat_struct->mispredicted_fwd \
    << ", \"mispredicted_bwd\": " << stat_struct->mispredicted_bwd \
    << "},"

struct bp_stats_t {
    private:
        uint32_t predicted_fwd;
        uint32_t predicted_bwd;
        uint32_t mispredicted_fwd;
        uint32_t mispredicted_bwd;
        // derived, only for summary
        uint32_t predicted;
        uint32_t mispredicted;
        uint32_t total;
    public:
        bp_stats_t() : predicted_fwd(0), predicted_bwd(0),
                       mispredicted_fwd(0), mispredicted_bwd(0) {}
        void eval(bool correct, b_dir_t dir) {
            if (correct) {
                if (dir == b_dir_t::forward) predicted_fwd++;
                else predicted_bwd++;
            } else {
                if (dir == b_dir_t::forward) mispredicted_fwd++;
                else mispredicted_bwd++;
            }
        }
        void summarize() {
            predicted = predicted_fwd + predicted_bwd;
            mispredicted = mispredicted_fwd + mispredicted_bwd;
            total = predicted + mispredicted;
        }
        void show() const {
            float_t acc = 0.0;
            if (total > 0) acc = TO_F32(predicted) / TO_F32(total) * 100;
            std::cout << "B: " << total
                      << ", P(f/b): " << predicted
                      << "(" << predicted_fwd << "/" << predicted_bwd
                      << "), MP(f/b): " << mispredicted
                      << "(" << mispredicted_fwd << "/" << mispredicted_bwd
                      << "), ACC: " << std::fixed << std::setprecision(2)
                      << acc << "%"
                      << std::endl;
        }
        void log(std::string name, std::ofstream& log_file) const {
            log_file << BP_JSON_ENTRY(name, this) << std::endl;
        }
};

class bp {
    private:
        std::string bp_name;
        bp_stats_t stats;
        b_dir_t dir;
        bool prof_active = false;

    public:
        bp() = delete;
        bp(std::string name) : bp_name(name) {}
        void profiling(bool enable) { prof_active = enable; }
        uint32_t predict(uint32_t pc, int32_t offset);
        void update(bool correct);
        void log_stats(std::ofstream& log_file);
        void finish() { stats.summarize(); show_stats(); }
    private:
        void show_stats();
};
