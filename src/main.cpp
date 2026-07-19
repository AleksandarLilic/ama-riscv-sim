#include <algorithm>
#include <chrono>
#include <filesystem>
#include <set>
#include <utility>
#include <vector>

#include "defines.h"
#include "cxxopts.hpp"
#include "memory.h"
#include "core.h"

#include "arg_parse.h"
#include "utils.h"
#include "build_info.h"

#ifdef TEST_BUILD
#define TRY_CATCH(x) \
    try { x; } \
    catch (const std::exception &e) { \
        std::cerr << "Error: " << e.what() << std::endl; \
        return 1; \
    }
#else
#define TRY_CATCH(x) x
#endif

#define CXXOPTS_VAL_STR cxxopts::value<std::string>()
#define CXXOPTS_VAL_BOOL cxxopts::value<bool>()

#define ARG_U32H(x) TO_U32(std::stoul(x.as<std::string>(), nullptr, 16))
#define ARG_UL(x) std::stoul(x.as<std::string>(), nullptr, 10)
#define ARG_I8(x) TO_U8(ARG_UL(x))
#define ARG_U32(x) TO_U32(ARG_UL(x))
#define ARG_U64(x) TO_U64(ARG_UL(x))
#define ARG_BOOL(x) x.as<bool>()

#define RESOLVE_ARG(str, map) \
    resolve_arg(str, result[str].as<std::string>(), map)
#define RESOLVE_ARG_LIST(str, map) \
    resolve_arg_list(str, result[str].as<std::vector<std::string>>(), map)

inline std::string saved_as(const std::string& s) {
    return "Saved as '" + s + "' under run directory";
}

// args maps
// available rf names
const ordered_map<rf_names_t> rf_names_map = {
    {"abi", rf_names_t::mode_abi},
    {"x", rf_names_t::mode_x}
};

const ordered_map<perf_event_t> perf_event_map = {
    // ==== PERF_EVENT AUTOGEN BEGIN ====
    {"ret_inst", perf_event_t::ret_inst},
    {"ret_ctrl_flow", perf_event_t::ret_ctrl_flow},
    {"ret_ctrl_flow_jr", perf_event_t::ret_ctrl_flow_jr},
    {"ret_ctrl_flow_br", perf_event_t::ret_ctrl_flow_br},
    {"ret_mem", perf_event_t::ret_mem},
    {"ret_mem_load", perf_event_t::ret_mem_load},
    {"ret_mul", perf_event_t::ret_mul},
    {"ret_div", perf_event_t::ret_div},
    {"ret_simd", perf_event_t::ret_simd},
    {"ret_simd_arith", perf_event_t::ret_simd_arith},
    {"ret_simd_arith_dot", perf_event_t::ret_simd_arith_dot},
    #ifdef HW_MODELS_EN
    {"bp_miss", perf_event_t::bp_miss},
    {"l1i_ref", perf_event_t::l1i_ref},
    {"l1i_miss", perf_event_t::l1i_miss},
    {"l1i_spec_miss", perf_event_t::l1i_spec_miss},
    {"l1i_spec_miss_bad", perf_event_t::l1i_spec_miss_bad},
    {"l1d_ref", perf_event_t::l1d_ref},
    {"l1d_ref_r", perf_event_t::l1d_ref_r},
    {"l1d_miss", perf_event_t::l1d_miss},
    {"l1d_miss_r", perf_event_t::l1d_miss_r},
    {"l1d_writeback", perf_event_t::l1d_writeback},
    #endif
    // ==== PERF_EVENT AUTOGEN END ====

    // perf muscle memory aliases
    {"instructions", perf_event_t::ret_inst},
    {"branches", perf_event_t::ret_ctrl_flow_br},
    #ifdef HW_MODELS_EN
    {"l1-icache-load-references", perf_event_t::l1i_ref},
    {"l1-icache-load-misses", perf_event_t::l1i_miss},
    {"l1-dcache-references", perf_event_t::l1d_ref},
    {"l1-dcache-misses", perf_event_t::l1d_miss},
    {"branch-misses", perf_event_t::bp_miss}
    #endif
};

