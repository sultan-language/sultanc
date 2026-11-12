/* Compiled by Jisr (compiler/jisr/lower.sn) into the program that imports it
   (`import "records.c"`). Records larger than 16 bytes passed and returned
   by value between C functions, through C function pointers (callbacks
   compiled here), and from SultanC: a callee that changes its parameter
   changes its own copy. */
struct big { long a, b, c, d, e; };

long total(struct big s) { return s.a + s.b + s.c + s.d + s.e; }

/* Changes its parameter, its own copy. */
long bump(struct big s) {
    s.a = s.a + 100;
    s.e = 0;
    return s.a + s.e;
}

struct big shifted(struct big s) {
    struct big r = { s.e, s.a, s.b, s.c, s.d };
    return r;
}

typedef long (*reader)(struct big);

static long first(struct big s) { return s.a; }
static long last(struct big s) { return s.e; }

/* A C callback that receives a record by value. */
long through(reader f, struct big s) { return f(s); }

long callbacks(struct big s) {
    long before = s.a;
    long n = bump(s) + through(first, s) + through(last, s) + through(total, s);
    /* bump changed only its copy. */
    return n + (s.a == before ? 0 : 1000);
}

/* x = f(x) on a local. */
long rotate(struct big s, int times) {
    int i;
    for (i = 0; i < times; i++) { s = shifted(s); }
    return s.a;
}
