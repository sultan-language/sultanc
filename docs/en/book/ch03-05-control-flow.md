[العربية](../../ar/book/ch03-05-control-flow.md) | **English**

# Control Flow

A program rarely runs straight from top to bottom. It decides between paths, and repeats work while
something is true. This section covers `if` and `match`, which decide, and `while` and `for`, which
repeat.

## `if`

`if` runs a block when its condition is true:

```sultan
import std.io.println;

def main(): int {
    let temperature: int = 35;
    if (temperature > 30) {
        println("hot");
    };
    return 0;
}
```

```text
hot
```

The condition is always in parentheses, and the block always in braces, even when it has one line. The
whole `if` is a statement, so it ends with `;` after its last `}`.

The condition must be a `bool`. A number is not a condition:

```sultan fail
def main(): int {
    let count: int = 3;
    if (count) {
        return 1;
    };
    return 0;
}
```

```text
error[SULTANC-TYPE-MISMATCH]: expected `bool`, found `int` — A condition must be a boolean value.
  --> main.sn:3:9..3:14
   |
 3 |     if (count) {
   |         ^^^^^
   |
```

Some languages treat any non-zero number as true. SultanC asks you to say what you mean:
`if (count != 0)`. The intent is then written in the code, and a number used by mistake is caught.

### `else` and `else if`

`else` gives a block to run when the condition is false, and `else if` adds another condition. The
conditions are tested in order, the first one that is true wins, and the rest are not tested:

```sultan
import std.io.println;

def grade(score: int): string {
    if (score >= 90) {
        return "excellent";
    } else if (score >= 75) {
        return "good";
    } else if (score >= 50) {
        return "pass";
    } else {
        return "fail";
    };
}

def main(): int {
    println(grade(95));
    println(grade(80));
    println(grade(60));
    println(grade(20));
    return 0;
}
```

```text
excellent
good
pass
fail
```

A score of 95 is also above 75 and above 50, but only the first matching branch runs.

## `match`

When one value decides between many cases, `match` is clearer than a chain of `else if`. It compares the
value with each *pattern* in turn and runs the branch of the first one that matches:

```sultan
import std.io.println;

def day_name(day: int): string {
    match day with
    | 5 -> { return "Friday"; }
    | 6 -> { return "Saturday"; }
    | 1 | 2 | 3 | 4 | 7 -> { return "weekday"; }
    | _ -> { return "not a day"; };
}

def main(): int {
    println(day_name(5));
    println(day_name(2));
    println(day_name(9));
    return 0;
}
```

```text
Friday
weekday
not a day
```

The parts of a `match`:

- `match value with` starts it.
- Each branch is `| pattern -> { … }`.
- Inside a pattern, `|` means "or": `1 | 2 | 3` matches any of the three.
- `_` matches anything. As the last branch it catches every value the branches above did not.
- A `;` after the last branch ends the whole `match`.

A *range* pattern matches every value between two limits, both included:

```sultan
import std.io.println;

def describe(n: int): string {
    match n with
    | 0 -> { return "zero"; }
    | range 1 to 9 -> { return "one digit"; }
    | range 10 to 99 -> { return "two digits"; }
    | _ -> { return "bigger, or negative"; };
}

def main(): int {
    println(describe(7));
    println(describe(42));
    println(describe(-3));
    return 0;
}
```

```text
one digit
two digits
bigger, or negative
```

`match` works on characters and on text as well:

```sultan
import std.io.println;

def answer(word: string): string {
    match word with
    | "yes" | "ok" -> { return "agreed"; }
    | "no" -> { return "refused"; }
    | _ -> { return "unclear"; };
}

def kind(letter: char): string {
    match letter with
    | range 'a' to 'z' -> { return "small Latin letter"; }
    | range '0' to '9' -> { return "digit"; }
    | _ -> { return "something else"; };
}

def main(): int {
    println(answer("ok"));
    println(answer("maybe"));
    println(kind('q'));
    println(kind('7'));
    return 0;
}
```

