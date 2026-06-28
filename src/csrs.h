#pragma once

#include "types.h"

// CSR map
namespace csr_map {
    // CSR numbers (12-bit). Addresses live in their own sub-namespace so a
    // register name (e.g. mie/mip) can be both an address and a bit-field group.
    namespace addr {
        constexpr uint16_t tohost = 0x51E;
        // Machine Information Registers
        constexpr uint16_t mvendorid = 0xF11; // MRO
        constexpr uint16_t marchid = 0xF12; // MRO
        constexpr uint16_t mimpid = 0xF13; // MRO
        constexpr uint16_t mhartid = 0xF14; // MRO
        constexpr uint16_t mconfigptr = 0xF15; // MRO
        // Machine Trap Setup
        constexpr uint16_t mstatus = 0x300; // MRW
        constexpr uint16_t mstatush = 0x310; // MRW
        constexpr uint16_t misa = 0x301; // MRW
        constexpr uint16_t mie = 0x304; // MWARL
        constexpr uint16_t mtvec = 0x305; // MRW
        // Machine Trap Handling
        constexpr uint16_t mscratch = 0x340; // MRW
        constexpr uint16_t mepc = 0x341; // MRW
        constexpr uint16_t mcause = 0x342; // MRW
        constexpr uint16_t mtval = 0x343; // MRW
        constexpr uint16_t mip = 0x344; // MRO
        // Machine Counter/Timers
        constexpr uint16_t mcycle = 0xB00; // MRW
        constexpr uint16_t minstret = 0xB02; // MRW
        constexpr uint16_t mcycleh = 0xB80; // MRW
        constexpr uint16_t minstreth = 0xB82; // MRW
        // Machine Hardware Performance Monitor (MHPM) counters & events
        constexpr uint16_t mhpmcounter3 = 0xB03; // MRW
        constexpr uint16_t mhpmcounter4 = 0xB04; // MRW
        constexpr uint16_t mhpmcounter5 = 0xB05; // MRW
        constexpr uint16_t mhpmcounter6 = 0xB06; // MRW
        constexpr uint16_t mhpmcounter7 = 0xB07; // MRW
        constexpr uint16_t mhpmcounter8 = 0xB08; // MRW
        constexpr uint16_t mhpmcounter9 = 0xB09; // MRW
        constexpr uint16_t mhpmcounter10 = 0xB0A; // MRW
        constexpr uint16_t mhpmcounter11 = 0xB0B; // MRW
        constexpr uint16_t mhpmcounter12 = 0xB0C; // MRW
        constexpr uint16_t mhpmcounter13 = 0xB0D; // MRW
        constexpr uint16_t mhpmcounter14 = 0xB0E; // MRW
        constexpr uint16_t mhpmcounter15 = 0xB0F; // MRW
        constexpr uint16_t mhpmcounter16 = 0xB10; // MRW
        constexpr uint16_t mhpmcounter17 = 0xB11; // MRW
        constexpr uint16_t mhpmcounter18 = 0xB12; // MRW
        constexpr uint16_t mhpmcounter19 = 0xB13; // MRW
        constexpr uint16_t mhpmcounter20 = 0xB14; // MRW
        constexpr uint16_t mhpmcounter21 = 0xB15; // MRW
        constexpr uint16_t mhpmcounter22 = 0xB16; // MRW
        constexpr uint16_t mhpmcounter23 = 0xB17; // MRW
        constexpr uint16_t mhpmcounter24 = 0xB18; // MRW
        constexpr uint16_t mhpmcounter25 = 0xB19; // MRW
        constexpr uint16_t mhpmcounter26 = 0xB1A; // MRW
        constexpr uint16_t mhpmcounter27 = 0xB1B; // MRW
        constexpr uint16_t mhpmcounter28 = 0xB1C; // MRW
        constexpr uint16_t mhpmcounter29 = 0xB1D; // MRW
        constexpr uint16_t mhpmcounter30 = 0xB1E; // MRW
        constexpr uint16_t mhpmcounter31 = 0xB1F; // MRW

