/* Calls through C function pointers that are machine addresses, compiled by
   Jisr (compiler/jisr/lower.sn, نادِ_عنوانا_جسر): a pointer from dlsym, and
   the address of a C library function. A pointer to a function compiled here
   is still the address of its marker and is called directly. Records pass
   by value as in a direct call (call shapes at an address, link name `*`). */
#include <dlfcn.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

static int add(int a, int b) { return a + b; }

int through_dlsym(void) {
    size_t (*length)(const char *) = (size_t (*)(const char *))dlsym(RTLD_DEFAULT, "strlen");
    if (!length) return -1;
    return (int)length("hello");
}

/* A double and a float result, pointer arguments. */
int through_dlsym_floating(void) {
    double (*to_double)(const char *, char **) = (double (*)(const char *, char **))dlsym(RTLD_DEFAULT, "strtod");
    float (*to_float)(const char *, char **) = (float (*)(const char *, char **))dlsym(RTLD_DEFAULT, "strtof");
    if (!to_double || !to_float) return 1;
    char *end = 0;
    if (to_double("2.5x", &end) != 2.5 || *end != 'x') return 2;
    if (to_float("0.75", 0) != 0.75f) return 3;
    return 0;
}

/* The address of a library function, a signed argument narrower than 64 bits. */
int through_library_address(void) {
    int (*absolute)(int) = abs;
    long (*absolute_long)(long) = labs;
    return absolute(-3) + (int)absolute_long(-4);
}

int through_marker(void) {
    int (*op)(int, int) = add;
    return op(20, 22);
}

/* Records by value through pointers: an 8-byte result in one register
   (div_t), a 16-byte one in two (ldiv_t, lldiv_t), and a 4-byte record
   argument (struct in_addr). They take the same convention as a direct call. */
int through_dlsym_records(void) {
    div_t (*divide)(int, int) = (div_t (*)(int, int))dlsym(RTLD_DEFAULT, "div");
    ldiv_t (*divide_long)(long, long) = ldiv;
    lldiv_t (*divide_long_long)(long long, long long) =
        (lldiv_t (*)(long long, long long))dlsym(RTLD_DEFAULT, "lldiv");
    char *(*dotted)(struct in_addr) = (char *(*)(struct in_addr))dlsym(RTLD_DEFAULT, "inet_ntoa");
    if (!divide || !divide_long_long || !dotted) return 1;
    div_t d = divide(-17, 5);
    if (d.quot != -3 || d.rem != -2) return 2;
    ldiv_t l = divide_long(1000000000007L, 1000L);
    if (l.quot != 1000000000L || l.rem != 7L) return 3;
    lldiv_t q = divide_long_long(-9000000000000000001LL, 1000000000LL);
    if (q.quot != -9000000000LL || q.rem != -1LL) return 4;
    struct in_addr address;
    address.s_addr = htonl(0x7F000102u);
    if (strcmp(dotted(address), "127.0.1.2") != 0) return 5;
    return 0;
}