```text
agreed
unclear
small Latin letter
digit
```

### Every value must be handled

A `match` must have a branch for every possible value. Without `_`, a `match` on an `int` leaves most
numbers without a branch, and the compiler refuses it:

```sultan fail
def size(n: int): int {
    match n with
    | 0 -> { return 0; }
    | 1 -> { return 1; };
}

def main(): int {
    return size(1);
}
```

```text
error[SULTANC-PATTERN-NON-EXHAUSTIVE]: non-exhaustive pattern
  --> main.sn:2:5..4:26
   |
 2 |     match n with
   |     ^
   |
```

This check is what made chapter 2's `match` on `read_line()` require a branch for the error. It is most
useful with enums, in [chapter 6](ch06-02-match.md): when you add a new case to an enum, the compiler
points at every `match` that does not handle it yet.

## Loops

### `while`

`while` repeats its block as long as its condition is true. The condition is tested before each round,
so if it is false at the start, the block never runs:

```sultan
import std.io.println;
import std.text.number;

def main(): int {
    let total: mutable int = 0;
    let n: mutable int = 1;
    while (n <= 100) {
        total := total + n;
        n := n + 1;
    }
    println(int_to_text(total));
    return 0;
}
```

```text
5050
```

Something inside the loop must eventually make the condition false, here `n := n + 1`. Note that the
block of a loop, unlike the block of an `if`, is not followed by `;`.

### `for`

A loop that counts has three parts: a variable that starts somewhere, a condition, and a step. `for`
writes them on one line, separated by `;`:

```sultan
import std.io.println;
import std.text.number;

def main(): int {
    for (let i: mutable int = 1; i <= 5; i := i + 1) {
        println(int_to_text(i * i));
    }
    return 0;
}
```

```text
1
4
9
16
25
```

The first part runs once, before the loop. The condition is tested before each round. The step runs
after each round. The counter `i` exists only inside the loop.

To go through a vector, count its positions from 0 to its length:

```sultan
import std.io.println;
import std.text.number;

def main(): int {
    let prices: vector int = [12, 45, 30];
    let total: mutable int = 0;
    for (let i: mutable int = 0; i < length(prices); i := i + 1) {
        total := total + prices[i];
    }
    println(int_to_text(total));
    return 0;
}
```

```text
87
```

### `break` and `continue`

`break` leaves the loop at once. `continue` skips the rest of the current round and starts the next
one; in a `for`, the step still runs:

```sultan
import std.io.println;
import std.text.number;

def main(): int {
    for (let i: mutable int = 1; i <= 20; i := i + 1) {
        if (i % 2 == 0) {
            continue;
        };
        if (i > 9) {
            break;
        };
        println(int_to_text(i));
    }
    return 0;
}
```

```text
1
3
5
7
9
```

In nested loops, `break` and `continue` act on the innermost loop.

A loop that runs until something happens inside it is written `while (true)`, with a `break`, as in
chapter 2's game. This one counts the steps of the Collatz sequence from 27 down to 1:

```sultan
import std.io.println;
import std.text.number;

def main(): int {
    let n: mutable int = 27;
    let steps: mutable int = 0;
    while (true) {
        if (n == 1) {
            break;
        };
        if (n % 2 == 0) {
            n := n / 2;
        } else {
            n := 3 * n + 1;
        };
        steps := steps + 1;
    }
    println(int_to_text(steps));
    return 0;
}
```

```text
111
```

## Summary

You have now seen the basic parts of a program: variables, types, functions, comments, and control
flow. To practise, try writing programs that:

- convert temperatures between Celsius and Fahrenheit;
- print the first 20 Fibonacci numbers;
- print the multiplication table from 1 × 1 to 9 × 9.

The next chapter is about a part of SultanC that most languages do not have: ownership.
