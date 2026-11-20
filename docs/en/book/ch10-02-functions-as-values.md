[العربية](../../ar/book/ch10-02-functions-as-values.md) | **English**

# Functions as Values

A function is a value. It can be stored in a variable, passed to another function, and returned from
one.

## Function types

The type of a function is written `fn(parameter types): return type`. `fn(int): int` is any function
that takes an `int` and returns an `int`; `fn(&string, &string): int` takes two borrowed strings and
returns an `int`.

A named function becomes a value when you write its name without calling it:

```sultan
import std.io.println;
import std.text.number;

def square(x: int): int {
    return x * x;
}

def twice(f: fn(int): int, x: int): int {
    return f(f(x));
}

def main(): int {
    let operation: fn(int): int = square;
    println(int_to_text(operation(5)));
    println(int_to_text(twice(square, 3)));
    return 0;
}
```

```text
25
81
```

`twice` does not know what `f` does. It knows only that `f` takes an `int` and returns one, and that is
enough to call it twice.

## Passing the step that differs

The most common use of a function parameter is to let a general function do the general work, and the
caller supply the one step that differs. `sort_by`, from `std.collections.sort`, sorts a vector of any
type, in any order, with a comparison you give it. The comparison returns a negative number when its
first argument should come first, zero when they are equal, and a positive number otherwise:

```sultan
import std.io.println;
import std.text.transform;
import std.collections.sort;

def by_length(a: &string, b: &string): int {
    return length(*a) - length(*b);
}

def main(): int {
    let words: mutable vector string = ["banana", "fig", "apple"];
    sort_by(&mutable words, by_length);
    println(join(&words, &", "));
    return 0;
}
```

```text
fig, apple, banana
```

## Generics and function values together

A generic function cannot compare two values of type `T` itself, because `T` may be a type with no
order. So it asks the caller for the comparison:

```sultan
import std.io.println;
import std.text.number;

type City = { name: string, population: int };

def largest<T>(values: &vector T, greater: fn(&T, &T): bool): option T {
    if (length(*values) == 0) {
        return option.none();
    };
    let best: mutable int = 0;
    for (let i: mutable int = 1; i < length(*values); i := i + 1) {
        if (greater(&(*values)[i], &(*values)[best])) {
            best := i;
        };
    }
    return option.some(clone(&(*values)[best]));
}

def bigger_number(a: &int, b: &int): bool {
    return *a > *b;
}

def bigger_city(a: &City, b: &City): bool {
    return (*a).population > (*b).population;
}

def main(): int {
    let numbers: vector int = [4, 19, 7];
    match largest(&numbers, bigger_number) with
    | option.some(n) -> { println(int_to_text(n)); }
    | option.none -> { println("empty"); };
    let cities: vector City = [
        { name: "Jeddah", population: 4700000 },
        { name: "Riyadh", population: 7700000 }
    ];
    match largest(&cities, bigger_city) with
    | option.some(city) -> { println(clone(&city.name)); }
    | option.none -> { println("empty"); };
    return 0;
}
```

```text
19
Riyadh
```

The loop is written once. The only part that depends on the type is the comparison, and each caller
gives its own. The standard library works the same way: `sort_by`, `binary_search_by`, `new_btree_map`
and `new_set` all take functions for the part that depends on the type.
