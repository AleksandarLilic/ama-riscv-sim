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
        bp_ideal* bp_ideal_arch = nullptr;
        bool bp_ideal_is_active;
        std::vector<std::unique_ptr<bp>> all_bps;
        std::map<uint32_t, bi_app_stats_t> bi_app_stats;
        bool to_dump_csv = false;

    public:
        bp_if() = delete;
        bp_if(std::string name, hw_cfg_t hw_cfg);
        void profiling(bool enable) {
            prof_active = enable;
            active_bp->profiling(enable);
            for (auto& p : all_bps) p->profiling(enable);
        }
        uint32_t predict(uint32_t pc, int32_t offset, uint32_t funct3);
        void update(uint32_t pc, uint32_t next_pc);
        void log_stats(std::ofstream& log_file);
        void finish(std::string out_dir, uint64_t profiled_insts);
        void ideal(uint32_t correct_pc) {
            if (bp_ideal_arch) bp_ideal_arch->goto_future(correct_pc);
            if (bp_ideal_is_active) active_bp->goto_future(correct_pc);
        }
        // for predictor from cli args
        std::unique_ptr<bp> create_predictor(bp_t bp_type, hw_cfg_t hw_cfg);
        // for predefined predictors (wrapper only)
        std::unique_ptr<bp> create_predictor(bp_t bp_type, bp_cfg_t bp_cfg);
        // for predefined combined predictors
        std::unique_ptr<bp> create_predictor(std::array<bp_def_t, 3> bp_defs);

    private:
        void show_stats(std::string out_dir);
        void update_app_stats(uint32_t pc, bool taken);
        std::string find_run_length(const std::vector<bool>& pattern);
        void dump_csv(std::string out_dir);

    private:
        // predefined branch predictors
        // e.g. some good performing predictors that can be used as a starting
        // point for further evaluation or quick comparison with the cli bp
        // but without the need to go all out on the sweep flow
        // more should be added as needed

        #define FN bp_pc_folds_t::none
        #define FA bp_pc_folds_t::all
        #define STTC(x) TO_U8(bp_sttc_t::x)

        inline static const
        std::array<bp_def_t, 15> arch_bp_defs = {{
            // {pc_bits, cnt_bits, hist_bits, ghr_bits, pc_fold_bits, type_name}
            // statics
            {bp_t::sttc, bp_cfg_t{0, STTC(at), 0, 0, FN, "d_static_at"}},
            {bp_t::sttc, bp_cfg_t{0, STTC(ant), 0, 0, FN, "d_static_ant"}},
            {bp_t::sttc, bp_cfg_t{0, STTC(btfn), 0, 0, FN, "d_static_btfn"}},
            // all or nothing
            {bp_t::none, bp_cfg_t{0, 0, 0, 0, FN, "d_none"}},
            {bp_t::ideal, bp_cfg_t{0, 0, 0, 0, FN, "d_ideal_"}},
            // smaller
            {bp_t::bimodal, bp_cfg_t{7, 3, 0, 0, FN, "d_bimodal_v1"}},
            {bp_t::local, bp_cfg_t{5, 3, 5, 0, FN, "d_local_v1"}},
            {bp_t::global, bp_cfg_t{0, 3, 0, 7, FN, "d_global_v1"}},
            {bp_t::gselect, bp_cfg_t{1, 3, 0, 6, FN, "d_gselect_v1"}},
            {bp_t::gshare, bp_cfg_t{8, 1, 0, 8, FN, "d_gshare_v1"}},
            // a bit larger
            {bp_t::bimodal, bp_cfg_t{8, 3, 0, 0, FN, "d_bimodal_v2"}},
            {bp_t::local, bp_cfg_t{5, 3, 9, 0, FN, "d_local_v2"}},
            {bp_t::global, bp_cfg_t{0, 3, 0, 9, FN, "d_global_v2"}},
            {bp_t::gselect, bp_cfg_t{3, 3, 0, 6, FN, "d_gselect_v2"}},
            {bp_t::gshare, bp_cfg_t{8, 3, 0, 8, FN, "d_gshare_v2"}},
        }};

        inline static const
        std::array<std::array<bp_def_t, 3>, 3> arch_bpc_defs = {{
        // {pc_bits, cnt_bits, hist_bits, ghr_bits, pc_fold_bits, type_name}
        {{
            // combined predictor 1
            {bp_t::combined, bp_cfg_t{4, 4, 0, 0, FN, "d_combined_v1"}},
            {bp_t::sttc, bp_cfg_t{0, 0, 0, 0, FN, "d_static_cv1"}},
            {bp_t::gshare, bp_cfg_t{8, 1, 0, 8, FN, "d_gshare_cv1"}},
        }},
        {{
            // combined predictor 2
            {bp_t::combined, bp_cfg_t{5, 4, 0, 0, FN, "d_combined_v2"}},
            {bp_t::sttc, bp_cfg_t{0, 0, 0, 0, FN, "d_static_cv2"}},
            {bp_t::gshare, bp_cfg_t{8, 3, 0, 8, FN, "d_gshare_cv2"}},
        }},
        {{
            // combined predictor 3
            {bp_t::combined, bp_cfg_t{4, 4, 0, 0, FN, "d_combined_v3"}},
            {bp_t::sttc, bp_cfg_t{STTC(btfn), 0, 0, 0, FN, "d_static_cv3"}},
            {bp_t::gselect, bp_cfg_t{2, 1, 0, 6, FN, "d_gselect_cv3"}},
        }},
        }};
};
