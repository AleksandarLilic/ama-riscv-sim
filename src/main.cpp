#include <filesystem>

#include "defines.h"
#include "cxxopts.hpp"
#include "memory.h"
#include "core.h"

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

std::string gen_log_path(const std::string& test_bin_path) {
    std::filesystem::path p(test_bin_path);
    std::string last_directory = p.parent_path().filename().string();
    std::string binary_name = p.stem().string();
    std::string path_out = "out_" + last_directory + "_" + binary_name + "/";
    if (!std::filesystem::exists(path_out) &&
        !std::filesystem::create_directory(path_out)) {
        std::cerr << "Failed to create directory: " << path_out << std::endl;
        throw std::runtime_error("Failed to create directory");
    }
    return path_out;
}

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
const std::unordered_map<std::string, rf_names_t> rf_names_map = {
    {"abi", rf_names_t::mode_abi},
    {"x", rf_names_t::mode_x}
};

// defaults
struct defs_t {
    static constexpr char log_pc_start[] = "0";
    static constexpr char log_pc_stop[] = "0";
    static constexpr char log_first_match_only[] = "false";
    static constexpr char rf_names[] = "abi";
};

#ifdef ENABLE_HW_PROF
struct hw_defs_t {
    static constexpr char icache_sets[] = "1";
    static constexpr char icache_ways[] = "4";
    static constexpr char dcache_sets[] = "1";
    static constexpr char dcache_ways[] = "8";
    static constexpr char roi_start[] = "0";
    static constexpr char roi_size[] = "0";
};
#endif

int main(int argc, char* argv[]) {
    cfg_t cfg;
    std::string test_bin;
    cxxopts::Options options(argv[0], "ama-riscv-sim");
    hw_cfg_t hw_cfg;

    options.add_options()
        ("p,path", "", cxxopts::value<std::string>())
        ("log_pc_start", "Start PC (hex) for logging and profiling",
         cxxopts::value<std::string>()->default_value(defs_t::log_pc_start))
        ("log_pc_stop", "Stop PC (hex) for logging and profiling",
         cxxopts::value<std::string>()->default_value(defs_t::log_pc_stop))
        ("log_first_match_only", "Log and profile only first match on PC",
         cxxopts::value<bool>()->default_value(defs_t::log_first_match_only))
        ("rf_names",
         "RF names for execution log. Options: " + gen_help_list(rf_names_map),
         cxxopts::value<std::string>()->default_value(defs_t::rf_names))
        #ifdef ENABLE_HW_PROF
        ("icache_sets", "Number of sets in I$",
         cxxopts::value<std::string>()->default_value(hw_defs_t::icache_sets))
        ("icache_ways", "Number of ways in I$",
         cxxopts::value<std::string>()->default_value(hw_defs_t::icache_ways))
        ("dcache_sets", "Number of sets in D$",
         cxxopts::value<std::string>()->default_value(hw_defs_t::dcache_sets))
        ("dcache_ways", "Number of ways in D$",
         cxxopts::value<std::string>()->default_value(hw_defs_t::dcache_ways))
        ("roi_start", "D$ Region of interest start address (hex)",
         cxxopts::value<std::string>()->default_value(hw_defs_t::roi_start))
        ("roi_size", "D$ Region of interest size",
         cxxopts::value<std::string>()->default_value(hw_defs_t::roi_size))
        #endif
        ("h,help", "Print usage");

    options.positional_help("<path_to_bin_file>");
    options.parse_positional({"path"});

    cxxopts::ParseResult result;
    try {
        result = options.parse(argc, argv);
    } catch (const cxxopts::exceptions::missing_argument& e) {
        std::cerr << e.what() << std::endl;
        std::cout << options.help() << std::endl;
        return 1;
    }

    if (result.count("help")) {
        std::cout << options.help() << std::endl;
        return 0;
    }

    try {
        test_bin = result["path"].as<std::string>();
        cfg.log_pc.start = TO_HEX(result["log_pc_start"]);
        cfg.log_pc.stop = TO_HEX(result["log_pc_stop"]);
        cfg.log_pc.first_match = TO_BOOL(result["log_first_match_only"]);
        cfg.rf_names = resolve_arg(
            "rf_names", result["rf_names"].as<std::string>(), rf_names_map);
        #ifdef ENABLE_HW_PROF
        hw_cfg.icache_sets = TO_SIZE(result["icache_sets"]);
        hw_cfg.icache_ways = TO_SIZE(result["icache_ways"]);
        hw_cfg.dcache_sets = TO_SIZE(result["dcache_sets"]);
        hw_cfg.dcache_ways = TO_SIZE(result["dcache_ways"]);
        hw_cfg.roi_start = TO_HEX(result["roi_start"]);
        hw_cfg.roi_size = TO_SIZE(result["roi_size"]);
        #endif
    } catch (const cxxopts::exceptions::option_has_no_value& e) {
        std::cerr << e.what() << std::endl;
        std::cout << options.help() << std::endl;
        return 1;
    } catch (const std::invalid_argument& e) {
        std::cout << options.help() << std::endl;
        return 1;
    }

    // print useful info about the run
    std::cout << "Binary file: " << test_bin << std::hex << std::endl;
    if (cfg.log_pc.start != 0) {
        std::cout << "Logging start pc: 0x" << cfg.log_pc.start << " ";
    }
    if (cfg.log_pc.stop != 0) {
        std::cout << "Logging stop pc: 0x" << cfg.log_pc.stop << " ";
    }
    if (cfg.log_pc.first_match) {
        std::cout << "Logging first match only";
    }
    std::cout << std::dec << std::endl;

    TRY_CATCH({
        memory mem(BASE_ADDR, test_bin, hw_cfg);
        core rv32(BASE_ADDR, &mem, gen_log_path(test_bin), cfg);
        rv32.exec();
    });

    return 0;
}
