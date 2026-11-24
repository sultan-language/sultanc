[العربية](../../ar/book/ch04-01-what-is-ownership.md) | **English**

# What Is Ownership?

## Two kinds of values

Some values have a fixed, small size, known when the program is compiled: an `int` is always 8 bytes, a
`bool` 1 byte. A variable of such a type holds the value itself, and copying it is as cheap as copying a
number. SultanC calls these values *plain*. Numbers, `bool`, `char`, and records and tuples made only of
plain values are plain.

Other values can grow while the program runs. A `string` can hold one character or a whole book; a
`vector` can hold three elements or three million. Their contents live in a separate area of memory, the
*heap*, that is set aside when needed. The variable holds only a small description: where the contents
are, and how many there are. These values *own* their memory, and SultanC calls them *owning*. A
`string`, a `vector`, and any record, tuple or enum that contains one are owning.

Plain values need no rules: when a plain variable ends, nothing needs to be given back. The ownership
rules are about owning values, whose memory must be freed exactly once.

## The ownership rules

1. Each owning value has one variable that owns it, its *owner*.
2. There is only one owner at a time.
3. When the owner goes out of scope, the value's memory is freed.

You saw scope in chapter 3: a variable lives from its declaration to the end of its block. Rule 3 says
that is also where its memory goes back:

```sultan
import std.io.println;

def main(): int {
    let greeting: string = "hello";
    println("the string exists");
    return 0;
}
```

```text
the string exists
```

When `main` ends, `greeting` goes out of scope, and the compiler has placed the code that frees its text
right there. Nothing in your code says so, and nothing needs to.

## Moving

Rule 2, one owner at a time, decides what happens when you assign an owning value to another variable:

```sultan fail
import std.io.println;

def main(): int {
    let s: string = "hi";
    let t: string = s;
    println(s);
    return 0;
}
```

```text
error[SULTANC-OWNERSHIP-USE-AFTER-MOVE]: use after move — The value used after being moved is `s`. The value is used after its ownership was moved.
  --> main.sn:6:13..6:14
   |
 6 |     println(s);
   |             ^ invalid use
   |
  ::: main.sn:4:5..4:26
   |
 4 |     let s: string = "hi";
   |     ^^^^^^^^^^^^^^^^^^^^^ original owner
   |
  ::: main.sn:5:21..5:22
   |
 5 |     let t: string = s;
   |                     ^ value moved here
   |
```

The message shows three places: where `s` was the *original owner*, where the *value moved* to `t`, and
the *invalid use* of `s` after that.

`let t: string = s;` did not copy the text. It *moved* it: `t` became the owner, and `s` no longer owns
anything. If both `s` and `t` owned the same text, both would free it at the end of `main`, and the
second free would damage memory. Moving instead of copying keeps one owner, so the text is freed once.

Moving is also fast: only the small description moves, never the contents, however large they are.

Plain values do not move; they are copied, and both copies stay usable:

```sultan
import std.io.println;
import std.text.number;

def main(): int {
    let a: int = 5;
    let b: int = a;
    println(int_to_text(a + b));
    return 0;
}
```

```text
10
```

A moved-from variable can get a new value, if it is `mutable`. It then owns the new value:

```sultan
import std.io.println;

def main(): int {
    let s: mutable string = "first";
    let t: string = s;
    s := "second";
    println(s);
    println(t);
    return 0;
}
```

```text
second
first
```

## Ownership and functions

Passing a value to a function is like assigning it to the function's parameter: an owning value moves
into the function. That is why this program, which looks harmless, does not compile:

```sultan fail
def count(values: vector int): int {
    return length(values);
}

def main(): int {
    let numbers: vector int = [1, 2, 3];
    let n: int = count(numbers);
    return length(numbers) + n;
}
```

```text
error[SULTANC-OWNERSHIP-USE-AFTER-MOVE]: use after move — The value used after being moved is `numbers`. The value is used after its ownership was moved.
  --> main.sn:8:19..8:26
   |
 8 |     return length(numbers) + n;
   |                   ^^^^^^^ invalid use
   |
  ::: main.sn:6:5..6:41
   |
 6 |     let numbers: vector int = [1, 2, 3];
   |     ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ original owner
   |
  ::: main.sn:7:24..7:31
   |
 7 |     let n: int = count(numbers);
   |                        ^^^^^^^ value moved here
   |
```

`count` became the owner of the vector, and freed it when it returned. The next section shows two ways
around this: borrowing, and making a copy.

`println` does not have this problem: you can print the same string twice. That is because `println`
only borrows its text, as the next section explains.

## Returning values moves them out

A function gives a value back by moving it to the caller. A function that builds a value and returns it
is the normal way to create owning values:

```sultan
import std.io.println;
import std.text.transform;

def make_greeting(name: &string): string {
    let greeting: string = concat3(&"Hello, ", name, &"!");
    return greeting;
}

def main(): int {
    let message: string = make_greeting(&"Noor");
    println(message);
    return 0;
}
```

```text
Hello, Noor!
```

`greeting` is created inside `make_greeting`, and `return` moves it to the caller, where `message`
becomes its owner. Nothing is copied, and nothing is freed until `message` goes out of scope at the end
of `main`.

Moving a value into a function and getting it back every time would be tedious. Most functions only
need to look at a value, not to own it. For that, SultanC has references.
