[العربية](../../ar/reference/formatting.md) | **English**

# Formatting

`std.fmt.fmt` formats a format string and its arguments: to new text, into a formatter, to any
std.io writer, to standard output and error, and to a logger. A placeholder in braces selects an
argument and a format spec (fill, alignment, sign, width, precision and a type such as Display,
Debug or hexadecimal). The page covers the API, the format-string grammar, custom formatters and the
compile-time check, and is written for programmers who format text and for those who work on
`std.fmt`.

```sultan
import std.fmt.fmt;

def main(): int {
    let name: string = "Sultan";
    match format("{:<8}|{:>5}|{:.2}", [fmt_arg.text(&name), fmt_arg.int(42), fmt_arg.f64(3.14159)]) with
    | result.ok(line) -> { let r: result uint, fmt_error = println_fmt("{}", [fmt_arg.text(&line)]); }
    | result.err(e) -> { return 1; };
    return 0;
}
```

The program prints `Sultan  |   42|3.14`. The format
string is a literal, so the compiler checks it against the arguments before the program runs.
More examples are in `examples/{en,ar}/fmt/` (`formatting`, `table`, `display`).

## Arguments

SultanC has no variadic functions and no traits, so the arguments are a vector of `fmt_arg`
values, with one constructor per type.

| Constructor | Holds |
|---|---|
| `int`, `uint` | `int`, `uint` |
| `i8` … `i64`, `u8` … `u64` | the fixed-width integers; the width matters for `x`, `o` and `b` of negative values |
| `f32`, `f64` | floats |
| `bool`, `char` | booleans and characters |
| `text` | `&string`: borrowed, never copied |
| `bytes` | `&vector u8` |
| `ints`, `texts` | `&vector int`, `&vector string` (Debug only) |
| `list` | `&vector fmt_arg`: any list, nested lists included (Debug only) |
| `custom` | `fn(&mutable formatter): result uint, fmt_error`, the Display and Debug of a user type |

Text and vectors are borrowed, so formatting copies nothing and the caller keeps its values. The
argument vector is passed by value, which costs nothing to move. It is usually a vector literal in
the call itself, which also lets the compiler check the format string against it.

## API

| Function | Writes to |
|---|---|
| `format(template, args): result string, fmt_error` | new text |
| `format_into(&mutable f, template, args): result uint, fmt_error` | a formatter, including the one a custom formatter is given |
| `write_fmt(&dest, writer, template, args)` | any std.io `writer<S>` |
| `write_fmt_with(&mutable f, &dest, writer, template, args)` | the same, reusing one formatter's buffer |
| `print_fmt`, `println_fmt`, `eprint_fmt`, `eprintln_fmt` | standard output or standard error, with or without a newline |
| `log_fmt(&logger, level, template, args)` (std.io.log) | a logger, at a level |

The write and print functions return the number of bytes written. The English print names carry
`_fmt` because `print` and `println` already name std.io's functions, and a program that imports
both would find the names ambiguous.

A `formatter` is one output buffer. `new_formatter`, `formatter_with_capacity`,
`formatter_text` (a copy), `take_formatter_text` (moved out, no copy), `formatter_length` and
`formatter_clear` (keeps the buffer) let a loop reuse one, as `write_fmt_with` does.

## The format string

```
format_string := text [ maybe_format text ] *
maybe_format  := '{' '{' | '}' '}' | format
format        := '{' [ argument ] [ ':' format_spec ] [ ws ] * '}'
argument      := integer
format_spec   := [[fill]align][sign]['#']['0'][width]['.' precision]type
fill          := character
align         := '<' | '^' | '>'
sign          := '+' | '-'
width         := count
precision     := count | '*'
type          := '' | '?' | 'x?' | 'X?' | 'x' | 'X' | 'o' | 'b' | 'e' | 'E'
count         := integer | integer '$'
```

There are no named arguments. `{}` takes the next argument, `{N}` takes argument
N, and `{{` and `}}` are literal braces. Every argument must be used.

| Element | Behavior |
|---|---|
| Fill and alignment | `{:<8}` `{:^8}` `{:>8}` `{:*^9}`. Text is left-aligned by default and numbers right-aligned. Centering puts the odd fill character after. |
| Sign | `{:+}` always prints a sign. `-` is accepted and ignored. NaN never has a sign. |
| `#` | The alternate form: `0x`, `0o` and `0b` prefixes. Pretty Debug (`{:#?}`) puts list entries on their own lines, indented by four spaces. |
| `0` | Sign-aware zero padding: `{:08}` of -42 is `-0000042` and `{:#010x}` of 255 is `0x000000ff`. Fill and alignment are then ignored. Zero padding applies to NaN and inf too. |
| Width and precision | Literals or arguments. `{:1$}` takes the width from argument 1, `{:.2$}` the precision from argument 2, and `{:.*}` takes the precision from the next argument before the value. Both are at most 65535 (16 bits), and an argument must be a non-negative integer. |
| Precision | Truncates text, rounds floats and is ignored by integers. Rounding uses the exact value, half to even: `{:.1}` of 0.25 is `0.2` and of 0.35 is `0.3`. |

