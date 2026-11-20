#include "units.h"

/* Each unit has its own static `calls` and `step`: same names, separate
   objects and functions. */
static int calls;

static unsigned step(unsigned n) { return n - 1; }

int is_even(unsigned n) {
    calls++;
    if (n == 0) return 1;
    return is_odd(step(n));
}

int count_calls(void) { return calls; }
