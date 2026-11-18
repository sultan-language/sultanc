[العربية](../../ar/book/ch15-03-shared-state.md) | **English**

# Shared State

Sometimes several threads must work on the same value. Two standard types make that safe:

- `arc<T>` lets several owners share one value. `arc_clone` makes another *owner* of the same value, not
  a copy of it, and the value is freed when the last owner is gone. Its name is short for *atomically
  reference-counted*: it counts its owners, safely across threads.
- `mutex<T>` lets only one thread at a time reach the value inside. `with_lock(&m, f)` waits for its
  turn, then calls `f` with a mutable reference to the value. The name is short for *mutual exclusion*.

Together, an `arc<mutex<int>>` is a counter that many threads can add to:

```sultan
import std.io.println;
import std.text.number;
import std.thread.thread;
import std.sync.arc;
import std.sync.mutex;

def add_one(v: &mutable int): int {
    *v := *v + 1;
    return *v;
}

def count_to_1000(counter: arc<mutex<int>>): void {
    for (let i: mutable int = 0; i < 1000; i := i + 1) {
        let now: int = with_lock(arc_get(&counter), fn(v: &mutable int): int -> add_one(v));
    }
}

def main(): int {
    let counter: arc<mutex<int>> = arc_new(mutex_new(0));
    let handles: mutable vector join_handle = [];
    for (let t: mutable int = 0; t < 4; t := t + 1) {
        let mine: arc<mutex<int>> = arc_clone(&counter);
        match spawn(fn(): void -> count_to_1000(mine)) with
        | result.ok(h) -> { append(handles, h); }
        | result.err(_) -> { return 1; };
    }
    for (let i: mutable int = 0; i < length(handles); i := i + 1) {
        join(&mutable handles[i]);
    }
    let total: int = with_lock(arc_get(&counter), fn(v: &mutable int): int -> *v);
    println(int_to_text(total));
    return 0;
}
```

```text
4000
```

Four threads each add 1000, and the total is always exactly 4000. The mutex makes each addition finish
before the next one starts. Without it, two threads could read the same old value, both add one, and
write back the same new value: one addition would be lost.

In SultanC you cannot forget the mutex. `arc_get` gives only a read-only reference, `&T`, to the shared
value, so an `arc` alone lets threads read but not change it. To change it, the value must be inside a
`mutex`, whose `with_lock` hands a mutable reference to one thread at a time.
