/* Compiled by Jisr (compiler/jisr/lower.sn) into the program that imports it
   (`import "shapes.c"`); SULTANC_C_COMPILER=cc compiles it with the system C
   compiler instead. */
#include "shapes.h"

size_t shape_size(void) { return sizeof(struct shape); }

size_t shape_offset(int field) {
    switch (field) {
    case 0: return offsetof(struct shape, tag);
    case 1: return offsetof(struct shape, scale);
    case 2: return offsetof(struct shape, origin);
    case 3: return offsetof(struct shape, corners);
    case 4: return offsetof(struct shape, name);
    default: return offsetof(struct shape, next);
    }
}

size_t number_size(void) { return sizeof(union number); }
size_t flags_size(void) { return sizeof(struct flags); }

struct point point_add(struct point a, struct point b) {
    struct point r = { a.x + b.x, a.y + b.y };
    return r;
}

double shape_area(const struct shape *s) {
    return s->scale * (double)s->origin.x * (double)s->origin.y + s->corners[2];
}

void shape_fill(struct shape *s, int32_t x, int32_t y) {
    s->tag = 'S';
    s->origin.x = x;
    s->origin.y = y;
    s->corners[0] = 1;
    s->corners[1] = 2;
    s->corners[2] = 3;
    s->name = "shape";
    s->next = s;
}

int64_t number_bits(union number n) { return n.i; }

union number number_of(int64_t i) {
    union number n;
    n.i = i;
    return n;
}

struct vec2 vec2_scale(struct vec2 v, double k) {
    struct vec2 r = { v.x * k, v.y * k };
    return r;
}

struct big big_rotate(struct big v) {
    struct big r = { v.b, v.c, v.a };
    return r;
}

float vec2_dot32(struct vec2 a, struct vec2 b) { return (float)(a.x * b.x + a.y * b.y); }
