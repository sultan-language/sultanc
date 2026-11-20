/* w2c.h: the runtime of the translated SultanC seed.
 * Copyright 2026 SultanC contributors. MIT license (see repository LICENSE).
 *
 * sultanc-wasm2c writes each WebAssembly instruction as C over the names
 * declared here: linear memory with bounds-checked loads and stores, integer
 * operations whose C form needs care (no signed overflow, no
 * implementation-defined conversions), traps, and the call-depth guard.
 * Everything is C99. The functions behind the declarations are in w2c.c;
 * the module's own data (globals, table, data segments, entry) is generated.
 */
#ifndef SULTANC_W2C_H
#define SULTANC_W2C_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#if defined(_MSC_VER) && _MSC_VER < 1900 && !defined(inline)
#define inline __inline /* C99 inline for older MSVC */
#endif

#if defined(__GNUC__) || defined(__clang__) || defined(__TINYC__)
#define W2C_NORETURN __attribute__((noreturn))
#elif defined(_MSC_VER)
#define W2C_NORETURN __declspec(noreturn)
#else
#define W2C_NORETURN
#endif

#if defined(__BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define W2C_BIG_ENDIAN 1
#else
#define W2C_BIG_ENDIAN 0
#endif

/* ---- traps -------------------------------------------------------------- */

/* Reports a trap on stderr and aborts. */
W2C_NORETURN void w2c_trap(const char *why);

/* ---- calls -------------------------------------------------------------- */

/* Module frames allowed at once. The compiler needs a few thousand on deeply
 * nested sources; the limit keeps native stack use bounded (each level is one
 * C frame of a translated function) and is the same on every host, so where
 * a program traps does not depend on the C compiler's frame sizes. */
#define W2C_CALL_DEPTH_LIMIT 100000u

extern uint32_t w2c_call_depth;
extern uintptr_t w2c_stack_low, w2c_stack_high;
W2C_NORETURN void w2c_call_exhausted(void);
/* Called by the host on the thread that runs w2c_run, with the native stack
 * (bytes) the guest may use from there. */
void w2c_limit_native_stack(size_t budget);

/* The prologue of every translated function: counts the frame and checks the
 * native stack (a safety net for hosts with small stacks). */
static inline void w2c_enter(void) {
    char probe;
    uintptr_t sp = (uintptr_t)&probe;
    if (++w2c_call_depth > W2C_CALL_DEPTH_LIMIT || sp < w2c_stack_low || sp > w2c_stack_high)
        w2c_call_exhausted();
}

/* The epilogue, before each return. */
static inline void w2c_leave(void) { w2c_call_depth--; }

/* Indirect calls: a table entry, checked against the expected signature. */
typedef void (*w2c_funcref)(void);
w2c_funcref w2c_indirect(uint32_t index, uint32_t type);

/* ---- linear memory ------------------------------------------------------ */

extern uint8_t *w2c_memory;
extern uint64_t w2c_memory_size;
extern const uint32_t w2c_memory_max_pages; /* generated */

/* Allocates `pages` zeroed pages; first checks W2C_BIG_ENDIAN against the
 * host. */
void w2c_init_memory(uint32_t pages);
uint32_t w2c_memory_pages(void);
uint32_t w2c_memory_grow(uint32_t pages);
void w2c_memory_copy(uint32_t dst, uint32_t src, uint32_t n);
void w2c_memory_fill(uint32_t dst, uint32_t value, uint32_t n);

/* The host address of `n` bytes at a + off, or a trap. */
static inline uint8_t *w2c_addr(uint32_t a, uint32_t off, uint32_t n) {
    uint64_t e = (uint64_t)a + off;
    if (e + n > w2c_memory_size) w2c_trap("out-of-bounds memory access");
    return w2c_memory + e;
}

/* WebAssembly memory is little-endian. */
static inline uint16_t w2c_le16(uint16_t x) { return W2C_BIG_ENDIAN ? (uint16_t)(x << 8 | x >> 8) : x; }
static inline uint32_t w2c_le32(uint32_t x) {
    return W2C_BIG_ENDIAN ? ((x & 0xffu) << 24 | (x & 0xff00u) << 8 | (x >> 8 & 0xff00u) | x >> 24) : x;
}
static inline uint64_t w2c_le64(uint64_t x) {
    return W2C_BIG_ENDIAN ? ((uint64_t)w2c_le32((uint32_t)x) << 32 | w2c_le32((uint32_t)(x >> 32))) : x;
}

static inline float w2c_f32_from_bits(uint32_t b) { float f; memcpy(&f, &b, 4); return f; }
static inline double w2c_f64_from_bits(uint64_t b) { double d; memcpy(&d, &b, 8); return d; }
static inline uint32_t w2c_f32_bits(float f) { uint32_t b; memcpy(&b, &f, 4); return b; }
static inline uint64_t w2c_f64_bits(double d) { uint64_t b; memcpy(&b, &d, 8); return b; }

