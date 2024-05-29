#include "main_memory.h"

main_memory::main_memory(size_t size, std::string test_bin) :
    dev(size)
{
    burn(test_bin);
}

void main_memory::burn(std::string test_bin) {
    std::ifstream bin_file(test_bin, std::ios::binary | std::ios::ate);
    if (!bin_file.is_open()) {
        throw std::runtime_error("BIN failed to open.");
    }

    size_t file_size = bin_file.tellg();
    if (file_size > MEM_SIZE) {
        std::cerr << "ERROR: File size is greater than memory size."
                  << " Binary size: " << file_size << "B"
                  << " Memory size: " << MEM_SIZE << "B"
                  << " Binary not loaded" << std::endl;
        throw std::runtime_error("File size is greater than memory size.");
    }

    bin_file.seekg(0, std::ios::beg);
    bin_file.read(reinterpret_cast<char*>(mem.data()), file_size);
    bin_file.close();
}
