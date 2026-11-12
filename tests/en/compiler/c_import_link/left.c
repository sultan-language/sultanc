#include "link.h"

static int base = 5;

int left_base(void) { return base; }

/* Calls functions right.c defines: right.c is imported by another SultanC
   module, so these calls bind to its compiled functions. */
int use_left(int v) {
    struct pt p = make_pt(v, 1);
    return twice(v) + sum_pt(p);
}
