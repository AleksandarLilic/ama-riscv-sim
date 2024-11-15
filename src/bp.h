#pragma once

#include "defines.h"
#include "bp_static.h"
#include "bp_bimodal.h"

class bp {
    private:
        std::string bp_name;
        bp_t bp_active;
        b_dir_t dir;
        uint32_t target_pc;
        bool taken;
        bool prof_active = false;
        bp_static static_bp;
        bp_bimodal bimodal_bp;

    public:
        bp() = delete;
        bp(std::string name, bp_t bp_type);
        void profiling(bool enable) { prof_active = enable; }
        uint32_t predict(uint32_t pc, int32_t offset);
        void update(uint32_t current_pc, bool taken);
        void log_stats(std::ofstream& log_file);
        void finish();
    private:
        void show_stats();
};
