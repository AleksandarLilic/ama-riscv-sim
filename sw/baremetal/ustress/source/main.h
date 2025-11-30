/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright 2022-2025 Arm Limited
 */

#ifndef _MAIN_H
#define _MAIN_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

void stress(long runs);

/* run each stress test about 1s */
struct {
    const char* exec;
    long runs;
} exec_runs[] = {
    {"branch_direct_workload", 20000000},
    {"call_return_workload", 15000},
    {"div32_workload", 200000000},
    {"div64_workload", 200000000},
    {"double2int_workload", 1500000000},
    {"fpdiv_workload", 120000000},
    {"fpmac_workload", 200000000},
    {"fpmul_workload", 260000000},
    {"fpsqrt_workload", 120000000},
    {"int2double_workload", 1500000000},
    {"isb_workload", 2800},
    {"l1d_cache_workload", 440000},
    {"l1d_tlb_workload", 1300000},
    {"l1i_cache_workload", 8000000},
    {"l2d_cache_workload", 4000},
    {"load_after_store_workload", 2300000},
    {"mac32_workload", 400000000},
    {"mac64_workload", 330000000},
    {"memcpy_workload", 2200000},
    {"mul32_workload", 400000000},
    {"mul64_workload", 330000000},
    {"store_buffer_full_workload", 30000000},
};

long runs_from_exec(const char *exec) {
    for (int i = 0; i < sizeof(exec_runs)/sizeof(exec_runs[0]); ++i) {
        if (strstr(exec, exec_runs[i].exec)) {
            return exec_runs[i].runs;
        }
    }
    abort();
    return 0;
}

void usage(const char *exec) {
    printf("usage: \n"
           "\t%s [MULTIPLIER]\n"
           "\t%s [--help]\n\n"
           "\tMULTIPLIER\tMultiply the number of iterations performed by this workload. (Default: 1.0)\n", exec, exec);
}

int main(int argc, char *argv[]) {
    if (argc > 2) {
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    double multiplier = 1.0;
    if (argc == 2) {
        if (strcmp(argv[1], "--help") == 0) {
            usage(argv[0]);
            return EXIT_SUCCESS;
        }

        multiplier = strtod(argv[1], NULL);
        if (multiplier <= 0.0) {
            usage(argv[0]);
            return EXIT_FAILURE;
        }
    }

    long runs = runs_from_exec(argv[0]) * multiplier;
    stress(runs);

    return EXIT_SUCCESS;
}

#endif
