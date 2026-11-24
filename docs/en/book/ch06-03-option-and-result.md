[العربية](../../ar/book/ch06-03-option-and-result.md) | **English**

# `option` and `result`

Two enums are built into the language, because almost every program needs them. You have used both since
chapter 2.

## `option`: a value that may be absent

Many questions have no answer in some cases: the first even number in a list that has none, the user
with a name nobody has. Many languages answer with a special value, `null`, that can stand in for any
value. The trouble with `null` is that any value might secretly be it, and forgetting to check for it is
one of the most common bugs there is.

SultanC has no `null`. A value that may be absent has the type `option T`, an enum with two cases:

- `option.some(value)`: the value is there;
- `option.none`: it is not.

```sultan
import std.io.println;
import std.text.number;

def first_even(values: &vector int): option int {
    for (let i: mutable int = 0; i < length(*values); i := i + 1) {
        if ((*values)[i] % 2 == 0) {
            return option.some((*values)[i]);
        };
    }
    return option.none();
}

def report(values: &vector int): void {
    match first_even(values) with
    | option.some(n) -> { println(int_to_text(n)); }
    | option.none -> { println("no even number"); };
}

def main(): int {
    let a: vector int = [3, 5, 8, 9];
    let b: vector int = [1, 7];
    report(&a);
    report(&b);
    return 0;
}
```

```text
8
no even number
```

The return type `option int` tells every caller that there may be no answer. And an `option int` is not
an `int`: the number cannot be used without first taking it out:

```sultan fail
def main(): int {
    let maybe: option int = option.some(5);
    let n: int = maybe + 1;
    return n;
}
```

```text
error[SULTANC-TYPE-MISMATCH]: expected `int`, found `option int` — The value does not match the declared type of its destination.
  --> main.sn:3:18..3:23
   |
 3 |     let n: int = maybe + 1;
   |                  ^^^^^
   |
```

The only way to the number is a `match`, and the `match` must also handle `option.none`. So "forgot to
check for nothing" cannot happen: the check is required before the value can be reached.

## `result`: an answer, or the reason there is none

`option` says *whether* there is a value. When a missing value has a reason worth reporting, use
`result`. Its type names two types, the value's and the error's: `result T, E`. It has two cases:

- `result.ok(value)`: it worked, and here is the value;
- `result.err(error)`: it failed, and here is why.

```sultan
import std.io.println;
import std.text.number;

def divide(a: int, b: int): result int, string {
    if (b == 0) {
        return result.err("division by zero");
    };
    return result.ok(a / b);
}

def show(outcome: result int, string): void {
    match outcome with
    | result.ok(n) -> { println(int_to_text(n)); }
    | result.err(message) -> { println(message); };
}

def main(): int {
    show(divide(10, 2));
    show(divide(1, 0));
    return 0;
}
```

```text
5
division by zero
```

`read_line`, `parse_uint` and `system_time_ns` from chapter 2 all return a `result`, because each can
fail for a reason outside the program's control: the input is closed, the text is not a number, the
clock cannot be read. The standard library uses `option` where absence is normal, and `result` where it
is a failure.

[Chapter 9](ch09-00-error-handling.md) shows how to pass a `result` on to the caller with `?` instead of
matching it everywhere, and how to describe errors with the standard `Error` type.

## Summary

- `type Name = | A | B of T | …;` declares an enum; a case may carry one value with `of`.
- `Name.A()` makes a value; `Name.B(x)` makes one that carries `x`.
- `match` chooses the case and names what it carries; patterns nest, and take tuples and records apart.
- Every case must be handled; prefer listing cases to `_`.
- `option T` is a value or nothing; `result T, E` is a value or an error. SultanC has no `null`.
