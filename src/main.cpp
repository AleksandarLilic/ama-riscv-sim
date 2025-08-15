#include <filesystem>

#include "defines.h"
#include "cxxopts.hpp"
#include "memory.h"
#include "core.h"

#include "utils.h"

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

#define TO_HEX(x) std::stoul(x.as<std::string>(), nullptr, 16)
#define TO_SIZE(x) std::stoul(x.as<std::string>(), nullptr, 10)
#define TO_BOOL(x) x.as<bool>()

#define RESOLVE_ARG(str, map) \
    resolve_arg(str, result[str].as<std::string>(), map)

// handle passed args
template <typename T>
T resolve_arg(
    const std::string& name,
    const std::string& arg,
    const std::unordered_map<std::string, T>& map) {
    auto it = map.find(arg);
    if (it == map.end()) {
        std::cout << "Invalid value for " << name << ": " << arg
                  << ". Valid options: " << gen_help_list(map) << std::endl;
        throw std::invalid_argument("");
    }
    return it->second;
}

template <typename T>
std::string gen_help_list(const std::unordered_map<std::string, T>& map) {
    std::ostringstream oss;
    for (const auto& entry : map) {
        if (oss.tellp() > 0) oss << ", "; // comma and space after first entry
        oss << entry.first;
    }
    return oss.str();
}

// args maps
// available rf names
const std::unordered_map<std::string, rf_names_t> rf_names_map = {
    {"abi", rf_names_t::mode_abi},
    {"x", rf_names_t::mode_x}
};

const std::unordered_map<std::string, perf_event_t> perf_event_map = {
    {"inst", perf_event_t::inst},
    {"branches", perf_event_t::branches},
    {"mem", perf_event_t::mem},
    {"simd", perf_event_t::simd},
    #ifdef HW_MODELS_EN
    {"icache_reference", perf_event_t::icache_reference},
    {"icache_miss", perf_event_t::icache_miss},
    {"dcache_reference", perf_event_t::dcache_reference},
    {"dcache_miss", perf_event_t::dcache_miss},
    {"bp_mispredict", perf_event_t::bp_mispredict}
    #endif
};

#ifdef HW_MODELS_EN
const std::unordered_map<std::string, cache_policy_t> cache_policy_map = {
    {"lru", cache_policy_t::lru}
};

// allowed options for static predictor methods
const std::unordered_map<std::string, bp_sttc_t> bp_sttc_map = {
    {"at", bp_sttc_t::at},
    {"ant", bp_sttc_t::ant},
    {"btfn", bp_sttc_t::btfn}
};

