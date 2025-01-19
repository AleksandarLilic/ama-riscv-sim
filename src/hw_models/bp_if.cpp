#include "bp_if.h"

// { pc_bits, cnt_bits, hist_bits, gr_bits, type_name }
#define BP_CFG_NONE { 0, 0, 0, 0, hw_cfg.bp_active_name.c_str() }
#define BP_BIMODAL_CFG { \
    hw_cfg.bp_bimodal_pc_bits, hw_cfg.bp_bimodal_cnt_bits, 0, 0, \
    hw_cfg.bp_active_name.c_str() }
#define BP_LOCAL_CFG { \
    hw_cfg.bp_local_pc_bits, hw_cfg.bp_local_cnt_bits, \
    hw_cfg.bp_local_hist_bits, 0, hw_cfg.bp_active_name.c_str() }
#define BP_GLOBAL_CFG { \
    0, hw_cfg.bp_global_cnt_bits, 0, hw_cfg.bp_global_gr_bits, \
    hw_cfg.bp_active_name.c_str() }
#define BP_GSELECT_CFG { \
    hw_cfg.bp_gselect_pc_bits, hw_cfg.bp_gselect_cnt_bits, \
    0, hw_cfg.bp_gselect_gr_bits, hw_cfg.bp_active_name.c_str() }
#define BP_GSHARE_CFG { \
    hw_cfg.bp_gshare_pc_bits, hw_cfg.bp_gshare_cnt_bits, \
    0, hw_cfg.bp_gshare_gr_bits, hw_cfg.bp_active_name.c_str() }
#define BP_COMBINED_CFG { \
    hw_cfg.bp_combined_pc_bits, hw_cfg.bp_combined_cnt_bits, 0, 0, \
    hw_cfg.bp_active_name.c_str() }

bp_if::bp_if(std::string name, hw_cfg_t hw_cfg) :
    bp_name(name),
    bp_active_type(hw_cfg.bp_active),
    bp_combined_p1_type(hw_cfg.bp_combined_p1),
    bp_combined_p2_type(hw_cfg.bp_combined_p2),
    bp_run_all(hw_cfg.bp_run_all),
    to_dump_csv(hw_cfg.bp_dump_csv)
    {
        active_bp = create_predictor(bp_active_type, hw_cfg);

        if (!bp_run_all) return;

        for (size_t i = 0; i < arch_bp_defs.size(); i++) {
            all_predictors.push_back(
                create_predictor(arch_bp_defs[i].type, arch_bp_defs[i].cfg));
            if (arch_bp_defs[i].type == bp_t::ideal) {
                ideal_bp = dynamic_cast<bp_ideal*>(all_predictors[i].get());
            }
        }

        for (size_t i = 0; i < arch_bpc_defs.size(); i++) {
            all_predictors.push_back(create_predictor(arch_bpc_defs[i]));
        }
}

std::unique_ptr<bp> bp_if::create_predictor(bp_t bp_type, hw_cfg_t hw_cfg) {
    std::unique_ptr<bp> bp_out, bp1, bp2;
    switch (bp_type) {
        case bp_t::sttc:
            bp_out = std::make_unique<bp_static, bp_cfg_t>(BP_CFG_NONE);
            break;
        case bp_t::bimodal:
            bp_out = std::make_unique<bp_bimodal, bp_cfg_t>(BP_BIMODAL_CFG);
            break;
        case bp_t::local:
            bp_out = std::make_unique<bp_local, bp_cfg_t>(BP_LOCAL_CFG);
            break;
        case bp_t::global:
            bp_out = std::make_unique<bp_global, bp_cfg_t>(BP_GLOBAL_CFG);
            break;
        case bp_t::gselect:
            bp_out = std::make_unique<bp_gselect, bp_cfg_t>(BP_GSELECT_CFG);
            break;
        case bp_t::gshare:
            bp_out = std::make_unique<bp_gshare, bp_cfg_t>(BP_GSHARE_CFG);
            break;
        case bp_t::ideal:
            bp_out = std::make_unique<bp_ideal, bp_cfg_t>(BP_CFG_NONE);
            break;
        case bp_t::none:
            bp_out = std::make_unique<bp_none, bp_cfg_t>(BP_CFG_NONE);
            break;
        case bp_t::combined:
            bp1 = create_predictor(bp_combined_p1_type, hw_cfg);
            bp2 = create_predictor(bp_combined_p2_type, hw_cfg);
            bp_out = std::make_unique<
                bp_combined,
                bp_cfg_t,
                std::array<std::unique_ptr<bp>, 2>>
                (
                    BP_COMBINED_CFG,
                    {std::move(bp1), std::move(bp2)}
                );
            break;
        default:
            throw std::runtime_error("Unknown branch predictor");
    }
    return bp_out;
}

