/* The target's C data model as Jisr presents it to C: the predefined macros
   select a branch, and the static assertions there are evaluated with the
   target's layout. A wrong macro, size or signedness fails the import. */
#include <stddef.h>
#include <stdint.h>

#if defined(__APPLE__) && defined(__aarch64__) && defined(__arm64__)
#  define PLATFORM 1
_Static_assert(sizeof(long) == 8 && sizeof(void *) == 8 && __SIZEOF_LONG_DOUBLE__ == 8, "Apple arm64 is LP64, long double is double");
_Static_assert((char)-1 < 0, "char is signed on Apple arm64");
#elif defined(__APPLE__) && defined(__x86_64__)
#  define PLATFORM 2
_Static_assert(sizeof(long) == 8 && __SIZEOF_LONG_DOUBLE__ == 16, "x86-64 Darwin is LP64 with an x87 long double");
#elif defined(__linux__) && defined(__x86_64__)
#  define PLATFORM 3
_Static_assert(sizeof(long) == 8 && _Alignof(long double) == 16, "x86-64 Linux is LP64 with an x87 long double");
_Static_assert((char)-1 < 0, "char is signed on x86-64");
#elif defined(__linux__) && defined(__aarch64__)
#  define PLATFORM 4
_Static_assert((char)-1 > 0 && (wchar_t)-1 > 0, "char and wchar_t are unsigned on AArch64 Linux");
#elif defined(_WIN64) && defined(_MSC_VER)
#  define PLATFORM 5
_Static_assert(sizeof(long) == 4 && sizeof(wchar_t) == 2 && sizeof(size_t) == 8, "Windows is LLP64 with a 16-bit wchar_t");
#elif defined(__i386__)
#  define PLATFORM 6
_Static_assert(sizeof(long) == 4 && sizeof(void *) == 4, "i386 is ILP32");
struct i386_pair { char c; long long l; };
_Static_assert(sizeof(struct i386_pair) == 12, "i386 System V aligns long long members to 4");
#else
#  define PLATFORM 0
#endif

#define LONG_BYTES ((int)sizeof(long))
#define POINTER_BYTES ((int)sizeof(void *))
#define SIZE_BYTES ((int)sizeof(size_t))
#define INT64_IS_LONG (sizeof(int64_t) == sizeof(long))

/* Records with long and pointer members change size with the model. */
struct model_probe { char tag; long value; void *pointer; };
enum { PROBE_BYTES = sizeof(struct model_probe) };
long probe_long(long value);
size_t probe_size(size_t value);