#ifdef HW_MODELS_EN
const ordered_map<cache_re_policy_t> cache_re_policy_map = {
    {"lru", cache_re_policy_t::lru}
};

const ordered_map<cache_in_policy_t> cache_in_policy_map = {
    {"update", cache_in_policy_t::update},
    {"no_update", cache_in_policy_t::no_update}
};

const ordered_map<cache_wr_policy_t> cache_wr_policy_map = {
    {"wt", cache_wr_policy_t::wt},
    {"wb", cache_wr_policy_t::wb}
};

const ordered_map<bp_sttc_t> bp_sttc_map = {
    {"at", bp_sttc_t::at},
    {"ant", bp_sttc_t::ant},
    {"btfn", bp_sttc_t::btfn}
};

const ordered_map<bp_t> bp_names_map = {
    {"static", bp_t::sttc},
    {"bimodal", bp_t::bimodal},
    {"local", bp_t::local},
    {"global", bp_t::global},
    {"gselect", bp_t::gselect},
    {"gshare", bp_t::gshare},
    {"ideal", bp_t::ideal},
    {"none", bp_t::none},
    //{"combined", bp_t::combined}
};

const ordered_map<bp_pc_folds_t> bp_pc_folds_map = {
    {"none", bp_pc_folds_t::none},
    {"all", bp_pc_folds_t::all}
};

#endif

// defaults
struct defs_t {
    static constexpr char show_state[] = "false";
    static constexpr char exit_on_trap[] = "false";
    static constexpr char mem_dump_start[] = "0";
    static constexpr char mem_dump_size[] = "0";
    static constexpr char run_insts[] = "0";
    static constexpr char run_steps[] = "0";
    #ifdef UART_EN
    static constexpr char uart_show[] = "false";
    #ifdef UART_INPUT_EN
    static constexpr char uart_in[] = "";
    #endif
    #endif
    #ifdef PROFILERS_EN
    static constexpr char prof_pc_start[] = "0";
    static constexpr char prof_pc_stop[] = "0";
    static constexpr char prof_pc_sm[] = "0";
    static constexpr char exit_on_prof_stop[] = "false";
    static constexpr char prof_trace[] = "false";
    static constexpr char perf_event[] = "ret_inst";
    static constexpr char rf_usage[] = "false";
    static constexpr char no_callstack[] = "false";
    static constexpr char prof_show[] = "false";
    #endif
    #ifdef DASM_EN
    static constexpr char log[] = "false";
    #ifdef PROFILERS_EN
    static constexpr char log_always[] = "false";
    #endif
    static constexpr char log_state[] = "false";
    static constexpr char rf_names[] = "x";
    static constexpr char log_hw_models[] = "false";
    #endif
};

#ifdef HW_MODELS_EN
struct hw_defs_t {
    // icache
    static constexpr char icache_sets[] = "32";
    static constexpr char icache_ways[] = "2";
    static constexpr char icache_re_policy[] = "lru";
    static constexpr char icache_in_policy[] = "update";
    // dcache
    static constexpr char dcache_sets[] = "16";
    static constexpr char dcache_ways[] = "4";
    static constexpr char dcache_re_policy[] = "lru";
    static constexpr char dcache_in_policy[] = "update";
    static constexpr char dcache_wr_policy[] = "wb";
    // caches other configs
    static constexpr char roi_start[] = "0";
    static constexpr char roi_size[] = "0";
    static constexpr char show_cache_state[] = "false";
    static constexpr char div_cache_entries[] = "1";
    // branch predictors
    static constexpr char bp[] = "bimodal";
    static constexpr char bp2[] = "none"; // global
    // supported predictors configurations
    static constexpr char bp_static_method[] = "btfn";
    static constexpr char bp_pc_bits[] = "7";
    static constexpr char bp_cnt_bits[] = "3";
    static constexpr char bp_lhist_bits[] = "5";
    static constexpr char bp_ghr_bits[] = "5";
    static constexpr char bp_fold_pc[] = "none";
    static constexpr char bp2_static_method[] = "at";
    static constexpr char bp2_pc_bits[] = "5";
    static constexpr char bp2_cnt_bits[] = "1";
    static constexpr char bp2_lhist_bits[] = "5";
    static constexpr char bp2_ghr_bits[] = "8";
    static constexpr char bp2_fold_pc[] = "none";
    static constexpr char bp_combined_pc_bits[] = "6";
    static constexpr char bp_combined_cnt_bits[] = "4";
    static constexpr char bp_combined_fold_pc[] = "none";
    // bp other configs
    static constexpr char bp_run_all[] = "false";
    static constexpr char bp_dump_csv[] = "false";
};
#endif

