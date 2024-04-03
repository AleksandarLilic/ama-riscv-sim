#include "defines.h"
#include "memory.h"
#include "core.h"
#include <filesystem>

std::string gen_log_name(const std::string& path) {
    std::filesystem::path p(path);
    std::string last_directory = p.parent_path().filename().string();
    std::string binary_name = p.stem().string();
    std::string formatted = last_directory + "_" + binary_name;
    return formatted;
}

int main(int argc, char* argv[]) {
    if(argc < 3) {
        std::cerr << "Usage: " << argv[0] 
                  << " <path_to_bin_file> <base_address>\n";
        return 1;
    }
    std::string test_bin = argv[1];
    uint32_t base_address = std::stoul(argv[2], nullptr, 0);
    memory mem(base_address, test_bin);
    core rv32(base_address, &mem, gen_log_name(test_bin));
    rv32.exec();
    return 0;
}
