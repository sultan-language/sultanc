[العربية](../../ar/book/ch18-00-calling-c.md) | **English**

# Calling C

A great deal of useful code is written in C: operating system interfaces, compression, databases,
graphics. SultanC can call it directly, in two ways:

- declare a C function yourself, with `extern`, and link the library that defines it;
- import a C header, or a whole C source file, and let the compiler read the declarations from it.

## Declaring a C function

An `extern def` declares a function that is defined outside SultanC: its name, its parameters and its
return type, with no body. Here is `labs`, from the C standard library, which returns the absolute value
of a `long`:

```sultan ignore
import std.io.println;
import std.text.number;

extern def labs(value: i64): i64;

def main(): int {
    let distance: i64 = labs(-1234);
    println(int_to_text(distance as int));
    return 0;
}
```

The declaration must match the C function exactly: a C `long` is 64 bits on 64-bit Linux and macOS, so
it is an `i64` here. The compiler cannot check this against the C library; a wrong declaration is a bug
that only shows when the program runs.

The C library has to be linked into the program. `-l c` links `libc`; `-l NAME` links `libNAME`, and
`-L FOLDER` adds a folder to search for it:

```console
$ sultanc main.sn -o distance -l c
$ ./distance
1234
```

Without `-l c`, the link fails, because nothing defines `labs`:

```text
error[SULTANC-LINK-UNDEFINED-SYMBOL]: a symbol used by the program is not defined in any linked input — The undefined symbol is `labs`. The ELF link refers to an external symbol that no input defines.
```

`sultanc run` needs no `-l`: the interpreter finds C library functions while the program runs.

## Importing C code

Writing every declaration by hand is slow, and each one can be wrong. Instead, SultanC can import C
directly. `import "file.h";` reads a C header and turns its declarations into a SultanC module.
`import "file.c";` does more: it also compiles the C code, as part of the program, with SultanC's own C
frontend. No C compiler is needed, on any target.

Here is a small C file beside the program:

<sub>Filename: geometry.c</sub>

```c file=geometry.c
#include <stdint.h>

int64_t rectangle_area(int64_t width, int64_t height) {
    return width * height;
}

int count_vowels(const char *text) {
    int count = 0;
    for (; *text; text++) {
        switch (*text) {
        case 'a': case 'e': case 'i': case 'o': case 'u':
            count++;
        }
    }
    return count;
}
```

And the program that uses it:

<sub>Filename: main.sn</sub>

```sultan
import "geometry.c";
import std.io.println;
import std.text.number;
import std.text.from_bytes;

def main(): int {
    println(int_to_text(rectangle_area(6, 7) as int));
    let word: vector u8 = nul_terminated_bytes_from_text("banana");
    println(int_to_text(count_vowels(&word[0]) as int));
    return 0;
}
```

```text
42
3
```

Compile it as any other program; the compiler finds `geometry.c` beside `main.sn`:

```console
$ sultanc main.sn -o shapes
```

## How C types arrive

Each C type becomes the SultanC type of the same size: `int` is `i32`, `int64_t` is `i64`, `double` is
`f64`. A pointer to `const char` is `*u8`, a pointer to a byte that the function only reads; without
`const` it is `*mutable u8`. A C struct whose layout SultanC can express becomes a record; others, and
structs that are only declared, become *opaque* types, used only through pointers.

A C string is a pointer to bytes ending in a zero byte. `nul_terminated_bytes_from_text`, from
`std.text.from_bytes`, makes such bytes from a `string`, and `&word[0]` points at the first of them.

Some C declarations have no SultanC form: `long double`, function-like macros. They are left out of the
module, and the compiler says so with a warning that names each one. Importing a large system header,
such as `stdlib.h`, shows several of these warnings; the declarations you use are still there.

## Raw pointers and `unsafe`

A C function can return a raw pointer, `*T`. Reading the memory a raw pointer points at is something the
compiler cannot check, so it must be written inside an `unsafe` block:

```sultan fail
def main(): int {
    let values: vector u8 = [1 as u8, 2 as u8];
    let pointer: *u8 = &values[0];
    let first: u8 = *pointer;
    return first as int;
}
```

```text
error[SULTANC-SAFETY-UNSAFE-OPERATION]: operation requires an explicit unsafe scope
  --> main.sn:4:21..4:29
   |
 4 |     let first: u8 = *pointer;
   |                     ^^^^^^^^
   |
```

```sultan
def main(): int {
    let values: vector u8 = [1 as u8, 2 as u8];
    let pointer: *u8 = &values[0];
    let first: mutable u8 = 0;
    unsafe {
        first := *pointer;
    }
    return first as int;
}
```

`unsafe` does not turn the checks off for the rest of the program. It marks the few lines where you, not
the compiler, are responsible for the memory being valid, so that a reader, or a reviewer, knows where to
look. Keep `unsafe` blocks small, and wrap them in functions with safe parameters, as the standard
library does for its own system calls.

[Jisr](../internals/jisr.md), in the internals documentation, describes how C declarations are
translated, and everything compiled C code may contain.
