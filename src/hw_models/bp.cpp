#include "bp.h"

bp::bp(std::string name, bp_t bp_type) :
    bp_name(name), bp_active(bp_type),
    static_bp("static"),
    bimodal_bp("bimodal", BP_BIMODAL_CNT_BITS) { }

uint32_t bp::predict(uint32_t pc, int32_t offset) {
    target_pc = TO_I32(pc) + offset;
    dir = (target_pc > pc) ? b_dir_t::forward : b_dir_t::backward;

    uint32_t static_predicted_pc = static_bp.predict(target_pc, pc);
    uint32_t bimodal_predicted_pc = bimodal_bp.predict(target_pc, pc);

    if (bp_active == bp_t::sttc) return static_predicted_pc;
    else if (bp_active == bp_t::bimodal) return bimodal_predicted_pc;
    else throw std::runtime_error("Unknown branch predictor type");
}

void bp::update(uint32_t current_pc, bool taken) {
    // internal states of all dynamic predictors are always updated
    bimodal_bp.update(target_pc, taken);
    // only update stats if profiling is active
    if (!prof_active) return;
    static_bp.update_stats(current_pc);
    bimodal_bp.update_stats(current_pc);
}

void bp::finish(uint64_t all_insts) {
    static_bp.stats.summarize(all_insts);
    bimodal_bp.stats.summarize(all_insts);
    show_stats();
}

void bp::show_stats() {
    // show all, but mark the active one (driving the icache)
    std::cout << bp_name << " (active: ";
    if (bp_active == bp_t::sttc) std::cout << static_bp.type_name;
    else if (bp_active == bp_t::bimodal) std::cout << bimodal_bp.type_name;
    std::cout << ")" << std::endl;
    // TODO: predictor size
    static_bp.stats.show("static");
    bimodal_bp.stats.show("bimodal");
    return;
    // TODO: optionally dump internal state for dynamic predictors
    //std::cout << std::endl;
}

void bp::log_stats(std::ofstream& log_file) {
    if (bp_active == bp_t::sttc) {
        static_bp.stats.log(bp_name, log_file);
    } else if (bp_active == bp_t::bimodal) {
        bimodal_bp.stats.log(bp_name, log_file);
    }
}
