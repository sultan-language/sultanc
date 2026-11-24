/* Jisr's preprocessor: macro expansion, conditional inclusion, guards. */
#pragma once
#include "guarded.h"
#include "guarded.h"
#include "once.h"
#include "once.h"

/* Function-like macros used by object-like ones. */
#define SQUARE(x) ((x) * (x))
#define ADD3(a, b, c) ((a) + (b) + (c))
#define AREA SQUARE(7)
#define SUM3 ADD3(1, SQUARE(2), 10)

/* # and ##. */
#define STR(x) #x
#define XSTR(x) STR(x)
#define CAT(a, b) a##b
#define VERSION_MAJOR 3
#define VERSION_TEXT XSTR(VERSION_MAJOR) "." STR(14)
#define CAT_VALUE CAT(0x, 1F)
#define BITS_7 CAT(BIT, 7)
#define BIT7 (1 << 7)

/* __VA_ARGS__: counting arguments, GNU's `, ## __VA_ARGS__`. */
#define COUNT_(_1, _2, _3, _4, n, ...) n
#define COUNT(...) COUNT_(__VA_ARGS__, 4, 3, 2, 1, 0)
#define ARGS_THREE COUNT(a, b, c)
#define FIRST(x, ...) (x)
#define FIRST_OF FIRST(41, 2, 3)
#define PICK(fmt, ...) ADD3(fmt, 0, 0 , ## __VA_ARGS__)
#define PICKED PICK(9)
#define OPT(x, ...) ((x) __VA_OPT__(+ 1))
#define OPT_SOME OPT(5, z)
#define OPT_NONE OPT(5)

/* A macro naming one defined after it: expanded when used. */
#define LATER (LATER_BASE + 1)
#define LATER_BASE 100

/* Conditional inclusion. */
#define LEVEL 4
#if defined(LEVEL) && LEVEL > 3 && !defined(NOT_DEFINED)
#  define CHOSEN 1
#elif LEVEL > 1
#  define CHOSEN 2
#else
#  define CHOSEN 3
#endif
#if __has_include("guarded.h") && !__has_include("missing_header_for_jisr.h")
#  define HAS_INCLUDE_WORKS 1
#endif
#ifdef __STDC_VERSION__
#  define C_VERSION __STDC_VERSION__
#endif
#if (1 ? 2 : (1 / 0)) == 2 && (0 && (1 / 0)) == 0
#  define SHORT_CIRCUIT 1
#endif

/* Redefinition after #undef. */
#define CHANGING 1
#undef CHANGING
#define CHANGING 2

/* Typed constants: C's types decide the SultanC ones. */
#define BIG_UNSIGNED 0xFFFFFFFFu
#define BIG_LONG_LONG 0x100000000LL
#define NEGATIVE_LIMIT (-2147483647 - 1)
#define CHAR_VALUE 'A'
#define ESCAPED_CHAR '\n'
#define SHIFTED (1u << 31)
#define SIZE_OF_INT ((int)sizeof(int))
#define HALF 0.5
#define TINY 1e-3
#define GREETING "hello, " "world"

/* Enumerators referring to macros and to one another. */
enum colors { RED = GUARD_VALUE, GREEN = RED * 2, BLUE = CAT(GR, EEN) + ONCE_VALUE };
