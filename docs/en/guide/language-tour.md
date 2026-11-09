[العربية](../../ar/guide/language-tour.md) | **English**

# Language Tour

This tour introduces SultanC through short programs, written here with the English keywords. It is written for
programmers who already know another systems language and want to read and write SultanC. Every example
compiles with the current compiler; the sections point to the reference pages for the full rules.

## Two surfaces, one language

Arabic and English are equal source surfaces. A keyword, a type name or a standard-library function has one
meaning and two spellings, and the compiler resolves both to the same canonical form before any later stage sees
them. A file can mix the two. Names you declare may use either script.

The entry function is `main` in English and `الرئيسية` in Arabic, and the `int` it returns is the process exit
status. The Arabic edition of this tour shows the same programs in Arabic. The complete vocabulary is in
[Arabic terminology](../reference/arabic-terminology.md).

## Values and types

`let` binds a name. A binding is immutable unless its type is wrapped in `mutable`, and `:=` assigns. Types are
written after the name. A top-level `let` is a module constant evaluated at compile time.

The builtin types are `int`, `uint`, `bool`, `char`, `string`, `void`, the fixed-width integers `i8`…`i64` and
`u8`…`u64`, and the floats `f32` and `f64`. Conversions between scalar types are explicit and use `as`.

```sultan
let limit: int = 100;

def main(): int {
    let byte: u8 = 255;
    let ratio: f64 = 0.75;
    let letter: char = 'S';
    let name: string = "Sultan";
    let counter: mutable int = 0;
    counter := counter + 1;
    let widened: i64 = (byte as i64) - 250;
    return counter + limit + (widened as int);
}
```

See [Type system](../reference/type-system.md) for the type model and the exact conversion rules.

## Functions

A function declares its parameter types and its result type. Anonymous functions capture the values around them,
and function types are written `fn(int): int`. A function takes type parameters in angle brackets.

```sultan
def make_adder(amount: int): fn(int): int {
    return fn(value: int): int -> value + amount;
}

def identity<T>(value: T): T {
    return value;
}

def main(): int {
    let add_ten: fn(int): int = make_adder(10);
    return add_ten(identity(5));
}
```

## Control flow

`if` takes a parenthesized condition and braced blocks, and the statement ends with a semicolon. `while`
tests a condition before each pass. `for` has the C form with an initializer, a condition and a
step. `continue` and `break` work in both loops.

```sultan
import std.io.println;
import std.text.number;

def main(): int {
    let total: mutable int = 0;
    let i: mutable int = 1;
    while (i <= 5) {
        total := total + i;
        i := i + 1;
    }
    for (let k: mutable int = 0; k < 10; k := k + 1) {
        if (k % 2 == 0) { continue; };
        if (k > 6) { break; };
        total := total + k;
    }
    if (total > 20) {
        println("large");
    } else {
        println("small");
    };
    println(int_to_text(total));
    return 0;
}
```

## Records, enums and pattern matching

A record type names a set of fields. A value is written as a literal in braces and a field is read with a dot.
`type` also names an existing type, as in `type Celsius = int;`. An enum lists its constructors, each optionally carrying a payload with `of`. `match` selects an
arm by pattern. The compiler checks that the arms are exhaustive. Patterns destructure payloads and records, and
match integer, character and string literals, alternatives separated by `|`, and ranges.

```sultan
type Point = { x: int, y: int };

type Shape =
    | Circle of int
    | Rectangle of Point
    | Dot
;

def area(shape: Shape): int {
    match shape with
    | Shape.Circle(r) -> { return 3 * r * r; }
    | Shape.Rectangle({ x: width, y: height }) -> { return width * height; }
    | Shape.Dot -> { return 0; };
}

def main(): int {
    let size: Point = { x: 3, y: 4 };
    return area(Shape.Circle(2)) + area(Shape.Rectangle(size)) + size.x;
}
```

A constructor without a payload is written without parentheses in a pattern and with them when it is built, as in
`Shape.Dot()` below the type.

## Vectors

A `vector T` is a growable sequence that owns its elements. A literal is written in brackets. Indexing
uses `[ ]`, `length` gives the element count, and `append` adds an element to a `mutable`
vector. Passing `&values` lends the vector without giving it away.

```sultan
def main(): int {
    let values: mutable vector int = [10, 20];
    append(values, 12);
    return values[0] + values[2] + length(values);
}
```

## Ownership and borrowing

Every value has one owner. Using an owning value, such as a `string`, a `vector` or a record that contains one,
moves it: the old name can no longer be used. Plain values such as numbers are copied. A reference `&T`
borrows a value for reading, and `&mutable T` borrows it for writing. A reference is dereferenced with
`*`. `clone` makes an independent deep copy. The compiler checks all of this before generating code,
and frees an owned value when its owner goes out of scope.