static inline uint32_t w2c_load_i32(uint32_t a, uint32_t o) { uint32_t v; memcpy(&v, w2c_addr(a, o, 4), 4); return w2c_le32(v); }
static inline uint64_t w2c_load_i64(uint32_t a, uint32_t o) { uint64_t v; memcpy(&v, w2c_addr(a, o, 8), 8); return w2c_le64(v); }
static inline float w2c_load_f32(uint32_t a, uint32_t o) { return w2c_f32_from_bits(w2c_load_i32(a, o)); }
static inline double w2c_load_f64(uint32_t a, uint32_t o) { return w2c_f64_from_bits(w2c_load_i64(a, o)); }
static inline uint32_t w2c_load_i32_8u(uint32_t a, uint32_t o) { return *w2c_addr(a, o, 1); }
static inline uint32_t w2c_load_i32_8s(uint32_t a, uint32_t o) { return (w2c_load_i32_8u(a, o) ^ 0x80u) - 0x80u; }
static inline uint32_t w2c_load_i32_16u(uint32_t a, uint32_t o) { uint16_t v; memcpy(&v, w2c_addr(a, o, 2), 2); return w2c_le16(v); }
static inline uint32_t w2c_load_i32_16s(uint32_t a, uint32_t o) { return (w2c_load_i32_16u(a, o) ^ 0x8000u) - 0x8000u; }
static inline uint64_t w2c_load_i64_8u(uint32_t a, uint32_t o) { return w2c_load_i32_8u(a, o); }
static inline uint64_t w2c_load_i64_8s(uint32_t a, uint32_t o) { return ((uint64_t)w2c_load_i32_8u(a, o) ^ 0x80u) - UINT64_C(0x80); }
static inline uint64_t w2c_load_i64_16u(uint32_t a, uint32_t o) { return w2c_load_i32_16u(a, o); }
static inline uint64_t w2c_load_i64_16s(uint32_t a, uint32_t o) { return ((uint64_t)w2c_load_i32_16u(a, o) ^ 0x8000u) - UINT64_C(0x8000); }
static inline uint64_t w2c_load_i64_32u(uint32_t a, uint32_t o) { return w2c_load_i32(a, o); }
static inline uint64_t w2c_load_i64_32s(uint32_t a, uint32_t o) { return ((uint64_t)w2c_load_i32(a, o) ^ UINT64_C(0x80000000)) - UINT64_C(0x80000000); }

static inline void w2c_store_i32(uint32_t a, uint32_t o, uint32_t v) { v = w2c_le32(v); memcpy(w2c_addr(a, o, 4), &v, 4); }
static inline void w2c_store_i64(uint32_t a, uint32_t o, uint64_t v) { v = w2c_le64(v); memcpy(w2c_addr(a, o, 8), &v, 8); }
static inline void w2c_store_f32(uint32_t a, uint32_t o, float v) { w2c_store_i32(a, o, w2c_f32_bits(v)); }
static inline void w2c_store_f64(uint32_t a, uint32_t o, double v) { w2c_store_i64(a, o, w2c_f64_bits(v)); }
static inline void w2c_store_i32_8(uint32_t a, uint32_t o, uint32_t v) { *w2c_addr(a, o, 1) = (uint8_t)v; }
static inline void w2c_store_i32_16(uint32_t a, uint32_t o, uint32_t v) { uint16_t x = w2c_le16((uint16_t)v); memcpy(w2c_addr(a, o, 2), &x, 2); }
static inline void w2c_store_i64_8(uint32_t a, uint32_t o, uint64_t v) { w2c_store_i32_8(a, o, (uint32_t)v); }
static inline void w2c_store_i64_16(uint32_t a, uint32_t o, uint64_t v) { w2c_store_i32_16(a, o, (uint32_t)v); }
static inline void w2c_store_i64_32(uint32_t a, uint32_t o, uint64_t v) { w2c_store_i32(a, o, (uint32_t)v); }

/* ---- integer operations ------------------------------------------------- */

/* Multiplication in unsigned arithmetic at least as wide as int: no signed
 * overflow after integer promotion. */
static inline uint32_t w2c_mul32(uint32_t a, uint32_t b) { return (uint32_t)((uint64_t)a * b); }
static inline uint64_t w2c_mul64(uint64_t a, uint64_t b) { return a * b; }

static inline uint32_t w2c_div_u32(uint32_t a, uint32_t b) { if (!b) w2c_trap("integer divide by zero"); return a / b; }
static inline uint64_t w2c_div_u64(uint64_t a, uint64_t b) { if (!b) w2c_trap("integer divide by zero"); return a / b; }
static inline uint32_t w2c_rem_u32(uint32_t a, uint32_t b) { if (!b) w2c_trap("integer divide by zero"); return a % b; }
static inline uint64_t w2c_rem_u64(uint64_t a, uint64_t b) { if (!b) w2c_trap("integer divide by zero"); return a % b; }

/* Signed division on magnitudes: no signed overflow, no implementation-
 * defined conversion. */
