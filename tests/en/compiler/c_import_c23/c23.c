/* C23 features and extensions, compiled by Jisr (compiler/jisr). Each
   part returns 0 when it behaves as C23 defines it, else the line that
   failed. */
#define CHECK(c) do { if (!(c)) return __LINE__; } while (0)

/* constexpr: named constants usable in constant expressions; a file-scope
   one is also a SultanC constant of the module. */
constexpr int WORD_LIMIT = 4;
constexpr unsigned long long BIG_MASK = 0xFFull << 40;

/* #embed: a resource's bytes as an initializer list. word.bin is
   "SultanC" (7 bytes); empty.bin has none. */
static const unsigned char word[] = {
#embed "word.bin"
};
static const unsigned char head[] = {
#embed "word.bin" limit(2 + 2) suffix(, 0)
};
static const int counted[] = {
#embed "empty.bin" prefix(1,) if_empty(-1)
};
#if __has_embed("word.bin") != 1 || __has_embed("empty.bin") != 2 || __has_embed("missing.bin") != 0
#error "__has_embed"
#endif

/* A vector type has no SultanC form: its declarations are written as a
   skip comment, never with a wrong type. */
typedef int v4si __attribute__((vector_size(16)));

int embedded(void) {
    CHECK(sizeof word == 7 && word[0] == 'S' && word[6] == 'C');
    CHECK(sizeof head == WORD_LIMIT + 1 && head[3] == 't' && head[4] == 0);
    CHECK(sizeof counted == sizeof(int) && counted[0] == -1);
    return 0;
}

int constants(void) {
    constexpr int local = WORD_LIMIT * 3;
    int table[local];               /* a constant length, not a VLA */
    CHECK(sizeof table == 12 * sizeof(int));
    switch (local) {
    case WORD_LIMIT * 3: break;     /* a case label */
    default: return __LINE__;
    }
    CHECK(BIG_MASK >> 40 == 0xFF);
    return 0;
}

int typeofs(int x) {
    typeof(x) twice = x * 2;
    typeof_unqual(const int) next = twice + 1;
    next += 1;                      /* not const */
    typeof(word[0]) byte = word[1];
    CHECK(twice == 2 * x && next == twice + 2 && byte == 'u');
    CHECK(sizeof(typeof(head)) == WORD_LIMIT + 1);
    return 0;
}

int bit_precise(void) {
    unsigned _BitInt(7) u = 120;
    u += 10;                         /* wraps modulo 2^7 */
    CHECK(u == 2);
    u = (unsigned _BitInt(7))(u - 3); /* 127 */
    CHECK(u == 127);
    u++;
    CHECK(u == 0);
    _BitInt(12) s = 2047;
    CHECK((s >> 4) == 127);
    s++;                             /* wraps to the most negative */
    CHECK(s == -2048);
    _BitInt(12) t = (_BitInt(12))5000;  /* 5000 - 4096 */
    CHECK(t == 904);
    /* Two bit-precise operands are not promoted to int. */
    unsigned _BitInt(4) a = 9, b = 9;
    CHECK((unsigned _BitInt(4))(a + b) == 2 && sizeof(a + b) == 1);
    CHECK(sizeof(s) == 2 && sizeof(_BitInt(33)) == 8);
    return 0;
}

/* __int128 (a GCC/Clang extension): no SultanC form in declarations, but
   compiled code computes with it in full 128 bits. */
typedef unsigned __int128 u128;
typedef __int128 i128;

static u128 global_wide = 7;
static i128 counter;

static u128 mul_add(u128 a, u128 b, unsigned long long c) { return a * b + c; }

int wide(void) {
    u128 one = 1;
    u128 big = one << 100;
    CHECK((unsigned long long)(big >> 64) == (1ull << 36));
    CHECK((unsigned long long)big == 0);
    u128 x = mul_add((u128)0xFFFFFFFFFFFFFFFFull, 0xFFFFFFFFFFFFFFFFull, 5);
    /* (2^64-1)^2 + 5 = 2^128 - 2^65 + 6 */
    CHECK((unsigned long long)(x >> 64) == 0xFFFFFFFFFFFFFFFEull && (unsigned long long)x == 6);
    CHECK(x / 3 == x / 3 && x % 1000 == x - (x / 1000) * 1000);
    i128 n = -1000;
    n *= 3;
    CHECK(n == -3000 && n < 0 && -n > 2999);
    CHECK(n / 7 == -428 && n % 7 == -4);
    CHECK((n >> 4) == -188);
    i128 m = (i128)1 << 126;
    CHECK(m > 0 && (m << 1) < 0);
    global_wide += 3;
    CHECK(global_wide == 10);
    counter++;
    counter--;
    counter -= 5;
    CHECK(counter == -5 && (int)counter == -5);
    double d = (double)big;
    CHECK(d > 1.2e30 && d < 1.3e30);
    u128 back = (u128)d;
    CHECK(back == big);
    CHECK(sizeof(i128) == 16 && _Alignof(u128) == 16);
    CHECK(~(u128)0 == ((one << 127) - 1) * 2 + 1);
    int flag = n ? 1 : 0;
    CHECK(flag && !(n + 3000));
    return 0;
}

int run_c23(void) {
    int r;
    if ((r = embedded()) != 0) return 1000 + r;
    if ((r = constants()) != 0) return 2000 + r;
    if ((r = typeofs(5)) != 0) return 3000 + r;
    if ((r = bit_precise()) != 0) return 4000 + r;
    if ((r = wide()) != 0) return 5000 + r;
    return 0;
}
