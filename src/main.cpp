#include "defines.h"
#include "memory.h"
#include "core.h"

int main(int argc, char* argv[]) {
    if(argc < 4) {
        std::cerr << "Usage: " << argv[0] 
                  << " <path_to_bin_file> <base_address> <exec_cycles>\n";
        return 1;
    }
    std::string test_bin = argv[1];
    uint32_t base_address = std::stoul(argv[2], nullptr, 0);
    uint32_t cycles = std::stoul(argv[3], nullptr, 0);
    memory mem(base_address, test_bin);
    core rv32(base_address, &mem);
    for (uint32_t i = 0; i < cycles; i++) rv32.exec();
    rv32.dump();
    return 0;
}
