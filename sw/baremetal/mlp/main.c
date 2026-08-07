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
int8_t input_img[1*FC1_WEIGHT_IN] __attribute__((aligned(CACHE_LINE_SIZE)));
#endif

void run(uint8_t* label, int8_t* input_img, size_t inf, size_t batch) {
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

    uint8_t predicted[inf];
    int8_t* img;
    uint8_t* pred;
    uint32_t runs = (inf/batch);
    uint32_t start_time = get_cpu_time();

    PROF_START;
    for (size_t r = 0; r < runs; r++) {
        img = (input_img + (r * batch * FC1_WEIGHT_IN));
        pred = (predicted + (r * batch));
        run_inference(img, pred, batch);
    }
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

    printf("Predicted (label): ");
    for (size_t i = 0; i < inf; i++) {
        printf("%u (%u)", predicted[i], label[i]);
        if (i < inf - 1) printf(", ");
    }
    printf("\n");
    uint32_t time_diff = (end_time - start_time); // us, for the whole batch
    uint32_t infs = ((1000000 * (uint32_t)inf) / time_diff); // per image
    uint32_t bps = ((1000000 * (uint32_t)runs) / time_diff); // per batch
    printf(
        "Performance: cycles: %u, time: %u us, Inf/s: %u, Batch/s: %u\n\n",
        clks, time_diff, infs, bps
    );

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
    bool failed = false;
    for (size_t i = 0; i < inf; i++) {
        bool mismatch = (predicted[i] != label[i]);
        if (mismatch) write_mismatch(predicted[i], label[i], i + 1);
        failed |= mismatch;
    }
    if (failed) fail();
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
        run(&label, input_img, 1);
        iter++;
        for (size_t i = 0; i < 1<<26; i++) { }
    }
}

#else
void main() {
    printf(
        "\nMLP model quantization: %s (inferences: %u, batch: %u)\n",
        q_str, INF, BATCH
    );
    run(label, input_img, INF, BATCH);
    pass();
}
#endif
