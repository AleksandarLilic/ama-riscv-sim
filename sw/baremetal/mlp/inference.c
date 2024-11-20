#include "inference.h"

// based on https://github.com/cpldcpu/BitNetMCU/blob/main/BitNetMCU_inference.c

static uint32_t relu_norm(int32_t* input, int8_t* output, uint32_t n_input) {
    int32_t max_val = -INT32_MAX;
    int32_t max_pos = 255;
    uint32_t scale;
    uint32_t shift;
    int32_t rounding;
    int32_t tmp;

    // Find the maximum value in the input array
    for (uint32_t i = 0; i < n_input; i++) {
        if (input[i] > max_val) {
            max_val = input[i];
            max_pos = i;
        }
    }

    // Normalization
    // Dynamic shift according to max value in the input array
    // define max range, all bits above 7 will be shifted down
    scale = max_val >> 7;
    shift = 0;

    while (scale>0) {
        shift++;
        scale >>= 1;
    }

    // impact of rounding is almost negligible (+0.03% in eval accuracy)
    // But rounding affects mismatch to python inference engine
    rounding = (1 << (shift)) >> 1;

    // Apply ReLU activation and normalize to 8-bit
    for (uint32_t i = 0; i < n_input; i++) {
        // Apply ReLU activation
        if (input[i] < 0) {
            output[i] = 0;
        } else {
            tmp = (input[i] + rounding) >> shift;
            // clipping needed to catch overflow from rounding
            if (tmp > 127) output[i] = 127;
            else output[i] = tmp;
        }
    }

    return max_pos;
}

static void fc_layer(int8_t* activations, const int8_t* weights,
                     int32_t* output, uint32_t n_input, uint32_t n_output) {

    for (uint32_t i = 0; i < n_output; i++) {
        // pointer to the weights of the current neuron
        const int8_t* weightidx = weights + i * n_input;
        output[i] = dot_product_int8(activations, weightidx, n_input);
    }
}

uint32_t run_inference(uint8_t* input_img) {
    int32_t layer_out[64];
    uint8_t layer_in[64];

    #ifdef CUSTOM_ISA
    LOAD_AND_RESERVE_SCP(input_img);
    LOAD_AND_RESERVE_SCP(input_img+64);
    LOAD_AND_RESERVE_SCP(input_img+128);
    LOAD_AND_RESERVE_SCP(input_img+192);
    #endif
    fc_layer(input_img, fc1_weight, layer_out, FC1_WEIGHT_IN, FC1_WEIGHT_OUT);
    #ifdef CUSTOM_ISA
    RELEASE_SCP(input_img);
    RELEASE_SCP(input_img+64);
    RELEASE_SCP(input_img+128);
    RELEASE_SCP(input_img+192);
    // and move 'layer_out' (allocated on the stack) to scp
    LOAD_AND_RESERVE_SCP(layer_out);
    LOAD_AND_RESERVE_SCP(layer_out+16);
    LOAD_AND_RESERVE_SCP(layer_out+32);
    LOAD_AND_RESERVE_SCP(layer_out+48);
    // no more space in scp, so LRU will have to do the job
    //LOAD_AND_RESERVE_SCP(layer_in);
    #endif
    relu_norm(layer_out, layer_in, FC1_WEIGHT_OUT);

    fc_layer(layer_in, fc2_weight, layer_out, FC2_WEIGHT_IN, FC2_WEIGHT_OUT);
    relu_norm(layer_out, layer_in, FC2_WEIGHT_OUT);

    fc_layer(layer_in, fc3_weight, layer_out, FC3_WEIGHT_IN, FC3_WEIGHT_OUT);
    relu_norm(layer_out, layer_in, FC3_WEIGHT_OUT);

    fc_layer(layer_in, fc_last_weight, layer_out,
             FC_LAST_WEIGHT_IN, FC_LAST_WEIGHT_OUT);
    uint32_t out = relu_norm(layer_out, layer_in, FC_LAST_WEIGHT_OUT);
    #ifdef CUSTOM_ISA
    // be a good citizen and release the scp
    RELEASE_SCP(layer_out);
    RELEASE_SCP(layer_out+16);
    RELEASE_SCP(layer_out+32);
    RELEASE_SCP(layer_out+48);
    #endif
    return out;
}
