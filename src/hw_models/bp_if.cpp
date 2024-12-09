#include "bp_if.h"

bp_if::bp_if(std::string name, hw_cfg_t hw_cfg) :
    bp_name(name), bp_active(hw_cfg.bp_active),
    bpc_1(hw_cfg.bpc_1), bpc_2(hw_cfg.bpc_2),
    static_bp("static", BP_STATIC_CFG),
    bimodal_bp("bimodal", BP_BIMODAL_CFG),
    local_bp("local", BP_LOCAL_CFG),
    global_bp("global", BP_GLOBAL_CFG),
    gselect_bp("gselect", BP_GSELECT_CFG),
    gshare_bp("gshare", BP_GSHARE_CFG),
    ideal_bp("_ideal_", BP_STATIC_CFG),
    combined_bp("combined", BP_COMBINED_CFG, {bpc_1, bpc_2})
    {
        predictors[TO_U8(bp_t::sttc)] = &static_bp;
        predictors[TO_U8(bp_t::bimodal)] = &bimodal_bp;
        predictors[TO_U8(bp_t::local)] = &local_bp;
        predictors[TO_U8(bp_t::global)] = &global_bp;
        predictors[TO_U8(bp_t::gselect)] = &gselect_bp;
        predictors[TO_U8(bp_t::gshare)] = &gshare_bp;
        predictors[TO_U8(bp_t::ideal)] = &ideal_bp;
        predictors[TO_U8(bp_t::combined)] = &combined_bp;

        combined_bp.add_size(predictors[TO_U8(bpc_1)]->get_size());
        combined_bp.add_size(predictors[TO_U8(bpc_2)]->get_size());
    }

uint32_t bp_if::predict(uint32_t pc, int32_t offset) {
    uint32_t target_pc = TO_U32(TO_I32(pc) + offset);
    b_dir_t dir = (target_pc > pc) ? b_dir_t::forward : b_dir_t::backward;
    if (prof_active) {
        if (bi_program_stats.find(pc) == bi_program_stats.end()) {
            bi_program_stats[pc] = {dir, 0, 0, {}};
        }
    }

    std::array<uint32_t, num_predictors> predicted_pcs;
    for (uint8_t i = 0; i < predictors.size() - 1; i++) {
        predicted_pcs[i] = predictors[i]->predict(target_pc, pc);
    }
    bp_t bp_sel = combined_bp.select(pc);
    uint32_t combined_bp_predicted_pc = predicted_pcs[TO_U8(bp_sel)];
    combined_bp.store_prediction(combined_bp_predicted_pc);
    combined_bp.store_direction(target_pc, pc);
    predicted_pcs[TO_U8(bp_t::combined)] = combined_bp_predicted_pc;
    return predicted_pcs[TO_U8(bp_active)];
}

void bp_if::update(uint32_t pc, uint32_t next_pc) {
    bool taken = next_pc != pc + 4;

    // internal states always updated
    std::array<bool, num_predictors> correct;
    for (uint8_t i = 0; i < predictors.size() - 1; i++) {
        correct[i] = predictors[i]->eval_and_update(taken, next_pc);
    }
    combined_bp.update(correct[TO_U8(bpc_1)], correct[TO_U8(bpc_2)]);

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

void bp_if::finish(std::string log_path) {
    for (auto& p : predictors) { p->summarize_stats(); }
    show_stats(log_path);
}

void bp_if::show_stats(std::string log_path) {
    std::ofstream bcsv;
    bcsv.open(log_path + "branches.csv");
    bcsv << "PC,Direction,Taken,Not_Taken,All,Taken%";
    for (auto& p : predictors) { bcsv << ",P_" << p->type_name; }
    for (auto& p : predictors) { bcsv << ",P_" << p->type_name << "%"; }
    //bcsv << ",P_Bimodal_idx,P_Local_idx";
    bcsv << ",Best,P_best,P_best%"
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

        size_t idx_best = TO_U8(bp_t::sttc); // defaults to the simplest one
        std::array<uint32_t, num_predictors> pred;
        for (uint8_t i = 0; i < num_predictors; i++) {
            pred[i] = predictors[i]->get_predicted_stats(pc);
            if (pred[i] > pred[idx_best]) idx_best = i;
        }

        std::array<float_t, num_predictors> acc;
        for (uint8_t i = 0; i < num_predictors; i++) {
            acc[i] = (TO_F32(pred[i]) / TO_F32(stats.total)) * 100;
        }

        std::string best_name = predictors[idx_best]->type_name;
        uint32_t best_p = pred[idx_best];
        float_t best_acc = acc[idx_best];

        bcsv << std::hex << pc << std::dec
             << std::fixed << std::setprecision(1)
             << "," << (stats.dir == b_dir_t::forward ? "F" : "B")
             << "," << stats.taken
             << "," << stats.total - stats.taken
             << "," << stats.total
             << "," << taken_ratio;
        for (auto& p : pred) { bcsv << "," << p; };
        for (auto& a : acc) { bcsv << "," << a; };
        bcsv << "," << best_name
             << "," << best_p
             << "," << best_acc;
        //bcsv << "," << TO_U32(bimodal_bp.get_idx(pc))
        //     << "," << TO_U32(local_bp.get_idx(pc));
        bcsv << "," << pattern_str << std::endl;
    }

    uint32_t branches = bi_program_stats.size();
    std::cout << "Branch stats: unique branches: " << branches << std::endl;
    // show all, but mark the active one (driving the icache)
    std::cout << bp_name << " (active: ";
    std::cout << predictors[TO_U8(bp_active)]->type_name << ")" << std::endl;

    for (auto& p : predictors) { p->show_stats(); }
    return;
    std::cout << "  Predictors internal state:" << std::endl;
    for (auto& p : predictors) { p->dump(); }
}

void bp_if::log_stats(std::ofstream& log_file) {
    predictors[TO_U8(bp_active)]->log_stats(bp_name, log_file);
}
