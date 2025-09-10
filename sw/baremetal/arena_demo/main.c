#include <stdint.h>
#include "common.h"
#include "arena_alloc.h"

#define ARENA_CAP 1024*4 // e.g. 4k
static uint8_t g_arena_buf[ARENA_CAP] __attribute__((aligned(CACHE_LINE_SIZE)));

void main() {
    arena_t a;
    arena_init(&a, g_arena_buf, sizeof g_arena_buf);

    const static size_t ints = 256;
    // allocate ints integers, aligned for uint32_t
    uint32_t* xs = ARENA_ALLOC_T(&a, uint32_t, ints);
    if (!xs) {
        printf("OOM\n");
        write_mismatch(1, 13, 1);
        fail();
    }

    // more arena features
    size_t mark = arena_mark(&a);

    // allocate a 64B temporary block aligned to 32 bytes
    uint32_t* tmp = arena_alloc_zero(&a, 64, 32);
    if (!tmp) {
        printf("OOM tmp\n");
        write_mismatch(1, 13, 2);
        fail();
    }

    // potentially do anything with temp buffer, then discard just the temp

    // discard temporaries
    arena_pop_to(&a, mark);

    // reuse memory from the mark onward
    char* msg = ARENA_ALLOC_T(&a, char, 32);
    if (!msg) {
        printf("OOM msg\n");
        write_mismatch(1, 13, 3);
        fail();
    }

    snprintf(msg, 32, "used=%u avail=%u\n", arena_used(&a), arena_avail(&a));
    printf(msg);

    const static uint32_t known_used = ints*4 + 32*1;
    ARENA_ASSERT(arena_used(&a)==known_used);

    // wipe logical state (does not zero memory)
    arena_reset(&a);

    pass();
}
