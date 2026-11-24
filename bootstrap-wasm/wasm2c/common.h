/* common.h: diagnostics, allocation and the growable text buffer shared by
 * the parts of sultanc-wasm2c. */
#ifndef W2C_COMMON_H
#define W2C_COMMON_H

#include <inttypes.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#if defined(__GNUC__) || defined(__clang__)
#define W2C_PRINTF(f, a) __attribute__((format(printf, f, a)))
#define W2C_NORETURN_TOOL __attribute__((noreturn))
#else
#define W2C_PRINTF(f, a)
#define W2C_NORETURN_TOOL
#endif

/* What is being translated ("module", "function 12"), for diagnostics. */
extern const char *current_context;

/* Reports an error in the input (or a translator bug) and exits with 1. */
W2C_NORETURN_TOOL void die(const char *fmt, ...) W2C_PRINTF(1, 2);

void *xmalloc(size_t n);
void *xcalloc(size_t count, size_t size);
void *xrealloc(void *p, size_t n);
char *xstrdup(const char *s);
/* A newly allocated, formatted string. */
char *xprintf(const char *fmt, ...) W2C_PRINTF(1, 2);

/* A growable, NUL-terminated text buffer. */
typedef struct {
    char *s;
    size_t n, cap;
} Buf;

void buf_put(Buf *b, const char *s, size_t n);
void buf_puts(Buf *b, const char *s);
void buf_spaces(Buf *b, size_t n);
void bprintf(Buf *b, const char *fmt, ...) W2C_PRINTF(2, 3);
void buf_free(Buf *b);

/* Writes `b` to the file PREFIX SUFFIX. */
void write_file(const char *prefix, const char *suffix, const Buf *b);

#endif
