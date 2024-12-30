#pragma once

#include "defines.h"

#define BP_STATS_JSON_ENTRY(type, stat_struct) \
    "\"type\": " << "\"" << type << "\"" \
    << ", \"branches\": " << stat_struct->total \
    << ", \"predicted\": " << stat_struct->predicted \
    << ", \"predicted_fwd\": " << stat_struct->predicted_fwd \
    << ", \"predicted_bwd\": " << stat_struct->predicted_bwd \
    << ", \"mispredicted\": " << stat_struct->mispredicted \
    << ", \"mispredicted_fwd\": " << stat_struct->mispredicted_fwd \
    << ", \"mispredicted_bwd\": " << stat_struct->mispredicted_bwd

// branch instruction stats
struct bi_program_stats_t {
    b_dir_t dir;
    uint32_t taken;
    uint32_t total;
    std::vector<bool> pattern;
};

struct bi_predictor_stats_t {
    uint32_t predicted;
    // internal counters? predictor specific
};

// branch predictor stats
struct bp_stats_t {
    private:
        uint32_t predicted_fwd;
        uint32_t predicted_bwd;
        uint32_t mispredicted_fwd;
        uint32_t mispredicted_bwd;
        std::map<uint32_t, bi_predictor_stats_t> bi_predictor_stats;
        // derived, only for summary
        uint32_t predicted;
        uint32_t mispredicted;
        uint32_t total;
        const std::string type_name;

    public:
        bp_stats_t(const std::string type_name)
        : predicted_fwd(0), predicted_bwd(0),
          mispredicted_fwd(0), mispredicted_bwd(0),
          type_name(type_name) { }
        void eval(uint32_t pc, bool correct, b_dir_t dir) {
            if (correct) {
                if (dir == b_dir_t::forward) predicted_fwd++;
                else predicted_bwd++;
            } else {
                if (dir == b_dir_t::forward) mispredicted_fwd++;
                else mispredicted_bwd++;
            }
            if (bi_predictor_stats.find(pc) == bi_predictor_stats.end()) {
                bi_predictor_stats[pc] = {0};
            }
            bi_predictor_stats[pc].predicted += correct;
        }
        void summarize() {
            predicted = predicted_fwd + predicted_bwd;
            mispredicted = mispredicted_fwd + mispredicted_bwd;
            total = predicted + mispredicted;
        }
        uint32_t get_predicted(uint32_t pc) const {
            if (bi_predictor_stats.find(pc) == bi_predictor_stats.end()) {
                return 0;
            }
            return bi_predictor_stats.at(pc).predicted;
        }
        void show() const {
            float_t acc = 0.0;
            if (total > 0) {
                acc = TO_F32(predicted) / TO_F32(total) * 100;
            }
            std::cout << std::fixed << std::setprecision(2)
                      << "P: " << predicted
                      << ", M: " << mispredicted
                      << ", ACC: "
                      //<< "P(f/b): " << predicted
                      //<< "(" << predicted_fwd << "/" << predicted_bwd
                      //<< "), M(f/b): " << mispredicted
                      //<< "(" << mispredicted_fwd << "/" << mispredicted_bwd
                      //<< "), ACC: "
                      << acc << "%";
        }
        void log(std::ofstream& log_file) const {
            log_file << BP_STATS_JSON_ENTRY(type_name, this);
        }
};
