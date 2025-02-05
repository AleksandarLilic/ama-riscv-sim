#pragma once

#include "defines.h"
#include "dev.h"
#include "trap.h"

class clint : public dev {
    private:
        //static const uint32_t MSIP = 0x0000;
        static const uint32_t MTIMECMP = 0x8; // 64-bit
        static const uint32_t MTIME = 0x10; // 64-bit
        uint32_t* csr_mip;
        trap *tu;

    public:
        clint();
        void trap_setup(trap* tu) { this->tu = tu; }
        virtual uint32_t rd(uint32_t addr, uint32_t size) override;
        virtual void wr(uint32_t addr, uint32_t data, uint32_t size) override;
        uint64_t get_mtime_shadow();
        void set_mip(uint32_t* csr_mip);
        void update_mtime(uint64_t mtime_elapsed);
        void update_mtime();
};
