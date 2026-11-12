/* C code compiled by Jisr (compiler/jisr/lower.sn): each function returns
   0 when its part of the language behaves as C defines it, else the line
   that failed. */
#include <stdarg.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>

#define CHECK(cond) do { if (!(cond)) return __LINE__; } while (0)

/* Integer arithmetic: promotions, wrapping, shifts, division. */
int arithmetic(void) {
    unsigned char a = 200, b = 100;
    CHECK(a + b == 300);
    unsigned u = 4000000000u;
    CHECK(u + u == 3705032704u);
    CHECK(0u - 1u == 4294967295u);
    unsigned long long big = 0xFFFFFFFFFFFFFFFFull;
    CHECK(big + 2 == 1);
    CHECK(big * big == 1);
    int s = -7;
    CHECK(s / 2 == -3 && s % 2 == -1);
    CHECK((s >> 1) == -4);
    CHECK((1u << 31) == 2147483648u);
    CHECK((1 << 4) == 16);
    signed char c = (signed char)200;
    CHECK(c == -56);
    long long m = -9223372036854775807LL - 1;
    CHECK(m < 0 && -(m + 1) == 9223372036854775807LL);
    unsigned short us = 65535;
    us++;
    CHECK(us == 0);
    CHECK(~0u == 4294967295u && (~5 & 0xF) == 10);
    CHECK((7 ^ 2) == 5 && (6 | 1) == 7);
    int i = 5;
    i += 3; i -= 1; i *= 2; i /= 3; i %= 3; i <<= 4; i >>= 1; i |= 1; i &= 13; i ^= 6;
    CHECK(i == 15);
    _Bool t = 42;
    CHECK(t == 1);
    return 0;
}

double fsum(const double *v, int n) { double s = 0; for (int k = 0; k < n; k++) s += v[k]; return s; }

int floats(void) {
    double v[3] = { 1.5, 2.25, -0.75 };
    CHECK(fsum(v, 3) == 3.0);
    float f = 1.5f;
    f *= 2;
    CHECK(f == 3.0f);
    CHECK((int)2.9 == 2 && (int)-2.9 == -2);
    CHECK((double)7 / 2 == 3.5);
    double d = 1e3;
    CHECK(d > 999.0 && d < 1001.0);
    return 0;
}

/* Pointers and arrays. */
static int table[5] = { 10, 20, 30, 40, 50 };
static int grid[2][3] = { { 1, 2, 3 }, { 4, 5, 6 } };

int pointers(void) {
    int *p = table;
    CHECK(*(p + 2) == 30 && p[4] == 50);
    int *q = &table[4];
    CHECK(q - p == 4);
    p++;
    CHECK(*p == 20);
    --q;
    CHECK(*q == 40);
    int sum = 0;
    for (int r = 0; r < 2; r++) for (int c = 0; c < 3; c++) sum += grid[r][c];
    CHECK(sum == 21);
    int local[4];
    for (int k = 0; k < 4; k++) local[k] = k * k;
    int *lp = local;
    CHECK(lp[3] == 9 && *(local + 2) == 4);
    char text[] = "jisr";
    CHECK(sizeof text == 5 && text[2] == 's' && text[4] == 0);
    const char *msg = "hello";
    CHECK(strlen(msg) == 5 && msg[1] == 'e');
    void *vp = &sum;
    CHECK(*(int *)vp == 21);
    int **pp = &p;
    **pp = 99;
    CHECK(table[1] == 99);
    table[1] = 20;
    CHECK(p != NULL && (char *)q > (char *)p);
    return 0;
}

/* Structs, unions and bit-fields. */
struct point { int x, y; };
struct rect { struct point a, b; char tag; };
union word { unsigned u; unsigned char bytes[4]; };
struct flags { unsigned ready : 1; unsigned mode : 3; int level : 4; unsigned rest : 24; };

static struct point origin = { 0, 0 };
static struct rect frame = { { 1, 2 }, { 11, 22 }, 'r' };

struct point add_points(struct point a, struct point b) {
    struct point r = { a.x + b.x, a.y + b.y };
    return r;
}

int area(const struct rect *r) { return (r->b.x - r->a.x) * (r->b.y - r->a.y); }