        constexpr uint16_t mhpmcounter3h = 0xB83; // MRW
        constexpr uint16_t mhpmcounter4h = 0xB84; // MRW
        constexpr uint16_t mhpmcounter5h = 0xB85; // MRW
        constexpr uint16_t mhpmcounter6h = 0xB86; // MRW
        constexpr uint16_t mhpmcounter7h = 0xB87; // MRW
        constexpr uint16_t mhpmcounter8h = 0xB88; // MRW
        constexpr uint16_t mhpmcounter9h = 0xB89; // MRW
        constexpr uint16_t mhpmcounter10h = 0xB8A; // MRW
        constexpr uint16_t mhpmcounter11h = 0xB8B; // MRW
        constexpr uint16_t mhpmcounter12h = 0xB8C; // MRW
        constexpr uint16_t mhpmcounter13h = 0xB8D; // MRW
        constexpr uint16_t mhpmcounter14h = 0xB8E; // MRW
        constexpr uint16_t mhpmcounter15h = 0xB8F; // MRW
        constexpr uint16_t mhpmcounter16h = 0xB90; // MRW
        constexpr uint16_t mhpmcounter17h = 0xB91; // MRW
        constexpr uint16_t mhpmcounter18h = 0xB92; // MRW
        constexpr uint16_t mhpmcounter19h = 0xB93; // MRW
        constexpr uint16_t mhpmcounter20h = 0xB94; // MRW
        constexpr uint16_t mhpmcounter21h = 0xB95; // MRW
        constexpr uint16_t mhpmcounter22h = 0xB96; // MRW
        constexpr uint16_t mhpmcounter23h = 0xB97; // MRW
        constexpr uint16_t mhpmcounter24h = 0xB98; // MRW
        constexpr uint16_t mhpmcounter25h = 0xB99; // MRW
        constexpr uint16_t mhpmcounter26h = 0xB9A; // MRW
        constexpr uint16_t mhpmcounter27h = 0xB9B; // MRW
        constexpr uint16_t mhpmcounter28h = 0xB9C; // MRW
        constexpr uint16_t mhpmcounter29h = 0xB9D; // MRW
        constexpr uint16_t mhpmcounter30h = 0xB9E; // MRW
        constexpr uint16_t mhpmcounter31h = 0xB9F; // MRW

        constexpr uint16_t mcountinhibit = 0x320; // MRO
        constexpr uint16_t mhpmevent3 = 0x323; // MRW
        constexpr uint16_t mhpmevent4 = 0x324; // MRW
        constexpr uint16_t mhpmevent5 = 0x325; // MRW
        constexpr uint16_t mhpmevent6 = 0x326; // MRW
        constexpr uint16_t mhpmevent7 = 0x327; // MRW
        constexpr uint16_t mhpmevent8 = 0x328; // MRW
        constexpr uint16_t mhpmevent9 = 0x329; // MRW
        constexpr uint16_t mhpmevent10 = 0x32A; // MRW
        constexpr uint16_t mhpmevent11 = 0x32B; // MRW
        constexpr uint16_t mhpmevent12 = 0x32C; // MRW
        constexpr uint16_t mhpmevent13 = 0x32D; // MRW
        constexpr uint16_t mhpmevent14 = 0x32E; // MRW
        constexpr uint16_t mhpmevent15 = 0x32F; // MRW
        constexpr uint16_t mhpmevent16 = 0x330; // MRW
        constexpr uint16_t mhpmevent17 = 0x331; // MRW
        constexpr uint16_t mhpmevent18 = 0x332; // MRW
        constexpr uint16_t mhpmevent19 = 0x333; // MRW
        constexpr uint16_t mhpmevent20 = 0x334; // MRW
        constexpr uint16_t mhpmevent21 = 0x335; // MRW
        constexpr uint16_t mhpmevent22 = 0x336; // MRW
        constexpr uint16_t mhpmevent23 = 0x337; // MRW
        constexpr uint16_t mhpmevent24 = 0x338; // MRW
        constexpr uint16_t mhpmevent25 = 0x339; // MRW
        constexpr uint16_t mhpmevent26 = 0x33A; // MRW
        constexpr uint16_t mhpmevent27 = 0x33B; // MRW
        constexpr uint16_t mhpmevent28 = 0x33C; // MRW
        constexpr uint16_t mhpmevent29 = 0x33D; // MRW
        constexpr uint16_t mhpmevent30 = 0x33E; // MRW
        constexpr uint16_t mhpmevent31 = 0x33F; // MRW
        // Unprivileged Counter/Timers
        constexpr uint16_t cycle = 0xC00; // URO
        constexpr uint16_t time = 0xC01; // URO
        constexpr uint16_t instret = 0xC02; // URO
        constexpr uint16_t cycleh = 0xC80; // URO
        constexpr uint16_t timeh = 0xC81; // URO
        constexpr uint16_t instreth = 0xC82; // URO
    }

