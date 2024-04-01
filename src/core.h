#pragma once

#include "defines.h"
#include "memory.h"
#include "profiler.h"

class core{
    public:
        core() = delete;
        core(uint32_t base_address, memory *mem);
        void exec();
        void exec_inst();
        void dump();
        uint32_t get_pc() { return pc; }
        uint32_t get_inst() { return inst; }
        uint32_t get_reg(uint32_t reg) { return rf[reg]; }
        uint32_t get_inst_cnt() { return inst_cnt; }
        #ifdef ENABLE_DASM
        std::string get_inst_asm() { return dasm.asm_str; }
        #endif
    
    private:
        void write_rf(uint32_t reg, uint32_t data) { if(reg) rf[reg] = data; }
        void write_csr(uint16_t addr, uint32_t data) { csr.at(addr).value = data; }
        
        // instruction decoders
        void al_reg();
        void al_imm();
        void load();
        void store();
        void branch();
        void jalr();
        void jal();
        void lui();
        void auipc();
        void system();
        void misc_mem();
        void unsupported();
        void reset();
        
        // instruction parsing
        uint32_t get_opcode();
        uint32_t get_rd();
        uint32_t get_funct3();
        uint32_t get_rs1();
        uint32_t get_rs2();
        uint32_t get_funct7();
        uint32_t get_funct7_b5();
        uint32_t get_imm_i();
        uint32_t get_imm_i_shamt();
        uint32_t get_csr_addr();
        uint32_t get_uimm_csr();
        uint32_t get_imm_s();
        uint32_t get_imm_b();
        uint32_t get_imm_u();
        uint32_t get_imm_j();
        
        // arithmetic and logic operations
        uint32_t al_add(uint32_t a, uint32_t b) {
            DASM_OP("add");
            PROF_AL(add);
            return int32_t(a) + int32_t(b); 
        };
        uint32_t al_sub(uint32_t a, uint32_t b) {
            DASM_OP("sub");
            PROF_AL(sub);
            return int32_t(a) - int32_t(b);
        };
        uint32_t al_sll(uint32_t a, uint32_t b) {
            DASM_OP("sll");
            PROF_AL(sll);
            return a << b;
        };
        uint32_t al_srl(uint32_t a, uint32_t b) {
            DASM_OP("srl");
            PROF_AL(srl);
            return a >> b;
        };
        uint32_t al_sra(uint32_t a, uint32_t b) {
            DASM_OP("sra");
            PROF_AL(sra);
            b &= 0x1f;
            return int32_t(a) >> b;
        };
        uint32_t al_slt(uint32_t a, uint32_t b) {
            DASM_OP("slt");
            PROF_AL(slt);
            return int32_t(a) < int32_t(b);
        };
        uint32_t al_sltu(uint32_t a, uint32_t b) {
            DASM_OP("sltu");
            PROF_AL(sltu);
            return a < b;
        };
        uint32_t al_xor(uint32_t a, uint32_t b) {
            DASM_OP("xor");
            PROF_AL(xor);
            return a ^ b;
        };
        uint32_t al_or(uint32_t a, uint32_t b) {
            DASM_OP("or");
            PROF_AL(or);
            return a | b;
        };
        uint32_t al_and(uint32_t a, uint32_t b) {
            DASM_OP("and");
            PROF_AL(and);
            return a & b;
        };
        uint32_t al_unsupported(uint32_t a, uint32_t b) {
            DASM_OP("unsupported");
            std::cout << "ERROR: ALU unsupported function with arguments: A: "
                      << a << " and B: " << b << std::endl;
            return 1u;
        };
        
        // load operations
        uint32_t load_byte(uint32_t address) {
            DASM_OP("lb");
            PROF_MEM(lb);
            return static_cast<uint32_t>(
                static_cast<int8_t>(mem->rd8(address)));
        }
        uint32_t load_half(uint32_t address) {
            DASM_OP("lh");
            PROF_MEM(lh);
            return static_cast<uint32_t>(
                static_cast<int16_t>(mem->rd16(address))); 
        }
        uint32_t load_word(uint32_t address) {
            DASM_OP("lw");
            PROF_MEM(lw);
            return mem->rd32(address);
        };
        uint32_t load_byte_u(uint32_t address) {
            DASM_OP("lbu");
            PROF_MEM(lbu);
            return mem->rd8(address);
        };
        uint32_t load_half_u(uint32_t address) {
            DASM_OP("lhu");
            PROF_MEM(lhu);
            return mem->rd16(address);
        };
        
