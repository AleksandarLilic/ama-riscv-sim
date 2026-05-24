
#include <fstream>

#include "profiler_rf.h"

void profiler_rf::add_te() {
    if (!active) return;
    trace.push_back(te);
    te.rst();
}

void profiler_rf::log_reg_use(reg_use_t reg_use, uint8_t reg) {
    if (active && rf_usage_en) prof_rf_usage[reg][TO_U8(reg_use)]++;
}

void profiler_rf::finish(const std::string& out_dir) {
    if (trace_en) {
        std::ofstream ofs(out_dir + "rf_trace.bin", std::ios::binary);
        ofs.write(reinterpret_cast<const char*>(trace.data()),
                  static_cast<std::streamsize>(trace.size() * sizeof(rf_trace_entry)));
    }
    if (rf_usage_en) {
        std::ofstream ofs(out_dir + "rf_usage.bin", std::ios::binary);
        ofs.write(
            reinterpret_cast<const char*>(prof_rf_usage.data()),
            static_cast<std::streamsize>(
                prof_rf_usage.size() * // num of regs
                prof_rf_usage[0].size() * // num of options for each reg
                sizeof(prof_rf_usage[0][0])) // counter width
        );
    }
}