    // MSTATUS bits
    namespace mstatus {
        constexpr uint32_t mie = 0x8;
        constexpr uint32_t mpie = 0x80;
    }

    // MIP bits - machine interrupt pending
    namespace mip {
        constexpr uint32_t msip = (1u << 3); // software
        constexpr uint32_t mtip = (1u << 7); // timer
        constexpr uint32_t meip = (1u << 11); // external
        constexpr uint32_t lcofip = (1u << 13); // local counter overflow
    }

    // MIE bits - machine interrupt enable (same positions as MIP)
    namespace mie {
        constexpr uint32_t msie = mip::msip;
        constexpr uint32_t mtie = mip::mtip;
        constexpr uint32_t meie = mip::meip;
        constexpr uint32_t lcofie = mip::lcofip;
    }

    // MCAUSE codes
    namespace mcause {
        // exception codes
        constexpr uint32_t inst_addr_misaligned = 0x0;
        constexpr uint32_t inst_access_fault = 0x1;
        constexpr uint32_t illegal_inst = 0x2;
        constexpr uint32_t breakpoint = 0x3;
        constexpr uint32_t load_addr_misaligned = 0x4;
        constexpr uint32_t load_access_fault = 0x5;
        constexpr uint32_t store_addr_misaligned = 0x6;
        constexpr uint32_t store_access_fault = 0x7;
        constexpr uint32_t machine_ecall = 0xB; // 11
        //constexpr uint32_t software_check = 0x12; // 18
        constexpr uint32_t hardware_error = 0x13; // 19
        // interrupt codes (top bit set)
        constexpr uint32_t interrupt_bit = (1u << 31);
        namespace intr {
            constexpr uint32_t machine_sw = (interrupt_bit | 0x3u);
            constexpr uint32_t machine_timer = (interrupt_bit | 0x7u);
            constexpr uint32_t machine_ext = (interrupt_bit | 0xBu); // 11
            constexpr uint32_t machine_lcof = (interrupt_bit | 0xDu); // 13
        }
    }
}

namespace csr_def {
    namespace m = ::csr_map;

    enum class perm_t {
        ro = 0b00, // read-only
        rw = 0b01, // read-write
        warl = 0b10, // write-any-read-legal
        war0 = 0b11, // war0 - unimplemented -> always returns 0
    };

    constexpr uint32_t tohost_early_exit = 0xF000'0000;
    constexpr uint16_t low_to_high_off = 0x80; // low counter CSR -> its *h pair
    constexpr uint32_t mhpmcounters_num = 6;
    constexpr uint32_t mstatus_value = 0x1800; // mpp = 3
    constexpr uint32_t misa_value = (
        (1u << 30) | // MXL = 1 (RV32)
        (1u << 8)  | // I
        (1u << 12) | // M
        (1u << 23)   // X (non-standard extensions present)
    );

    struct CSR {
        const char* name;
        uint32_t value;
        const perm_t perm;
        const uint32_t wmask; // writable bits; 0-bits are hardwired to boot val
        CSR(
            const char* name,
            uint32_t value,
            const perm_t perm,
            uint32_t wmask
        ) : name(name), value(value), perm(perm), wmask(wmask) {}
    };

    struct CSR_entry {
        const uint16_t csr_addr;
        const char* csr_name;
        const perm_t perm;
        const uint32_t boot_val;
        const uint32_t wmask = 0xFFFF'FFFF; // fully writable by default
    };

