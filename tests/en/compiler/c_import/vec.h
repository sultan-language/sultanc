/* Declarations a SultanC program reads with `import "vec.h"`. */
#ifndef VEC_H
#define VEC_H
#include <stddef.h>
#include <stdint.h>

#define VEC_VERSION 0x010203
#define VEC_NAME "vec"
#define VEC_LIMIT (VEC_VERSION >> 8)
#define VEC_NEG (-5)
#define VEC_MASK (1u << 31)
#define VEC_SQUARE(x) ((x) * (x))

typedef enum vec_kind { VEC_EMPTY, VEC_SMALL = 4, VEC_LARGE } vec_kind;
typedef struct vec vec;
typedef int (*vec_visit)(int64_t value, void *context);

vec *vec_new(size_t capacity);
void vec_free(vec *v);
int vec_push(vec *v, int64_t value);
int64_t vec_sum(const vec *v);
size_t vec_length(const vec *v);
double vec_mean(const vec *v);
int vec_each(const vec *v, vec_visit visit, void *context);
size_t vec_name_copy(char *out, size_t size);
int vec_printf(const char *format, ...);
#endif
