/* A function-like macro called with one argument too few. */
#define PAIR(a, b) ((a) + (b))
int table[PAIR(1)];
