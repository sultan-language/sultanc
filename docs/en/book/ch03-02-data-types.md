[العربية](../../ar/book/ch03-02-data-types.md) | **English**

# Data Types

Every value in SultanC has a *type*, and the type decides what the value can hold and what you can do
with it. The compiler knows the type of every value before the program runs, and it refuses an
operation that does not fit the type. That is how chapter 2's comparison of a `string` with a `uint`
was caught.

This section covers two groups of types. *Scalar* types hold a single value: integers, floating-point
numbers, booleans and characters. *Compound* types group several values: tuples and vectors.

## Integers

An integer is a whole number. SultanC has several integer types, which differ in size and in whether
they can be negative:

| Size | Signed (can be negative) | Unsigned (never negative) |
| --- | --- | --- |
| 8 bits | `i8` | `u8` |
| 16 bits | `i16` | `u16` |
| 32 bits | `i32` | `u32` |
| 64 bits | `i64`, `int` | `u64`, `uint` |

`int` and `uint` are the 64-bit types, and the ones to use unless you have a reason to choose another:
`int` for ordinary counting and arithmetic, including positions in a vector, and `uint` for values
that can never be negative. The fixed sizes are for data whose width is set by something outside your program: a file
format, a network protocol, a C library.

Each type has a range. An `n`-bit signed type holds from -2<sup>n-1</sup> to 2<sup>n-1</sup> - 1, so
`i8` holds -128 to 127. An `n`-bit unsigned type holds from 0 to 2<sup>n</sup> - 1, so `u8` holds 0 to
255. `int` reaches about 9.2 × 10<sup>18</sup> in each direction.

Integer literals can be written in decimal, in hexadecimal after `0x`, or in binary after `0b`. An
underscore can separate digits, to make long numbers easier to read:

```sultan
import std.io.println;
import std.text.number;

def main(): int {
    let decimal: int = 255;
    let hex: int = 0xFF;
    let binary: int = 0b1111_1111;
    let million: int = 1_000_000;
    let small: i8 = -5;
    println(int_to_text(decimal + hex + binary));
    println(int_to_text(million));
    println(int_to_text(small as int));
    return 0;
}
```

```text
765
1000000
-5
```

### Integer overflow

What happens when a result does not fit its type? Here `127 + 1` is computed in an `i8`:

```sultan
import std.io.println;
import std.text.number;

def main(): int {
    let big: i8 = 127;
    let result: i8 = big + 1;
    println(int_to_text(result as int));
    return 0;
}
```

The program compiles, because the compiler cannot always know the values. But when it runs, it stops at
the addition and prints nothing:

```console
$ sultanc main.sn -o overflow
$ ./overflow
Illegal instruction
$ echo $?
132
```

The program does not continue with a wrong number. That is a deliberate choice: a wrong value that goes
unnoticed spreads through the rest of a program, while a stopped program shows you exactly where the
problem is. The same happens when a `uint` goes below zero (`0 - 1`), and when dividing by zero.

When wrapping around is what you want, for example in a hash function, `std.math.wrapping` has a
function for each operation and type. Its `checked_` functions return an `option` instead, which is
empty when the result does not fit:

```sultan
import std.io.println;
import std.text.number;
import std.math.wrapping;

def main(): int {
    let top: u8 = 255;
    println(uint_to_text(wrapping_add_u8(top, 1) as uint));
    match checked_add_i32(2147483647, 1) with
    | option.some(n) -> { println(int_to_text(n as int)); }
    | option.none -> { println("too big for i32"); };
    return 0;
}
```

```text
0
too big for i32
```

### Arithmetic

The operators are `+`, `-`, `*`, `/` and `%` (remainder). Integer division drops the fractional part,
rounding toward zero, and the remainder has the sign of the left side:

```sultan
import std.io.println;
import std.text.number;

def main(): int {
    println(int_to_text(7 / 2));
    println(int_to_text(7 % 2));
    println(int_to_text(-7 / 2));
    println(int_to_text(-17 % 5));
    println(int_to_text((2 + 3) * 4));
    return 0;
}
```

```text
3
1
-3
-2
20
```

Multiplication, division and remainder come before addition and subtraction, and parentheses change the
order.

### Types never mix by themselves

Both sides of an operator must have the same type. An `int` and a `uint` cannot be added directly:

```sultan fail
def main(): int {
    let a: int = 1;
    let b: uint = 2;
    return a + b;
}
```

```text
error[SULTANC-TYPE-MISMATCH]: expected `int`, found `uint` — The two operands of this operator have different types.
  --> main.sn:4:16..4:17
   |
 4 |     return a + b;
   |                ^
   |
  = help: convert the value explicitly with `as`
```

The conversion is written with `as`:

```sultan
import std.io.println;
import std.text.number;

def main(): int {
    let a: int = 1;
    let b: uint = 2;
    println(int_to_text(a + (b as int)));
    println(int_to_text(300 as u8 as int));
    println(int_to_text(3.99 as int));
    return 0;
}
```

```text
3
44
3
```

`as` converts between any two number types. To a smaller integer type it keeps the low bits: 300 is
256 + 44, so `300 as u8` is 44. From a floating-point number to an integer, it drops the fractional
part. Because every conversion is written out, every place where a number may change size or lose
precision is visible in the code.

