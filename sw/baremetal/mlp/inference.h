#include "stdbool.h"
#include "common.h"
#include "common_math.h"
#include "model.h"

static uint32_t relu_norm(
    int32_t* input, int8_t* output, uint32_t n_input, bool get_idx);

static void fc_layer(int8_t* activations, const int8_t* weights,
                     int32_t* output, uint32_t n_input, uint32_t n_output);

uint32_t run_inference(uint8_t* input_img);