// all available branch predictors
const std::unordered_map<std::string, bp_t> bp_names_map = {
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

const std::unordered_map<std::string, bp_pc_folds_t> bp_pc_folds_map = {
    {"none", bp_pc_folds_t::none},
    {"all", bp_pc_folds_t::all}
};

#endif

// defaults
struct defs_t {
    static constexpr char rf_names[] = "abi";
    static constexpr char end_dump_state[] = "false";
    static constexpr char exit_on_trap[] = "false";
    static constexpr char mem_dump_start[] = "0";
    static constexpr char mem_dump_size[] = "0";
    static constexpr char run_insts[] = "0";
    #ifdef UART_EN
    static constexpr char sink_uart[] = "false";
    #endif
    #ifdef PROFILERS_EN
    static constexpr char prof_pc_start[] = "0";
    static constexpr char prof_pc_stop[] = "0";
    static constexpr char prof_pc_sm[] = "0";
    static constexpr char exit_on_prof_stop[] = "false";
    static constexpr char prof_trace[] = "false";
    static constexpr char perf_event[] = "inst";
    static constexpr char rf_usage[] = "false";
    #endif
    #ifdef DASM_EN
    static constexpr char log[] = "false";
    static constexpr char log_always[] = "false";
    static constexpr char log_state[] = "false";
    static constexpr char log_hw_models[] = "false";
    #endif
};

#ifdef HW_MODELS_EN
struct hw_defs_t {
    // caches
    static constexpr char icache_sets[] = "2";
    static constexpr char icache_ways[] = "2";
    static constexpr char icache_policy[] = "lru";
    static constexpr char dcache_sets[] = "8";
    static constexpr char dcache_ways[] = "2";
    static constexpr char dcache_policy[] = "lru";
    // caches other configs
    static constexpr char roi_start[] = "0";
    static constexpr char roi_size[] = "0";
    // branch predictors
    static constexpr char bp[] = "gshare";
    static constexpr char bp2[] = "none";
    // supported predictors configurations
    static constexpr char bp_static_method[] = "btfn";
    static constexpr char bp_pc_bits[] = "5";
    static constexpr char bp_cnt_bits[] = "2";
    static constexpr char bp_lhist_bits[] = "5";
    static constexpr char bp_gr_bits[] = "5";
    static constexpr char bp_fold_pc[] = "none";
    static constexpr char bp2_static_method[] = "at";
    static constexpr char bp2_pc_bits[] = "5";
    static constexpr char bp2_cnt_bits[] = "2";
    static constexpr char bp2_lhist_bits[] = "5";
    static constexpr char bp2_gr_bits[] = "5";
    static constexpr char bp2_fold_pc[] = "none";
    static constexpr char bp_combined_pc_bits[] = "6";
    static constexpr char bp_combined_cnt_bits[] = "3";
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
        ("rf_names",
         "Register file names used for output. Options: " +
         gen_help_list(rf_names_map),
         CXXOPTS_VAL_STR->default_value(defs_t::rf_names))
        ("end_dump_state", "Dump all registers at the end of simulation",
         CXXOPTS_VAL_BOOL->default_value(defs_t::end_dump_state))
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
        ("run_insts", "Number of instructions to run. Set to 0 for no limit",
         CXXOPTS_VAL_STR->default_value(defs_t::run_insts))
        #ifdef UART_EN
        ("sink_uart",
         "Don't print UART output to terminal. UART still fully operational",
         CXXOPTS_VAL_BOOL->default_value(defs_t::sink_uart))
        #endif

        #ifdef PROFILERS_EN
        ("prof_pc_start", "Start PC (hex) for profiling",
         CXXOPTS_VAL_STR->default_value(defs_t::prof_pc_start))
        ("prof_pc_stop", "Stop PC (hex) for profiling",
         CXXOPTS_VAL_STR->default_value(defs_t::prof_pc_stop))
        ("prof_pc_single_match",
         "Run profiling only for match number (0 for all matches)",
         CXXOPTS_VAL_STR->default_value(defs_t::prof_pc_sm))
        ("exit_on_prof_stop",
         "Exit simulation immediately after profiling finishes."
         "Exits on the first 'prof_pc_stop' if 'prof_pc_single_match' is unused"
         "Otherwise exits on the 'prof_pc_stop' of the 'prof_pc_single_match'",
         CXXOPTS_VAL_BOOL->default_value(defs_t::exit_on_prof_stop))
        ("t,prof_trace", "Record profiler trace",
         CXXOPTS_VAL_BOOL->default_value(defs_t::prof_trace))
        ("e,perf_event",
         "Performance event to track. Options: " +
         gen_help_list(perf_event_map),
         CXXOPTS_VAL_STR->default_value(defs_t::perf_event))
        ("rf_usage", "Enable profiling register file usage",
         CXXOPTS_VAL_BOOL->default_value(defs_t::rf_usage))
        #endif

        #ifdef DASM_EN
        ("l,log",
         "Enable logging",
         CXXOPTS_VAL_BOOL->default_value(defs_t::log))
        ("log_always",
         "Always log execution. Otherwise, log during profiling only",
         CXXOPTS_VAL_BOOL->default_value(defs_t::log_always))
        ("log_state", "Log state after each executed instruction",
         CXXOPTS_VAL_BOOL->default_value(defs_t::log_state))
        #ifdef HW_MODELS_EN
        ("log_hw_models", "Log HW model stats for each executed instruction",
         CXXOPTS_VAL_BOOL->default_value(defs_t::log_hw_models))
        #endif
        #endif

        #ifdef HW_MODELS_EN
        // caches
        ("icache_sets", "Number of sets in I$",
         CXXOPTS_VAL_STR->default_value(hw_defs_t::icache_sets))
        ("icache_ways", "Number of ways in I$",
         CXXOPTS_VAL_STR->default_value(hw_defs_t::icache_ways))
        ("icache_policy", "I$ replacement policy. \nOptions: " +
         gen_help_list(cache_policy_map),
         CXXOPTS_VAL_STR->default_value(hw_defs_t::icache_policy))
        ("dcache_sets", "Number of sets in D$",
         CXXOPTS_VAL_STR->default_value(hw_defs_t::dcache_sets))
        ("dcache_ways", "Number of ways in D$",
         CXXOPTS_VAL_STR->default_value(hw_defs_t::dcache_ways))
        ("dcache_policy", "D$ replacement policy. \nOptions: " +
         gen_help_list(cache_policy_map),
         CXXOPTS_VAL_STR->default_value(hw_defs_t::dcache_policy))
        // caches other configs
        ("roi_start", "Region of interest start address (hex)",
         CXXOPTS_VAL_STR->default_value(hw_defs_t::roi_start))
        ("roi_size", "Region of interest size",
         CXXOPTS_VAL_STR->default_value(hw_defs_t::roi_size))

        // branch predictors
        ("bp",
         "First branch predictor. Defaults as active, driving the I$."
         "For combined predictor, counters will be weakly biased towards this" "predictor (impacts warm-up period)\n"
         "Options: " + gen_help_list(bp_names_map) + "\n ",
         CXXOPTS_VAL_STR->default_value(hw_defs_t::bp))
        ("bp2",
         "Second branch predictor, only for combined predictor. If provided,"
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
        ("bp_gr_bits", "Branch predictor - global register bits",
         CXXOPTS_VAL_STR->default_value(hw_defs_t::bp_gr_bits))
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
        ("bp2_gr_bits", "Branch predictor 2 - global register bits",
         CXXOPTS_VAL_STR->default_value(hw_defs_t::bp2_gr_bits))
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
         CXXOPTS_VAL_BOOL->default_value(hw_defs_t::bp_dump_csv))

        #endif

        ("h,help", "Print usage");

    options.positional_help("<path_to_elf_file>");
    options.parse_positional({"path"});

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

    try {
        test_elf = result["path"].as<std::string>();
        cfg.rf_names = RESOLVE_ARG("rf_names", rf_names_map);
        cfg.end_dump_state = TO_BOOL(result["end_dump_state"]);
        cfg.exit_on_trap = TO_BOOL(result["exit_on_trap"]);
        cfg.mem_dump_start = TO_HEX(result["mem_dump_start"]);
        cfg.mem_dump_size = TO_SIZE(result["mem_dump_size"]);
        cfg.run_insts = TO_SIZE(result["run_insts"]);
        out_dir_tag = result["out_dir_tag"].as<std::string>();
        #ifdef UART_EN
        cfg.sink_uart = TO_BOOL(result["sink_uart"]);
        #endif

        #ifdef PROFILERS_EN
        cfg.prof_pc.start = TO_HEX(result["prof_pc_start"]);
        cfg.prof_pc.stop = TO_HEX(result["prof_pc_stop"]);
        cfg.prof_pc.single_match_num = TO_SIZE(result["prof_pc_single_match"]);
        cfg.prof_pc.exit_on_prof_stop = TO_BOOL(result["exit_on_prof_stop"]);
        cfg.prof_trace = TO_BOOL(result["prof_trace"]);
        cfg.perf_event = RESOLVE_ARG("perf_event", perf_event_map);
        cfg.rf_usage = TO_BOOL(result["rf_usage"]);
        #endif

        #ifdef DASM_EN
        cfg.log = TO_BOOL(result["log"]);
        cfg.log_always = TO_BOOL(result["log_always"]);
        cfg.log_state = TO_BOOL(result["log_state"]);
        cfg.log_hw_models = TO_BOOL(result["log_hw_models"]);
        #endif

        #ifdef HW_MODELS_EN
        // caches
        hw_cfg.icache_sets = TO_SIZE(result["icache_sets"]);
        hw_cfg.icache_ways = TO_SIZE(result["icache_ways"]);
        hw_cfg.icache_policy = RESOLVE_ARG("icache_policy", cache_policy_map);
        hw_cfg.dcache_sets = TO_SIZE(result["dcache_sets"]);
        hw_cfg.dcache_ways = TO_SIZE(result["dcache_ways"]);
        hw_cfg.dcache_policy = RESOLVE_ARG("dcache_policy", cache_policy_map);
        // caches other configs
        hw_cfg.roi_start = TO_HEX(result["roi_start"]);
        hw_cfg.roi_size = TO_SIZE(result["roi_size"]);
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
        hw_cfg.bp_pc_bits = TO_SIZE(result["bp_pc_bits"]);
        hw_cfg.bp_cnt_bits = TO_SIZE(result["bp_cnt_bits"]);
        hw_cfg.bp_lhist_bits = TO_SIZE(result["bp_lhist_bits"]);
        hw_cfg.bp_gr_bits = TO_SIZE(result["bp_gr_bits"]);
        hw_cfg.bp_fold_pc = RESOLVE_ARG("bp_fold_pc", bp_pc_folds_map);
        hw_cfg.bp2_static_method = RESOLVE_ARG("bp2_static_method",bp_sttc_map);
        hw_cfg.bp2_pc_bits = TO_SIZE(result["bp2_pc_bits"]);
        hw_cfg.bp2_cnt_bits = TO_SIZE(result["bp2_cnt_bits"]);
        hw_cfg.bp2_lhist_bits = TO_SIZE(result["bp2_lhist_bits"]);
        hw_cfg.bp2_gr_bits = TO_SIZE(result["bp2_gr_bits"]);
        hw_cfg.bp2_fold_pc = RESOLVE_ARG("bp2_fold_pc", bp_pc_folds_map);

        hw_cfg.bp_combined_pc_bits = TO_SIZE(result["bp_combined_pc_bits"]);
        hw_cfg.bp_combined_cnt_bits = TO_SIZE(result["bp_combined_cnt_bits"]);
        hw_cfg.bp_combined_fold_pc =
            RESOLVE_ARG("bp_combined_fold_pc", bp_pc_folds_map);

        // bp other configs
        hw_cfg.bp_run_all = TO_BOOL(result["bp_run_all"]);
        hw_cfg.bp_dump_csv = TO_BOOL(result["bp_dump_csv"]);
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
        if (cfg.log_always) std::cout << ", Logging always";
        std::cout << "\n";
    }
    #endif
    std::cout << std::endl;

    cfg.out_dir = gen_out_dir(test_elf, out_dir_tag);
    TRY_CATCH({
        memory mem(test_elf, cfg, hw_cfg);
        core rv32(&mem, cfg, hw_cfg);
        rv32.exec();
    });

    return 0;
}