std::unique_ptr<bp> bp_if::create_predictor(bp_t bp_type, bp_cfg_t bp_cfg) {
    // cast to an array for function with the array arg
    std::array<bp_def_t, 3> t = {{
        {bp_type, bp_cfg},
        {{}, {}},
        {{}, {}},
    }};
    return create_predictor(t);
}

std::unique_ptr<bp> bp_if::create_predictor(std::array<bp_def_t, 3> bp_defs) {
    auto bp_type = bp_defs[0].type;
    auto bp_cfg = bp_defs[0].cfg;
    std::unique_ptr<bp> bp_out;
    std::array<std::unique_ptr<bp>, 2> bps; // only for combined
    switch (bp_type) {
        case bp_t::sttc:
            bp_out = std::make_unique<bp_static>(bp_cfg);
            break;
        case bp_t::bimodal:
            bp_out = std::make_unique<bp_bimodal>(bp_cfg);
            break;
        case bp_t::local:
            bp_out = std::make_unique<bp_local>(bp_cfg);
            break;
        case bp_t::global:
            bp_out = std::make_unique<bp_global>(bp_cfg);
            break;
        case bp_t::gselect:
            bp_out = std::make_unique<bp_gselect>(bp_cfg);
            break;
        case bp_t::gshare:
            bp_out = std::make_unique<bp_gshare>(bp_cfg);
            break;
        case bp_t::ideal:
            bp_out = std::make_unique<bp_ideal>(bp_cfg);
            break;
        case bp_t::none:
            bp_out = std::make_unique<bp_none>(bp_cfg);
            break;
        case bp_t::combined:
            bps = {
                create_predictor(bp_defs[1].type, bp_defs[1].cfg),
                create_predictor(bp_defs[2].type, bp_defs[2].cfg)
            };
            bp_out = std::make_unique<bp_combined>(bp_cfg, std::move(bps));
            break;
        default:
            throw std::runtime_error("Unknown branch predictor");
    }
    return bp_out;
}

uint32_t bp_if::predict(uint32_t pc, int32_t offset) {
    uint32_t target_pc = TO_U32(TO_I32(pc) + offset);
    b_dir_t dir = (target_pc > pc) ? b_dir_t::forward : b_dir_t::backward;
    if (prof_active) {
        if (bi_app_stats.find(pc) == bi_app_stats.end()) {
            bi_app_stats[pc] = {dir, 0, 0, {}};
        }
    }
    for (auto& p : all_predictors) p->predict(target_pc, pc);
    return active_bp->predict(target_pc, pc);
}

void bp_if::update(uint32_t pc, uint32_t next_pc) {
    bool taken = next_pc != pc + 4;
    active_bp->eval_and_update(taken, next_pc);
    for (auto& p : all_predictors) p->eval_and_update(taken, next_pc);

    // only update stats if profiling is active
    if (!prof_active) return;

    active_bp->update_stats(pc, next_pc);
    for (auto& p : all_predictors) p->update_stats(pc, next_pc);
    // no need to update app stats if csv is not dumped at the end
    if (to_dump_csv) update_app_stats(pc, taken);
}

