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

std::string gen_log_name(const std::string& path) {
    std::filesystem::path p(path);
    std::string last_directory = p.parent_path().filename().string();
    std::string binary_name = p.stem().string();
    std::string formatted = last_directory + "_" + binary_name;
    return formatted;
}

int main(int argc, char* argv[]) {
    if(argc < 2) {
        std::cerr << "Missing arguments. Usage: " << argv[0]
                  << " <path_to_bin_file>" << std::endl;
        return 1;
    }
    std::string test_bin = argv[1];

    // Parse optional arguments
    uint32_t pc_start = NULL;
    uint32_t pc_stop = NULL;
    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        i++;
        if (arg == "--log_pc_start") {
            if (i < argc) pc_start = std::stoll(argv[i], nullptr, 16);
            else PARSE_ERROR("Missing argument for --log_pc_start");
        } else if (arg == "--log_pc_stop") {
            if (i < argc) pc_stop = std::stoll(argv[i], nullptr, 16);
            else PARSE_ERROR("Missing argument for --log_pc_stop");
        } else {
            PARSE_ERROR("Unknown argument: " + arg);
        }
    }

    if (pc_start != 0)
        std::cout << "Logging start pc: 0x" << std::hex << pc_start << " ";
    if (pc_stop != 0)
        std::cout << "Logging end pc: 0x" << std::hex << pc_stop;
    std::cout << std::dec << std::endl;

    TRY_CATCH({
        memory mem(BASE_ADDR, test_bin);
        core rv32(BASE_ADDR, &mem, gen_log_name(test_bin), {pc_start, pc_stop});
        rv32.exec();
    });
    return 0;
}
