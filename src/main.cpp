#include "defines.h"
#include "memory.h"
#include "core.h"
#include <filesystem>

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

int main(int argc, char* argv[]) {
    if(argc < 2) {
        std::cerr << "Missing arguments. Usage: " << argv[0]
                  << " <path_to_bin_file>" << std::endl;
        return 1;
    }
    std::string test_bin = argv[1];

    // Parse optional arguments
    uint32_t pc_start = 0;
    uint32_t pc_stop = 0;
    bool first_match = false;
    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        i++;
        if (arg == "--log_pc_start") {
            if (i < argc) pc_start = std::stoll(argv[i], nullptr, 16);
            else PARSE_ERROR("Missing argument for --log_pc_start");
        } else if (arg == "--log_pc_stop") {
            if (i < argc) pc_stop = std::stoll(argv[i], nullptr, 16);
            else PARSE_ERROR("Missing argument for --log_pc_stop");
        } else if (arg == "--log_first_match_only") {
            first_match = true;
        } else {
            PARSE_ERROR("Unknown argument: " + arg);
        }
    }

    if (pc_start != 0)
        std::cout << "Logging start pc: 0x" << std::hex << pc_start << " ";
    if (pc_stop != 0)
        std::cout << "Logging end pc: 0x" << std::hex << pc_stop;
    if (first_match)
        std::cout << "Logging first match only";
    std::cout << std::dec << std::endl;

    logging_pc_t log_pc = {pc_start, pc_stop, first_match};
    TRY_CATCH({
        memory mem(BASE_ADDR, test_bin);
        core rv32(BASE_ADDR, &mem, gen_log_path(test_bin), log_pc);
        rv32.exec();
    });
    return 0;
}
