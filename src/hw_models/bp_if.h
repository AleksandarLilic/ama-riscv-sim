#pragma once

#include "defines.h"
#include "bp_static.h"
#include "bp_bimodal.h"
#include "bp_local.h"
#include "bp_global.h"
#include "bp_gselect.h"
#include "bp_gshare.h"
#include "bp_combined.h"
#include "bp_ideal.h"

class bp_if {
    private:
        std::string bp_name;
        bool prof_active = false;
        bp_t bp_active;
        bp_t bpc_1;
        bp_t bpc_2;
        bp_static static_bp;
        bp_bimodal bimodal_bp;
        bp_local local_bp;
        bp_global global_bp;
        bp_gselect gselect_bp;
        bp_gshare gshare_bp;
        bp_ideal ideal_bp;
        bp_combined combined_bp;
        static constexpr uint8_t num_predictors = TO_U8(bp_t::_count);
        std::array<bp*, num_predictors> predictors;
        std::map<uint32_t, bi_program_stats_t> bi_program_stats;

    public:
        bp_if() = delete;
        bp_if(std::string name, hw_cfg_t hw_cfg);
        void profiling(bool enable) { prof_active = enable; }
        uint32_t predict(uint32_t pc, int32_t offset);
        void update(uint32_t pc, uint32_t next_pc);
        void log_stats(std::ofstream& log_file);
        void finish(std::string log_path, uint64_t all_insts);
        void ideal(uint32_t correct_pc) { ideal_bp.goto_future(correct_pc); }

    private:
        void show_stats(std::string log_path);
        void update_stats(uint32_t pc, bool taken);

};
