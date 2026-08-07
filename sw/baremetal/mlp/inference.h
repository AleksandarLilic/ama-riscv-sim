#ifndef INFERENCE_H
#define INFERENCE_H

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

#ifndef UART_INPUT
// uart runs batch of one, one inf at the time
#ifndef BATCH
#define BATCH 4
#endif
_Static_assert(
    (BATCH >= 1) && (BATCH <= 16),
    "BATCH: must be between 1 and 16"
);

#ifndef INF
#define INF 16
#endif
_Static_assert(
    (INF >= 1) && (INF <= 16) && (INF >= BATCH) && ((INF % BATCH) == 0),
    "INF: must be between 1 and 16, "
    "be greater than or equal to BATCH, and be multiple of BATCH"
);
#endif

void run_inference(const int8_t* img, uint8_t* predicted, const size_t batch);

#endif // INFERENCE_H
