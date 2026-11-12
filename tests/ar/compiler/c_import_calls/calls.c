/* Compiled by Jisr (compiler/jisr/lower.sn) into the program that imports it
   (`import "calls.c"`); SULTANC_C_COMPILER=cc compiles it with the system C
   compiler instead. */
#include <stdarg.h>
#include "calls.h"

int counter_variable = 0;
const unsigned table_size = 3;
_Thread_local int per_thread = 0;
int whole_table[4] = { 1, 2, 3, 4 };
void bump_counter(void) { counter_variable += 2; }
static int installed = 0;

static int add(int a, int b) { return a + b; }
static int mul(int a, int b) { return a * b; }
static void remember(int code) { installed = code; }

int apply(binary_op op, int a, int b) { return op(a, b); }
binary_op pick(int which) { return which ? mul : add; }

handler_t install(int code, handler_t handler) {
    handler(code);
    return remember;
}

void (*install_raw(int code, void (*handler)(int)))(int) { return install(code, handler); }
int last_installed(void) { return installed; }
handler_t default_handler(void) { return remember; }
int count_args(int first, ...) { return first; }

/* count pairs of (int, double) after the count. */
double mixed_sum(int count, ...) {
    va_list ap;
    va_start(ap, count);
    double s = 0;
    for (int i = 0; i < count; i++) { s += va_arg(ap, int); s += va_arg(ap, double); }
    va_end(ap);
    return s;
}

int sum_array(const int values[], unsigned count) {
    int s = 0;
    for (unsigned i = 0; i < count; i++) s += values[i];
    return s;
}

static int grid[2][3] = { { 1, 2, 3 }, { 4, 5, 6 } };
int (*matrix(void))[3] { return grid; }

int sum_rows(int (*rows)[3], int count) {
    int s = 0;
    for (int r = 0; r < count; r++) s += rows[r][0] + rows[r][1] + rows[r][2];
    return s;
}

static const char *const name_table[] = { "alpha", "beta", 0 };
const char *const *names(void) { return name_table; }
static char *slots[2];
char **name_slots(void) { return slots; }

long long_add(long a, long b) { return a + b; }
unsigned long long wide_mul(unsigned long long a, unsigned long long b) { return a * b; }
signed char narrow(short value) { return (signed char)value; }
double scale(double x, float y) { return x * (double)y; }
_Bool is_even(int x) { return x % 2 == 0; }

unsigned char *fill_bytes(unsigned char *out, unsigned char value, unsigned long count) {
    for (unsigned long i = 0; i < count; i++) out[i] = value;
    return out;
}

int unprototyped() { return 7; }
long double precise(long double x) { return x; }
