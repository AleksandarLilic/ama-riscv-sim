#include "clint.h"

clint::clint() : dev(CLINT_SIZE) {
    std::fill(mem.begin(), mem.end(), 0x0);
}

void clint::set_mip(uint32_t* csr_mip) {
    this->csr_mip = csr_mip;
}

uint64_t clint::get_mtime_shadow() {
    return dev::rd_64(MTIME);
}

void clint::update_mtime(uint64_t mtime_elapsed) {
    // time in us, assume 10ns period (100MHz clck)
    // => 100 inst per us, as mtime counts insts in isa sim
    uint64_t mtime_prev = dev::rd_64(MTIME);
    mtime_ticks += mtime_elapsed;
    dev::wr_64(MTIME, mtime_prev + (mtime_ticks / 100));
    mtime_ticks = mtime_ticks % 100; // for next go around

    if (dev::rd_64(MTIME) >= dev::rd_64(MTIMECMP)) *csr_mip |= MIP_MTIP;
    else *csr_mip &= ~MIP_MTIP;
}

void clint::update_mtime() {
    update_mtime(1);
}

uint64_t clint::rd_64(uint32_t address) {
    if ((address >= MTIMECMP) && (address < MTIME + 8)) {
        return dev::rd_64(address);
    } else {
        tu->e_dmem_access_fault(address, "clint: read error", mem_op_t::read);
        return 0;
    }
}

void clint::wr_64(uint32_t address, uint64_t data) {
    if ((address >= MTIMECMP) && (address < MTIME + 8)) {
        dev::wr_64(address, data);
    } else {
        tu->e_dmem_access_fault(address, "clint: write error", mem_op_t::write);
    }
}
