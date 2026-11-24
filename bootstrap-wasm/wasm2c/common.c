/* common.c: diagnostics, allocation and text buffers. */
#include "common.h"

#include <stdlib.h>
#include <string.h>

const char *current_context = "module";

void die(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "sultanc-wasm2c: error (%s): ", current_context);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputc('\n', stderr);
    exit(1);
}

void *xmalloc(size_t n) {
    void *p = malloc(n ? n : 1);
    if (!p) die("out of memory");
    return p;
}

void *xcalloc(size_t count, size_t size) {
    void *p = calloc(count ? count : 1, size ? size : 1);
    if (!p) die("out of memory");
    return p;
}

void *xrealloc(void *p, size_t n) {
    p = realloc(p, n ? n : 1);
    if (!p) die("out of memory");
    return p;
}

char *xstrdup(const char *s) {
    size_t n = strlen(s) + 1;
    return memcpy(xmalloc(n), s, n);
}

char *xprintf(const char *fmt, ...) {
    va_list ap;
    int n;
    char *s;
    va_start(ap, fmt);
    n = vsnprintf(NULL, 0, fmt, ap);
    va_end(ap);
    if (n < 0) die("internal: format failure");
    s = xmalloc((size_t)n + 1);
    va_start(ap, fmt);
    vsnprintf(s, (size_t)n + 1, fmt, ap);
    va_end(ap);
    return s;
}

void buf_put(Buf *b, const char *s, size_t n) {
    if (b->n + n + 1 > b->cap) {
        size_t cap = b->cap ? b->cap : 4096;
        while (cap < b->n + n + 1) cap *= 2;
        b->s = xrealloc(b->s, cap);
        b->cap = cap;
    }
    memcpy(b->s + b->n, s, n);
    b->n += n;
    b->s[b->n] = 0;
}

void buf_puts(Buf *b, const char *s) { buf_put(b, s, strlen(s)); }

void buf_spaces(Buf *b, size_t n) {
    static const char spaces[] = "                                                                ";
    while (n) {
        size_t k = n < sizeof(spaces) - 1 ? n : sizeof(spaces) - 1;
        buf_put(b, spaces, k);
        n -= k;
    }
}

void bprintf(Buf *b, const char *fmt, ...) {
    char tmp[1024];
    int n;
    va_list ap;
    va_start(ap, fmt);
    n = vsnprintf(tmp, sizeof(tmp), fmt, ap);
    va_end(ap);
    if (n < 0) die("internal: format failure");
    if ((size_t)n >= sizeof(tmp)) {
        char *big = xmalloc((size_t)n + 1);
        va_start(ap, fmt);
        vsnprintf(big, (size_t)n + 1, fmt, ap);
        va_end(ap);
        buf_put(b, big, (size_t)n);
        free(big);
    } else {
        buf_put(b, tmp, (size_t)n);
    }
}

void buf_free(Buf *b) {
    free(b->s);
    b->s = NULL;
    b->n = b->cap = 0;
}

void write_file(const char *prefix, const char *suffix, const Buf *b) {
    char *path = xprintf("%s%s", prefix, suffix);
    FILE *f = fopen(path, "wb");
    if (!f) die("cannot create %s", path);
    if (b->n && fwrite(b->s, 1, b->n, f) != b->n) die("cannot write %s", path);
    if (fclose(f) != 0) die("cannot write %s", path);
    free(path);
}
