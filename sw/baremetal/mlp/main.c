#include <stdint.h>

#include "inference.h"

uint8_t label_0 = 7;
int8_t input_img_0[] __attribute__((aligned(64))) = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 10, 8, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 40, 85, 79, 57, 43, 42, 42, 42, 36, 10, 0, 0, 0, 0, 0, 0, 25, 49, 56, 82, 88, 93, 94, 92, 109, 40, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 5, 8, 9, 52, 96, 21, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 15, 98, 60, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 55, 96, 17, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 12, 89, 50, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6, 65, 92, 11, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 46, 104, 45, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 16, 90, 71, 7, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 64, 112, 31, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 14, 104, 96, 20, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8, 49, 25, 1, 0, 0, 0, 0, 0, 0, 0 };

uint8_t label_1 = 2;
int8_t input_img_1[] __attribute__((aligned(64))) = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 18, 30, 29, 11, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 9, 69, 109, 114, 115, 77, 5, 0, 0, 0, 0, 0, 0, 0, 0, 2, 52, 113, 69, 41, 74, 105, 16, 0, 0, 0, 0, 0, 0, 0, 0, 2, 42, 56, 6, 10, 84, 99, 10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 2, 48, 113, 71, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 23, 98, 96, 19, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 83, 108, 36, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 37, 115, 70, 7, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 12, 89, 107, 27, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 46, 123, 54, 4, 2, 0, 0, 2, 8, 15, 12, 1, 0, 0, 0, 0, 50, 125, 93, 77, 76, 58, 57, 76, 88, 96, 81, 19, 0, 0, 0, 0, 25, 88, 97, 103, 119, 111, 94, 86, 72, 55, 38, 11, 0, 0, 0, 0, 0, 9, 12, 16, 24, 22, 13, 7, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

void main(void) {
    // read both clk and time, though only one can be used for known fixed freq
    uint32_t start_clk = get_cpu_cycles();
    uint32_t start_time = get_cpu_time();
    LOG_START;
    uint32_t predicted = run_inference(input_img_0);
    LOG_STOP;
    uint32_t end_clk = get_cpu_cycles();
    uint32_t end_time = get_cpu_time();

    uint32_t clk_diff = end_clk - start_clk;
    uint32_t time_diff = (end_time - start_time) / 1000;
    printf("Predicted: %d (label: %d); "
           "Performance: cycles: %d, time: %d ms, Inf/s: %d\n",
           predicted, label_0, clk_diff, time_diff, (1000 / time_diff));

    // assumed model is accurate for the provided input
    if (predicted != label_0) {
        write_mismatch(predicted, label_0, 1);
        fail();
    }

    pass();
}
