[العربية](../../ar/book/ch03-01-variables-and-mutability.md) | **English**

# Variables and Mutability

A variable gives a name to a value. In chapter 2 you met `let`; this section shows its full rules.

Create a new project for the examples in this chapter, or use `sultanc run main.sn` on single files.
Each listing here is a complete program.

## Declaring a variable

```sultan
import std.io.println;
import std.text.number;

def main(): int {
    let year: int = 2026;
    println(int_to_text(year));
    return 0;
}
```

```text
2026
```

Read `let year: int = 2026;` as "let `year`, of type `int`, be 2026". The parts always come in this
order: the name, a colon, the type, `=`, and the value.

The type is always written. SultanC never works out a variable's type from its value, even when it
could. A program is read far more often than it is written, and with the type next to every name you
never have to trace where a value came from to know what it is. One `let` can also name each part of
a tuple, `let (quotient, remainder): (int, int) = divide(17, 5);`, again with the type written; the
next section shows it ([Taking a tuple apart](ch03-02-data-types.md#taking-a-tuple-apart)).

`println` prints text, not numbers, so `int_to_text` (from `std.text.number`) turns the number into
text first.

## Variables are immutable by default

Once a variable has a value, the value does not change. Try to change it:

```sultan fail
import std.io.println;
import std.text.number;

def main(): int {
    let count: int = 5;
    println(int_to_text(count));
    count := 6;
    println(int_to_text(count));
    return 0;
}
```

```text
error[SULTANC-MUTABILITY-WRITE-TO-IMMUTABLE]: cannot write to an immutable value — The immutable value being written to is `count`. An assignment targets an immutable variable.
  --> main.sn:7:5..7:10
   |
 7 |     count := 6;
   |     ^^^^^ invalid use
   |
  ::: main.sn:5:5..5:24
   |
 5 |     let count: int = 5;
   |     ^^^^^^^^^^^^^^^^^^^ declared here without `mutable`
   |
  = help: declare the variable `mutable` to write to it
```

The compiler says that `count` is *immutable*: it cannot be changed. This protects you from a common
kind of bug. When one part of a program relies on a value staying the same, and another part changes
it, the first part goes wrong in a way that is hard to trace. If a variable cannot change, you never
need to look for the place that changed it.

But changing values is often exactly what you need: a counter, a running total, a position. To allow
it, write `mutable` before the type:

```sultan
import std.io.println;
import std.text.number;

def main(): int {
    let count: mutable int = 5;
    println(int_to_text(count));
    count := 6;
    println(int_to_text(count));
    return 0;
}
```

```text
5
6
```

`mutable` belongs to the type, so it is the first thing you read about a variable, and it tells the
reader of the code: this value will change, watch for it.

Notice the two different signs. `=` appears only in a declaration, where a name is created. `:=` only
in an assignment, where an existing variable gets a new value. Seeing `:=` always means a change.

There are no combined operators such as `+=`; write the whole expression:

```sultan
import std.io.println;
import std.text.number;

def main(): int {
    let total: mutable int = 0;
    total := total + 10;
    total := total * 3;
    println(int_to_text(total));
    return 0;
}
```

```text
30
```

## Declaring now, giving the value later

A variable can be declared without a value, and given one later. Chapter 2 did this with `guess`, whose
value came from a `match`. It is useful whenever the value depends on a decision:

```sultan
import std.io.println;

def main(): int {
    let temperature: int = 31;
    let advice: string;
    if (temperature > 30) {
        advice := "drink water";
    } else {
        advice := "enjoy the day";
    };
    println(advice);
    return 0;
}
```

```text
drink water
```

`advice` is still immutable: it gets one value, on whichever path the program takes. The compiler
checks two things. First, every path gives the variable a value before it is used:

```sultan fail
import std.io.println;

def main(): int {
    let temperature: int = 31;
    let advice: string;
    if (temperature > 30) {
        advice := "drink water";
    };
    println(advice);
    return 0;
}
```

```text
error[SULTANC-FLOW-USE-BEFORE-INITIALIZATION]: use before initialization — The value used before initialization is `advice`. The value is used before it is initialized.
  --> main.sn:9:13..9:19
   |
 9 |     println(advice);
   |             ^^^^^^ invalid use
   |
  ::: main.sn:5:5..5:24
   |
 5 |     let advice: string;
   |     ^^^^^^^^^^^^^^^^^^^ declared here
   |
```

If the temperature were 20, `advice` would have no value when `println` uses it. The compiler finds
this path without running the program. The message marks the use that can find `advice` empty, and
shows where `advice` was declared.

Second, an immutable variable gets its value only once: a second `:=` to `advice` on the same path is
the same immutability error as before.

## Constants

A `let` outside every function declares a *constant*:

```sultan
import std.io.println;
import std.text.number;

let seconds_per_minute: int = 60;
let seconds_per_hour: int = seconds_per_minute * 60;

def main(): int {
    println(int_to_text(seconds_per_hour * 24));
    return 0;
}
```

```text
86400
```

A constant is computed while the program compiles, not while it runs. Its value is already inside the
finished program, so using it costs nothing. If computing it would fail, for example by dividing by
zero, the program does not compile.

Constants are useful for values that many parts of a program share and that should be named once: a
limit, a size, a unit conversion. Changing the value later means changing one line.

## Scope

A variable exists from its declaration to the end of the *block* that contains it. A block is the code
between `{` and `}`: the body of a function, a branch of an `if`, the body of a loop. This region is the
variable's *scope*.

```sultan
import std.io.println;
import std.text.number;

def main(): int {
    let outer: int = 60;
    if (outer > 0) {
        let inner: int = outer + 5;
        println(int_to_text(inner));
    };
    println(int_to_text(outer));
    return 0;
}
```

```text
65
60
```

`inner` exists only inside the `if`. After its closing `}`, the name is gone:

```sultan fail
import std.io.println;
import std.text.number;

def main(): int {
    let outer: int = 60;
    if (outer > 0) {
        let inner: int = outer + 5;
    };
    println(int_to_text(inner));
    return 0;
}
```

```text
error[SULTANC-NAME-UNKNOWN]: unknown name: `inner` — The required name was not found during resolution.
  --> main.sn:9:25..9:30
   |
 9 |     println(int_to_text(inner));
   |                         ^^^^^
   |
```

In one block, a name can be declared only once:

```sultan fail
def main(): int {
    let price: int = 10;
    let price: int = 12;
    return price;
}
```

```text
error[SULTANC-NAME-DUPLICATE-DECLARATION]: duplicate declaration — The redeclared name is `price`. The same name is declared more than once in this scope.
  --> main.sn:3:5..3:25
   |
 3 |     let price: int = 12;
   |     ^^^^^^^^^^^^^^^^^^^^
   |
  ::: main.sn:2:5..2:25
   |
 2 |     let price: int = 10;
   |     ^^^^^^^^^^^^^^^^^^^^ related context
   |
  = note: the previous declaration is shown as related source
```

So a second `let price` can never silently hide the first one in the same block. If the value needs to
change, make the variable `mutable`; if it is a different value, give it a different name. An inner
block may declare a name that an outer block already uses; inside the inner block, the name means the
inner variable.

## Names

A name starts with a letter or `_`, and continues with letters, digits and `_`. The letters may be
Latin or Arabic, in the same program. Latin names are case sensitive: `total` and `Total` are two
different names.

In Arabic names, the compiler treats `أ`, `إ`, `آ` and `ا` as the same letter and ignores the tatweel
`ـ`, so `أحمد` and `احمد` are one name. Arabic writers spell these letters differently, and a program
should not break because of it.

By convention, variables, constants and functions use lowercase words joined by `_`:
`total_price`, `seconds_per_hour`.

## Summary

- `let name: type = value;` declares a variable. The type is always written.
- Variables are immutable unless their type says `mutable`; `:=` gives a mutable variable a new value.
- A variable may get its value after its declaration, once, on every path before it is used.
- A `let` outside functions is a constant, computed at compile time.
- A variable lives until the end of its block. A name is declared once per block.
