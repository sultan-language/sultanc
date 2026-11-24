/* A GCC vector type in code Jisr compiles: it has no SultanC form, so
   the function using it is reported, not compiled with a wrong type. */
typedef int v4si __attribute__((vector_size(16)));

int sum_lanes(void) {
    v4si v = { 1, 2, 3, 4 };
    return v[0] + v[3];
}
