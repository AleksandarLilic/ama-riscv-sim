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
    if(argc < 2) {
        std::cerr << "Usage: " << argv[0] 
                  << " <path_to_bin_file>\n";
        return 1;
    }
    std::string test_bin = argv[1];
    memory mem(BASE_ADDR, test_bin);
    core rv32(BASE_ADDR, &mem, gen_log_name(test_bin));
    rv32.exec();
    mem.dump(BASE_ADDR, 64);
    //mem.dump(4092, 8*4);
    mem.dump(BASE_ADDR+MEM_SIZE, UART0_SIZE);
    return 0;
}
