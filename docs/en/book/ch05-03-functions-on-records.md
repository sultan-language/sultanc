[العربية](../../ar/book/ch05-03-functions-on-records.md) | **English**

# Functions That Work on Records

In SultanC, the operations on a record are ordinary functions that take the record as a parameter. There
is no separate kind of function that belongs to a type. A function that works on a `Rectangle` is
written like any other, and called like any other: `area(&rect)`.

What you choose is how the function takes the record. The rules from chapter 4 decide:

| The function… | takes | the call writes |
| --- | --- | --- |
| only reads the record | `&Rectangle` | `area(&rect)` |
| changes the caller's record | `&mutable Rectangle` | `scale(&mutable rect, 2)` |
| keeps the record, or uses it up | `Rectangle` | `store(rect)` |
| makes a new record | nothing; it returns `Rectangle` | `let r: Rectangle = square(5);` |

## Reaching fields through a reference

When a function takes `&Rectangle`, its parameter is a reference, not a record. The record is
`*rectangle`, and its field is `(*rectangle).width`. The parentheses are needed because `.` binds more
tightly than `*`: `*rectangle.width` would mean "the value that `rectangle.width` points at".

Leaving out the `*` is a mistake the compiler catches:

```sultan fail
type Rectangle = { width: int, height: int };

def area(rectangle: &Rectangle): int {
    return rectangle.width * rectangle.height;
}

def main(): int {
    let rect: Rectangle = { width: 30, height: 50 };
    return area(&rect);
}
```

```text
error[SULTANC-TYPE-UNKNOWN-RECORD-FIELD]: unknown record field — The unknown record field is `width`. The referenced field is not declared in the record type.
  --> main.sn:4:22..4:27
   |
 4 |     return rectangle.width * rectangle.height;
   |                      ^^^^^
   |
```

The message says `width` is unknown, although `Rectangle` declares it. That is because a reference has
no fields of its own: the fields belong to the record it points at. When you see this message for a
field you know exists, check whether the value is a reference, and write `(*name).field`.

## A function that changes a record

```sultan
import std.io.println;
import std.text.number;

type Account = { owner: string, balance: int };

def deposit(account: &mutable Account, amount: int): void {
    (*account).balance := (*account).balance + amount;
}

def print_account(account: &Account): void {
    println(clone(&(*account).owner));
    println(int_to_text((*account).balance));
}

def main(): int {
    let savings: mutable Account = { owner: "Layla", balance: 100 };
    deposit(&mutable savings, 50);
    print_account(&savings);
    return 0;
}
```

```text
Layla
150
```

`deposit` changes the caller's account, so it takes `&mutable Account`, and the call says
`&mutable savings`. `print_account` only reads, so it takes `&Account`.

## Functions that make records

A function that builds and returns a record is the usual way to create values that must follow a rule.
Here the two fields of a `Temperature` must always agree, and `from_celsius` is the one place that
computes them:

```sultan
import std.io.println;
import std.text.number;

type Temperature = { celsius: int, fahrenheit: int };

def from_celsius(c: int): Temperature {
    let t: Temperature = { celsius: c, fahrenheit: c * 9 / 5 + 32 };
    return t;
}

def main(): int {
    let today: Temperature = from_celsius(30);
    println(int_to_text(today.fahrenheit));
    return 0;
}
```

```text
86
```

Functions like this are often called *constructors*. The convention for their names is `new_` followed by
the type, or a name that says where the value comes from, like `from_celsius`.

## Putting it together

Here is the rectangle program from the last section, with three functions on `Rectangle`:

```sultan
import std.io.println;
import std.text.number;

type Rectangle = { width: int, height: int };

def square(size: int): Rectangle {
    let r: Rectangle = { width: size, height: size };
    return r;
}

def area(rectangle: &Rectangle): int {
    return (*rectangle).width * (*rectangle).height;
}

def can_hold(outer: &Rectangle, inner: &Rectangle): bool {
    return (*outer).width > (*inner).width && (*outer).height > (*inner).height;
}

def scale(rectangle: &mutable Rectangle, factor: int): void {
    (*rectangle).width := (*rectangle).width * factor;
    (*rectangle).height := (*rectangle).height * factor;
}

def main(): int {
    let big: mutable Rectangle = { width: 30, height: 50 };
    let small: Rectangle = square(10);
    println(int_to_text(area(&big)));
    if (can_hold(&big, &small)) {
        println("the big one holds the small one");
    };
    scale(&mutable big, 2);
    println(int_to_text(area(&big)));
    return 0;
}
```

```text
1500
the big one holds the small one
6000
```

When a program has many functions on one type, put them in a module of their own, next to the type.
[Chapter 7](ch07-00-packages-and-modules.md) shows how.

## Summary

- Records are declared with `type Name = { field: type, … };` and built with `{ field: value, … }`.
- Every field must be given; a `mutable` record's fields change with `:=`.
- Functions on a record take `&T` to read, `&mutable T` to change, or `T` to keep it.
- Through a reference, fields are `(*r).field`.
- A function that returns a new record is the place to keep the rules of its fields.

Records group values that are all present at once. The next chapter is about values that are one of
several possibilities.
