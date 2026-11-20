[العربية](../../ar/book/ch15-04-channels.md) | **English**

# Channels

Instead of sharing a value, threads can send each other values. A *channel* has two ends: values go in at
the sender and come out at the receiver, in order. Each value moves from one thread to the other, so at
any time exactly one thread owns it.

`channel_receiver<T>()`, from `std.sync.channel`, makes a channel and returns its receiving end;
`sender_for(&receiver)` makes a sender for it. `send(&sender, value)` puts a value in, and
`recv(&receiver)` waits for the next one:

```sultan
import std.io.println;
import std.text.number;
import std.thread.thread;
import std.sync.channel;

def produce(out: &sender<int>): void {
    for (let i: mutable int = 1; i <= 5; i := i + 1) {
        match send(out, i * i) with
        | result.ok(_) -> { }
        | result.err(_) -> { };
    }
}

def main(): int {
    let inbox: receiver<int> = channel_receiver<int>();
    let out: sender<int> = sender_for(&inbox);
    let worker: mutable join_handle;
    match spawn(fn(): void -> produce(&out)) with
    | result.ok(h) -> { worker := h; }
    | result.err(_) -> { return 1; };
    let done: mutable bool = false;
    while (!done) {
        match recv(&inbox) with
        | result.ok(value) -> { println(int_to_text(value)); }
        | result.err(_) -> { done := true; };
    }
    join(&mutable worker);
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

The worker thread owns the sender, captured by its closure. When the thread finishes, the closure and
the sender are freed, the channel has no senders left, and `recv` returns an error instead of waiting
forever. That is how `main` knows that nothing more will come.

`sender_clone` makes another sender for the same channel, so that several threads can send to one
receiver. The `examples/en/sync` folder of the repository has a program for every primitive in
`std.sync`: atomics, read-write locks, condition variables, bounded queues.

## Summary

- `spawn_value(f)` starts a thread; `join_value(&mutable h)` waits for it and returns its result.
- A closure sent to a thread owns what it captures; it may not capture a reference.
- `arc<T>` shares one value between owners; `mutex<T>` gives one thread at a time access to it, through
  `with_lock`.
- A channel moves values from thread to thread; `recv` fails once every sender is gone.
- The compiler checks these rules: a program that compiles has no data races.
