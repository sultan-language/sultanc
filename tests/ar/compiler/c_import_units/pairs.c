#include "units.h"

struct pair make_pair(int first, int second) {
    struct pair p = { first, second };
    return p;
}

/* A record made by this unit's make_pair, summed after the parity units
   ran: is_even(10) alternates between even.c and odd.c. */
int pair_sum(struct pair p) {
    struct pair q = make_pair(p.second, p.first);
    return q.first + q.second + is_even(10) + is_odd(7);
}
