[العربية](../../ar/book/ch03-03-how-functions-work.md) | **English**

# Functions

A function gives a name to a piece of work, so the work can be done many times from many places. You
have written one already: `main`, where every program starts. This section shows how to write others.

## Defining and calling a function

```sultan
import std.io.println;

def main(): int {
    println("Hello from main");
    another_function();
    return 0;
}

def another_function(): void {
    println("Hello from another function");
}
```

```text
Hello from main
Hello from another function
```

`def` starts a function definition, followed by the name, the parameters in parentheses, a colon and
the type of value the function gives back. `another_function` gives back nothing, which is written
`void`. A `void` function needs no `return` at its end.

`main` calls `another_function` by writing its name and parentheses. Functions may be defined in any
order in a file: `main` can call a function defined below it.

## Parameters

A function can take values from its caller, called *parameters*. Each parameter has a name and a type,
and the type is always written:

```sultan
import std.io.println;
import std.text.number;
import std.text.transform;

def print_measurement(value: int, unit: &string): void {
    println(concat(&int_to_text(value), unit));
}

def main(): int {
    print_measurement(5, &" cm");
    print_measurement(12, &" kg");
    return 0;
}
```

```text
5 cm
12 kg
```

The values given in the call, `5` and `&" cm"`, are the *arguments*. They are matched to the parameters
in order.

The second parameter has type `&string`: a reference to a string. The function reads the text without
taking it from the caller. [Chapter 4](ch04-02-references-and-borrowing.md) explains this in depth; for
now, a function that only reads text should take `&string`.

The compiler checks every call against the definition. A missing argument is an error:

```sultan fail
def add(a: int, b: int): int {
    return a + b;
}

def main(): int {
    return add(1);
}
```

```text
error[SULTANC-CALL-ARGUMENT-COUNT-MISMATCH]: call argument count does not match — The function expects 2 arguments, but the call passes 1 argument. The call supplies a different number of arguments than the function accepts.
  --> main.sn:6:12..6:18
   |
 6 |     return add(1);
   |            ^^^^^^
   |
```

## Returning a value

A function that gives back a value names its type after the colon, and gives the value with `return`:

```sultan
import std.io.println;
import std.text.number;

def square(x: int): int {
    return x * x;
}

def main(): int {
    println(int_to_text(square(7)));
    println(int_to_text(square(3) + square(4)));
    return 0;
}
```

```text
49
25
```

A call is replaced by the value the function returns, so it can be used anywhere a value can, including
inside another call or another expression.

`return` ends the function immediately, so it may appear anywhere in the body. Checking special cases
first and returning early keeps the rest of the function simple:

```sultan
import std.io.println;

def describe(age: int): string {
    if (age < 0) {
        return "invalid";
    };
    if (age < 18) {
        return "minor";
    };
    return "adult";
}

def main(): int {
    println(describe(-1));
    println(describe(12));
    println(describe(40));
    return 0;
}
```

```text
invalid
minor
adult
```

Every path through a function that returns a value must reach a `return`. If one can end without it,
the program does not compile:

```sultan fail
def sign(n: int): int {
    if (n > 0) {
        return 1;
    };
}

def main(): int {
    return sign(2);
}
```

```text
error[SULTANC-FLOW-MISSING-RETURN]: not all execution paths return a value — A path through the function ends without returning a value.
  --> main.sn:1:1..5:2
   |
 1 | def sign(n: int): int {
   | ^
   |
```

For `n` of 0 or less, `sign` would end with no value to give back. The fix is to say what it returns on
that path too, for example `return 0;` before the final `}`.

### Statements, not expressions

The body of a function is a list of *statements*, each ending in `;`: a `let`, an assignment, a call,
an `if`, a `match`, a loop, a `return`. A value on its own is not a statement, so the last value in a
body is not returned by itself; `return` is always written. Likewise `if` and `match` choose which
statements run; they do not produce a value. This makes every place where a function gives back a
value easy to find: search for `return`.

## Parameters are immutable

A parameter is a variable like any other, and immutable unless its type says `mutable`. Changing a
`mutable` parameter changes the function's own copy only; the caller's value stays as it was:

```sultan
import std.io.println;
import std.text.number;

def countdown(n: mutable int): void {
    while (n > 0) {
        println(int_to_text(n));
        n := n - 1;
    }
}

def main(): int {
    let start: int = 3;
    countdown(start);
    println(int_to_text(start));
    return 0;
}
```

```text
3
2
1
3
```

## Changing the caller's variable

To let a function change a variable that belongs to its caller, pass a *mutable reference*. The
parameter's type is `&mutable T`, the call writes `&mutable name`, and the function reads and writes
the variable through `*`:

```sultan
import std.io.println;
import std.text.number;

def add_bonus(score: &mutable int, bonus: int): void {
    *score := *score + bonus;
}

def main(): int {
    let score: mutable int = 10;
    add_bonus(&mutable score, 5);
    println(int_to_text(score));
    return 0;
}
```

```text
15
```

The `&mutable` in the call is required. Reading `main`, you can see that `add_bonus` may change `score`
without looking at `add_bonus` itself.

## Returning two values

A function returns one value, but that value can be a tuple. Build the tuple in a variable, then return
the variable:

```sultan
import std.io.println;
import std.text.number;

def divide(a: int, b: int): (int, int) {
    let answer: (int, int) = (a / b, a % b);
    return answer;
}

def main(): int {
    match divide(17, 5) with
    | (quotient, remainder) -> {
        println(int_to_text(quotient));
        println(int_to_text(remainder));
    };
    return 0;
}
```

```text
3
2
```

## A function that calls itself

A function may call itself. This is *recursion*, and it suits problems defined in terms of a smaller
version of themselves:

```sultan
import std.io.println;
import std.text.number;

def factorial(n: int): int {
    if (n <= 1) {
        return 1;
    };
    return n * factorial(n - 1);
}

def main(): int {
    println(int_to_text(factorial(10)));
    return 0;
}
```

```text
3628800
```

Every recursive function needs a case that does not call itself, here `n <= 1`. Without it, the
function would call itself until the program runs out of memory for calls.

## Summary

- `def name(param: type, …): return_type { … }` defines a function; every type is written.
- A function that gives back nothing returns `void`.
- `return` gives back a value and ends the function. Every path of a function that returns a value must
  reach one.
- Parameters are immutable unless `mutable`; changing one does not change the caller's value.
- `&mutable T` lets a function change its caller's variable, through `*`.
- A tuple returns two or more values at once.
