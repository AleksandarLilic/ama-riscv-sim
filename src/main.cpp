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
const std::unordered_map<std::string, bp_sttc_t> bp_static_map = {
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
    {"combined", bp_t::combined}
};

// allowed options for combined predictor
const std::unordered_map<std::string, bp_t> bpc_names_map = {
    {"static", bp_t::sttc},
    {"bimodal", bp_t::bimodal},
    {"local", bp_t::local},
    {"global", bp_t::global},
    {"gselect", bp_t::gselect},
    {"gshare", bp_t::gshare},
    {"ideal", bp_t::ideal}
};
#endif

// defaults
struct defs_t {
    static constexpr char prof_pc_start[] = "0";
    static constexpr char prof_pc_stop[] = "0";
    static constexpr char prof_pc_sm[] = "0";
    static constexpr char rf_names[] = "abi";
    static constexpr char end_dump_state[] = "false";
    static constexpr char perf_event[] = "inst";
    #ifdef DASM_EN
    static constexpr char log_always[] = "false";
    static constexpr char log_state[] = "false";
    #endif
};

#ifdef HW_MODELS_EN
struct hw_defs_t {
    static constexpr char icache_sets[] = "1";
    static constexpr char icache_ways[] = "4";
    static constexpr char icache_policy[] = "lru";
    static constexpr char dcache_sets[] = "1";
    static constexpr char dcache_ways[] = "8";
    static constexpr char dcache_policy[] = "lru";
    static constexpr char roi_start[] = "0";
    static constexpr char roi_size[] = "0";
    static constexpr char bp_active[] = "combined";
    static constexpr char bp_combined_p1[] = "static";
    static constexpr char bp_combined_p2[] = "gselect";
    static constexpr char bp_run_all[] = "false";
    static constexpr char bp_dump_csv[] = "false";
    // supported predictors configurations
    static constexpr char bp_static_method[] = "btfn";
    static constexpr char bp_bimodal_pc_bits[] = "7";
    static constexpr char bp_bimodal_cnt_bits[] = "3";
    static constexpr char bp_local_pc_bits[] = "5";
    static constexpr char bp_local_hist_bits[] = "5";
    static constexpr char bp_local_cnt_bits[] = "3";
    static constexpr char bp_global_gr_bits[] = "7";
    static constexpr char bp_global_cnt_bits[] = "3";
    static constexpr char bp_gselect_pc_bits[] = "2";
    static constexpr char bp_gselect_gr_bits[] = "6";
    static constexpr char bp_gselect_cnt_bits[] = "1";
    static constexpr char bp_gshare_pc_bits[] = "8";
    static constexpr char bp_gshare_gr_bits[] = "8";
    static constexpr char bp_gshare_cnt_bits[] = "1";
    static constexpr char bp_combined_pc_bits[] = "6";
    static constexpr char bp_combined_cnt_bits[] = "3";
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
        ("p,path", "Path to the ELF file to load",
         cxxopts::value<std::string>())
        ("prof_pc_start", "Start PC (hex) for profiling",
         cxxopts::value<std::string>()->default_value(defs_t::prof_pc_start))
        ("prof_pc_stop", "Stop PC (hex) for profiling",
         cxxopts::value<std::string>()->default_value(defs_t::prof_pc_stop))
        ("prof_pc_single_match",
         "Run profiling only for match number (0 for all matches)",
         cxxopts::value<std::string>()->default_value(defs_t::prof_pc_sm))
        ("rf_names",
         "Register file names used for output. Options: " +
         gen_help_list(rf_names_map),
         cxxopts::value<std::string>()->default_value(defs_t::rf_names))
        ("e,perf_event",
         "Performance event to track. Options: " +
         gen_help_list(perf_event_map),
         cxxopts::value<std::string>()->default_value(defs_t::perf_event))
        ("end_dump_state", "Dump all registers at the end of simulation",
         cxxopts::value<bool>()->default_value(defs_t::end_dump_state))
        ("out_dir_tag", "Tag (suffix) for output directory",
         cxxopts::value<std::string>()->default_value(""))

        #ifdef DASM_EN
        ("log_always",
         "Always log execution. Otherwise, log during profiling only",
         cxxopts::value<bool>()->default_value(defs_t::log_always))
        ("log_state", "Log state after each executed instruction",
         cxxopts::value<bool>()->default_value(defs_t::log_state))
        #endif

