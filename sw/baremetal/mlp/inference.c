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

// both operands are already k-contiguous
// - weights are (n_output x n_input)
// - activation panel is (batch x n_input)
// so nothing is transposed or packed;
// 'c_t' writes C as `c[n][m]`:
// each image's outputs stay contiguous for relu_norm,
// and become the next layer's activation row unchanged
static INLINE void fc_layer(
    const int8_t* activations, const int8_t* weights,
    int32_t* output, size_t n_input, size_t n_output,
    size_t batch)
{
    #if defined(W8A8)
    #define FUNC m_gemm_i8_i8
    #elif defined(W4A8)
    #define FUNC m_gemm_i4_i8
    #elif defined(W2A8)
    #define FUNC m_gemm_i2_i8
    #endif
    FUNC(
        n_output, batch, n_input,
        weights, n_input, activations, n_input, output, n_output, true
    );
    #undef FUNC
}

// max and shift are per image
// due to c_t, images are contiguous in the array
static INLINE void relu_norm_batch(
    int32_t* input, int8_t* output, uint32_t n, size_t batch)
{
    for (size_t bi = 0; bi < batch; bi++) {
        relu_norm((input + bi*n), (output + bi*n), n, false);
    }
}

void run_inference(
    const int8_t* img, uint8_t* predicted, const size_t batch)
{
    // align so that both fit nicely on cache boundaries;
    // sized for 'BATCH' and the widest layer (64 outputs), used up to 'batch'
    int32_t __attribute__((aligned(CACHE_LINE_SIZE))) layer_out[BATCH*64];
    int8_t __attribute__((aligned(CACHE_LINE_SIZE))) layer_in[BATCH*64];

    fc_layer(img, fc1_weight, layer_out, FC1_WEIGHT_IN, FC1_WEIGHT_OUT, batch);
    relu_norm_batch(layer_out, layer_in, FC1_WEIGHT_OUT, batch);

    fc_layer(
        layer_in, fc2_weight, layer_out, FC2_WEIGHT_IN, FC2_WEIGHT_OUT, batch
    );
    relu_norm_batch(layer_out, layer_in, FC2_WEIGHT_OUT, batch);

    fc_layer(
        layer_in, fc3_weight, layer_out, FC3_WEIGHT_IN, FC3_WEIGHT_OUT, batch
    );
    relu_norm_batch(layer_out, layer_in, FC3_WEIGHT_OUT, batch);

    fc_layer(
        layer_in, fc_last_weight, layer_out,
        FC_LAST_WEIGHT_IN, FC_LAST_WEIGHT_OUT, batch
    );

    // last layer needs no normalization, only the argmax, per image
    for (size_t b = 0; b < batch; b++) {
        const size_t o = (b * FC_LAST_WEIGHT_OUT);
        predicted[b] = (uint8_t)relu_norm(
            (layer_out + o), (layer_in + o), FC_LAST_WEIGHT_OUT, true
        );
    }
}
