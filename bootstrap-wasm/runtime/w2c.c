/* w2c.c: the runtime functions of the translated SultanC seed (see w2c.h).
 * Copyright 2026 SultanC contributors. MIT license (see repository LICENSE).
 */
#include "w2c.h"

#include <stdio.h>
#include <stdlib.h>

#define W2C_STRINGIFY_(x) #x
#define W2C_STRINGIFY(x) W2C_STRINGIFY_(x)

void w2c_trap(const char *why) {
    fflush(stdout);
    fprintf(stderr, "sultanc (wasm bootstrap): trap: %s\n", why);
    abort();
}

/* ---- calls -------------------------------------------------------------- */

uint32_t w2c_call_depth;
uintptr_t w2c_stack_low = 0, w2c_stack_high = UINTPTR_MAX;

void w2c_call_exhausted(void) {
    if (w2c_call_depth > W2C_CALL_DEPTH_LIMIT)
        w2c_trap("call stack exhausted (more than " W2C_STRINGIFY(W2C_CALL_DEPTH_LIMIT) " nested calls)");
    w2c_trap("call stack exhausted (native stack)");
}

/* The check keeps a reserve below the bound for the callee's frame, the WASI
 * host functions and the trap report. */
void w2c_limit_native_stack(size_t budget) {
    char here;
    uintptr_t base = (uintptr_t)&here, reserve = budget / 8 < 1048576u ? budget / 8 : 1048576u;
    uintptr_t room = budget > reserve ? (uintptr_t)(budget - reserve) : 0;
    w2c_stack_low = base > room ? base - room : 0;
    w2c_stack_high = UINTPTR_MAX - base > room ? base + room : UINTPTR_MAX;
}

/* ---- linear memory ------------------------------------------------------ */

uint8_t *w2c_memory;
uint64_t w2c_memory_size;

void w2c_init_memory(uint32_t pages) {
    const uint32_t one = 1;
    unsigned char first;
    memcpy(&first, &one, 1);
    if ((first == 1) == (W2C_BIG_ENDIAN != 0)) w2c_trap("host byte order detection disagrees with W2C_BIG_ENDIAN");
    w2c_memory_size = (uint64_t)pages * 65536u;
    w2c_memory = (uint8_t *)calloc(1, (size_t)(w2c_memory_size ? w2c_memory_size : 1));
    if (!w2c_memory) w2c_trap("cannot allocate linear memory");
}

uint32_t w2c_memory_pages(void) { return (uint32_t)(w2c_memory_size / 65536u); }

uint32_t w2c_memory_grow(uint32_t pages) {
    uint64_t old = w2c_memory_size / 65536u, next = old + pages;
    uint8_t *p;
    if (next > w2c_memory_max_pages || next > 65536u) return UINT32_C(0xffffffff);
    if (!pages) return (uint32_t)old;
    if ((uint64_t)(size_t)(next * 65536u) != next * 65536u) return UINT32_C(0xffffffff);
    p = (uint8_t *)realloc(w2c_memory, (size_t)(next * 65536u));
    if (!p) return UINT32_C(0xffffffff);
    memset(p + w2c_memory_size, 0, (size_t)(pages * UINT64_C(65536)));
    w2c_memory = p;
    w2c_memory_size = next * 65536u;
    return (uint32_t)old;
}

void w2c_memory_copy(uint32_t d, uint32_t s, uint32_t n) {
    if ((uint64_t)d + n > w2c_memory_size || (uint64_t)s + n > w2c_memory_size)
        w2c_trap("out-of-bounds memory.copy");
    if (n) memmove(w2c_memory + d, w2c_memory + s, n);
}

void w2c_memory_fill(uint32_t d, uint32_t v, uint32_t n) {
    if ((uint64_t)d + n > w2c_memory_size) w2c_trap("out-of-bounds memory.fill");
    if (n) memset(w2c_memory + d, (int)(v & 0xffu), n);
}
