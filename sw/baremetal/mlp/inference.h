#include "stdbool.h"
#include "common.h"
#include "common_math.h"

#ifdef W8A8
#include "model_w8a8_64-64-64-10.h"
#elif defined(W4A8)
#include "model_w4a8_64-64-64-10.h"
#elif defined(W2A8)
#include "model_w2a8_64-64-64-10.h"
#else // default
#define W8A8
#include "model_w8a8_64-64-64-10.h"
//_Static_assert(0, "Unsupported model selected");
#endif

static uint32_t relu_norm(
    int32_t* input, int8_t* output, uint32_t n_input, bool get_idx);

static void fc_layer(int8_t* activations, const int8_t* weights,
                     int32_t* output, uint32_t n_input, uint32_t n_output);

uint32_t run_inference(uint8_t* input_img);
