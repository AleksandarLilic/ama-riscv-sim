#include "inference.h"

// based on https://github.com/cpldcpu/BitNetMCU/blob/main/BitNetMCU_inference.c

static uint32_t relu_norm(
    int32_t* input, int8_t* output, uint32_t n_input, bool get_idx) {
    int32_t max_val = -INT32_MAX;
    int32_t max_pos = 255;
    // Find the maximum value in the input array
    if (get_idx) {
        for (uint32_t i = 0; i < n_input; i++) {
            max_val = max(input[i], max_val);
            if (max_val == input[i]) max_pos = i;
        }
    } else {
        // dc about the index except in the last layer
        for (uint32_t i = 0; i < n_input; i++) {
            max_val = max(input[i], max_val);
        }
    }

    // Normalization
    // Dynamic shift according to max value in the input array
    // define max range, all bits above 7 will be shifted down
    uint32_t scale = max_val >> 7;
    uint32_t shift = 0;
    while (scale>0) {
        shift++;
        scale >>= 1;
    }

    // Apply ReLU activation and normalize to 8-bit
    for (uint32_t i = 0; i < n_input; i++) {
        int32_t relu_out = max(input[i], 0);
        output[i] = min(relu_out >> shift, 127);
    }

    return max_pos;
}

static void fc_layer(int8_t* activations, const int8_t* weights,
                     int32_t* output, uint32_t n_input, uint32_t n_output) {

    for (uint32_t i = 0; i < n_output; i++) {
        // pointer to the weights of the current neuron
        #ifdef W8A8
        const int8_t* weightidx = weights + i * n_input;
        #ifdef CUSTOM_ISA
        output[i] = _simd_dot_product_int8(activations, weightidx, n_input);
        #else
        output[i] = dot_product_int8(activations, weightidx, n_input);
        #endif

        #elif defined(W4A8)
        const int8_t* weightidx = weights + (i * (n_input >> 1));
        #ifdef CUSTOM_ISA
        output[i] = _simd_dot_product_int8_int4(activations, weightidx,n_input);
        #else
        output[i] = dot_product_int8_int4(activations, weightidx, n_input);
        #endif
        #endif
    }
}

uint32_t run_inference(uint8_t* input_img) {
    int32_t layer_out[64];
    uint8_t layer_in[64];

    #ifdef CUSTOM_ISA
    //#pragma GCC unroll 4 // force if gcc doesn't unroll
    for (int i = 0; i < 4; i++) LOAD_AND_RESERVE_SCP(input_img + 64*i);
    #endif

    fc_layer(input_img, fc1_weight, layer_out, FC1_WEIGHT_IN, FC1_WEIGHT_OUT);

    #ifdef CUSTOM_ISA
    for (int i = 0; i < 4; i++) RELEASE_SCP(input_img + 64*i);
    // and move 'layer_out' (allocated on the stack) to scp
    for (int i = 0; i < 4; i++) LOAD_AND_RESERVE_SCP(layer_out + 16*i);
    // more contention d$ if 'layer_in' is also moved to scp
    //LOAD_AND_RESERVE_SCP(layer_in);
    #endif

    relu_norm(layer_out, layer_in, FC1_WEIGHT_OUT, false);

    fc_layer(layer_in, fc2_weight, layer_out, FC2_WEIGHT_IN, FC2_WEIGHT_OUT);
    relu_norm(layer_out, layer_in, FC2_WEIGHT_OUT, false);

    fc_layer(layer_in, fc3_weight, layer_out, FC3_WEIGHT_IN, FC3_WEIGHT_OUT);
    relu_norm(layer_out, layer_in, FC3_WEIGHT_OUT, false);

    fc_layer(layer_in, fc_last_weight, layer_out,
             FC_LAST_WEIGHT_IN, FC_LAST_WEIGHT_OUT);
    uint32_t out = relu_norm(layer_out, layer_in, FC_LAST_WEIGHT_OUT, true);

    #ifdef CUSTOM_ISA
    // be a good citizen and release the scp
    for (int i = 0; i < 4; i++) RELEASE_SCP(layer_out + 16*i);
    #endif

    return out;
}
