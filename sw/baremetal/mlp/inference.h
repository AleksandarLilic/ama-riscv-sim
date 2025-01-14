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

uint32_t run_inference(int8_t* input_img);