void bp_if::update_app_stats(uint32_t pc, bool taken) {
    bi_app_stats_t* ptr = &bi_app_stats[pc];
    ptr->taken += taken;
    ptr->total++;
    ptr->pattern.push_back(taken);
}

void bp_if::finish(std::string log_path) {
    for (auto& p : all_predictors) p->summarize_stats();
    active_bp->summarize_stats();
    show_stats(log_path);
}

void bp_if::show_stats(std::string log_path) {
    std::cout << "Branch stats: unique branches: " << bi_app_stats.size()
              << std::endl;
    // show all, but mark the active one (driving the icache)
    std::cout << bp_name << " (active: " << active_bp->type_name << ")"
              << std::endl;

    all_predictors.insert(all_predictors.begin(), std::move(active_bp));
    if (to_dump_csv) dump_csv(log_path);
    for (auto& p : all_predictors) p->show_stats();
    active_bp = std::move(all_predictors[0]); // restore active_bp
    all_predictors.erase(all_predictors.begin()); // remove invalid pointer
    return;
    std::cout << "  Predictors internal state:" << std::endl;
    for (auto& p : all_predictors) p->dump();
}

void bp_if::dump_csv(std::string log_path) {
    std::ofstream bcsv;
    bcsv.open(log_path + "branches.csv");
    bcsv << "PC,Direction,Taken,Not_Taken,All,Taken%";
    for (auto& p : all_predictors) bcsv << ",P_" << p->type_name;
    for (auto& p : all_predictors) bcsv << ",P_" << p->type_name << "%";
    //bcsv << ",P_Bimodal_idx,P_Local_idx";
    bcsv << ",Best,P_best,P_best%,Pattern" << std::endl;

    for (auto& [pc, stats] : bi_app_stats) {
        float_t branches_total = TO_F32(stats.total);
        size_t idx_best = 0; // defaults to first, the active_bp
        std::vector<uint32_t> pred;
        std::vector<float_t> acc;
        for (uint8_t i = 0; i < all_predictors.size(); i++) {
            pred.push_back(all_predictors[i]->get_predicted_stats(pc));
            acc.push_back((TO_F32(pred[i])/branches_total)*100);
            if (all_predictors[i].get() == ideal_bp) continue;
            if (pred[i] > pred[idx_best]) idx_best = i;
        }

        std::string best_name = all_predictors[idx_best]->type_name;
        uint32_t best_p = pred[idx_best];
        float_t best_acc = acc[idx_best];
        float_t taken_ratio = (TO_F32(stats.taken)/branches_total)*100;
        std::string pattern_str = find_run_length(stats.pattern);

        bcsv << std::hex << pc << std::dec
             << std::fixed << std::setprecision(1)
             << "," << (stats.dir == b_dir_t::forward ? "F" : "B")
             << "," << stats.taken
             << "," << stats.total - stats.taken
             << "," << stats.total
             << "," << taken_ratio;
        for (auto& p : pred) bcsv << "," << p;
        for (auto& a : acc) bcsv << "," << a;
        bcsv << "," << best_name
             << "," << best_p
             << "," << best_acc;
        //bcsv << "," << TO_U32(bimodal_bp.get_idx(pc))
        //     << "," << TO_U32(local_bp.get_idx(pc));
        bcsv << "," << pattern_str << std::endl;
    }
    bcsv.close();
}

std::string bp_if::find_run_length(const std::vector<bool>& pattern) {
    // summarize taken/not pattern, e.g. 1110011 makes a string "3T 2N 2T"
    std::string pattern_str;
    uint32_t count = 1;
    for (uint32_t i = 1; i < pattern.size(); i++) {
        if (pattern[i] == pattern[i - 1]) {
            count++;
        } else {
            pattern_str += std::to_string(count) +
                           (pattern[i - 1] ? "T " : "N ");
            count = 1;
        }
    }
    // add the direction of the last pattern
    pattern_str += std::to_string(count) + (pattern.back() ? "T" : "N");
    return pattern_str;
}

void bp_if::log_stats(std::ofstream& log_file) {
    active_bp->log_stats(bp_name, log_file);
}
