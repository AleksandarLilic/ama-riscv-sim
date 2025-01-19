#pragma once

#include "defines.h"
#include "bp.h"
#include "bp_static.h"
#include "bp_bimodal.h"
#include "bp_local.h"
#include "bp_global.h"
#include "bp_gselect.h"
#include "bp_gshare.h"
#include "bp_combined.h"
#include "bp_ideal.h"
#include "bp_none.h"

struct bp_def_t {
    const bp_t type;
    const bp_cfg_t cfg;
};

class bp_if {
    private:
        std::string bp_name;
        bool prof_active = false;
        bp_t bp_active_type;
        bp_t bp_combined_p1_type;
        bp_t bp_combined_p2_type;
        bool bp_run_all;
        std::unique_ptr<bp> active_bp;
        bp_ideal* ideal_bp;
        std::vector<std::unique_ptr<bp>> all_predictors;
        std::map<uint32_t, bi_app_stats_t> bi_app_stats;
        bool to_dump_csv = false;

    public:
        bp_if() = delete;
        bp_if(std::string name, hw_cfg_t hw_cfg);
        void profiling(bool enable) { prof_active = enable; }
        uint32_t predict(uint32_t pc, int32_t offset);
        void update(uint32_t pc, uint32_t next_pc);
        void log_stats(std::ofstream& log_file);
        void finish(std::string log_path);
        void ideal(uint32_t correct_pc) {
            if (ideal_bp) ideal_bp->goto_future(correct_pc);
            if (bp_active_type == bp_t::ideal) active_bp->goto_future(correct_pc);
        }
        // for predictor from cli args
        std::unique_ptr<bp> create_predictor(bp_t bp_type, hw_cfg_t hw_cfg);
        // for predefined predictors (wrapper only)
        std::unique_ptr<bp> create_predictor(bp_t bp_type, bp_cfg_t bp_cfg);
        // for predefined combined predictors
        std::unique_ptr<bp> create_predictor(std::array<bp_def_t, 3> bp_defs);

    private:
        void show_stats(std::string log_path);
        void update_app_stats(uint32_t pc, bool taken);
        std::string find_run_length(const std::vector<bool>& pattern);
        void dump_csv(std::string log_path);

    private:
        // predefined branch predictors
        // { pc_bits, cnt_bits, hist_bits, gr_bits, type_name }
        static constexpr std::array<bp_def_t, 13> arch_bp_defs = {{
            {bp_t::sttc, bp_cfg_t{ 0, 0, 0, 0, "d_static" }},
            {bp_t::none, bp_cfg_t{ 0, 0, 0, 0, "d_none" }},
            {bp_t::ideal, bp_cfg_t{ 0, 0, 0, 0, "d_ideal_" }},
            {bp_t::bimodal, bp_cfg_t{ 7, 3, 0, 0, "d_bimodal_v1" }},
            {bp_t::local, bp_cfg_t{ 5, 3, 5, 0, "d_local_v1" }},
            {bp_t::global, bp_cfg_t{ 0, 3, 0, 7, "d_global_v1" }},
            {bp_t::gselect, bp_cfg_t{ 1, 3, 0, 6, "d_gselect_v1" }},
            {bp_t::gshare, bp_cfg_t{ 8, 1, 0, 8, "d_gshare_v1" }},
            {bp_t::bimodal, bp_cfg_t{ 8, 3, 0, 0, "d_bimodal_v2" }},
            {bp_t::local, bp_cfg_t{ 5, 3, 9, 0, "d_local_v2" }},
            {bp_t::global, bp_cfg_t{ 0, 3, 0, 9, "d_global_v2" }},
            {bp_t::gselect, bp_cfg_t{ 3, 3, 0, 6, "d_gselect_v2" }},
            {bp_t::gshare, bp_cfg_t{ 8, 3, 0, 8, "d_gshare_v2" }},
        }};

        static constexpr std::array<std::array<bp_def_t, 3>, 2> arch_bpc_defs =
        {{
            // { pc_bits, cnt_bits, hist_bits, gr_bits, type_name }
            {{
                {bp_t::combined, bp_cfg_t{ 4, 4, 0, 0, "d_combined_v1" }},
                {bp_t::sttc, bp_cfg_t{ 0, 0, 0, 0, "d_static_cv1" }},
                {bp_t::gshare, bp_cfg_t{ 8, 1, 0, 8, "d_gshare_cv1" }},
            }},
            {{
                {bp_t::combined, bp_cfg_t{ 5, 4, 0, 0, "d_combined_v2" }},
                {bp_t::sttc, bp_cfg_t{ 0, 0, 0, 0, "d_static_cv2" }},
                {bp_t::gshare, bp_cfg_t{ 8, 3, 0, 8, "d_gshare_cv2" }},
            }},
        }};

};
