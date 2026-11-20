#include "va.h"

/* Each call passes its variadic arguments to sum.c's compiled function in
   8-byte slots, as a call within one unit does. */
int run_calls(void) {
    if (vsum(3, 10, 20, 5) != 35) return 1;
    if (vsum(0) != 0) return 2;
    if ((int)vmix(2, 1.5, 2) != 7) return 3;
    unsigned char small = 200;
    if (vsum(2, small, -8) != 192) return 4;
    if (vlen("ab", "cde", (const char *)0) != 5) return 5;
    return 0;
}
