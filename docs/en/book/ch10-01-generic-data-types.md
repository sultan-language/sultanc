[العربية](../../ar/book/ch10-01-generic-data-types.md) | **English**

# Generic Data Types

## Generic functions

A type parameter is declared in angle brackets after a function's name, and then used like any type:

```sultan
import std.io.println;
import std.text.number;

def first_or<T>(values: &vector T, fallback: T): T {
    if (length(*values) == 0) {
        return fallback;
    };
    return clone(&(*values)[0]);
}

def main(): int {
    let numbers: vector int = [7, 8, 9];
    let empty: vector int = [];
    let names: vector string = ["Huda", "Zaid"];
    println(int_to_text(first_or(&numbers, 0)));
    println(int_to_text(first_or(&empty, -1)));
    println(first_or(&names, "nobody"));
    return 0;
}
```

```text
7
-1
Huda
```

Read `def first_or<T>(values: &vector T, fallback: T): T` as: for any type `T`, `first_or` takes a
borrowed vector of `T` and a fallback `T`, and returns a `T`.

The caller does not write which type `T` is: the compiler works it out from the arguments. With
`&numbers`, `T` is `int`; with `&names`, it is `string`.

The `clone` is there because `T` may be an owning type such as `string`, and a value cannot be moved out
of a borrowed vector. Generic code must be correct for every type it may be used with.

A function may have several type parameters, separated by commas: `def swap<A, B>(pair: (A, B)): (B, A)`.

## Generic records

Types take type parameters too. Here is a stack that works for any element type:

```sultan
import std.io.println;
import std.text.number;

type Stack<T> = { items: vector T };

def new_stack<T>(): Stack<T> {
    let empty: vector T = [];
    let stack: Stack<T> = { items: empty };
    return stack;
}

def push<T>(stack: &mutable Stack<T>, item: T): void {
    append(stack.items, item);
}

def size<T>(stack: &Stack<T>): int {
    return length(stack.items);
}

def top<T>(stack: &Stack<T>): option T {
    let count: int = size(stack);
    if (count == 0) {
        return option.none();
    };
    return option.some(clone(&stack.items[count - 1]));
}

def main(): int {
    let numbers: mutable Stack<int> = new_stack();
    push(&mutable numbers, 10);
    push(&mutable numbers, 20);
    println(int_to_text(size(&numbers)));
    match top(&numbers) with
    | option.some(value) -> { println(int_to_text(value)); }
    | option.none -> { println("empty"); };
    let words: mutable Stack<string> = new_stack();
    push(&mutable words, "first");
    push(&mutable words, "second");
    match top(&words) with
    | option.some(word) -> { println(word); }
    | option.none -> { println("empty"); };
    return 0;
}
```

```text
2
20
second
```

`Stack<int>` and `Stack<string>` are two different types made from one definition. The built-in types
you have used are generic in the same way: `vector T`, `option T` and `result T, E`.

## Generic enums

An enum can take type parameters as well. `option` is built in, but the same shape can be written
directly:

```sultan
import std.io.println;
import std.text.number;

type Maybe<T> = | Nothing | Just of T;

def value_or<T>(value: Maybe<T>, fallback: T): T {
    match value with
    | Maybe.Nothing -> { return fallback; }
    | Maybe.Just(item) -> { return item; };
}

def main(): int {
    let present: Maybe<int> = Maybe.Just(42);
    let missing: Maybe<int> = Maybe.Nothing();
    println(int_to_text(value_or(present, 0)));
    println(int_to_text(value_or(missing, -1)));
    return 0;
}
```

```text
42
-1
```

## Generic code costs nothing at run time

The compiler checks generic code when it is used with a type, and produces a separate copy of the
function for each type it is used with: one `first_or` for `int`, another for `string`. So generic code
runs exactly as fast as code written for one type. The price is a little more compiled code, one copy
per type.

Start with concrete types. When you find yourself writing the same function a second time for another
type, make it generic.
