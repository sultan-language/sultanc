#include <string.h>
#include "common.h"

point_t scale_point(point_t p, int32_t k) {
    point_t r;
    memset(&r, 0, sizeof r);
    r.x = p.x * k;
    r.y = p.y * k;
    return r;
}

size_t point_bytes(void) { return sizeof(point_t); }
