[العربية](../../ar/book/ch15-00-concurrency.md) | **English**

# Concurrency

A computer has several processor cores, and a program that uses one leaves the others idle. *Threads*
let one program run several pieces of work at the same time.

Threads are also the classic source of the hardest bugs. Two threads change the same value at once, and
the result depends on which got there first: a *data race*. Such a bug may appear once in a thousand
runs, on one machine, under load, and vanish when you look for it.

SultanC's ownership rules extend to threads. The compiler checks what each thread may touch, and a
program that compiles cannot have a data race. This chapter shows how.

| Module | What it has |
| --- | --- |
| `std.thread.thread` | `spawn`, `join`, `spawn_value`, `join_value` |
| `std.sync.arc` | `arc<T>`: one value shared by several owners |
| `std.sync.mutex` | `mutex<T>`: a value that one thread at a time may change |
| `std.sync.channel` | channels, which send values from one thread to another |
| `std.sync.atomic`, `rwlock`, `condvar`, `once` | the other standard primitives |

- [Using Threads](ch15-01-threads.md)
- [What a Thread May Capture](ch15-02-thread-captures.md)
- [Shared State](ch15-03-shared-state.md)
- [Channels](ch15-04-channels.md)
