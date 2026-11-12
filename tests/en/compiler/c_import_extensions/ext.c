/* C constructs Jisr compiles beyond C11's core: variable-length arrays,
   computed goto (labels as values), _BitInt wider than 64 bits,
   thread-local objects, and long double where it is double. */

static int fill(int *p, int n) {
    int s = 0;
    for (int i = 0; i < n; i++) { p[i] = i * i; s += p[i]; }
    return s;
}

/* Variable-length arrays: storage made where declared (each iteration),
   sizeof at run time, an inner constant dimension. */
int vla_test(int n) {
    int a[n];
    int s = fill(a, n);
    long rows[n][2];
    for (int i = 0; i < n; i++) { rows[i][0] = i; rows[i][1] = 2 * i; }
    for (int k = 1; k <= 3; k++) {
        char buf[k * 4];
        buf[k * 4 - 1] = 7;
        s += buf[k * 4 - 1] + (int)sizeof buf;
    }
    /* 5 + 45 + 12 + 3 + 4 = 69 for n = 3 */
    return s + (int)sizeof(a) + (int)(sizeof(rows) / sizeof(rows[0])) + (int)rows[n - 1][1];
}

/* Computed goto: a table of label addresses, a threaded interpreter. */
static int fall(int k) {
    static void *table[] = { &&one, &&ten, &&hundred };
    int r = 0;
    goto *table[k];
one: r += 1;
ten: r += 10;
hundred: r += 100;
    return r;
}

static int interp(const unsigned char *code) {
    static const void *ops[] = { &&op_halt, &&op_inc, &&op_dbl };
    int acc = 0;
    void *here = &&op_inc;
    if (here == 0) return -1;
#define NEXT goto *ops[*code++]
    NEXT;
op_inc: acc += 1; NEXT;
op_dbl: acc *= 2; NEXT;
op_halt: return acc;
}

int goto_test(void) {
    static const unsigned char code[] = { 1, 1, 2, 1, 2, 2, 1, 1, 2, 0 };
    /* 111 + 110 + 100 - 333 + 44 = 32 */
    return fall(0) + fall(1) + fall(2) - 333 + interp(code);
}

/* _BitInt(65..128): held in 128 bits, wrapping within N bits. */
int bitint_test(void) {
    unsigned _BitInt(100) x = 1;
    x <<= 70;
    x += 5;
    int a = (int)(x >> 70) + (int)(x & 0xff);             /* 1 + 5 */
    unsigned _BitInt(100) y = 0;
    y -= 1;                                                /* 2^100 - 1 */
    int b = (int)(y >> 96);                                /* 15 */
    _BitInt(90) s = -3;
    s *= 7;                                                /* -21 */
    _BitInt(90) low = 1;
    low <<= 89;                                            /* the minimum */
    int c = low < 0;                                       /* 1 */
    unsigned _BitInt(65) w = 0xFFFFFFFFFFFFFFFFu;
    w = w * 2 + 1;
    w += 1;                                                /* 2^65 wraps to 0 */
    return a + b + (int)(s + 40) + c + (w == 0);           /* 6 + 15 + 19 + 1 + 1 = 42 */
}

/* Thread-local objects: file scope and block scope, with initializers. */
_Thread_local int counter = 3;
static __thread int hidden;
static __thread double scale = 1.5;

int bump(void) {
    static _Thread_local long calls = 10;
    calls++;
    hidden++;
    return ++counter + hidden + (int)(scale * 2) + (int)calls;
}

int tls_test(void) {
    int a = bump();                                        /* 4 + 1 + 3 + 11 = 19 */
    int b = bump();                                        /* 5 + 2 + 3 + 12 = 22 */
    return a + b + 1;                                      /* 42 */
}

/* long double: an f64 where the target's long double is double (Apple
   arm64, Windows); elsewhere it has no form and is not used here. */
#if __SIZEOF_LONG_DOUBLE__ == 8
static long double half(long double v) { return v / 2; }
int long_double_test(void) { return (int)half(84.0L); }
#else
int long_double_test(void) { return 42; }
#endif
