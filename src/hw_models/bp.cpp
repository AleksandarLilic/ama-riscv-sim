#include "bp.h"

bp::bp(std::string name, bp_t bp_type) :
    bp_name(name), bp_active(bp_type),
    static_bp("static"),
    bimodal_bp("bimodal", BP_BIMODAL_CNT_BITS),
    local_bp("local", BP_LOCAL_HIST_BITS, BP_LOCAL_CNT_BITS) {}

uint32_t bp::predict(uint32_t pc, int32_t offset) {
    target_pc = TO_I32(pc) + offset;
    dir = (target_pc > pc) ? b_dir_t::forward : b_dir_t::backward;
    if (prof_active) {
        if (bi_program_stats.find(pc) == bi_program_stats.end()) {
            bi_program_stats[pc] = {dir, 0, 0, {}};
        }
    }

    uint32_t static_predicted_pc = static_bp.predict(target_pc, pc);
    uint32_t bimodal_predicted_pc = bimodal_bp.predict(target_pc, pc);
    uint32_t local_predicted_pc = local_bp.predict(target_pc, pc);

    if (bp_active == bp_t::sttc) return static_predicted_pc;
    else if (bp_active == bp_t::bimodal) return bimodal_predicted_pc;
    else if (bp_active == bp_t::local) return local_predicted_pc;
    else throw std::runtime_error("Unknown branch predictor type");
}

void bp::update(uint32_t pc, uint32_t next_pc) {
    bool taken = next_pc != pc + 4;
    // internal states of all dynamic predictors are always updated
    bimodal_bp.update(taken);
    local_bp.update(taken);
    // but only update stats if profiling is active
    if (!prof_active) return;
    update_stats(pc, taken);
    static_bp.update_stats(pc, next_pc);
    bimodal_bp.update_stats(pc, next_pc);
    local_bp.update_stats(pc, next_pc);
}

void bp::update_stats(uint32_t pc, bool taken) {
    bi_program_stats_t* ptr = &bi_program_stats[pc];
    ptr->taken += taken;
    ptr->total++;
    ptr->pattern.push_back(taken);
}

void bp::finish(std::string log_path, uint64_t all_insts) {
    static_bp.stats.summarize(all_insts);
    bimodal_bp.stats.summarize(all_insts);
    local_bp.stats.summarize(all_insts);
    show_stats(log_path);
}

void bp::show_stats(std::string log_path) {
    // find n as a largest number of digits - for alignment in stdout
    int32_t n = 0;
    uint32_t branches = 0;
    for (auto& [pc, stats] : bi_program_stats) {
        n = std::max(n, TO_I32(std::to_string(stats.total).size()));
        branches += 1;
    }

    // print global stats
    std::cout << "Program branch stats: unique branches: " << branches
              << std::endl;

    // open file for write
    std::ofstream bcsv;
    bcsv.open(log_path + "branches.csv");

    bcsv << "PC,Direction,Taken,Not_Taken,All,Taken%"
         << ",P_Static,P_Bimodal,P_Bimodal_idx,P_Local,P_Local_idx"
         << ",P_Static%,P_Bimodal%,P_Local%"
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
        uint32_t predicted_static = static_bp.stats.get_predicted(pc);
        uint32_t predicted_bimodal = bimodal_bp.stats.get_predicted(pc);
        uint32_t predicted_local = local_bp.stats.get_predicted(pc);
        float_t acc_static =
            (TO_F32(predicted_static) / TO_F32(stats.total)) * 100;
        float_t acc_bimodal =
            (TO_F32(predicted_bimodal) / TO_F32(stats.total)) * 100;
        float_t acc_local =
            (TO_F32(predicted_local) / TO_F32(stats.total)) * 100;
        bcsv << std::hex << pc << std::dec
             << std::fixed << std::setprecision(0)
             << "," << (stats.dir == b_dir_t::forward ? "F" : "B")
             << "," << stats.taken
             << "," << stats.total - stats.taken
             << "," << stats.total
             << "," << taken_ratio
             << "," << predicted_static
             << "," << predicted_bimodal
             << "," << TO_U32(bimodal_bp.get_idx(pc))
             << "," << predicted_local
             << "," << TO_U32(local_bp.get_idx(pc))
             << "," << acc_static
             << "," << acc_bimodal
             << "," << acc_local
             << "," << pattern_str
             << std::endl;
    }

    // show all, but mark the active one (driving the icache)
    std::cout << bp_name << " (active: ";
    if (bp_active == bp_t::sttc) std::cout << static_bp.type_name;
    else if (bp_active == bp_t::bimodal) std::cout << bimodal_bp.type_name;
    else if (bp_active == bp_t::local) std::cout << local_bp.type_name;
    std::cout << ")" << std::endl;

    // TODO: predictor size
    static_bp.stats.show("static");
    bimodal_bp.stats.show("bimodal");
    local_bp.stats.show("local");
    return;
    std::cout << "  Predictors internal state:" << std::endl;
    bimodal_bp.dump();
    local_bp.dump();
}

void bp::log_stats(std::ofstream& log_file) {
    if (bp_active == bp_t::sttc) {
        static_bp.stats.log(bp_name, log_file);
    } else if (bp_active == bp_t::bimodal) {
        bimodal_bp.stats.log(bp_name, log_file);
    } else if (bp_active == bp_t::local) {
        local_bp.stats.log(bp_name, log_file);
    }
}