        #ifdef HW_MODELS_EN
        ("icache_sets", "Number of sets in I$",
         cxxopts::value<std::string>()->default_value(hw_defs_t::icache_sets))
        ("icache_ways", "Number of ways in I$",
         cxxopts::value<std::string>()->default_value(hw_defs_t::icache_ways))
        ("icache_policy", "I$ replacement policy. \nOptions: " +
         gen_help_list(cache_policy_map),
         cxxopts::value<std::string>()->default_value(hw_defs_t::icache_policy))
        ("dcache_sets", "Number of sets in D$",
         cxxopts::value<std::string>()->default_value(hw_defs_t::dcache_sets))
        ("dcache_ways", "Number of ways in D$",
         cxxopts::value<std::string>()->default_value(hw_defs_t::dcache_ways))
        ("dcache_policy", "D$ replacement policy. \nOptions: " +
         gen_help_list(cache_policy_map),
         cxxopts::value<std::string>()->default_value(hw_defs_t::dcache_policy))
        ("roi_start", "D$ Region of interest start address (hex)",
         cxxopts::value<std::string>()->default_value(hw_defs_t::roi_start))
        ("roi_size", "D$ Region of interest size",
         cxxopts::value<std::string>()->default_value(hw_defs_t::roi_size))
        ("bp_active",
         "Active branch predictor (driving I$).\nOptions: " +
         gen_help_list(bp_names_map) + "\n ",
         cxxopts::value<std::string>()->default_value(hw_defs_t::bp_active))
        ("bp_combined_p1",
         "First branch predictor for combined predictor. Counters will be "
         "weakly biased towards this predictor (impacts warm-up period)\n"
         "Options: " +
         gen_help_list(bpc_names_map) + "\n ",
         cxxopts::value<std::string>()->default_value(hw_defs_t::bp_combined_p1))
        ("bp_combined_p2",
         "Second branch predictor for combined predictor.\n"
         "Options: " +
         gen_help_list(bpc_names_map) + "\n ",
         cxxopts::value<std::string>()->default_value(hw_defs_t::bp_combined_p2))
        ("bp_run_all", "Run all branch predictors",
         cxxopts::value<bool>()->default_value(hw_defs_t::bp_run_all))
        ("bp_dump_csv", "Dump branch predictor stats to CSV",
         cxxopts::value<bool>()->default_value(hw_defs_t::bp_dump_csv))

        // supported predictors configurations
        ("bp_static_method",
         "Static predictor - method. \nOptions: " +
         gen_help_list(bp_static_map),
         cxxopts::value<std::string>()->
            default_value(hw_defs_t::bp_static_method))
        ("bp_bimodal_pc_bits", "Bimodal predictor - PC bits",
         cxxopts::value<std::string>()->
            default_value(hw_defs_t::bp_bimodal_pc_bits))
        ("bp_bimodal_cnt_bits", "Bimodal predictor - counter bits",
         cxxopts::value<std::string>()->
            default_value(hw_defs_t::bp_bimodal_cnt_bits))
        ("bp_local_pc_bits", "Local predictor - PC bits",
         cxxopts::value<std::string>()->
            default_value(hw_defs_t::bp_local_pc_bits))
        ("bp_local_cnt_bits", "Local predictor - counter bits",
         cxxopts::value<std::string>()->
            default_value(hw_defs_t::bp_local_cnt_bits))
        ("bp_local_hist_bits", "Local predictor - local history bits",
         cxxopts::value<std::string>()->
            default_value(hw_defs_t::bp_local_hist_bits))
        ("bp_global_cnt_bits", "Global predictor - counter bits",
         cxxopts::value<std::string>()->
            default_value(hw_defs_t::bp_global_cnt_bits))
        ("bp_global_gr_bits", "Global predictor - global register bits",
         cxxopts::value<std::string>()->
            default_value(hw_defs_t::bp_global_gr_bits))
        ("bp_gselect_cnt_bits", "Gselect predictor - counter bits",
         cxxopts::value<std::string>()->
            default_value(hw_defs_t::bp_gselect_cnt_bits))
        ("bp_gselect_gr_bits", "Gselect predictor - global register bits",
         cxxopts::value<std::string>()->
            default_value(hw_defs_t::bp_gselect_gr_bits))
        ("bp_gselect_pc_bits", "Gselect predictor - PC bits",
         cxxopts::value<std::string>()->
            default_value(hw_defs_t::bp_gselect_pc_bits))
        ("bp_gshare_cnt_bits", "Gshare predictor - counter bits",
         cxxopts::value<std::string>()->
            default_value(hw_defs_t::bp_gshare_cnt_bits))
        ("bp_gshare_gr_bits", "Gshare predictor - global register bits",
         cxxopts::value<std::string>()->
            default_value(hw_defs_t::bp_gshare_gr_bits))
        ("bp_gshare_pc_bits", "Gshare predictor - PC bits",
         cxxopts::value<std::string>()->
            default_value(hw_defs_t::bp_gshare_pc_bits))
        ("bp_combined_pc_bits", "Combined predictor - PC bits",
         cxxopts::value<std::string>()->
            default_value(hw_defs_t::bp_combined_pc_bits))
        ("bp_combined_cnt_bits", "Combined predictor - counter bits",
         cxxopts::value<std::string>()->
            default_value(hw_defs_t::bp_combined_cnt_bits))
        #endif

