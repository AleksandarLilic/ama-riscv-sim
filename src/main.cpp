#include <filesystem>

#include "defines.h"
#include "memory.h"
#include "core.h"
#include <external/cxxopts/include/cxxopts.hpp>

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

#define PARSE_ERROR(msg) \
    { \
        std::cerr << "Error: " << msg << std::endl; \
        return 1; \
    }

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

struct defs_t {
    static constexpr char log_pc_start[] = "0";
    static constexpr char log_pc_stop[] = "0";
    static constexpr char log_first_match_only[] = "false";
};

int main(int argc, char* argv[]) {
    logging_pc_t log_pc = {0, 0, false};
    std::string test_bin;
    cxxopts::Options options(argv[0], "ama-riscv-sim");

    options.add_options()
        ("p,path", "Path to binary executable", cxxopts::value<std::string>())
        ("log_pc_start", "Start PC (hex) for logging and profiling",
         cxxopts::value<std::string>()->default_value(defs_t::log_pc_start))
        ("log_pc_stop", "Stop PC (hex) for logging and profiling",
         cxxopts::value<std::string>()->default_value(defs_t::log_pc_stop))
        ("log_first_match_only", "Log and profile only first match on PC",
         cxxopts::value<bool>()->default_value(defs_t::log_first_match_only))
        ("h,help", "Print usage");

    options.parse_positional({"path"});

    auto result = options.parse(argc, argv);
    if (result.count("help")) {
        std::cout << options.help() << std::endl;
        return 0;
    }

     if (!result.count("path")) {
        std::cerr << "Missing binary file path. Usage: " << argv[0]
                  << " <path_to_bin_file> [options]" << std::endl;
        return 1;
    }

    test_bin = result["path"].as<std::string>();
    log_pc.start =
        std::stoul(result["log_pc_start"].as<std::string>(), nullptr, 16);
    log_pc.stop =
        std::stoul(result["log_pc_stop"].as<std::string>(), nullptr, 16);
    log_pc.first_match = result["log_first_match_only"].as<bool>();

    std::cout << "Binary file: " << test_bin << std::hex << std::endl;
    if (log_pc.start != 0)
        std::cout << "Logging start pc: 0x" << log_pc.start << " ";
    if (log_pc.stop != 0)
        std::cout << "Logging stop pc: 0x" << log_pc.stop;
    if (log_pc.first_match)
        std::cout << "Logging first match only";
    std::cout << std::dec << std::endl;

    TRY_CATCH({
        memory mem(BASE_ADDR, test_bin);
        core rv32(BASE_ADDR, &mem, gen_log_path(test_bin), log_pc);
        rv32.exec();
    });

    return 0;
}
