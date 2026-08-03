#include "inference.h"
#include "common_math_simd_intrinsics.h"
#include "common_math_simd_v_load_store.h"

// based on https://github.com/cpldcpu/BitNetMCU/blob/main/BitNetMCU_inference.c

static uint32_t relu_norm(
    int32_t* input, int8_t* output, uint32_t n_input, bool get_idx)
{
    int32_t max_val = INT32_MIN;
    int32_t max_pos = 255;

    // find the maximum value in the input array,
    // used only for the last layer which doesn't need normalization
    if (get_idx) {
        for (uint32_t i = 0; i < n_input; i++) {
            max_val = max(input[i], max_val);
            if (max_val == input[i]) max_pos = i;
        }
        return max_pos;
    }

    uint32_t i = 0;
    // hide load-to-use delay
    const uint32_t unroll_end = (n_input & ~1u);
    for (; i < unroll_end; i += 2) {
        const int32_t in0 = input[i];
        const int32_t in1 = input[i+1];
        // force back-to-back 'max' for scheduling
        //max_val = max(in0, max_val);
        //max_val = max(in1, max_val);
        asm volatile(
            ".insn r 0x33, 0x6, 0x5, %[out], %[out], %[in0]\n\t"
            ".insn r 0x33, 0x6, 0x5, %[out], %[out], %[in1]\n\t"
            : [out] "+r" (max_val)
            : [in0] "r" (in0), [in1] "r" (in1)
        );
    }
    for (; i < n_input; i++) {
        max_val = max(input[i], max_val);
    }

    // Normalization
    // Dynamic shift according to max value in the input array
    // define max range, all bits above 7 will be shifted down
    // values less than 127 (incl. negative) have no shift
    uint32_t scale = (max_val > 127) ? (uint32_t)max_val >> 7 : 0;
    uint32_t shift = 0;
    while (scale > 0) {
        shift++;
        scale >>= 1;
    }

    // Apply ReLU activation and normalize to 8-bit
    i = 0;
    #ifdef __riscv_xsimd
    const uint32_t simd_end = (n_input & ~3u);
    const int8x4_t zero = {.v = 0};
    for (; i < simd_end; i += 4) {
        // load all inputs first to hide the load-to-use delay
        const int32_t x0 = input[i];
        const int32_t x1 = input[i + 1];
        const int32_t x2 = input[i + 2];
        const int32_t x3 = input[i + 3];

        const int16x2_t lo = _qnarrow32(x0 >> shift, x1 >> shift);
        const int16x2_t hi = _qnarrow32(x2 >> shift, x3 >> shift);
        int8x4_t packed = _qnarrow16(lo, hi);
        packed = _max8(packed, zero);
        v_store_int8x4(output + i, packed);
    }
    #endif
    for (; i < n_input; i++) {
        int32_t relu_out = max(input[i], 0);
        output[i] = min(relu_out >> shift, 127);
    }

    return max_pos;
}

static INLINE void fc_layer(
    int8_t* activations, const int8_t* weights,
    int32_t* output, uint32_t n_input, uint32_t n_output)
{
    #ifdef W8A8
    #define FUNC m_gemv_i8_i8
    #elif defined(W4A8)
    #define FUNC m_gemv_i4_i8
    #elif defined(W2A8)
    #define FUNC m_gemv_i2_i8
    #endif
    FUNC(n_output, n_input, weights, n_input, activations, output);
    #undef FUNC
}

uint32_t run_inference(int8_t* input_img) {
    // align so that both fit nicely on cache boundaries
    int32_t __attribute__((aligned(CACHE_LINE_SIZE))) layer_out[64];
    int8_t __attribute__((aligned(CACHE_LINE_SIZE))) layer_in[64];
    #ifdef CUSTOM_ISA_SCP
    size_t stride;
    #endif

    #ifdef CUSTOM_ISA_SCP
    //#pragma GCC unroll 4 // force if gcc doesn't unroll
    stride = CACHE_LINE_SIZE/sizeof(input_img[0]);
    for (int i = 0; i < 4; i++) SCP_LCL(input_img + stride*i);
    #endif

    fc_layer(input_img, fc1_weight, layer_out, FC1_WEIGHT_IN, FC1_WEIGHT_OUT);

    #ifdef CUSTOM_ISA_SCP
    for (int i = 0; i < 4; i++) SCP_REL(input_img + stride*i);
    // and move 'layer_out' (allocated on the stack) to scp
    stride = CACHE_LINE_SIZE/sizeof(layer_out[0]);
    for (int i = 0; i < 4; i++) SCP_LCL(layer_out + stride*i);
    SCP_LCL(layer_in);
    #endif

    relu_norm(layer_out, layer_in, FC1_WEIGHT_OUT, false);

    fc_layer(layer_in, fc2_weight, layer_out, FC2_WEIGHT_IN, FC2_WEIGHT_OUT);
    relu_norm(layer_out, layer_in, FC2_WEIGHT_OUT, false);

    fc_layer(layer_in, fc3_weight, layer_out, FC3_WEIGHT_IN, FC3_WEIGHT_OUT);
    relu_norm(layer_out, layer_in, FC3_WEIGHT_OUT, false);

    fc_layer(layer_in, fc_last_weight, layer_out,
             FC_LAST_WEIGHT_IN, FC_LAST_WEIGHT_OUT);
    uint32_t out = relu_norm(layer_out, layer_in, FC_LAST_WEIGHT_OUT, true);

    #ifdef CUSTOM_ISA_SCP
    // be a good citizen and release the scp
    for (int i = 0; i < 4; i++) SCP_REL(layer_out + stride*i);
    SCP_REL(layer_in);
    #endif

    return out;
}
