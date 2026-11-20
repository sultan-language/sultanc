/* A C caller: it calls whatever function pointer it is given with the C
 * calling convention, as qsort calls its comparator. */
typedef long long (*binary_fn)(long long, long long);
typedef double (*mixed_fn)(double, int, double);
long long apply(binary_fn f, long long a, long long b) { return f(a, b); }
double apply_mixed(mixed_fn f) { return f(1.5, 2, 0.25); }