    #define CSR_ENTRY(s, perm, boot_val) \
        {m::addr::s, #s, perm, boot_val}

    #define CSR_ENTRY_WM(s, perm, boot_val, wmask) \
        {m::addr::s, #s, perm, boot_val, wmask}

    static constexpr CSR_entry supported_csrs[] = {
        CSR_ENTRY(tohost, perm_t::rw, 0u),

        // Machine Information Registers
        CSR_ENTRY(mvendorid, perm_t::war0, 0u),
        CSR_ENTRY(marchid, perm_t::war0, 0u),
        CSR_ENTRY(mimpid, perm_t::war0, 0u),
        CSR_ENTRY(mhartid, perm_t::ro, 0u),
        CSR_ENTRY(mconfigptr, perm_t::ro, 0u),

        // Machine Trap Setup
        CSR_ENTRY_WM(mstatus, perm_t::rw, mstatus_value, ~mstatus_value),
        CSR_ENTRY(mstatush, perm_t::rw, 0u),
        CSR_ENTRY(misa, perm_t::ro, misa_value),
        CSR_ENTRY(mie, perm_t::warl, 0u),
        CSR_ENTRY_WM(mtvec, perm_t::rw, 0u, ~inst::align::pc_low_bits_mask),

        // Machine Trap Handling
        CSR_ENTRY(mscratch, perm_t::rw, 0u),
        CSR_ENTRY_WM(mepc, perm_t::rw, 0u, ~inst::align::pc_low_bits_mask),
        CSR_ENTRY(mcause, perm_t::rw, 0u),
        CSR_ENTRY(mtval, perm_t::rw, 0u),
        CSR_ENTRY(mip, perm_t::ro, 0u),

        // Machine Counter/Timers
        CSR_ENTRY(mcycle, perm_t::rw, 0u),
        CSR_ENTRY(minstret, perm_t::rw, 0u),
        CSR_ENTRY(mcycleh, perm_t::rw, 0u),
        CSR_ENTRY(minstreth, perm_t::rw, 0u),

        // Machine Hardware Performance Monitor (MHPM) counters & events
        CSR_ENTRY(mhpmcounter3, perm_t::rw, 0u),
        CSR_ENTRY(mhpmcounter4, perm_t::rw, 0u),
        CSR_ENTRY(mhpmcounter5, perm_t::rw, 0u),
        CSR_ENTRY(mhpmcounter6, perm_t::rw, 0u),
        CSR_ENTRY(mhpmcounter7, perm_t::rw, 0u),
        CSR_ENTRY(mhpmcounter8, perm_t::rw, 0u),
        CSR_ENTRY(mhpmcounter9, perm_t::war0, 0u),
        CSR_ENTRY(mhpmcounter10, perm_t::war0, 0u),
        CSR_ENTRY(mhpmcounter11, perm_t::war0, 0u),
        CSR_ENTRY(mhpmcounter12, perm_t::war0, 0u),
        CSR_ENTRY(mhpmcounter13, perm_t::war0, 0u),
        CSR_ENTRY(mhpmcounter14, perm_t::war0, 0u),
        CSR_ENTRY(mhpmcounter15, perm_t::war0, 0u),
        CSR_ENTRY(mhpmcounter16, perm_t::war0, 0u),
        CSR_ENTRY(mhpmcounter17, perm_t::war0, 0u),
        CSR_ENTRY(mhpmcounter18, perm_t::war0, 0u),
        CSR_ENTRY(mhpmcounter19, perm_t::war0, 0u),
        CSR_ENTRY(mhpmcounter20, perm_t::war0, 0u),
        CSR_ENTRY(mhpmcounter21, perm_t::war0, 0u),
        CSR_ENTRY(mhpmcounter22, perm_t::war0, 0u),
        CSR_ENTRY(mhpmcounter23, perm_t::war0, 0u),
        CSR_ENTRY(mhpmcounter24, perm_t::war0, 0u),
        CSR_ENTRY(mhpmcounter25, perm_t::war0, 0u),
        CSR_ENTRY(mhpmcounter26, perm_t::war0, 0u),
        CSR_ENTRY(mhpmcounter27, perm_t::war0, 0u),
        CSR_ENTRY(mhpmcounter28, perm_t::war0, 0u),
        CSR_ENTRY(mhpmcounter29, perm_t::war0, 0u),
        CSR_ENTRY(mhpmcounter30, perm_t::war0, 0u),
        CSR_ENTRY(mhpmcounter31, perm_t::war0, 0u),

        CSR_ENTRY(mhpmcounter3h, perm_t::rw, 0u),
        CSR_ENTRY(mhpmcounter4h, perm_t::rw, 0u),
        CSR_ENTRY(mhpmcounter5h, perm_t::rw, 0u),
        CSR_ENTRY(mhpmcounter6h, perm_t::rw, 0u),
        CSR_ENTRY(mhpmcounter7h, perm_t::rw, 0u),
        CSR_ENTRY(mhpmcounter8h, perm_t::rw, 0u),
        CSR_ENTRY(mhpmcounter9h, perm_t::war0, 0u),
        CSR_ENTRY(mhpmcounter10h, perm_t::war0, 0u),
        CSR_ENTRY(mhpmcounter11h, perm_t::war0, 0u),
        CSR_ENTRY(mhpmcounter12h, perm_t::war0, 0u),
        CSR_ENTRY(mhpmcounter13h, perm_t::war0, 0u),
        CSR_ENTRY(mhpmcounter14h, perm_t::war0, 0u),
        CSR_ENTRY(mhpmcounter15h, perm_t::war0, 0u),
        CSR_ENTRY(mhpmcounter16h, perm_t::war0, 0u),
        CSR_ENTRY(mhpmcounter17h, perm_t::war0, 0u),
        CSR_ENTRY(mhpmcounter18h, perm_t::war0, 0u),
        CSR_ENTRY(mhpmcounter19h, perm_t::war0, 0u),
        CSR_ENTRY(mhpmcounter20h, perm_t::war0, 0u),
        CSR_ENTRY(mhpmcounter21h, perm_t::war0, 0u),
        CSR_ENTRY(mhpmcounter22h, perm_t::war0, 0u),
        CSR_ENTRY(mhpmcounter23h, perm_t::war0, 0u),
        CSR_ENTRY(mhpmcounter24h, perm_t::war0, 0u),
        CSR_ENTRY(mhpmcounter25h, perm_t::war0, 0u),
        CSR_ENTRY(mhpmcounter26h, perm_t::war0, 0u),
        CSR_ENTRY(mhpmcounter27h, perm_t::war0, 0u),
        CSR_ENTRY(mhpmcounter28h, perm_t::war0, 0u),
        CSR_ENTRY(mhpmcounter29h, perm_t::war0, 0u),
        CSR_ENTRY(mhpmcounter30h, perm_t::war0, 0u),
        CSR_ENTRY(mhpmcounter31h, perm_t::war0, 0u),

        CSR_ENTRY(mcountinhibit, perm_t::war0, 0u),
        CSR_ENTRY(mhpmevent3, perm_t::rw, 0u),
        CSR_ENTRY(mhpmevent4, perm_t::rw, 0u),
        CSR_ENTRY(mhpmevent5, perm_t::rw, 0u),
        CSR_ENTRY(mhpmevent6, perm_t::rw, 0u),
        CSR_ENTRY(mhpmevent7, perm_t::rw, 0u),
        CSR_ENTRY(mhpmevent8, perm_t::rw, 0u),
        CSR_ENTRY(mhpmevent9, perm_t::war0, 0u),
        CSR_ENTRY(mhpmevent10, perm_t::war0, 0u),
        CSR_ENTRY(mhpmevent11, perm_t::war0, 0u),
        CSR_ENTRY(mhpmevent12, perm_t::war0, 0u),
        CSR_ENTRY(mhpmevent13, perm_t::war0, 0u),
        CSR_ENTRY(mhpmevent14, perm_t::war0, 0u),
        CSR_ENTRY(mhpmevent15, perm_t::war0, 0u),
        CSR_ENTRY(mhpmevent16, perm_t::war0, 0u),
        CSR_ENTRY(mhpmevent17, perm_t::war0, 0u),
        CSR_ENTRY(mhpmevent18, perm_t::war0, 0u),
        CSR_ENTRY(mhpmevent19, perm_t::war0, 0u),
        CSR_ENTRY(mhpmevent20, perm_t::war0, 0u),
        CSR_ENTRY(mhpmevent21, perm_t::war0, 0u),
        CSR_ENTRY(mhpmevent22, perm_t::war0, 0u),
        CSR_ENTRY(mhpmevent23, perm_t::war0, 0u),
        CSR_ENTRY(mhpmevent24, perm_t::war0, 0u),
        CSR_ENTRY(mhpmevent25, perm_t::war0, 0u),
        CSR_ENTRY(mhpmevent26, perm_t::war0, 0u),
        CSR_ENTRY(mhpmevent27, perm_t::war0, 0u),
        CSR_ENTRY(mhpmevent28, perm_t::war0, 0u),
        CSR_ENTRY(mhpmevent29, perm_t::war0, 0u),
        CSR_ENTRY(mhpmevent30, perm_t::war0, 0u),
        CSR_ENTRY(mhpmevent31, perm_t::war0, 0u),

        // Unprivileged Counter/Timers
        CSR_ENTRY(cycle, perm_t::ro, 0u),
        CSR_ENTRY(time, perm_t::ro, 0u),
        CSR_ENTRY(instret, perm_t::ro, 0u),
        CSR_ENTRY(cycleh, perm_t::ro, 0u),
        CSR_ENTRY(timeh, perm_t::ro, 0u),
        CSR_ENTRY(instreth, perm_t::ro, 0u),
    };

    #undef CSR_ENTRY
    #undef CSR_ENTRY_WM
}
