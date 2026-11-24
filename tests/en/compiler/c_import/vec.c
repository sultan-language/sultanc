/* Compiled by Jisr (compiler/jisr/lower.sn) into the program that imports it
   (`import "vec.c"`); SULTANC_C_COMPILER=cc compiles it with the system C
   compiler instead. */
#include <stdlib.h>
#include <string.h>
#include "vec.h"

struct vec { int64_t *items; size_t length, capacity; };

static int grow(vec *v) {
    size_t capacity = v->capacity ? v->capacity * 2 : 4;
    int64_t *items = realloc(v->items, capacity * sizeof *items);
    if (!items) return 0;
    v->items = items;
    v->capacity = capacity;
    return 1;
}

vec *vec_new(size_t capacity) {
    vec *v = calloc(1, sizeof *v);
    if (v && capacity) { v->items = malloc(capacity * sizeof *v->items); v->capacity = capacity; }
    return v;
}
void vec_free(vec *v) { if (v) { free(v->items); free(v); } }
int vec_push(vec *v, int64_t value) {
    if (v->length == v->capacity && !grow(v)) return 0;
    v->items[v->length++] = value;
    return 1;
}
int64_t vec_sum(const vec *v) {
    int64_t s = 0;
    for (size_t i = 0; i < v->length; i++) s += v->items[i];
    return s;
}
size_t vec_length(const vec *v) { return v->length; }
double vec_mean(const vec *v) { return v->length ? (double)vec_sum(v) / (double)v->length : 0.0; }
int vec_each(const vec *v, vec_visit visit, void *context) {
    for (size_t i = 0; i < v->length; i++) if (!visit(v->items[i], context)) return 0;
    return 1;
}
size_t vec_name_copy(char *out, size_t size) {
    size_t n = strlen(VEC_NAME);
    if (size > n) memcpy(out, VEC_NAME, n + 1);
    return n;
}
int vec_printf(const char *format, ...) { (void)format; return 0; }
