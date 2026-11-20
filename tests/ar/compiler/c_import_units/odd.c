#include "units.h"

static int calls;

static unsigned step(unsigned n) { return n - 1; }

int is_odd(unsigned n) {
    calls += 100;
    if (n == 0) return 0;
    return is_even(step(n));
}