### Bit operators

For flags and binary formats, integers also have the bit operators: `&` (and), `|` (or), `^` (exclusive
or), `~` (not), `<<` (shift left), and two right shifts: `>>` fills the left with zeros, and `>>>` keeps
the sign of a negative number:

```sultan
import std.io.println;
import std.text.number;

def main(): int {
    println(int_to_text(6 & 3));
    println(int_to_text(6 | 3));
    println(int_to_text(6 ^ 3));
    println(int_to_text(~6));
    println(int_to_text(1 << 4));
    let negative: int = -8;
    println(int_to_text(negative >>> 1));
    return 0;
}
```

```text
2
7
5
-7
16
-4
```

## Floating-point numbers

`f64` and `f32` hold numbers with a fractional part, in 64 and 32 bits. Use `f64` unless memory is
tight. A floating-point literal always has a point: `7.0`, not `7`.

```sultan
import std.io.println;
import std.text.float;

def main(): int {
    let width: f64 = 7.0;
    let half: f64 = width / 2.0;
    println(f64_to_string(half));
    println(f64_to_string(0.1 + 0.2));
    println(f64_to_string(10 as f64));
    return 0;
}
```

```text
3.5
0.30000000000000004
10
```

`f64_to_string`, from `std.text.float`, prints the shortest text that reads back as the same number.
That is why `0.1 + 0.2` shows `0.30000000000000004`: neither 0.1 nor 0.2 has an exact binary form, so
their sum is slightly more than 0.3. Every standard binary floating-point number behaves this way. To
print a fixed number of digits, use formatting, from [chapter 13](ch13-00-formatting.md).

## Booleans

A `bool` is `true` or `false`. Comparisons produce one:

| Operator | Meaning |
| --- | --- |
| `==` | equal |
| `!=` | not equal |
| `<`, `<=` | less than, less than or equal |
| `>`, `>=` | greater than, greater than or equal |

`&&` (and), `||` (or) and `!` (not) combine them. `&&` and `||` evaluate their right side only when
needed: in `a && b`, if `a` is false the result is false, and `b` is not evaluated.

```sultan
import std.io.println;

def main(): int {
    let age: int = 20;
    let has_ticket: bool = true;
    let can_enter: bool = age >= 18 && has_ticket;
    if (can_enter) {
        println("welcome");
    };
    if (!(age == 18)) {
        println("not exactly 18");
    };
    return 0;
}
```

```text
welcome
not exactly 18
```

## Characters

A `char` is one Unicode character, between single quotes: `'A'`, `'7'`, `'س'`. Converting it to `u32`
gives its Unicode number, the *code point*:

```sultan
import std.io.println;
import std.text.number;

def main(): int {
    let letter: char = 'A';
    let arabic: char = 'س';
    println(uint_to_text(letter as u32 as uint));
    println(uint_to_text(arabic as u32 as uint));
    return 0;
}
```

```text
65
1587
```

Text, the `string` type, is a sequence of characters stored as UTF-8 bytes. It has rules of its own,
about ownership and about bytes versus characters, and [chapter 8](ch08-02-strings.md) covers them.

## Tuples

A *tuple* groups a fixed number of values, which may have different types. The type lists the types in
parentheses, and the value lists the values:

```sultan
import std.io.println;
import std.text.number;

def main(): int {
    let point: (int, int) = (3, 4);
    let entry: (string, int) = ("dates", 45);
    match point with
    | (x, y) -> { println(int_to_text(x * x + y * y)); };
    match entry with
    | (name, price) -> {
        println(name);
        println(int_to_text(price));
    };
    return 0;
}
```

```text
25
dates
45
```

`match` takes a tuple apart: the pattern `(x, y)` gives each part a name. Tuples are handy for
returning two values from a function, as the next section shows. When the parts have meanings worth
naming, a record, from [chapter 5](ch05-00-records.md), is clearer.

## Vectors

A *vector* holds any number of values of one type, one after another. Its type is `vector` followed by
the element type, and a vector value lists its elements in square brackets:

```sultan
import std.io.println;
import std.text.number;

def main(): int {
    let primes: vector int = [2, 3, 5, 7, 11];
    println(int_to_text(primes[0]));
    println(int_to_text(primes[4]));
    println(int_to_text(length(primes)));
    return 0;
}
```

```text
2
11
5
```

`primes[0]` is the first element: positions start at 0. `length` gives the number of elements. A position past the end stops the program, the same way an overflow does, instead of reading
memory that is not part of the vector.

Vectors can grow, be passed to functions, and be sorted; [chapter 8](ch08-01-vectors.md) covers them.

## Summary

- `int` and `uint` are the default integers; `i8` to `i64` and `u8` to `u64` have exact sizes.
- Overflow, going below zero in an unsigned type, division by zero and a position past the end of a
  vector stop the program.
- Types never mix by themselves: `as` converts, and is always written.
- `f64` is the default floating-point type; its literals have a point.
- `bool` comes from comparisons; `&&`, `||` and `!` combine them.
- `char` is one Unicode character; tuples group a fixed set of values; vectors hold many of one type.
