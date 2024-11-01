#include "memory.h"

memory::memory(uint32_t base_address, std::string test_bin) :
    mm(MEM_SIZE, test_bin),
    #ifdef UART_ENABLE
    uart0(UART0_SIZE),
    #endif
    mem_map {{
        {base_address, MEM_SIZE, &mm},
        #ifdef UART_ENABLE
        {base_address + MEM_SIZE, UART0_SIZE, &uart0}
        #endif
    }}
 {
    dev_ptr = nullptr;
 }

uint32_t memory::set_addr(uint32_t address) {
    #ifdef UART_ENABLE
    bool dev_set = false;
    if (address < BASE_ADDR)
        MEM_OUT_OF_RANGE(address, "below base address");

    for (auto &entry : mem_map) {
        if (address < entry.base + entry.size) {
            dev_ptr = entry.ptr;
            address -= entry.base;
            dev_set = true;
            break;
        }
    }

    if (!dev_set)
        MEM_OUT_OF_RANGE(address, "above base address");

    #else
    address = address - mem_map[0].base;
    dev_ptr = mem_map[0].ptr;
    #endif

    return address;
}

uint8_t memory::rd8(uint32_t address) {
    address = set_addr(address);
    CHECK_ADDRESS(address, 1u)
    return dev_ptr->rd8(address);
}

uint16_t memory::rd16(uint32_t address) {
    address = set_addr(address);
    CHECK_ADDRESS(address, 2u)
    return dev_ptr->rd16(address);
}

uint32_t memory::rd32(uint32_t address) {
    address = set_addr(address);
    CHECK_ADDRESS(address, 4u)
    return dev_ptr->rd32(address);
}

uint32_t memory::get_inst(uint32_t address) {
    address = address - mem_map[0].base;
    dev_ptr = mem_map[0].ptr;
    main_memory* mm_ptr = static_cast<main_memory*>(this->dev_ptr);
    return TO_U32(mm_ptr->rd_inst(address));
}

void memory::wr8(uint32_t address, uint32_t data) {
    address = set_addr(address);
    CHECK_ADDRESS(address, 1u)
    dev_ptr->wr8(address, TO_U8(data));
}

void memory::wr16(uint32_t address, uint32_t data) {
    address = set_addr(address);
    CHECK_ADDRESS(address, 2u)
    dev_ptr->wr16(address, TO_U16(data));
}

void memory::wr32(uint32_t address, uint32_t data) {
    address = set_addr(address);
    CHECK_ADDRESS(address, 4u)
    dev_ptr->wr32(address, data);
}

// Dump (private)
void memory::mem_dump(uint32_t start, uint32_t size) {
    constexpr uint32_t bytes_per_row = 16;
    constexpr uint32_t word_boundary = 4;

    uint32_t aligned_start = start - (start % bytes_per_row);
    uint32_t offset = start - aligned_start; // alignment shifts it left
    uint32_t addr;

    for (uint32_t i = aligned_start; i < aligned_start + size + offset; i++) {
        if (i % bytes_per_row == 0) // insert address at the start of each row
            std::cout << std::endl << MEM_ADDR_FORMAT(i) << ": ";
        addr = set_addr(i);
        std::cout << std::right << std::setw(2) << std::setfill('0')
                  << dev_ptr->rd8(addr) << " ";
        if (i % word_boundary == 3)
            std::cout << "  ";
    }
    std::cout << std::dec << std::left << std::endl;
}

// Dump entire memory to console
void memory::dump() {
    mem_dump(BASE_ADDR, MEM_SIZE);
}

// Dump memory range to console
void memory::dump(uint32_t start, uint32_t size) {
    mem_dump(start, size);
}

#ifdef ENABLE_HW_PROF
void memory::log_cache_stats(std::ofstream& log_file) {
    main_memory* mm_ptr = static_cast<main_memory*>(this->mem_map[0].ptr);
    mm_ptr->log_cache_stats(log_file);
}
#endif