```sultan
import std.io.println;
import std.text.number;

type Account = { owner: string, balance: int };

def deposit(account: &mutable Account, amount: int): void {
    (*account).balance := (*account).balance + amount;
}

def close(account: Account): string {
    return clone(&account.owner);
}

def main(): int {
    let account: mutable Account = { owner: "Layla", balance: 100 };
    deposit(&mutable account, 50);
    println(int_to_text(account.balance));
    println(close(account));
    return 0;
}
```

`close` takes its `Account` by value, so `account` is moved at the call. Using `account` afterwards is rejected
with `SULTANC-OWNERSHIP-USE-AFTER-MOVE`, which points at both the original owner and the move. A mutable borrow
cannot coexist with another borrow of the same value (`SULTANC-BORROW-CONFLICT`), and an owned
value cannot be moved out of borrowed storage (`SULTANC-OWNERSHIP-MOVE-OUT-OF-REFERENCE`). The rules, the drop
model and `clone` are in [Ownership and destruction](../reference/ownership.md).

## Errors

Failure is a value. `result T, E` holds `result.ok(value)` or `result.err(error)`. `option T` holds
`option.some(value)` or `option.none`. `match` takes either apart. The postfix `?` returns the error or the absence to the
caller and otherwise yields the value, as in `let q: int = divide(a, b)?;`. The error type must be the same on both
sides of the call.

```sultan
import std.io.println;
import std.text.number;

def divide(a: int, b: int): result int, string {
    if (b == 0) { return result.err("division by zero"); };
    return result.ok(a / b);
}

def half_quotient(a: int, b: int): result int, string {
    let q: int = divide(a, b)?;
    return result.ok(q / 2);
}

def show(outcome: result int, string): void {
    match outcome with
    | result.ok(q) -> { println(int_to_text(q)); }
    | result.err(message) -> { println(message); };
}

def main(): int {
    show(half_quotient(84, 2));
    show(half_quotient(1, 0));
    return 0;
}
```

`panic` stops the program with a message for conditions that should never happen. `main` may itself
return a `result`, in which case an error is printed and the process exits with status 1. The std error value, its
helpers and the exact semantics of `?` are in [Errors](../reference/errors.md).

## Modules and imports

Each `.sn` file is a module. `public` exposes a declaration to importers, and everything else stays private.
There are three import forms: a quoted path relative to the importing file, a dotted module path, and a selective
import of names from a directory. `as` gives a module a local name used as a qualifier.

```sultan
import "geometry_en.sn";
import std.text.number as numbers;
from std.io import println;

def main(): int {
    let room: Rectangle = { width: 5, height: 4 };
    println(numbers.int_to_text(area(&room)));
    return 0;
}
```

The module they import:

```sultan
public type Rectangle = { width: int, height: int };

public def area(shape: &Rectangle): int {
    return (*shape).width * (*shape).height;
}
```

`import std.text.number;` brings every public name of that std module into scope, and `import std.text.number
as numbers;` makes them reachable only as `numbers.name`. `std` has the Arabic spelling `المكتبة`, and its
subdirectories have Arabic names as well (`المكتبة.نصوص.number`). [Modules and imports](../reference/modules-and-imports.md)
specifies resolution.

## Using the standard library

`std.io.println` writes a line to standard output, `std.text.number` converts numbers to text and back, and `std.fmt`
formats text with Rust-style placeholders. Each argument to a format function is wrapped in a value that carries
its type, and a literal format string is checked at compile time.

```sultan
import std.fmt.fmt;

def main(): int {
    let name: string = "Sultan";
    let r1: result uint, fmt_error = println_fmt("Hello, {}! You have {} messages.", [fmt_arg.text(&name), fmt_arg.int(3)]);
    let r2: result uint, fmt_error = println_fmt("[{0:>8}] {1:#x} {2:.2}", [fmt_arg.text(&name), fmt_arg.int(255), fmt_arg.f64(3.14159)]);
    return 0;
}
```

```text
Hello, Sultan! You have 3 messages.
[  Sultan] 0xff 3.14
```

The format-spec grammar, custom formatters and alignment rules are in [Formatting](../reference/formatting.md).
[Std coverage](../reference/std-coverage.md) lists what the library provides.

## See also

- [Getting started](getting-started.md)
- [Type system](../reference/type-system.md)
- [Ownership and destruction](../reference/ownership.md)
- [Errors](../reference/errors.md)
- [Compile-time evaluation](../reference/comptime.md)
- [Concurrency](../reference/concurrency.md)
- The `examples/` directory, which holds many more runnable programs in both languages