        // store operations
        void store_byte(uint32_t address, uint32_t data) {
            DASM_OP("sb");
            PROF_MEM(sb);
            mem->wr8(address, data); 
        }
        void store_half(uint32_t address, uint32_t data) {
            DASM_OP("sh");
            PROF_MEM(sh);
            mem->wr16(address, data); 
        }
        void store_word(uint32_t address, uint32_t data) {
            DASM_OP("sw");
            PROF_MEM(sw);
            mem->wr32(address, data); 
        }
        
        // branch operations
        bool branch_eq() {
            DASM_OP("beq");
            return rf[get_rs1()] == rf[get_rs2()];
        };
        bool branch_ne() {
            DASM_OP("bne");
            return rf[get_rs1()] != rf[get_rs2()];
        };
        bool branch_lt() {
            DASM_OP("blt");
            return int32_t(rf[get_rs1()]) < int32_t(rf[get_rs2()]);
        };
        bool branch_ge() {
            DASM_OP("bge");
            return int32_t(rf[get_rs1()]) >= int32_t(rf[get_rs2()]);
        };
        bool branch_ltu() {
            DASM_OP("bltu");
            return (uint32_t)rf[get_rs1()] < (uint32_t)rf[get_rs2()];
        }
        bool branch_geu() {
            DASM_OP("bgeu");
            return (uint32_t)rf[get_rs1()] >= (uint32_t)rf[get_rs2()];
        }
        
        // csr operations
        void csr_access();
        void csr_rw(uint32_t init_val_rs1) {
            DASM_OP("csrrw");
            PROF_CSR(csrrw);
            W_CSR(init_val_rs1);
        }
        void csr_rs(uint32_t init_val_rs1) {
            DASM_OP("csrrs");
            PROF_CSR(csrrs);
            W_CSR(csr.at(get_csr_addr()).value | init_val_rs1);
        }
        void csr_rc(uint32_t init_val_rs1) {
            DASM_OP("csrrc");
            PROF_CSR(csrrc);
            W_CSR(csr.at(get_csr_addr()).value & ~init_val_rs1);
        }
        void csr_rwi() {
            DASM_OP("csrrwi");
            PROF_CSR(csrrwi);
            W_CSR(get_uimm_csr());
        }
        void csr_rsi() {
            DASM_OP("csrrsi");
            PROF_CSR(csrrsi);
            W_CSR(csr.at(get_csr_addr()).value | get_uimm_csr());
        }
        void csr_rci() {
            DASM_OP("csrrci");
            PROF_CSR(csrrci);
            W_CSR(csr.at(get_csr_addr()).value & ~get_uimm_csr());
        }

    private:
        bool running;
        std::array<int32_t, 32> rf;
        uint32_t pc;
        uint32_t next_pc;
        uint32_t inst;
        uint64_t inst_cnt;
        #ifdef ENABLE_DASM
        dasm_str dasm;
        #endif
        memory *mem;
        static constexpr std::array<CSR_entry, 2> supported_csrs = {{
            {0x51e, "tohost"},
            {0x340, "mscratch"}
        }};
        #ifdef ENABLE_PROF
        profiler prof;
        #endif
        
        // csr map
        std::unordered_map<uint16_t, CSR> csr;
        
        // register names
        static constexpr std::array<std::array<const char*, 2>, 32> 
        rf_names = {{
            {{"x0", "zero"}}, // hard-wired zero
            {{"x1", "ra"}},   // return address
            {{"x2", "sp"}},   // stack pointer 
            {{"x3", "gp"}},   // global pointer
            {{"x4", "tp"}},   // thread pointer
            {{"x5", "t0"}},   // temporary/alternate link register
            {{"x6", "t1"}},   // temporary
            {{"x7", "t2"}},   // temporary
            {{"x8", "s0"}},   // saved register/frame pointer
            {{"x9", "s1"}},   // saved register
            {{"x10", "a0"}},  // function argument/return value
            {{"x11", "a1"}},  // function argument/return value
            {{"x12", "a2"}},  // function argument
            {{"x13", "a3"}},  // function argument
            {{"x14", "a4"}},  // function argument
            {{"x15", "a5"}},  // function argument
            {{"x16", "a6"}},  // function argument
            {{"x17", "a7"}},  // function argument
            {{"x18", "s2"}},  // saved register
            {{"x19", "s3"}},  // saved register
            {{"x20", "s4"}},  // saved register
            {{"x21", "s5"}},  // saved register
            {{"x22", "s6"}},  // saved register
            {{"x23", "s7"}},  // saved register
            {{"x24", "s8"}},  // saved register
            {{"x25", "s9"}},  // saved register
            {{"x26", "s10"}}, // saved register
            {{"x27", "s11"}}, // saved register
            {{"x28", "t3"}},  // temporary
            {{"x29", "t4"}},  // temporary
            {{"x30", "t5"}},  // temporary
            {{"x31", "t6"}},  // temporary
        }};
};
