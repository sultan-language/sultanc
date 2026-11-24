#include <stdarg.h>
#include "va.h"

int vsum(int count, ...) {
    va_list ap;
    va_start(ap, count);
    int s = 0;
    for (int i = 0; i < count; i++) s += va_arg(ap, int);
    va_end(ap);
    return s;
}

/* A double, then an int, scaled. */
double vmix(int scale, ...) {
    va_list ap;
    va_start(ap, scale);
    double d = va_arg(ap, double);
    d += va_arg(ap, int);
    va_end(ap);
    return d * scale;
}

/* The total length of strings up to a null pointer. */
long vlen(const char *first, ...) {
    va_list ap;
    va_start(ap, first);
    long n = 0;
    for (const char *s = first; s; s = va_arg(ap, const char *))
        while (*s++) n++;
    va_end(ap);
    return n;
}
