/* Declarators of every shape, and calls through them. */
#ifndef CALLS_H
#define CALLS_H

typedef int (*binary_op)(int, int);
typedef void (*handler_t)(int);

int apply(binary_op op, int a, int b);
binary_op pick(int which);
/* signal()'s shape, through a typedef and spelled out. */
handler_t install(int code, handler_t handler);
void (*install_raw(int code, void (*handler)(int)))(int);
int last_installed(void);
handler_t default_handler(void);
/* Variadic: callable on every target, with the fixed arguments only or
   through a declared call shape (SYMBOL...N). */
int count_args(int first, ...);
double mixed_sum(int count, ...);

/* An array parameter is a pointer; a pointer to an array. */
int sum_array(const int values[], unsigned count);
int sum_rows(int (*rows)[3], int count);
int (*matrix(void))[3];

/* Pointers to pointers, const at each level. */
const char *const *names(void);
char **name_slots(void);

/* Scalars of each width and floating types. */
long long_add(long a, long b);
unsigned long long wide_mul(unsigned long long a, unsigned long long b);
signed char narrow(short value);
double scale(double x, float y);
_Bool is_even(int x);
unsigned char *fill_bytes(unsigned char *out, unsigned char value, unsigned long count);

/* Defined below with an empty list: no parameters. */
int unprototyped();
/* Objects: external data, read where used. */
extern int counter_variable;
extern const unsigned table_size;
void bump_counter(void);

/* Left out, each with a reason. */
int declared_only();
static int hidden(int x) { return x; }
static inline int inline_helper(int x) { return x + 1; }
extern _Thread_local int per_thread;
extern int whole_table[4];
long double precise(long double x);

#endif
