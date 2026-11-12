/* libinterp_second: defines interp_lib_which as well, and calls
   interp_lib_add, which it leaves to the library loaded before it. */
int interp_lib_add(int a, int b);
int interp_lib_which(void) { return 2; }
int interp_lib_through_first(int x) { return interp_lib_add(x, -1000) * 2; }
