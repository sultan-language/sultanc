[العربية](../../ar/book/ch09-02-recoverable-errors.md) | **English**

# Recoverable Errors with `result` and `?`

Most failures are not bugs. A file is missing because the user typed its name wrong; a number does not
parse because the input had a typo. The program should notice, and decide what to do: ask again, use a
default, or report the problem and stop politely.

## `result`

As chapter 6 showed, a function that can fail returns `result T, E`: either `result.ok(value)`, a value
of type `T`, or `result.err(problem)`, an error of type `E`:

```sultan
import std.io.println;
import std.text.number;

def divide(a: int, b: int): result int, string {
    if (b == 0) {
        return result.err("division by zero");
    };
    return result.ok(a / b);
}

def main(): int {
    match divide(84, 2) with
    | result.ok(value) -> { println(int_to_text(value)); }
    | result.err(message) -> { println(message); };
    match divide(1, 0) with
    | result.ok(value) -> { println(int_to_text(value)); }
    | result.err(message) -> { println(message); };
    return 0;
}
```

```text
42
division by zero
```

The caller cannot use the number without a `match` that also answers "and if it failed?". A failure can
be handled, passed on, or deliberately ignored, but not forgotten.

The error type `E` can be anything: a `string` message, an enum listing the possible problems, or the
standard `Error` type shown below.

## Passing an error up with `?`

Often a function cannot fix an error itself, and should hand it to its caller. With `match`, that is
four lines for every call:

```sultan ignore
    let first: int;
    match divide(total, count) with
    | result.ok(value) -> { first := value; }
    | result.err(problem) -> { return result.err(problem); };
```

`?` after a call does the same in one character. If the call returned `ok`, `?` gives its value. If it
returned `err`, the function returns that error at once:

```sultan
import std.io.println;
import std.text.number;

def divide(a: int, b: int): result int, string {
    if (b == 0) {
        return result.err("division by zero");
    };
    return result.ok(a / b);
}

def average_of_two(total: int, count: int, extra: int): result int, string {
    let first: int = divide(total, count)?;
    let second: int = divide(first, extra)?;
    return result.ok(second);
}

def main(): int {
    match average_of_two(100, 5, 2) with
    | result.ok(v) -> { println(int_to_text(v)); }
    | result.err(e) -> { println(e); };
    match average_of_two(100, 0, 2) with
    | result.ok(v) -> { println(int_to_text(v)); }
    | result.err(e) -> { println(e); };
    return 0;
}
```

```text
10
division by zero
```

`?` is still visible in the code: reading `average_of_two`, you can see exactly which calls may end the
function early.

Two rules apply to `?`. It can be used only in a function that itself returns a `result`, since it may
return one. And the error types must match: `?` on a `result int, string` works in a function whose
error type is also `string`. Here is `?` in a function that returns `int`:

```sultan fail
def divide(a: int, b: int): result int, string {
    if (b == 0) {
        return result.err("division by zero");
    };
    return result.ok(a / b);
}

def main(): int {
    let half: int = divide(10, 2)?;
    return half;
}
```

```text
error[SULTANC-TYPE-RETURN-MISMATCH]: return type does not match — The return type was expected to be `int` but is `result int, string`. The returned expression type does not match the function return type.
  --> main.sn:9:34..9:35
   |
 9 |     let half: int = divide(10, 2)?;
   |                                  ^
   |
```

The message points at the `?`: on an error, it would return a `result int, string` from `main`, which
returns an `int`. Handle the error with a `match` there, or make the function return a `result`.

`?` works on an `option` too, in a function that returns an `option`: it gives the value, or returns
`none` at once.

## The standard error type

A program built from many libraries needs one error type that every part understands. The standard
library provides it: `Error`, in `std.error.failure`. An `Error` has a message, and can also carry an
identity (a domain and a kind, for code that must decide what to do), a cause, and notes.

```sultan
import std.io.println;
import std.text.number;
import std.error.failure;

def parse_age(text: &string): result int, Error {
    match parse_int(text) with
    | result.ok(n) -> {
        if (n < 0) {
            return result.err(error_new("an age cannot be negative"));
        };
        return result.ok(n);
    }
    | result.err(_) -> {
        return result.err(error_new("an age must be a number"));
    };
}

def main(): int {
    match parse_age(&"31") with
    | result.ok(age) -> { println(int_to_text(age)); }
    | result.err(problem) -> { println(error_describe(&problem)); };
    match parse_age(&"thirty") with
    | result.ok(age) -> { println(int_to_text(age)); }
    | result.err(problem) -> { println(error_describe(&problem)); };
    return 0;
}
```

```text
31
an age must be a number
```

`error_new` makes an error from a message, and `error_describe` gives its full description, causes
included. [Errors](../reference/errors.md) in the reference covers identities, causes, notes and
messages in several languages.

## `main` can return a `result`

`main` may return `result int, Error` instead of `int`. Then `?` works in `main` too. When `main` returns
an error, the program prints it on standard error and exits with status 1:

```sultan exit=1
import std.io.println;
import std.text.number;
import std.error.failure;

def parse_age(text: &string): result int, Error {
    match parse_int(text) with
    | result.ok(n) -> { return result.ok(n); }
    | result.err(_) -> { return result.err(error_new("an age must be a number")); };
}

def main(): result int, Error {
    let first: int = parse_age(&"31")?;
    println(int_to_text(first));
    let second: int = parse_age(&"thirty")?;
    println(int_to_text(second));
    return result.ok(0);
}
```

Standard output has the first age:

```text
31
```

and standard error the error:

```text stderr
error[SULTANC-RUNTIME-PROGRAM-ERROR]: the program ended with an error
an age must be a number
```

The number in `result.ok(0)` is the exit status on success. The error type of `main` must be `Error`:
a `main` that returns `result int, string` is refused, with the message marking `string` and a fix that
writes `Error` in its place.