void show_help(const cxxopts::Options& options) {
    std::cout << options.help() << std::endl;
}

int main(int argc, char* argv[]) {
    cfg_t cfg;
    std::string test_elf;
    std::string out_dir_tag;
    cxxopts::Options options(argv[0], "ama-riscv-sim");
    options.set_width(116);
    hw_cfg_t hw_cfg;

    options.add_options()
        ("p,path", "Path to the ELF file to load", CXXOPTS_VAL_STR)
        ("show_state", "Show architectural state at the end of simulation",
         CXXOPTS_VAL_BOOL->default_value(defs_t::show_state))
        ("exit_on_trap",
         "Exit sim on trap instead of going to trap handler",
         CXXOPTS_VAL_BOOL->default_value(defs_t::exit_on_trap))
        ("mem_dump_start",
         "Start address (hex) for memory dump at the end of simulation",
         CXXOPTS_VAL_STR->default_value(defs_t::mem_dump_start))
        ("mem_dump_size",
         "Size of the region for memory dump at the end of simulation",
         CXXOPTS_VAL_STR->default_value(defs_t::mem_dump_size))
        ("out_dir_tag", "Tag (suffix) for output directory",
         CXXOPTS_VAL_STR->default_value(""))
        ("run_insts",
         "Number of instructions to execute. Set to 0 for no limit",
         CXXOPTS_VAL_STR->default_value(defs_t::run_insts))
        ("run_steps",
         "Number of steps (inst & wfi period) to run. Set to 0 for no limit",
         CXXOPTS_VAL_STR->default_value(defs_t::run_steps))
        #ifdef UART_EN
        ("uart_show",
         "Print UART output to stdout. UART still fully operational. "
         "Log always kept. " + saved_as("uart.log"),
         CXXOPTS_VAL_BOOL->default_value(defs_t::uart_show))
        #ifdef UART_INPUT_EN
        ("uart_in",
         "UART RX input bytes, drained at the baud-rate instruction stride "
         "(e.g. --uart_in \"$(cat in.txt)\"). If omitted, reads stdin non-blocking",
         CXXOPTS_VAL_STR->default_value(defs_t::uart_in))
        #endif
        #endif
        ;

    #ifdef PROFILERS_EN
    options.add_options("Profiler")
        ("prof_pc_start", "Start PC (hex) for profiling",
         CXXOPTS_VAL_STR->default_value(defs_t::prof_pc_start))
        ("prof_pc_stop", "Stop PC (hex) for profiling",
         CXXOPTS_VAL_STR->default_value(defs_t::prof_pc_stop))
        ("prof_pc_single_match",
         "Run profiling only for match number (0 for all matches)",
         CXXOPTS_VAL_STR->default_value(defs_t::prof_pc_sm))
        ("exit_on_prof_stop",
         "Exit simulation immediately after profiling finishes. "
         "Exits on the first 'prof_pc_stop' if 'prof_pc_single_match' is not "
         "set, otherwise exits on the 'prof_pc_stop' that matched "
         "'prof_pc_single_match' count",
         CXXOPTS_VAL_BOOL->default_value(defs_t::exit_on_prof_stop))
        ("t,prof_trace",
         "Record profiler traces. " + saved_as("trace.bin/rf_trace.bin"),
         CXXOPTS_VAL_BOOL->default_value(defs_t::prof_trace))
        ("e,perf_event",
         "Performance event(s) to track, comma-separated. Options: " +
         gen_help_list(perf_event_map),
         cxxopts::value<std::vector<std::string>>()
            ->default_value(defs_t::perf_event))
        ("rf_usage",
         "Enable profiling register file usage. " + saved_as("rf_usage.bin"),
         CXXOPTS_VAL_BOOL->default_value(defs_t::rf_usage))
        ("no_callstack", "Disable callstack tracing",
         CXXOPTS_VAL_BOOL->default_value(defs_t::no_callstack))
        ("prof_show",
         "Show profiler stats to stdout at the end of sim. "
         "Logs and traces always saved",
         CXXOPTS_VAL_BOOL->default_value(defs_t::prof_show));
    #endif

    #ifdef DASM_EN
    options.add_options("Logging")
        ("l,log",
         "Enable logging of each executed instrucion. " + saved_as("exec.log"),
         CXXOPTS_VAL_BOOL->default_value(defs_t::log))
        #ifdef PROFILERS_EN
        ("log_always",
         "Always log execution. Otherwise, log during profiling only",
         CXXOPTS_VAL_BOOL->default_value(defs_t::log_always))
        #endif
        ("log_state", "Log state after each executed instruction",
         CXXOPTS_VAL_BOOL->default_value(defs_t::log_state))
        ("rf_names",
         "Register file names used for output. Options: " +
         gen_help_list(rf_names_map),
         CXXOPTS_VAL_STR->default_value(defs_t::rf_names))
        #ifdef HW_MODELS_EN
        ("log_hw_models", "Log HW model stats for each executed instruction",
         CXXOPTS_VAL_BOOL->default_value(defs_t::log_hw_models))
        #endif
        ;
    #endif

    #ifdef HW_MODELS_EN
    options.add_options("HW model - Caches")
        // icache
        ("icache_sets", "Number of sets in I$",
         CXXOPTS_VAL_STR->default_value(hw_defs_t::icache_sets))
        ("icache_ways", "Number of ways in I$",
         CXXOPTS_VAL_STR->default_value(hw_defs_t::icache_ways))
        ("icache_re_policy", "I$ replacement policy. \nOptions: " +
         gen_help_list(cache_re_policy_map),
         CXXOPTS_VAL_STR->default_value(hw_defs_t::icache_re_policy))
        ("icache_in_policy", "I$ insertion policy. \nOptions: " +
         gen_help_list(cache_in_policy_map),
         CXXOPTS_VAL_STR->default_value(hw_defs_t::icache_in_policy))
        // dcache
        ("dcache_sets", "Number of sets in D$",
         CXXOPTS_VAL_STR->default_value(hw_defs_t::dcache_sets))
        ("dcache_ways", "Number of ways in D$",
         CXXOPTS_VAL_STR->default_value(hw_defs_t::dcache_ways))
        ("dcache_re_policy", "D$ replacement policy. \nOptions: " +
         gen_help_list(cache_re_policy_map),
         CXXOPTS_VAL_STR->default_value(hw_defs_t::dcache_re_policy))
        ("dcache_in_policy", "D$ insertion policy. \nOptions: " +
         gen_help_list(cache_in_policy_map),
         CXXOPTS_VAL_STR->default_value(hw_defs_t::dcache_in_policy))
        ("dcache_wr_policy", "D$ write policy. \nOptions: " +
         gen_help_list(cache_wr_policy_map),
         CXXOPTS_VAL_STR->default_value(hw_defs_t::dcache_wr_policy))
        // caches other configs
        ("roi_start", "Region of interest start address (hex)",
         CXXOPTS_VAL_STR->default_value(hw_defs_t::roi_start))
        ("roi_size", "Region of interest size",
         CXXOPTS_VAL_STR->default_value(hw_defs_t::roi_size))
        ("show_cache_state",
         "Show per cache line references at the end of simulation",
         CXXOPTS_VAL_BOOL->default_value(hw_defs_t::show_cache_state));

    options.add_options("HW model - Branch Predictor")
        ("bp",
         "First branch predictor. Defaults as active, driving the I$. "
         "For combined predictor, counters will be weakly biased towards this "
         "predictor (impacts warm-up period)\n"
         "Options: " + gen_help_list(bp_names_map) + "\n ",
         CXXOPTS_VAL_STR->default_value(hw_defs_t::bp))
        ("bp2",
         "Second branch predictor, only for combined predictor. If provided, "
         "combined predictor becomes active\n"
         "Options: " + gen_help_list(bp_names_map) + "\n ",
         CXXOPTS_VAL_STR->default_value(hw_defs_t::bp2))

        // supported predictors configurations
        ("bp_static_method",
         "Static predictor - method. \nOptions: " +
         gen_help_list(bp_sttc_map),
         CXXOPTS_VAL_STR->default_value(hw_defs_t::bp_static_method))
        ("bp_pc_bits", "Branch predictor - PC bits",
         CXXOPTS_VAL_STR->default_value(hw_defs_t::bp_pc_bits))
        ("bp_cnt_bits", "Branch predictor - counter bits",
         CXXOPTS_VAL_STR->default_value(hw_defs_t::bp_cnt_bits))
        ("bp_lhist_bits", "Branch predictor - local history bits",
         CXXOPTS_VAL_STR->default_value(hw_defs_t::bp_lhist_bits))
        ("bp_ghr_bits", "Branch predictor - global register bits",
         CXXOPTS_VAL_STR->default_value(hw_defs_t::bp_ghr_bits))
        ("bp_fold_pc",
         "Branch predictor - Fold higher order PC bits for indexing"
         "\nOptions: " +
         gen_help_list(bp_pc_folds_map),
         CXXOPTS_VAL_STR->default_value(hw_defs_t::bp_fold_pc))

        ("bp2_static_method",
         "Static predictor - method. \nOptions: " +
         gen_help_list(bp_sttc_map),
         CXXOPTS_VAL_STR->default_value(hw_defs_t::bp2_static_method))
        ("bp2_pc_bits", "Branch predictor 2 - PC bits",
         CXXOPTS_VAL_STR->default_value(hw_defs_t::bp2_pc_bits))
        ("bp2_cnt_bits", "Branch predictor 2 - counter bits",
         CXXOPTS_VAL_STR->default_value(hw_defs_t::bp2_cnt_bits))
        ("bp2_lhist_bits", "Branch predictor 2 - local history bits",
         CXXOPTS_VAL_STR->default_value(hw_defs_t::bp2_lhist_bits))
        ("bp2_ghr_bits", "Branch predictor 2 - global register bits",
         CXXOPTS_VAL_STR->default_value(hw_defs_t::bp2_ghr_bits))
        ("bp2_fold_pc",
         "Branch predictor 2 - Fold higher order PC bits for indexing"
         "\nOptions: " +
         gen_help_list(bp_pc_folds_map),
         CXXOPTS_VAL_STR->default_value(hw_defs_t::bp2_fold_pc))

        ("bp_combined_pc_bits", "Combined predictor - PC bits",
         CXXOPTS_VAL_STR->default_value(hw_defs_t::bp_combined_pc_bits))
        ("bp_combined_cnt_bits", "Combined predictor - counter bits",
         CXXOPTS_VAL_STR->default_value(hw_defs_t::bp_combined_cnt_bits))
        ("bp_combined_fold_pc",
         "Combined predictor - Fold higher order PC bits for indexing"
         "\nOptions: " +
         gen_help_list(bp_pc_folds_map),
         CXXOPTS_VAL_STR->default_value(hw_defs_t::bp_combined_fold_pc))

        // bp other configs
        ("bp_run_all", "Run all branch predictors",
         CXXOPTS_VAL_BOOL->default_value(hw_defs_t::bp_run_all))
        ("bp_dump_csv", "Dump branch predictor stats to CSV",
         CXXOPTS_VAL_BOOL->default_value(hw_defs_t::bp_dump_csv));

    options.add_options("HW model - Divider")
        ("div_cache_entries",
         "Number of entries in divider result cache",
         CXXOPTS_VAL_STR->default_value(hw_defs_t::div_cache_entries));

    #endif

    options.add_options("Help")
        ("h,help", "Print usage")
        ("v,version", "Print version/build info and exit");

    options.parse_positional({"path"});
    options.positional_help("");
    options.custom_help("<path_to_elf_file> [OPTION...]");

    cxxopts::ParseResult result;
    try {
        result = options.parse(argc, argv);
    } catch (const cxxopts::exceptions::missing_argument& e) {
        std::cerr << e.what() << std::endl;
        show_help(options);
        return 1;
    } catch (const cxxopts::exceptions::no_such_option& e) {
        std::cerr << e.what() << std::endl;
        show_help(options);
        return 1;
    }

    if (result.count("help")) {
        show_help(options);
        return 0;
    }

    if (result.count("version")) {
        std::cout << build_info::as_text();
        return 0;
    }

    try {
        test_elf = result["path"].as<std::string>();
        cfg.show_state = ARG_BOOL(result["show_state"]);
        cfg.exit_on_trap = ARG_BOOL(result["exit_on_trap"]);
        cfg.mem_dump_start = ARG_U32H(result["mem_dump_start"]);
        cfg.mem_dump_size = ARG_U32(result["mem_dump_size"]);
        cfg.run_insts = ARG_U64(result["run_insts"]);
        cfg.run_steps = ARG_U64(result["run_steps"]);
        out_dir_tag = result["out_dir_tag"].as<std::string>();
        #ifdef UART_EN
        cfg.uart_show = ARG_BOOL(result["uart_show"]);
        #ifdef UART_INPUT_EN
        cfg.uart_in = result["uart_in"].as<std::string>();
        #endif
        #endif

        #ifdef PROFILERS_EN
        cfg.prof_pc.start = ARG_U32H(result["prof_pc_start"]);
        cfg.prof_pc.stop = ARG_U32H(result["prof_pc_stop"]);
        cfg.prof_pc.single_match_num = ARG_U32(result["prof_pc_single_match"]);
        cfg.prof_pc.exit_on_prof_stop = ARG_BOOL(result["exit_on_prof_stop"]);
        cfg.prof_trace = ARG_BOOL(result["prof_trace"]);
        cfg.perf_events = RESOLVE_ARG_LIST("perf_event", perf_event_map);
        cfg.rf_usage = ARG_BOOL(result["rf_usage"]);
        cfg.no_callstack = ARG_BOOL(result["no_callstack"]);
        cfg.prof_show = ARG_BOOL(result["prof_show"]);
        #endif

        #ifdef DASM_EN
        cfg.log = ARG_BOOL(result["log"]);
        #ifdef PROFILERS_EN
        cfg.log_always = ARG_BOOL(result["log_always"]);
        #else
        cfg.log_always = true;
        #endif
        cfg.log_state = ARG_BOOL(result["log_state"]);
        cfg.rf_names = RESOLVE_ARG("rf_names", rf_names_map);
        #ifdef HW_MODELS_EN
        cfg.log_hw_models = ARG_BOOL(result["log_hw_models"]);
        #endif
        #endif

        #ifdef HW_MODELS_EN
        // icache
        hw_cfg.icache_sets = ARG_U32(result["icache_sets"]);
        hw_cfg.icache_ways = ARG_U32(result["icache_ways"]);
        hw_cfg.icache_re_policy =
            RESOLVE_ARG("icache_re_policy", cache_re_policy_map);
        hw_cfg.icache_in_policy =
            RESOLVE_ARG("icache_in_policy", cache_in_policy_map);
        // dcache
        hw_cfg.dcache_sets = ARG_U32(result["dcache_sets"]);
        hw_cfg.dcache_ways = ARG_U32(result["dcache_ways"]);
        hw_cfg.dcache_re_policy =
            RESOLVE_ARG("dcache_re_policy", cache_re_policy_map);
        hw_cfg.dcache_in_policy =
            RESOLVE_ARG("dcache_in_policy", cache_in_policy_map);
        hw_cfg.dcache_wr_policy =
            RESOLVE_ARG("dcache_wr_policy", cache_wr_policy_map);
        // caches other configs
        hw_cfg.roi_start = ARG_U32H(result["roi_start"]);
        hw_cfg.roi_size = ARG_U32(result["roi_size"]);
        hw_cfg.show_cache_state = ARG_BOOL(result["show_cache_state"]);
        hw_cfg.div_cache_entries = ARG_U32(result["div_cache_entries"]);

        // branch predictors
        hw_cfg.bp = RESOLVE_ARG("bp", bp_names_map);
        hw_cfg.bp2 = RESOLVE_ARG("bp2", bp_names_map);
        if (hw_cfg.bp2 != bp_t::none) {
            hw_cfg.bp_active = bp_t::combined;
            hw_cfg.bp_active_name = "combined";
        } else {
            hw_cfg.bp_active = hw_cfg.bp;
            hw_cfg.bp_active_name = result["bp"].as<std::string>();
        }
        // per predictor configurations
        hw_cfg.bp_static_method = RESOLVE_ARG("bp_static_method",bp_sttc_map);
        hw_cfg.bp_pc_bits = ARG_I8(result["bp_pc_bits"]);
        hw_cfg.bp_cnt_bits = ARG_I8(result["bp_cnt_bits"]);
        hw_cfg.bp_lhist_bits = ARG_I8(result["bp_lhist_bits"]);
        hw_cfg.bp_ghr_bits = ARG_I8(result["bp_ghr_bits"]);
        hw_cfg.bp_fold_pc = RESOLVE_ARG("bp_fold_pc", bp_pc_folds_map);

        hw_cfg.bp2_static_method = RESOLVE_ARG("bp2_static_method",bp_sttc_map);
        hw_cfg.bp2_pc_bits = ARG_I8(result["bp2_pc_bits"]);
        hw_cfg.bp2_cnt_bits = ARG_I8(result["bp2_cnt_bits"]);
        hw_cfg.bp2_lhist_bits = ARG_I8(result["bp2_lhist_bits"]);
        hw_cfg.bp2_ghr_bits = ARG_I8(result["bp2_ghr_bits"]);
        hw_cfg.bp2_fold_pc = RESOLVE_ARG("bp2_fold_pc", bp_pc_folds_map);

        hw_cfg.bp_combined_pc_bits = ARG_I8(result["bp_combined_pc_bits"]);
        hw_cfg.bp_combined_cnt_bits = ARG_I8(result["bp_combined_cnt_bits"]);
        hw_cfg.bp_combined_fold_pc =
            RESOLVE_ARG("bp_combined_fold_pc", bp_pc_folds_map);

        // bp other configs
        hw_cfg.bp_run_all = ARG_BOOL(result["bp_run_all"]);
        hw_cfg.bp_dump_csv = ARG_BOOL(result["bp_dump_csv"]);
        #endif

    } catch (const cxxopts::exceptions::option_has_no_value& e) {
        std::cerr << e.what() << std::endl;
        show_help(options);
        return 1;
    } catch (const std::invalid_argument& e) {
        show_help(options);
        return 1;
    }

    // print useful info about the run
    std::cout << "Running: " << test_elf << std::hex << "\n";

    #ifdef PROFILERS_EN
    if (cfg.prof_pc.start != 0) {
        std::cout << "Profiling start PC: 0x" << cfg.prof_pc.start << "\n";
    }
    if (cfg.prof_pc.stop != 0) {
        std::cout << "Profiling stop PC:  0x" << cfg.prof_pc.stop << "\n";
    }
    std::cout << std::dec;
    if (cfg.prof_pc.single_match_num) {
        std::cout << "Profiling only match number: "
                  << cfg.prof_pc.single_match_num << "\n";
    }
    #endif

    #ifdef DASM_EN
    if (cfg.log) {
        std::cout << "Logging enabled";
        #ifdef PROFILERS_EN
        if (cfg.log_always) std::cout << ", Logging always";
        #endif
        std::cout << "\n";
    }
    #endif
    std::cout << std::endl;

    cfg.out_dir = gen_out_dir(test_elf, out_dir_tag);
    uint64_t sim_inst_cnt = 0;
    double sim_elapsed_s = 0.0;
    TRY_CATCH({
        memory mem(test_elf, cfg, hw_cfg);
        core rv32(&mem, cfg, hw_cfg);
        auto t_start = std::chrono::steady_clock::now();
        sim_inst_cnt = rv32.run();
        auto t_end = std::chrono::steady_clock::now();
        sim_elapsed_s = std::chrono::duration<double>(t_end - t_start).count();
    });

    double mips = (sim_elapsed_s > 0.0)
        ? (static_cast<double>(sim_inst_cnt) / sim_elapsed_s / 1e6) : 0.0;
    std::cout << std::fixed << std::setprecision(2)
              << "\nSimulation performance: " << mips << " MIPS ("
              << sim_inst_cnt << " instructions in "
              << sim_elapsed_s << "s)" << std::endl;

    return 0;
}
