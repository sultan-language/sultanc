[العربية](../../ar/book/ch15-01-threads.md) | **English**

# Using Threads

## Starting a thread

`spawn_value(f)` runs the function `f` in a new thread, and returns a *handle* to it. `join_value(&mutable
handle)` waits for the thread to finish, and gives back what `f` returned:

```sultan
import std.io.println;
import std.text.number;
import std.thread.thread;

def main(): int {
    match spawn_value(fn(): int -> 20 * 2) with
    | result.err(_) -> { return 1; }
    | result.ok(handle) -> {
        let thread: mutable value_join_handle<int> = handle;
        match join_value(&mutable thread) with
        | option.some(value) -> { println(int_to_text(value)); }
        | option.none -> { return 2; };
    };
    return 0;
}
```

```text
40
```

Starting a thread can fail, because the system may refuse to create another one, so `spawn_value` returns
a `result`. `join_value` returns an `option`, which is `none` if the thread stopped without a value. For
a thread that returns nothing, `spawn` and `join` work the same way.

The function given to `spawn_value` here is an anonymous function with no parameters, from chapter 10.

## Splitting the work

A thread gets data to work on by capturing it. A captured owning value moves into the thread, so the
thread is its only owner, and nothing else can touch it while the thread runs:

```sultan
import std.io.println;
import std.text.number;
import std.thread.thread;

def sum_range(values: &vector int, start: int, stop: int): int {
    let total: mutable int = 0;
    for (let i: mutable int = start; i < stop; i := i + 1) {
        total := total + (*values)[i];
    }
    return total;
}

def sum_in_thread(values: vector int): result value_join_handle<int>, thread_error {
    return spawn_value(fn(): int -> sum_range(&values, 0, length(values)));
}

def main(): int {
    let first: mutable vector int = [];
    let second: mutable vector int = [];
    for (let i: mutable int = 1; i <= 1000; i := i + 1) {
        if (i <= 500) {
            append(first, i);
        } else {
            append(second, i);
        };
    }
    let total: mutable int = 0;
    match sum_in_thread(first) with
    | result.err(_) -> { return 1; }
    | result.ok(h1) -> {
        match sum_in_thread(second) with
        | result.err(_) -> { return 1; }
        | result.ok(h2) -> {
            let a: mutable value_join_handle<int> = h1;
            let b: mutable value_join_handle<int> = h2;
            match join_value(&mutable a) with
            | option.some(v) -> { total := total + v; }
            | option.none -> { return 2; };
            match join_value(&mutable b) with
            | option.some(v) -> { total := total + v; }
            | option.none -> { return 2; };
        };
    };
    println(int_to_text(total));
    return 0;
}
```

```text
500500
```

Each half of the numbers is summed in its own thread, at the same time, and `main` adds the two results.
`sum_in_thread` takes its vector by value: the vector moves into the function, then into the closure,
then into the new thread.
