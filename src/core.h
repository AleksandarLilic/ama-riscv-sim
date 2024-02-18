#pragma once

#include "defines.h"
#include "memory.h"

class core{
    public:
        core() = delete;
        core(uint32_t base_address, memory *mem);
        void exec();
        void dump();
    private:
        void write_rf(uint32_t reg, uint32_t data) { if(reg) rf[reg] = data; }
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
        void unsupported();
        void reset();
        using decoder = void (core::*)();
        // instruction parsing
        uint32_t get_opcode();
        uint32_t get_rd();
        uint32_t get_funct3();
        uint32_t get_rs1();
        uint32_t get_rs2();
        uint32_t get_funct7();
        uint32_t get_funct7_b5();
        uint32_t get_imm_i();
        uint32_t get_imm_s();
        uint32_t get_imm_b();
        uint32_t get_imm_u();
        uint32_t get_imm_j();
        // arithmetic and logic operations
        uint32_t al_add(uint32_t a, uint32_t b) { return int32_t(a) + int32_t(b); };
        uint32_t al_sub(uint32_t a, uint32_t b) { return a - b; };
        uint32_t al_sll(uint32_t a, uint32_t b) { return a << b; };
        uint32_t al_srl(uint32_t a, uint32_t b) { return a >> b; };
        uint32_t al_sra(uint32_t a, uint32_t b) { return int32_t(a) >> b; };
        uint32_t al_slt(uint32_t a, uint32_t b) { return int32_t(a) < int32_t(b); };
        uint32_t al_sltu(uint32_t a, uint32_t b) { return a < b; };
        uint32_t al_xor(uint32_t a, uint32_t b) { return a ^ b; };
        uint32_t al_or(uint32_t a, uint32_t b) { return a | b; };
        uint32_t al_and(uint32_t a, uint32_t b) { return a & b; };
        uint32_t al_unsupported(uint32_t a, uint32_t b) { 
            std::cout << "ERROR: ALU unsupported function with arguments: A: "
                      << a << " and B: " << b << std::endl;
            return 1u;
        };
        using alu_op = uint32_t (core::*)(uint32_t, uint32_t);
        // load
        uint32_t load_byte(uint32_t address) { 
            return static_cast<uint32_t>(
                static_cast<int8_t>(mem->rd8(address)));
        }
        uint32_t load_half(uint32_t address) { 
            return static_cast<uint32_t>(
                static_cast<int16_t>(mem->rd16(address))); 
        }
        uint32_t load_word(uint32_t address) { return mem->rd32(address); }
        uint32_t load_byte_u(uint32_t address) { return mem->rd8(address); }
        uint32_t load_half_u(uint32_t address) { return mem->rd16(address); }
        using load_op = uint32_t (core::*)(uint32_t);
        // store
        void store_byte(uint32_t address, uint32_t data) { 
            mem->wr8(address, data); 
        }
        void store_half(uint32_t address, uint32_t data) { 
            mem->wr16(address, data); 
        }
        void store_word(uint32_t address, uint32_t data) { 
            mem->wr32(address, data); 
        }
        using store_op = void (core::*)(uint32_t, uint32_t);
        // branch
        bool branch_eq() { return rf[get_rs1()] == rf[get_rs2()]; }
        bool branch_ne() { return rf[get_rs1()] != rf[get_rs2()]; }
        bool branch_lt() { return int32_t(rf[get_rs1()]) < int32_t(rf[get_rs2()]); }
        bool branch_ge() { return int32_t(rf[get_rs1()]) >= int32_t(rf[get_rs2()]); }
        bool branch_ltu() { return rf[get_rs1()] < rf[get_rs2()]; }
        bool branch_geu() { return rf[get_rs1()] >= rf[get_rs2()]; }
        using branch_op = bool (core::*)();

    private:
        int32_t rf[32];
        uint32_t pc;
        uint32_t next_pc;
        uint32_t inst;
        uint64_t inst_cnt = 0;
        memory *mem;
        std::unordered_map<int8_t, decoder> decoder_map;
        std::unordered_map<int8_t, alu_op> alu_map;
        std::unordered_map<int8_t, load_op> load_map;
        std::unordered_map<int8_t, store_op> store_map;
        std::unordered_map<int8_t, branch_op> branch_map;
};