static inline uint32_t w2c_div_s32(uint32_t a, uint32_t b) {
    uint32_t na = a >> 31, nb = b >> 31, ma = na ? 0u - a : a, mb = nb ? 0u - b : b, q;
    if (!b) w2c_trap("integer divide by zero");
    if (a == UINT32_C(0x80000000) && b == UINT32_C(0xffffffff)) w2c_trap("integer overflow");
    q = ma / mb;
    return (na ^ nb) ? 0u - q : q;
}
static inline uint64_t w2c_div_s64(uint64_t a, uint64_t b) {
    uint64_t na = a >> 63, nb = b >> 63, ma = na ? 0u - a : a, mb = nb ? 0u - b : b, q;
    if (!b) w2c_trap("integer divide by zero");
    if (a == UINT64_C(0x8000000000000000) && b == UINT64_C(0xffffffffffffffff)) w2c_trap("integer overflow");
    q = ma / mb;
    return (na ^ nb) ? 0u - q : q;
}
static inline uint32_t w2c_rem_s32(uint32_t a, uint32_t b) {
    uint32_t na = a >> 31, ma = na ? 0u - a : a, mb = (b >> 31) ? 0u - b : b, r;
    if (!b) w2c_trap("integer divide by zero");
    r = ma % mb;
    return na ? 0u - r : r;
}
static inline uint64_t w2c_rem_s64(uint64_t a, uint64_t b) {
    uint64_t na = a >> 63, ma = na ? 0u - a : a, mb = (b >> 63) ? 0u - b : b, r;
    if (!b) w2c_trap("integer divide by zero");
    r = ma % mb;
    return na ? 0u - r : r;
}

/* Arithmetic shift right without shifting a negative value. */
static inline uint32_t w2c_shr_s32(uint32_t x, uint32_t n) { uint32_t s = 0u - (x >> 31); n &= 31u; return ((x ^ s) >> n) ^ s; }
static inline uint64_t w2c_shr_s64(uint64_t x, uint64_t n) { uint64_t s = 0u - (x >> 63); n &= 63u; return ((x ^ s) >> n) ^ s; }
static inline uint32_t w2c_rotl32(uint32_t x, uint32_t n) { n &= 31u; return n ? (x << n) | (x >> (32u - n)) : x; }
static inline uint32_t w2c_rotr32(uint32_t x, uint32_t n) { n &= 31u; return n ? (x >> n) | (x << (32u - n)) : x; }
static inline uint64_t w2c_rotl64(uint64_t x, uint64_t n) { n &= 63u; return n ? (x << n) | (x >> (64u - n)) : x; }
static inline uint64_t w2c_rotr64(uint64_t x, uint64_t n) { n &= 63u; return n ? (x >> n) | (x << (64u - n)) : x; }

static inline uint32_t w2c_clz32(uint32_t x) { uint32_t n = 0; if (!x) return 32; while (!(x & UINT32_C(0x80000000))) { x <<= 1; n++; } return n; }
static inline uint64_t w2c_clz64(uint64_t x) { uint64_t n = 0; if (!x) return 64; while (!(x & UINT64_C(0x8000000000000000))) { x <<= 1; n++; } return n; }
static inline uint32_t w2c_ctz32(uint32_t x) { uint32_t n = 0; if (!x) return 32; while (!(x & 1u)) { x >>= 1; n++; } return n; }
static inline uint64_t w2c_ctz64(uint64_t x) { uint64_t n = 0; if (!x) return 64; while (!(x & 1u)) { x >>= 1; n++; } return n; }
static inline uint32_t w2c_popcnt32(uint32_t x) { uint32_t n = 0; while (x) { x &= x - 1; n++; } return n; }
static inline uint64_t w2c_popcnt64(uint64_t x) { uint64_t n = 0; while (x) { x &= x - 1; n++; } return n; }

/* ---- float operations without libm --------------------------------------- */

static inline float w2c_f32_abs(float x) { return w2c_f32_from_bits(w2c_f32_bits(x) & UINT32_C(0x7fffffff)); }
static inline float w2c_f32_neg(float x) { return w2c_f32_from_bits(w2c_f32_bits(x) ^ UINT32_C(0x80000000)); }
static inline float w2c_f32_copysign(float x, float y) {
    return w2c_f32_from_bits((w2c_f32_bits(x) & UINT32_C(0x7fffffff)) | (w2c_f32_bits(y) & UINT32_C(0x80000000)));
}
static inline double w2c_f64_abs(double x) { return w2c_f64_from_bits(w2c_f64_bits(x) & UINT64_C(0x7fffffffffffffff)); }
static inline double w2c_f64_neg(double x) { return w2c_f64_from_bits(w2c_f64_bits(x) ^ UINT64_C(0x8000000000000000)); }
static inline double w2c_f64_copysign(double x, double y) {
    return w2c_f64_from_bits((w2c_f64_bits(x) & UINT64_C(0x7fffffffffffffff)) | (w2c_f64_bits(y) & UINT64_C(0x8000000000000000)));
}

/* ---- the module --------------------------------------------------------- */

/* Initializes memory, globals and data, then runs the start function and
 * _start (generated). */
void w2c_run(void);

#endif
