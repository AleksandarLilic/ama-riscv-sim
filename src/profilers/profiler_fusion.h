#pragma once

#include <iostream>
#include <string>

#include "inst_parser.h"

enum class state {
    START,
    LEA_1, // load effective address
    LEA_2,
    LEA_MATCH,
    IL_1, // indexed load
    IL_2,
    IL_MATCH,
    LEA_IL_3, // lea + indexed load
    LEA_IL_MATCH
};

enum class trigger {
    slli_lea,
    add_il,
};

struct inst_opt {
    trigger trig;
    uint32_t inst;
    uint32_t inst_nx;
    bool rvc;
};

class profiler_fusion {
    public:
        profiler_fusion() : current_state(state::START) {}
        void attack(inst_opt opt);
        void finish();

    private:
        void transition();
        state lea_1();
        state lea_2();
        uint64_t lea_match_cnt = 0;

    private:
        state current_state;
        inst_parser ip;
        inst_opt opt;
};
