[العربية](../../ar/book/ch16-00-comptime.md) | **English**

# Compile-Time Evaluation

Some values never change between runs: a table of squares, the size of a buffer derived from a few
constants, a check that a setting is in range. SultanC can compute them while it compiles the program,
so the finished program contains only the results. This chapter shows the three ways to do that.

## Module constants

You have already used the first: a `let` outside any function is computed at compile time
([chapter 3](ch03-01-variables-and-mutability.md)). If computing it fails, for example by dividing by zero or overflowing,
the program does not compile.

## `comptime` expressions

`comptime` before an expression runs it while compiling. It can call ordinary functions, with loops and
everything else, as long as they do no input or output:

```sultan
import std.io.println;
import std.text.number;

def sum_of_squares(limit: int): int {
    let total: mutable int = 0;
    for (let i: mutable int = 1; i <= limit; i := i + 1) {
        total := total + i * i;
    }
    return total;
}

def main(): int {
    let table_size: int = comptime sum_of_squares(10);
    println(int_to_text(table_size));
    return 0;
}
```

```text
385
```

When the program runs, `table_size` is simply 385: the loop ran inside the compiler, once.

A `comptime` block computes a value with several statements. Its value is the last expression in it,
written without a semicolon:

```sultan
import std.io.println;
import std.text.number;

def main(): int {
    let fibonacci_20: int = comptime {
        let a: mutable int = 0;
        let b: mutable int = 1;
        let i: mutable int = 0;
        while (i < 20) {
            let next: int = a + b;
            a := b;
            b := next;
            i := i + 1;
        }
        a
    };
    println(int_to_text(fibonacci_20));
    return 0;
}
```

```text
6765
```

## Compile-time parameters

A parameter marked `comptime` must be given a value known while compiling. The function can then check
that value, and `compile_error` (from `std.meta.reflect`) stops the build with a message when it is
wrong:

```sultan
import std.io.println;
import std.text.number;
import std.meta.reflect;

def checked_width(comptime bits: int): int {
    if (bits > 64) {
        compile_error("width must be at most 64 bits");
    };
    return bits;
}

def main(): int {
    println(int_to_text(checked_width(8)));
    return 0;
}
```

```text
8
```

Changing `checked_width(8)` to `checked_width(80)` makes the program fail to compile, with that message.
A mistake that would otherwise be found by running the program, or by a user, is found by the compiler.

## Limits

Compile-time code cannot read files, print, use the network, or see anything about the machine. Its
results must be integers, booleans, characters, text, or vectors of these. It has a budget of
instructions and of call depth, so a loop that never ends becomes a compile error rather than a
compiler that never finishes.

[Compile-time evaluation](../reference/comptime.md) in the reference covers reflection, annotations and
specialization.

## Summary

- Module constants are computed at compile time.
- `comptime expression` and `comptime { …; value }` run code while compiling.
- `comptime` parameters take compile-time values; `compile_error` rejects bad ones during the build.
- Compile-time code does no I/O and has a time budget.
