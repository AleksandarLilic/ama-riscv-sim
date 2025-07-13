#pragma once

#include "defines.h"

#define BP_STATS_JSON_ENTRY(type, stat_struct) \
    JSON_N << "\"type\": " << "\"" << type << "\","\
    << std::fixed << std::setprecision(2) \
    << JSON_N << "\"branches\": " << stat_struct->total_branches << ","\
    << JSON_N << "\"predicted\": " << stat_struct->predicted << ","\
    << JSON_N << "\"predicted_fwd\": " << stat_struct->predicted_fwd << ","\
    << JSON_N << "\"predicted_bwd\": " << stat_struct->predicted_bwd << ","\
    << JSON_N << "\"mispredicted\": " << stat_struct->mispredicted << ","\
    << JSON_N << "\"mispredicted_fwd\": " << stat_struct->mispredicted_fwd<<","\
    << JSON_N << "\"mispredicted_bwd\": " << stat_struct->mispredicted_bwd<<","\
    << JSON_N << "\"accuracy\": " << stat_struct->accuracy <<","\
    << JSON_N << "\"mpki\": " << stat_struct->mpki


// branch instruction stats
struct bi_app_stats_t {
    b_dir_t dir;
    uint8_t funct3;
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
        uint64_t predicted_fwd;
        uint64_t predicted_bwd;
        uint64_t mispredicted_fwd;
        uint64_t mispredicted_bwd;
        std::map<uint32_t, bi_predictor_stats_t> bi_predictor_stats;
        // derived, only for summary
        uint64_t predicted;
        uint64_t mispredicted;
        uint64_t total_branches;
        uint64_t total_insts;
        float_t accuracy = -1.0; // i.e. never seen a branch
        float_t mpki = 0.0; // mispredicted per 1k instruction
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
        void summarize(uint64_t total_insts) {
            this->total_insts = total_insts;
            predicted = predicted_fwd + predicted_bwd;
            mispredicted = mispredicted_fwd + mispredicted_bwd;
            total_branches = predicted + mispredicted;
            if (total_branches > 0) {
                accuracy = TO_F32(predicted) / TO_F32(total_branches) * 100.0;
            }
            if (total_insts > 0) {
                mpki = TO_F32(mispredicted) / (TO_F32(total_insts) / 1000.0);
            }
        }
        uint32_t get_predicted(uint32_t pc) const {
            if (bi_predictor_stats.find(pc) == bi_predictor_stats.end()) {
                return 0;
            }
            return bi_predictor_stats.at(pc).predicted;
        }
        void show() {
            std::cout << std::fixed << std::setprecision(2)
                      << "P: " << predicted
                      << ", M: " << mispredicted
                      //<< "P(f/b): " << predicted
                      //<< "(" << predicted_fwd << "/" << predicted_bwd
                      //<< "), M(f/b): " << mispredicted
                      //<< "(" << mispredicted_fwd << "/" << mispredicted_bwd
                      << /* << ")" << */ ", ACC: " << accuracy << "%"
                      << ", MPKI: " << mpki;
        }
        void log(std::ofstream& log_file) const {
            log_file << BP_STATS_JSON_ENTRY(type_name, this);
        }
};
