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
    bool dev_set = false;
    if (address < BASE_ADDR)
        throw std::runtime_error("Address out of range: below base address");
    
    for (auto &entry : mem_map) {
        if (address < entry.base + entry.size) {
            dev_ptr = entry.ptr;
            address -= entry.base;
            dev_set = true;
            break;
        }
    }
    
    if (!dev_set)
        throw std::runtime_error("Address out of range: above max address");
    
    return address;
}

/* Read 8-bit data */
uint8_t memory::rd8(uint32_t address) {
    address = set_addr(address);
    CHECK_ADDRESS(address, 1u)
    return dev_ptr->rd(address);
}

/* Read 16-bit data */
uint16_t memory::rd16(uint32_t address) {
    address = set_addr(address);
    CHECK_ADDRESS(address, 2u)
    return static_cast<uint16_t>(dev_ptr->rd(address)) |
           (static_cast<uint16_t>(dev_ptr->rd(address + 1)) << 8);
}

/* Read 32-bit data */
uint32_t memory::rd32(uint32_t address) {
    address = set_addr(address);
    CHECK_ADDRESS(address, 4u)
    return static_cast<uint32_t>(dev_ptr->rd(address)) |
           (static_cast<uint32_t>(dev_ptr->rd(address + 1)) << 8) |
           (static_cast<uint32_t>(dev_ptr->rd(address + 2)) << 16) |
           (static_cast<uint32_t>(dev_ptr->rd(address + 3)) << 24);
}

uint32_t memory::get_inst(uint32_t address) {
    address = address - mem_map[0].base;
    dev_ptr = mem_map[0].ptr;
    return static_cast<uint32_t>(dev_ptr->rd(address)) |
           (static_cast<uint32_t>(dev_ptr->rd(address + 1)) << 8) |
           (static_cast<uint32_t>(dev_ptr->rd(address + 2)) << 16) |
           (static_cast<uint32_t>(dev_ptr->rd(address + 3)) << 24);
}

/* Write 8-bit data */
void memory::wr8(uint32_t address, uint32_t data) {
    address = set_addr(address);
    CHECK_ADDRESS(address, 1u)
    dev_ptr->wr(address, static_cast<uint8_t>(data));
}

/* Write 16-bit data */
void memory::wr16(uint32_t address, uint32_t data) {
    address = set_addr(address);
    CHECK_ADDRESS(address, 2u)
    dev_ptr->wr(address, static_cast<uint8_t>(data & 0xFF));
    dev_ptr->wr(address + 1, static_cast<uint8_t>((data >> 8) & 0xFF));
}

/* Write 32-bit data */
void memory::wr32(uint32_t address, uint32_t data) {
    address = set_addr(address);
    CHECK_ADDRESS(address, 4u)
    dev_ptr->wr(address, static_cast<uint8_t>(data & 0xFF));
    dev_ptr->wr(address + 1, static_cast<uint8_t>((data >> 8) & 0xFF));
    dev_ptr->wr(address + 2, static_cast<uint8_t>((data >> 16) & 0xFF));
    dev_ptr->wr(address + 3, static_cast<uint8_t>((data >> 24) & 0xFF));
}

/* Dump (private) */
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
                  << static_cast<int>(dev_ptr->rd(addr)) << " "; 
        if (i % word_boundary == 3)
            std::cout << "  ";
    }
    std::cout << std::dec << std::left << std::endl;
}

/* Dump entire memory to console */
void memory::dump() { 
    mem_dump(BASE_ADDR, MEM_SIZE);
}

/* Dump memory range to console */
void memory::dump(uint32_t start, uint32_t size) {
    mem_dump(start, size);
}
