/* Records whose layout SultanC and the C compiler must agree on. The
   static assertions hold for the LP64 targets the test builds for; Jisr
   evaluates them with its own layout, the C compiler with its own. */
#ifndef SHAPES_H
#define SHAPES_H
#include <stddef.h>
#include <stdint.h>

struct point { int32_t x; int32_t y; };

/* Padding after a char, a nested record, an array field. */
struct shape {
    char tag;
    double scale;
    struct point origin;
    int16_t corners[3];
    const char *name;
    struct shape *next;
};

/* A union: opaque storage of its size and alignment. */
union number { int64_t i; double d; uint8_t bytes[8]; };

/* Bit-fields: opaque storage. */
struct flags { unsigned ready : 1; unsigned mode : 3; unsigned count : 12; uint8_t tail; };

/* Packed: opaque storage, size 5. */
struct __attribute__((packed)) packed_pair { uint8_t a; uint32_t b; };

/* A flexible array member is not a field. */
struct buffer { uint32_t length; uint8_t data[]; };

#define SHAPE_SIZE 48
#define SHAPE_ORIGIN_OFFSET 16
#define SHAPE_NAME_OFFSET 32
_Static_assert(sizeof(struct shape) == SHAPE_SIZE, "shape size");
_Static_assert(offsetof(struct shape, origin) == SHAPE_ORIGIN_OFFSET, "origin offset");
_Static_assert(offsetof(struct shape, name) == SHAPE_NAME_OFFSET, "name offset");
_Static_assert(sizeof(union number) == 8 && _Alignof(union number) == 8, "number");
_Static_assert(sizeof(struct flags) == 4, "flags");
_Static_assert(sizeof(struct packed_pair) == 5, "packed");
_Static_assert(sizeof(struct buffer) == 4, "buffer");

/* Jisr's sizeof and offsetof, as constants. */
enum layout_facts {
    FACT_POINT = sizeof(struct point),
    FACT_SHAPE = sizeof(struct shape),
    FACT_CORNERS = offsetof(struct shape, corners),
    FACT_NEXT = offsetof(struct shape, next),
    FACT_NUMBER_ALIGN = _Alignof(union number)
};

/* The C compiler's sizeof and offsetof, from shapes.c. */
size_t shape_size(void);
size_t shape_offset(int field);
size_t number_size(void);
size_t flags_size(void);

struct point point_add(struct point a, struct point b);
double shape_area(const struct shape *s);
void shape_fill(struct shape *s, int32_t x, int32_t y);
int64_t number_bits(union number n);
union number number_of(int64_t i);

/* Floating records (SSE / HFA classes) and a record passed in memory. */
struct vec2 { double x; double y; };
struct vec2 vec2_scale(struct vec2 v, double k);
struct big { int64_t a; int64_t b; int64_t c; };
struct big big_rotate(struct big v);
float vec2_dot32(struct vec2 a, struct vec2 b);
#endif
