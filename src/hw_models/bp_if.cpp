#include "bp_if.h"

bp_if::bp_if(std::string name, bp_t bp_type) :
    bp_name(name), bp_active(bp_type),
    static_bp("static"),
    bimodal_bp("bimodal", {BP_BIMODAL_ENTRIES, BP_BIMODAL_CNT_BITS}),
    local_bp("local", {BP_LOCAL_ENTRIES, BP_LOCAL_HIST_BITS, BP_LOCAL_CNT_BITS})
    {
        predictors[TO_U8(bp_t::sttc)] = &static_bp;
        predictors[TO_U8(bp_t::bimodal)] = &bimodal_bp;
        predictors[TO_U8(bp_t::local)] = &local_bp;
    }

uint32_t bp_if::predict(uint32_t pc, int32_t offset) {
    target_pc = TO_I32(pc) + offset;
    dir = (target_pc > pc) ? b_dir_t::forward : b_dir_t::backward;
    if (prof_active) {
        if (bi_program_stats.find(pc) == bi_program_stats.end()) {
            bi_program_stats[pc] = {dir, 0, 0, {}};
        }
    }

    for (uint8_t i = 0; i < predictors.size(); i++) {
        predicted_pcs[i] = predictors[i]->predict(target_pc, pc);
    }

    return predicted_pcs[TO_U8(bp_active)];
}

void bp_if::update(uint32_t pc, uint32_t next_pc) {
    bool taken = next_pc != pc + 4;
    // internal states always updated
    for (auto& p : predictors) { p->update(taken); }
    // but only update stats if profiling is active
    if (!prof_active) return;
    update_stats(pc, taken);
    for (auto& p : predictors) { p->update_stats(pc, next_pc); }
}

void bp_if::update_stats(uint32_t pc, bool taken) {
    bi_program_stats_t* ptr = &bi_program_stats[pc];
    ptr->taken += taken;
    ptr->total++;
    ptr->pattern.push_back(taken);
}

void bp_if::finish(std::string log_path, uint64_t all_insts) {
    for (auto& p : predictors) { p->stats.summarize(all_insts); }
    show_stats(log_path);
}

void bp_if::show_stats(std::string log_path) {
    std::ofstream bcsv;
    bcsv.open(log_path + "branches.csv");
    bcsv << "PC,Direction,Taken,Not_Taken,All,Taken%"
         << ",P_Static,P_Bimodal,P_Local"
         << ",P_Static%,P_Bimodal%,P_Local%"
         // << ",P_Bimodal_idx,P_Local_idx"
         << ",Pattern" << std::endl;

    for (auto& [pc, stats] : bi_program_stats) {
        // summarize taken/not pattern, e.g. 1110011 makes a string "3T 2N 2T"
        std::string pattern_str;
        uint32_t count = 1;
        for (uint32_t i = 1; i < stats.pattern.size(); i++) {
            if (stats.pattern[i] == stats.pattern[i - 1]) {
                count++;
            } else {
                pattern_str += std::to_string(count) +
                               (stats.pattern[i - 1] ? "T " : "N ");
                count = 1;
            }
        }
        // add the direction of the last pattern
        pattern_str += std::to_string(count) +
                       (stats.pattern.back() ? "T" : "N");

        float_t taken_ratio =
            (TO_F32(stats.taken) / TO_F32(stats.total)) * 100;

        std::array<uint32_t, num_predictors> predicted;
        for (uint8_t i = 0; i < num_predictors; i++) {
            predicted[i] = predictors[i]->stats.get_predicted(pc);
        }

        std::array<float_t, num_predictors> acc;
        for (uint8_t i = 0; i < num_predictors; i++) {
            acc[i] = (TO_F32(predicted[i]) / TO_F32(stats.total)) * 100;
        }

        bcsv << std::hex << pc << std::dec
             << std::fixed << std::setprecision(0)
             << "," << (stats.dir == b_dir_t::forward ? "F" : "B")
             << "," << stats.taken
             << "," << stats.total - stats.taken
             << "," << stats.total
             << "," << taken_ratio
             // enforce order to match the header
             << "," << predicted[TO_U8(bp_t::sttc)]
             << "," << predicted[TO_U8(bp_t::bimodal)]
             << "," << predicted[TO_U8(bp_t::local)]
             << "," << acc[TO_U8(bp_t::sttc)]
             << "," << acc[TO_U8(bp_t::bimodal)]
             << "," << acc[TO_U8(bp_t::local)]
             //<< "," << TO_U32(bimodal_bp.get_idx(pc))
             //<< "," << TO_U32(local_bp.get_idx(pc))
             << "," << pattern_str << std::endl;
    }

    uint32_t branches = bi_program_stats.size();
    std::cout << "Program branch stats: unique branches: " << branches
              << std::endl;
    // show all, but mark the active one (driving the icache)
    std::cout << bp_name << " (active: ";
    std::cout << predictors[TO_U8(bp_active)]->type_name << ")" << std::endl;

    // TODO: predictor size
    for (auto& p : predictors) { p->stats.show(); }
    return;
    std::cout << "  Predictors internal state:" << std::endl;
    for (auto& p : predictors) { p->dump(); }
}

void bp_if::log_stats(std::ofstream& log_file) {
    predictors[TO_U8(bp_active)]->stats.log(bp_name, log_file);
}