int records(void) {
    struct point p = { 3, 4 };
    struct point q = p;
    q.x = 10;
    CHECK(p.x == 3 && q.x == 10 && q.y == 4);
    struct point s = add_points(p, q);
    CHECK(s.x == 13 && s.y == 8);
    CHECK(area(&frame) == 200 && frame.tag == 'r');
    struct rect copy = frame;
    copy.b.x = 6;
    CHECK(area(&copy) == 100 && area(&frame) == 200);
    CHECK(origin.x == 0 && sizeof(struct rect) == 20);
    union word w;
    w.u = 0x01020304;
    CHECK(w.bytes[0] == 4 || w.bytes[0] == 1);
    struct flags f = { 1, 5, -3, 0 };
    CHECK(f.ready == 1 && f.mode == 5 && f.level == -3);
    f.mode = 2;
    f.level += 1;
    f.rest = 0xABCDEF;
    CHECK(f.mode == 2 && f.level == -2 && f.ready == 1 && f.rest == 0xABCDEF);
    struct point arr[2] = { [1] = { .y = 9, .x = 8 } };
    CHECK(arr[0].x == 0 && arr[1].x == 8 && arr[1].y == 9);
    struct point *ptr = &(struct point){ 5, 6 };
    CHECK(ptr->y == 6);
    CHECK(offsetof(struct rect, tag) == 16);
    return 0;
}

/* Control flow. */
int control(void) {
    int n = 0;
    for (int k = 0; k < 10; k++) {
        if (k % 2) continue;
        if (k == 8) break;
        n += k;
    }
    CHECK(n == 12);
    int d = 0;
    do { d++; if (d < 3) continue; } while (d < 5);
    CHECK(d == 5);
    int w = 0;
    while (1) { if (++w == 4) break; }
    CHECK(w == 4);
    int acc = 0;
    for (int v = 0; v < 6; v++) {
        switch (v) {
        case 0: acc += 1;
        case 1: acc += 10; break;
        case 2 ... 3: acc += 100; continue;
        default: acc += 1000;
        }
        acc += 5;
    }
    CHECK(acc == 1 + 10 + 5 + 10 + 5 + 200 + 2010);
    int g = 0;
again:
    g++;
    if (g < 3) goto again;
    CHECK(g == 3);
    int x = 3, y = (x++, x * 2);
    CHECK(x == 4 && y == 8);
    int z = x > 3 ? 1 : 2;
    CHECK(z == 1 && (0 || 2) == 1 && (1 && 0) == 0 && !5 == 0);
    return 0;
}

/* Duff's device: case labels inside a loop. */
int duff(int count) {
    int n = (count + 3) / 4, out = 0;
    switch (count % 4) {
    case 0: do { out++;
    case 3: out++;
    case 2: out++;
    case 1: out++;
            } while (--n > 0);
    }
    return out;
}

/* Static locals and globals. */
int counter_value = 100;
const char *greeting = "salam";
static const char *words[] = { "one", "two", "three" };

int next_id(void) { static int id = 0; return ++id; }

int statics(void) {
    CHECK(next_id() == 1 && next_id() == 2 && next_id() == 3);
    CHECK(strcmp(words[2], "three") == 0 && greeting[0] == 's');
    counter_value++;
    return 0;
}

/* Function pointers. */
static int twice(int v) { return 2 * v; }
static int square(int v) { return v * v; }
typedef int (*unary)(int);
static unary ops[2] = { twice, square };

int apply_all(int v) { int s = 0; for (int k = 0; k < 2; k++) s += ops[k](v); return s; }

/* Variadic definitions and calls. */
int vsum(int count, ...) {
    va_list ap;
    va_start(ap, count);
    int s = 0;
    for (int k = 0; k < count; k++) s += va_arg(ap, int);
    va_end(ap);
    return s;
}

double vavg(int count, ...) {
    va_list ap;
    va_start(ap, count);
    double s = 0;
    for (int k = 0; k < count; k++) s += va_arg(ap, double);
    va_end(ap);
    return s / count;
}

int format(char *out, int size, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(out, size, fmt, ap);
    va_end(ap);
    return n;
}

int variadics(void) {
    CHECK(vsum(4, 1, 2, 3, 4) == 10);
    CHECK(vavg(2, 1.0, 2.0) == 1.5);
    char buffer[64];
    int n = snprintf(buffer, sizeof buffer, "%d-%s-%.1f", 42, "x", 2.5);
    CHECK(n == 8 && strcmp(buffer, "42-x-2.5") == 0);
    n = format(buffer, sizeof buffer, "%s:%d", "ok", 7);
    CHECK(n == 4 && buffer[3] == '7');
    /* An object of the C library, read and addressed by its symbol. */
    CHECK(stderr != NULL && &stderr != NULL);
    /* Writing through it reaches the real stream (its descriptor is 2). */
    CHECK(fileno(stderr) == 2);
    return 0;
}

/* All parts, each its own code. */
int run_all(void) {
    int r;
    if ((r = arithmetic()) != 0) return 1000 + r;
    if ((r = floats()) != 0) return 2000 + r;
    if ((r = pointers()) != 0) return 3000 + r;
    if ((r = records()) != 0) return 4000 + r;
    if ((r = control()) != 0) return 5000 + r;
    if (duff(7) != 7 || duff(8) != 8 || duff(1) != 1) return 6000;
    if ((r = statics()) != 0) return 7000 + r;
    if (apply_all(5) != 35) return 8000;
    if ((r = variadics()) != 0) return 9000 + r;
    return 0;
}
