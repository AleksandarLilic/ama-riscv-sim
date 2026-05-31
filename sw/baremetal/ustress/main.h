#include <stdint.h>
#include "common.h"

#ifndef USTRESS_TEST_NAME
#define USTRESS_TEST_NAME "undefined name"
#endif

#ifndef LOOPS
#define LOOPS 1000
#endif

#ifdef MHPM
#ifdef MHPM_TDA
tda_cnt_t tda_pe = {0ul};
#else
hw_cnt_t hw_pe = {0ul};
#endif
#endif

void stress(long runs);

void main() {
    uint32_t start_time, end_time, clks, time_diff;
    printf("\nRunning ustress %s...\n", USTRESS_TEST_NAME);
    volatile uint32_t loops = LOOPS;

    #ifdef MHPM
    #ifdef MHPM_TDA
    init_tda_counters();
    #else
    init_hw_counters();
    #endif
    #endif

    start_time = get_cpu_time();
    PROF_START;
    stress(loops);
    PROF_STOP;
    end_time = get_cpu_time();

    #ifdef MHPM
    #ifdef MHPM_TDA
    save_tda_counters(&tda_pe);
    //print_tda_counters(&tda_pe);
    print_tda_counters_json(&tda_pe);
    clks = tda_pe.cycles;
    #else
    save_hw_counters(&hw_pe);
    //print_hw_counters(&hw_pe);
    print_hw_counters_json(&hw_pe);
    clks = hw_pe.cycles;
    #endif
    #else // !MHPM
    clks = get_cpu_cycles();
    #endif

    time_diff = ((end_time - start_time) / 1000);
    printf("Ran for %d cycles and %d ms\n", clks, time_diff);

    pass();
}
