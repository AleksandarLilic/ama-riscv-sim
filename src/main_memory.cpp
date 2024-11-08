#include "main_memory.h"

// TODO: pass from cli
#define ICACHE_SETS 1
#define ICACHE_WAYS 8

#define DCACHE_SETS 1
#define DCACHE_WAYS 8

#ifdef ENABLE_HW_PROF
#define CACHE_ACCESS(cache, width, address) \
    cache.width(address);
#else
#define CACHE_ACCESS(cache, width, address)
#endif

main_memory::main_memory(size_t size, std::string test_bin) :
    dev(size)
    #ifdef ENABLE_HW_PROF
    ,
    icache(ICACHE_SETS, ICACHE_WAYS, "Icache"),
    dcache(DCACHE_SETS, DCACHE_WAYS, "Dcache")
    #endif
{
    burn(test_bin);
    #ifdef ENABLE_HW_PROF
    dcache.set_roi(0x00007100, 0x000071FF); // TODO: pass from cli as well
    #endif
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

//std::array<uint8_t, CACHE_LINE_SIZE> dev::rd_line(uint32_t address) {
//    address = set_addr(address);
//    CHECK_ADDRESS(address, CACHE_LINE_SIZE)
//    std::array<uint8_t, CACHE_LINE_SIZE> data;
//    for (uint32_t i = 0; i < CACHE_LINE_SIZE; i++)
//        data[i] = dev_ptr->rd(address + i);
//    return data;
//}

uint32_t main_memory::rd_inst(uint32_t address) {
    CACHE_ACCESS(icache, rd32, address);
    return dev::rd32(address);
}

uint8_t main_memory::rd8(uint32_t address) {
    CACHE_ACCESS(dcache, rd8, address);
    return dev::rd8(address);
}

uint16_t main_memory::rd16(uint32_t address) {
    CACHE_ACCESS(dcache, rd16, address);
    return dev::rd16(address);
}

uint32_t main_memory::rd32(uint32_t address) {
    CACHE_ACCESS(dcache, rd32, address);
    return dev::rd32(address);
}

void main_memory::wr8(uint32_t address, uint32_t data) {
    CACHE_ACCESS(dcache, wr8, address);
    dev::wr8(address, data);
}

void main_memory::wr16(uint32_t address, uint32_t data) {
    CACHE_ACCESS(dcache, wr16, address);
    dev::wr16(address, data);
}

void main_memory::wr32(uint32_t address, uint32_t data) {
    CACHE_ACCESS(dcache, wr32, address);
    dev::wr32(address, data);
}