### Types

| Type | Integers | Floats | Text | Other |
|---|---|---|---|---|
| `{}` | decimal | shortest round trip, positional (`1e21` prints all its digits, `-0.0` is `-0`) | as is | `true`/`false`; a char; bytes as UTF-8 text |
| `{:?}` | decimal | shortest; exponent form below 1e-4 and from 1e16; `.0` for an integral value (`1.0`, `1e16`) | quoted and escaped | lists as `[a, b]`; a char quoted |
| `{:x}` `{:X}` | hexadecimal; a negative value's two's complement at its width | | | bytes as hex digits |
| `{:o}` `{:b}` | octal, binary | | | |
| `{:e}` `{:E}` | exponent form (`1.234e3`) | exponent form, shortest or with a precision (`1.2345e3`, `{:.2e}` is `1.23e3`) | | |
| `{:x?}` `{:X?}` | Debug in hexadecimal, also inside lists | | | |

A type that does not apply to an argument (`{:x}` of text, `{}` of a list) is the error
`mismatched_type`.

Float digits come from `std.text.float`: the shortest decimal that reads back to the same value,
or, with a precision, the exact value rounded half to even (`f64_fixed_digits`,
`f64_significant_digits`). Both try Grisu
(shortest and counted modes) and fall back to the exact big-integer algorithm when Grisu cannot be
sure (Grisu, then Dragon).

### Text width and Arabic

