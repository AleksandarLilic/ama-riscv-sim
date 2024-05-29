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

    TRY_CATCH({
        memory mem(BASE_ADDR, test_bin);
        core rv32(BASE_ADDR, &mem, gen_log_name(test_bin));
        rv32.exec();
    });
    return 0;
}
