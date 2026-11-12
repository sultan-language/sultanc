/* libinterp_broken: needs libinterp_gone, which the check deletes. */
int interp_gone(void);
int interp_lib_broken(void) { return interp_gone(); }
