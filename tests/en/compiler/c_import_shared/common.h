/* Shared by geometry.c and scale.c: both C imports of main.sn declare
   struct point, point_t, SHARED_LIMIT and the prototypes; the program
   sees each once. */
#ifndef COMMON_H
#define COMMON_H
#include <stdint.h>
#include <stddef.h>

typedef struct point { int32_t x; int32_t y; } point_t;

#define SHARED_LIMIT 40

int32_t point_sum(const point_t *p);
point_t scale_point(point_t p, int32_t k);
size_t point_bytes(void);
#endif
