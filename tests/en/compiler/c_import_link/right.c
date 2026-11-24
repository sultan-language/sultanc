#include "link.h"

int twice(int v) { return v * 2; }

int sum_pt(struct pt p) { return p.x + p.y; }

pt_t make_pt(int x, int y) {
    pt_t p = { x, y };
    return p;
}

/* And back: left.c's function. */
int right_total(void) { return left_base() + twice(1); }
