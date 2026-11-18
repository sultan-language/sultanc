[العربية](../../ar/book/ch13-00-formatting.md) | **English**

# Formatting Output

Joining text with `concat` and `int_to_text` works, as chapter 12 did, but a line that mixes words and
numbers is easier to write as one template with holes for the values. `std.fmt` provides such
templates, called *format strings*. They also give control over the shape of the output: a fixed number
of digits, columns that line up, numbers in hexadecimal.

## A first format string

```sultan
import std.fmt.fmt;
import std.io.print_formatted;

def main(): int {
    let name: string = "Sultan";
    println_fmt("Hello, {}! You have {} messages.", [fmt_arg.text(&name), fmt_arg.int(3)]);
    return 0;
}
```

```text
Hello, Sultan! You have 3 messages.
```

Each `{}` is a placeholder, filled with the next argument. The arguments are given as a vector, and each
one says what kind of value it is:

| Argument | For a value of type |
| --- | --- |
| `fmt_arg.int(x)`, `fmt_arg.uint(x)` | `int`, `uint` |
| `fmt_arg.i8(x)` … `fmt_arg.u64(x)` | the fixed-size integers |
| `fmt_arg.f64(x)`, `fmt_arg.f32(x)` | floats |
| `fmt_arg.bool(x)`, `fmt_arg.char(x)` | `bool`, `char` |
| `fmt_arg.text(&x)` | `string` (borrowed) |
| `fmt_arg.ints(&v)`, `fmt_arg.texts(&v)` | a `vector int`, a `vector string` |

`println_fmt` prints the result and a new line; `print_fmt` prints without the new line. Both come from
`std.io.print_formatted`; the argument types come from `std.fmt.fmt`.

## The compiler checks the format string

When the format string is written in the code, the compiler checks it against the arguments. A
placeholder with no argument does not compile:

```sultan fail
import std.fmt.fmt;
import std.io.print_formatted;

def main(): int {
    println_fmt("{} {}", [fmt_arg.int(1)]);
    return 0;
}
```

```text
error[SULTANC-FORMAT-STRING]: invalid format string — The format argument's zero-based position is 1. The format string refers to an argument the call does not pass.
  --> main.sn:5:21..5:22
   |
 5 |     println_fmt("{} {}", [fmt_arg.int(1)]);
   |                     ^
   |
```

## Width, alignment and fill

After a `:` inside the braces comes a specification. A number is the minimum width; `<`, `^` and `>`
align left, centre and right; a character before the alignment fills the space:

```sultan
import std.fmt.fmt;
import std.io.print_formatted;

def main(): int {
    let name: string = "Sultan";
    println_fmt("[{:<10}]", [fmt_arg.text(&name)]);
    println_fmt("[{:^10}]", [fmt_arg.text(&name)]);
    println_fmt("[{:>10}]", [fmt_arg.text(&name)]);
    println_fmt("[{:*^10}]", [fmt_arg.text(&name)]);
    return 0;
}
```

```text
[Sultan    ]
[  Sultan  ]
[    Sultan]
[**Sultan**]
```

This is how to print tables whose columns line up. The width counts characters, not bytes, so Arabic
text lines up as well as Latin text:

```sultan
import std.fmt.fmt;
import std.io.print_formatted;

type Item = { name: string, price: int };

def main(): int {
    let items: vector Item = [
        { name: "Tea", price: 12 },
        { name: "Dates", price: 45 },
        { name: "Coffee", price: 30 }
    ];
    for (let i: mutable int = 0; i < length(items); i := i + 1) {
        println_fmt("{:<8}{:>5}", [fmt_arg.text(&items[i].name), fmt_arg.int(items[i].price)]);
    }
    return 0;
}
```

```text
Tea        12
Dates      45
Coffee     30
```

## Numbers

| Specification | Effect | `42` gives |
| --- | --- | --- |
| `{:+}` | always show the sign | `+42` |
| `{:05}` | pad with zeros to width 5 | `00042` |
| `{:x}`, `{:X}` | hexadecimal | `2a`, `2A` |
| `{:#x}` | hexadecimal with `0x` | `0x2a` |
| `{:b}`, `{:o}` | binary, octal | `101010`, `52` |

For floats, `.N` sets the number of digits after the point, and `e` uses the exponent form:

```sultan
import std.fmt.fmt;
import std.io.print_formatted;

def main(): int {
    println_fmt("{:.2}", [fmt_arg.f64(3.14159)]);
    println_fmt("{:8.3}|", [fmt_arg.f64(2.5)]);
    println_fmt("{:e}", [fmt_arg.f64(1234.5)]);
    println_fmt("{:+} {:05} {:#x}", [fmt_arg.int(42), fmt_arg.int(42), fmt_arg.int(42)]);
    return 0;
}
```

```text
3.14
   2.500|
1.2345e3
+42 00042 0x2a
```

## Positions and braces

`{0}`, `{1}`… name an argument by its position, so one argument can be used twice. `{{` and `}}` print
literal braces:

```sultan
import std.fmt.fmt;
import std.io.print_formatted;

def main(): int {
    let name: string = "Noor";
    println_fmt("{0} and {0} again; {{braces}}", [fmt_arg.text(&name)]);
    return 0;
}
```

```text
Noor and Noor again; {braces}
```

## Formatting without printing

`format` returns the text instead of printing it, as a `result`: a format string built while the
program runs can only be checked then, and a mistake in it is an error value, not a crash.

```sultan
import std.fmt.fmt;
import std.io.println;

def main(): int {
    match format("{:>6}|", [fmt_arg.int(42)]) with
    | result.ok(text) -> { println(text); }
    | result.err(_) -> { println("bad format string"); };
    return 0;
}
```

```text
    42|
```

[Formatting](../reference/formatting.md) in the reference specifies every option, including custom
formatters for your own types.

## Summary

- `println_fmt("… {} …", [fmt_arg.int(x), …])` prints a template with its holes filled.
- Each argument is wrapped by its type: `fmt_arg.int`, `fmt_arg.text(&s)`, `fmt_arg.f64`….
- A format string written in the code is checked at compile time.
- `{:<10}`, `{:^10}`, `{:>10}` align; `{:05}`, `{:x}`, `{:.2}` shape numbers.
- `format` returns the text as a `result`.
