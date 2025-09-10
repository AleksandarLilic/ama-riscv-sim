// barebones bump/arena allocator (RISC-V GCC)

#ifndef ARENA_H
#define ARENA_H

#include "common.h"

void *memset(void *dst, int c, size_t n); // fwd decl., available as memset.S

// build with '-DARENA_ASSERT=' if dc about asserts and want smaller code size
#ifndef ARENA_ASSERT
// assert that value exists (in this case arena pointer)
#include <assert.h>
#define ARENA_ASSERT(x) assert(x)
#endif

typedef struct arena_t {
    uint8_t *base; // buffer start
    size_t capacity; // total bytes in buffer
    size_t offset; // next free byte offset
} arena_t;

// helpers
static inline bool arena_is_pow2(size_t x) { return x && ((x & (x - 1)) == 0); }

static inline size_t arena_align_up_size(size_t v, size_t align) {
    // align must be power-of-two
    return (v + (align - 1u)) & ~(align - 1u);
}

static inline uintptr_t arena_align_up_ptr(uintptr_t p, size_t align) {
    return (uintptr_t)arena_align_up_size((size_t)p, align);
}

// initialize arena over a user-provided buffer
static inline void arena_init(arena_t *a, void *buffer, size_t capacity_bytes) {
    ARENA_ASSERT(a && buffer);
    a->base = (uint8_t*)buffer;
    a->capacity = capacity_bytes;
    a->offset = 0u;
}

// reset to empty (does not clear memory)
static inline void arena_reset(arena_t *a) {
    ARENA_ASSERT(a);
    a->offset = 0u;
}

// Save/restore a mark (cheap LIFO-style frees)
static inline size_t arena_mark(const arena_t *a) {
    ARENA_ASSERT(a);
    return a->offset;
}

static inline void arena_pop_to(arena_t *a, size_t mark) {
    ARENA_ASSERT(a && mark <= a->capacity);
    a->offset = mark;
}

// allocate 'size' bytes with 'align' (power-of-two)
// returns NULL on OOM or bad align
static inline void *arena_alloc(arena_t *a, size_t size, size_t align) {
    ARENA_ASSERT(a);
    if (size == 0) return NULL;
    if (align == 0) align = 1u;
    if (!arena_is_pow2(align)) return NULL;

    uintptr_t base_addr = (uintptr_t)a->base;
    uintptr_t curr_addr = base_addr + (uintptr_t)a->offset;
    uintptr_t aligned_addr = arena_align_up_ptr(curr_addr, align);
    size_t aligned_off = (size_t)(aligned_addr - base_addr);

    // overflow / OOM protection
    if (aligned_off > a->capacity) return NULL;
    if (size > (a->capacity - aligned_off)) return NULL;

    a->offset = aligned_off + size;
    return (void*)aligned_addr;
}

// allocate and zero-init
static inline void *arena_alloc_zero(arena_t *a, size_t size, size_t align) {
    void *p = arena_alloc(a, size, align);
    if (p) memset(p, 0, size);
    return p;
}

// introspection
static inline size_t
arena_used(const arena_t *a) { return a ? a->offset : 0u; }
static inline size_t
arena_avail(const arena_t *a) { return a ? (a->capacity - a->offset) : 0u; }
static inline void
*arena_ptr(const arena_t *a) { return a ? (a->base + a->offset) : NULL; }

// convenience macros (requires C11 or newer for _Alignof)
#if __STDC_VERSION__ >= 201112L
#define ARENA_ALLOC_T(arena_ptr, T, count) \
    ((T*)arena_alloc((arena_ptr), sizeof(T)*(size_t)(count), _Alignof(T)))
#define ARENA_ALLOC_ZERO_T(arena_ptr, T, count) \
    ((T*)arena_alloc_zero((arena_ptr), sizeof(T)*(size_t)(count), _Alignof(T)))
#endif

#endif // ARENA_H
