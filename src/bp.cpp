#include "bp.h"

uint32_t bp::predict(uint32_t pc, int32_t offset) {
    uint32_t taken_pc = TO_I32(pc) + offset;
    dir = (taken_pc > pc) ? b_dir_t::forward : b_dir_t::backward;
    if (dir == b_dir_t::forward) return pc + 4; // forward not taken
    else return taken_pc; // backward taken
}

void bp::update(bool correct) {
    // TODO: for dynamic pred. internal state should be updated regardless
    if (!prof_active) return; // only update if profiling is active
    stats.eval(correct, dir);
}

void bp::show_stats() {
    // dynamic predictors will have size
    std::cout << bp_name
              << " (s: " << 0
              << " B): ";
    stats.show();
    // TODO: internal state for dynamic predictors
    //std::cout << std::endl;
}

void bp::log_stats(std::ofstream& log_file) {
    stats.log(bp_name, log_file);
}