        ("h,help", "Print usage");

    options.positional_help("<path_to_bin_file>");
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
        out_dir_tag = result["out_dir_tag"].as<std::string>();
        cfg.prof_pc.start = TO_HEX(result["prof_pc_start"]);
        cfg.prof_pc.stop = TO_HEX(result["prof_pc_stop"]);
        cfg.prof_pc.single_match_num = TO_SIZE(result["prof_pc_single_match"]);
        cfg.rf_names = RESOLVE_ARG("rf_names", rf_names_map);
        cfg.perf_event = RESOLVE_ARG("perf_event", perf_event_map);
        cfg.end_dump_state = TO_BOOL(result["end_dump_state"]);

        #ifdef DASM_EN
        cfg.log_always = TO_BOOL(result["log_always"]);
        cfg.log_state = TO_BOOL(result["log_state"]);
        #endif

        #ifdef HW_MODELS_EN
        hw_cfg.icache_sets = TO_SIZE(result["icache_sets"]);
        hw_cfg.icache_ways = TO_SIZE(result["icache_ways"]);
        hw_cfg.icache_policy = RESOLVE_ARG("icache_policy", cache_policy_map);
        hw_cfg.dcache_sets = TO_SIZE(result["dcache_sets"]);
        hw_cfg.dcache_ways = TO_SIZE(result["dcache_ways"]);
        hw_cfg.dcache_policy = RESOLVE_ARG("dcache_policy", cache_policy_map);
        hw_cfg.roi_start = TO_HEX(result["roi_start"]);
        hw_cfg.roi_size = TO_SIZE(result["roi_size"]);
        hw_cfg.bp_active = RESOLVE_ARG("bp_active", bp_names_map);
        hw_cfg.bp_active_name = result["bp_active"].as<std::string>();
        hw_cfg.bp_combined_p1 = RESOLVE_ARG("bp_combined_p1", bpc_names_map);
        hw_cfg.bp_combined_p2 = RESOLVE_ARG("bp_combined_p2", bpc_names_map);
        hw_cfg.bp_run_all = TO_BOOL(result["bp_run_all"]);
        hw_cfg.bp_dump_csv = TO_BOOL(result["bp_dump_csv"]);
        // per predictor configurations
        hw_cfg.bp_static_method = RESOLVE_ARG("bp_static_method",bp_static_map);
        hw_cfg.bp_bimodal_pc_bits = TO_SIZE(result["bp_bimodal_pc_bits"]);
        hw_cfg.bp_bimodal_cnt_bits = TO_SIZE(result["bp_bimodal_cnt_bits"]);
        hw_cfg.bp_local_pc_bits = TO_SIZE(result["bp_local_pc_bits"]);
        hw_cfg.bp_local_cnt_bits = TO_SIZE(result["bp_local_cnt_bits"]);
        hw_cfg.bp_local_hist_bits = TO_SIZE(result["bp_local_hist_bits"]);
        hw_cfg.bp_global_cnt_bits = TO_SIZE(result["bp_global_cnt_bits"]);
        hw_cfg.bp_global_gr_bits = TO_SIZE(result["bp_global_gr_bits"]);
        hw_cfg.bp_gselect_cnt_bits = TO_SIZE(result["bp_gselect_cnt_bits"]);
        hw_cfg.bp_gselect_gr_bits = TO_SIZE(result["bp_gselect_gr_bits"]);
        hw_cfg.bp_gselect_pc_bits = TO_SIZE(result["bp_gselect_pc_bits"]);
        hw_cfg.bp_gshare_cnt_bits = TO_SIZE(result["bp_gshare_cnt_bits"]);
        hw_cfg.bp_gshare_gr_bits = TO_SIZE(result["bp_gshare_gr_bits"]);
        hw_cfg.bp_gshare_pc_bits = TO_SIZE(result["bp_gshare_pc_bits"]);
        hw_cfg.bp_combined_pc_bits = TO_SIZE(result["bp_combined_pc_bits"]);
        hw_cfg.bp_combined_cnt_bits = TO_SIZE(result["bp_combined_cnt_bits"]);
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
    #ifdef DASM_EN
    if (cfg.log_always) std::cout << "Logging always" << "\n";
    #endif
    std::cout << std::endl;

    TRY_CATCH({
        memory mem(test_elf, hw_cfg);
        core rv32(&mem, gen_out_dir(test_elf, out_dir_tag), cfg, hw_cfg);
        rv32.exec();
    });

    return 0;
}