Width and precision of text count extended grapheme clusters (UAX #29, `std.unicode.grapheme`).
A letter with its harakat is one character, so `{:>6}` of «مُحَمَّد» (8 scalar values, 4 clusters)
adds 2 fill characters, and a precision never separates a letter from its marks. ASCII text
without CR takes a fast path. For text without combining marks, clusters and Unicode scalar
values agree. A lam-alef pair is two clusters although fonts draw one glyph.

`std.fmt` inserts no direction marks. The caller isolates bidirectional text where it needs to,
with `std.text.bidi`.

### Debug text

`{:?}` quotes text and escapes `\t \r \n \\ \0` and the quote. It writes control, format
(including the direction marks U+200E and U+200F, and isolates), separator (other than space),
private-use and unassigned characters as `\u{…}`. A combining mark
is escaped where it starts the value. A mark after a letter inside text is kept,
so Arabic harakat stay readable. Width does not apply to Debug text or chars.

## Custom formatters

A `custom` argument holds a closure that writes into the formatter: the Display and Debug forms of
a user type.

```sultan
import std.fmt.fmt;

type Point = { x: int, y: int };

def show_point(p: &Point, f: &mutable formatter): result uint, fmt_error {
    if (formatter_debug(f)) {
        return format_into(f, "Point {{ x: {}, y: {} }}", [fmt_arg.int((*p).x), fmt_arg.int((*p).y)]);
    };
    return format_into(f, "({}, {})", [fmt_arg.int((*p).x), fmt_arg.int((*p).y)]);
}

def main(): int {
    let p: Point = { x: 1, y: 2 };
    let r: result uint, fmt_error = println_fmt("{} {:?}", [
        fmt_arg.custom(fn(f: &mutable formatter): result uint, fmt_error -> show_point(&p, f)),
        fmt_arg.int(3)
    ]);
    return 0;
}
```

The closure can read the placeholder's spec with `formatter_width`, `formatter_precision`,
`formatter_fill`, `formatter_align`, `formatter_alternate`, `formatter_sign_plus`,
`formatter_sign_aware_zero_pad`, `formatter_kind` and `formatter_debug`. It writes with
`format_into`, `write_str` and `write_char`, and honors width, alignment and precision with `pad`.
Output written without `pad` ignores the width.

The formatter's spec is restored after the closure, so the next element of a list gets the list's
spec. A custom element of a pretty-printed list is indented line by line.

### What a custom closure can capture

SultanC closures capture plain values only: numbers, booleans, enums and records of them, and
other closures. A value that owns text or vectors cannot be captured. Format it with its own
function into a formatter and pass `take_formatter_text` as a `text` argument, which moves the text
out without a copy.

Capturing a reference, so that a formatter could borrow `&p` of a value that owns text, is not
supported because it cannot be made sound. A closure value is a counted environment whose function
type `fn(...)` carries no lifetime, and closures flow freely through parameters, fields, vectors
and returns. The ownership checker would have to treat every closure value as a possible borrow
carrier and every `fn` type as lifetime-bounded. A capture of
`&p` accepted without that could outlive `p`.

## Compile-time check

A literal format string is checked at compile time. The check applies to a call of a std format function (the functions of
`std.fmt.fmt` and `std.io.log` whose last two parameters are `القالب` and `الوسائط`) whose format
string is a literal.

For such a call, lowering adds a compile-time obligation before the call: a call of std.fmt's
checker `format_compile_time_check` with the literal and, when the
arguments are a vector literal, the constructor of each element
(`msir/lowering/expressions.sn`, `أصدر_تحقق_تنسيق_خفض`). The driver evaluates it with the
interpreter like any compile-time call and removes it (`driver/compile_time.sn`). The checker
formats the string into a scratch formatter with stand-in arguments of those kinds, through the
same parser as at run time, so the compile-time and run-time checks cannot disagree. The checks of
one compilation share one interpreter context, so the cost per call is the formatting of one
string.

| The call | Checked at compile time |
|---|---|
| `format("{} {:x}", [fmt_arg.text(&t), fmt_arg.int(1)])` | everything: the syntax, the argument count, positional indices, that every argument is used, a type that fits each placeholder (`x` only for integers and bytes, `e` for numbers, `?` for lists), and that width and precision arguments are integers |
| an element that is not a constructor call (`[a]` with `a: fmt_arg`) | counted, its type not checked |
| arguments bound with `let` (`format("{} {}", args)`) | the syntax only; the arguments at run time |
| a format string built at run time (`format(template, args)`) | nothing; the run-time check below applies |

A rejected format string is `SULTANC-FORMAT-STRING` / `سلسلة_تنسيق_غير_صالحة`, with a reason per
error (see [Errors](#errors)) and, for an argument error, the argument's position, in Arabic and
English. The diagnostic points at the offending byte inside the literal (the `{` of the
placeholder, or the bad character), or at the whole literal for an unused argument or a literal
with escapes:

```
error[SULTANC-FORMAT-STRING]: invalid format string — The format argument's zero-based position is 1. The placeholder's format type does not apply to the argument it formats.
  --> main.sn:11:22..11:23
   |
11 |     match format("{} {:X}", [fmt_arg.int(1), fmt_arg.text(&name)]) with
   |                      ^
```

The tests are `tests/{ar,en}/stdlib/fmt/compile_check.sn` and `reject_format_*.sn`.

## Errors

A format string that is not a literal is parsed and checked against the arguments at run time.
Each error is a
`fmt_error` value. On an error the formatter is left as it was and nothing is written or printed.

| Variant | Payload |
|---|---|
| `unclosed_brace` | offset of the `{` |
| `unmatched_close_brace` | offset |
| `invalid_spec` | offset |
| `unknown_type` | offset |
| `named_argument` | offset |
| `missing_argument` | argument index |
| `unused_argument` | argument index |
| `mismatched_type` | argument index |
| `invalid_count` | argument index |
| `write_failed` | the writer's status |
| `custom_failed` | none |

At compile time the first nine are the reasons of `SULTANC-FORMAT-STRING`:
`format.unclosed-brace`, `format.unmatched-close-brace`, `format.invalid-spec`,
`format.unknown-type`, `format.named-argument`, `format.missing-argument`,
`format.unused-argument`, `format.mismatched-type` and `format.invalid-count`.

`std.error` converts the error with `error_from_fmt_error`; see
[Errors](errors.md).

## Spec tests

The spec tests, `tests/{en,ar}/stdlib/fmt/spec_reference.sn`, check every element of the format-spec
mini-language against reference text for the same format strings and arguments. They are generated
by `tools/generate/generate_fmt_tests.py`.

## Performance

Each call uses one output buffer (or the reused formatter's), reserved once for the literal text
and the arguments. Integers, text, chars and booleans are written into it directly. Padding needs
the width of the content, which for non-ASCII text with a width or precision is measured with the
grapheme segmentation. Floats allocate their digits. A run of 10^5 mixed formatting rounds leaves
the allocator's live bytes unchanged (`tests/{en,ar}/stdlib/fmt/rss_flat.sn`).

## Limitations

- Named arguments (`{name}`) are not supported.
- Custom closures capture plain values only.
- Debug-only argument kinds (`ints`, `texts`, `list`) reject Display placeholders.

`std.text.format` (`format_text`, `format_int`, …) also exists as a set of older helpers; new code uses
`std.fmt`.

## See also

- [Errors](errors.md): converting `fmt_error` to the common error value.
- [Standard library coverage](std-coverage.md): what `std.fmt` covers and what it lacks.
- [Arabic terminology](arabic-terminology.md#formatting-stdfmt): the Arabic names in this page.
