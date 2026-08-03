#include <stdint.h>

#include "common.h"
#include "inference.h"

#if defined(W8A8)
static const char* q_str = "W8A8";
#elif defined(W4A8)
static const char* q_str = "W4A8";
#elif defined(W2A8)
static const char* q_str = "W2A8";
#endif

#ifndef UART_INPUT
#include "input_img.h"

#else
int8_t input_img[FC1_WEIGHT_IN] __attribute__((aligned(CACHE_LINE_SIZE)));

#endif

void run(uint8_t* label, int8_t* input_img) {
    #ifdef MHPM
    #ifdef MHPM_TDA
    tda_cnt_t tda_pe = {0ul};
    init_tda_counters();
    #else
    hw_cnt_t hw_pe = {0ul};
    init_hw_counters();
    #endif
    #else // !MHPM
    set_cpu_cycles(0u);
    #endif

    uint32_t start_time = get_cpu_time();
    PROF_START;
    uint32_t predicted = run_inference(input_img);
    PROF_STOP;
    uint32_t end_time = get_cpu_time();
    uint32_t clks;

    #ifdef MHPM
    #ifdef MHPM_TDA
    save_tda_counters(&tda_pe);
    clks = tda_pe.cycles;
    #else
    save_hw_counters(&hw_pe);
    clks = hw_pe.cycles;
    #endif
    #else // !MHPM
    clks = get_cpu_cycles();
    #endif

    uint32_t time_diff = (end_time - start_time); // us
    printf("Predicted: %u (label: %u); "
           "Performance: cycles: %u, time: %u us, Inf/s: %u\n\n",
           predicted, *label, clks, time_diff, (1000000 / time_diff));

    #ifdef MHPM
    #ifdef MHPM_TDA
    print_tda_counters(&tda_pe);
    print_tda_counters_json(&tda_pe);
    #else
    print_hw_counters(&hw_pe);
    print_hw_counters_json(&hw_pe);
    #endif
    #endif

    // assumed model is accurate for the provided input
    if (predicted != *label) {
        write_mismatch(predicted, *label, 1);
        fail();
    }
}

#ifdef UART_INPUT
void main() {
    uint32_t iter = 0;
    printf("\nMLP model quantization: %s\n", q_str);
    while(1) {
        printf("\nIter %0d\nWaiting on input...\n", iter);
        for (size_t i = 0; i < FC1_WEIGHT_IN; i++) {
            while (!UART0_RX_VALID);
            input_img[i] = UART0->rx_data;
        }
        printf("Input saved. Waiting on label...\n");
        while (!UART0_RX_VALID);
        uint8_t label = UART0->rx_data;
        printf("Got label %0d. Running...\n", label);
        run(&label, input_img);
        iter++;
        for (size_t i = 0; i < 1<<26; i++) { }
    }
}

#else
void main() {
    printf("\nMLP model quantization: %s\n", q_str);
    run(&label_0, input_img_0);
    pass();
}
#endif
