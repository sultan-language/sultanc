/* Declarations shared by the translation units of main.sn's import
   "even.c+odd.c+pairs.c": each unit calls functions the others define. */
#ifndef UNITS_H
#define UNITS_H

struct pair { int first; int second; };

int is_even(unsigned n);
int is_odd(unsigned n);
struct pair make_pair(int first, int second);
int pair_sum(struct pair p);
int count_calls(void);

#endif
