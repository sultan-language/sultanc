[العربية](../../ar/book/ch15-02-thread-captures.md) | **English**

# What a Thread May Capture

A thread may run longer than the function that started it. If it held a reference to that function's
variables, the reference would point at memory that no longer exists once the function returned. So a
closure sent to another thread may not capture a reference:

```sultan fail
import std.thread.thread;

def show(v: &vector int): int {
    return length(*v);
}

def start(values: &vector int): void {
    match spawn_value(fn(): int -> show(values)) with
    | result.ok(_) -> { }
    | result.err(_) -> { };
}

def main(): int {
    let v: vector int = [1];
    start(&v);
    return 0;
}
```

```text
error[SULTANC-THREAD-CAPTURE-REFERENCE]: a closure sent to another thread captures a borrowed reference — The symbol is `values`. The type is `&vector int`.
  --> main.sn:8:41..8:47
   |
 8 |     match spawn_value(fn(): int -> show(values)) with
   |                                         ^^^^^^
   |
```

`start` lends `values` to the closure, but `start` returns at once, while the thread may still be
running. The compiler refuses it before anything can go wrong.

The fix is to give the thread a value of its own: move the value in, as `sum_in_thread` did in the last
section, or `clone` it when the caller still needs it.

The same check covers values that would let two threads change one thing at the same time: a value can
be shared between threads only through the types made for it, which the next section shows.
[Concurrency](../reference/concurrency.md) in the reference gives the full rules.
