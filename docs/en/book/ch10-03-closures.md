[العربية](../../ar/book/ch10-03-closures.md) | **English**

# Closures

Writing a named function for every small comparison gets tedious. A function can also be written right
where it is needed, without a name:

```sultan ignore
fn(x: int): int -> x * 2
```

This is an *anonymous function*: `fn`, the parameters with their types, the return type, then `->` and
one expression whose value the function returns.

```sultan
import std.io.println;
import std.text.transform;
import std.collections.sort;

def main(): int {
    let words: mutable vector string = ["banana", "fig", "apple"];
    sort_by(&mutable words, fn(a: &string, b: &string): int -> length(*a) - length(*b));
    println(join(&words, &", "));
    return 0;
}
```

```text
fig, apple, banana
```

The body of an anonymous function is one expression. When the work needs statements, a loop or a
`match`, write a named function and pass its name.

## Capturing values

An anonymous function can use the variables around the place where it is written. It *captures* them,
and keeps them even after the function that created it has returned:

```sultan
import std.io.println;
import std.text.number;

def make_adder(amount: int): fn(int): int {
    return fn(value: int): int -> value + amount;
}

def main(): int {
    let add_ten: fn(int): int = make_adder(10);
    let add_two: fn(int): int = make_adder(2);
    println(int_to_text(add_ten(5)));
    println(int_to_text(add_two(add_ten(30))));
    return 0;
}
```

```text
15
42
```

Each call to `make_adder` produces a new function with its own `amount`. A function that carries
captured values with it is called a *closure*.

A closure can capture owning values too. The value moves into the closure, which then owns it, and frees
it when the closure itself goes away:

```sultan
import std.io.println;
import std.text.transform;

def make_greeter(greeting: string): fn(&string): string {
    return fn(name: &string): string -> concat3(&greeting, &" ", name);
}

def main(): int {
    let hello: fn(&string): string = make_greeter("Hello");
    println(hello(&"Noor"));
    println(hello(&"Adam"));
    return 0;
}
```

```text
Hello Noor
Hello Adam
```

## A few general functions instead of many loops

With function parameters, a handful of general functions cover many loops. `keep` keeps the values that
pass a test; `fold` combines all values into one:

```sultan
import std.io.println;
import std.text.number;

def keep(values: &vector int, test: fn(int): bool): vector int {
    let output: mutable vector int = [];
    for (let i: mutable int = 0; i < length(*values); i := i + 1) {
        if (test((*values)[i])) {
            append(output, (*values)[i]);
        };
    }
    return output;
}

def fold(values: &vector int, start: int, combine: fn(int, int): int): int {
    let total: mutable int = start;
    for (let i: mutable int = 0; i < length(*values); i := i + 1) {
        total := combine(total, (*values)[i]);
    }
    return total;
}

def main(): int {
    let numbers: vector int = [1, 2, 3, 4, 5, 6];
    let evens: vector int = keep(&numbers, fn(n: int): bool -> n % 2 == 0);
    println(int_to_text(fold(&evens, 0, fn(a: int, b: int): int -> a + b)));
    println(int_to_text(fold(&numbers, 1, fn(a: int, b: int): int -> a * b)));
    return 0;
}
```

```text
12
720
```

The sum of the even numbers and the product of all numbers are two lines, with the loops written once.

## Summary

- `def name<T>(…)` and `type Name<T> = …;` declare generic functions and types; the compiler infers `T`
  at each call, and makes a copy for each type.
- `fn(A, B): R` is the type of a function; a named function's name is a value of that type.
- `fn(x: T): R -> expression` is an anonymous function with a one-expression body.
- An anonymous function captures the variables it uses, and keeps them: it is a closure.
- There are no traits: generic code takes a function for comparing or combining values of a type
  parameter.
