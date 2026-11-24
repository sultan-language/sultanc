/* The address of a file-scope object Jisr keeps as a SultanC variable (a
   scalar or pointer static), passed to a function that writes through it
   (compiler/jisr/lower.sn, عنوان_عام_جسر: the pointer's type is the
   variable's own, تمثيلات_العوام). */
static void *handle;
static long counter;
static unsigned char flag;
static double ratio;

static void set_once(void **slot, void *value) { if (!*slot) *slot = value; }
static void bump(long *slot) { *slot += 1; }
static void raise_flag(unsigned char *slot) { *slot = 1; }
static void scale(double *slot, double by) { *slot = *slot * by; }

long static_addresses(void) {
    ratio = 1.5;
    set_once(&handle, (void *)32);
    set_once(&handle, (void *)64);
    bump(&counter);
    bump(&counter);
    raise_flag(&flag);
    scale(&ratio, 4.0);
    return (long)handle + counter + flag + (long)ratio;
}
