[العربية](../../ar/book/ch04-02-references-and-borrowing.md) | **English**

# References and Borrowing

The last section ended with a problem: passing a value to a function moves it, so the caller loses it.
Most functions only need to look at a value. For them, the caller can lend the value instead of giving
it away.

## References

A *reference* points at a value that someone else owns. Its type is written `&T`, and a reference to a
variable is made with `&name`:

```sultan
import std.io.println;
import std.text.number;

def count(values: &vector int): int {
    return length(*values);
}

def main(): int {
    let numbers: vector int = [1, 2, 3];
    let n: int = count(&numbers);
    println(int_to_text(n + length(numbers)));
    return 0;
}
```

```text
6
```

`count(&numbers)` lends the vector: `count` gets a reference, and `main` keeps ownership. When `count`
returns, its reference ends, but the vector does not, because the reference never owned it. `main` can
use `numbers` afterwards.

Inside `count`, `*values` is the vector the reference points at. `*` follows a reference to its value;
this is called *dereferencing*.

Lending a value through a reference is called *borrowing*. As in life, what you borrow you must give
back, and you cannot give it to someone else.

You have used references since chapter 2: `trim(&line)`, `concat(&"You guessed: ", &guess)`. Text
functions take `&string` because they only read the text.

## Mutable references

A plain `&T` lets the borrower read, not change. To let a function change a value it borrows, lend it
with `&mutable`. The type is `&mutable T`, the call writes `&mutable name`, and the owner must itself be
`mutable`:

```sultan
import std.io.println;
import std.text.number;

def add_item(cart: &mutable vector string, item: &string): void {
    append(*cart, clone(item));
}

def main(): int {
    let cart: mutable vector string = [];
    add_item(&mutable cart, &"bread");
    add_item(&mutable cart, &"milk");
    println(int_to_text(length(cart)));
    return 0;
}
```

```text
2
```

`append` adds an element at the end of a vector. `clone(item)` makes the cart its own copy of the text,
because `item` is only borrowed: the cart cannot keep a value that belongs to someone else. The next
section covers `clone`.

## The rules of references

The compiler checks two rules for every reference.

**First: a mutable reference excludes every other reference to the same value.** While one part of the
program may change a value, no other part may read it:

```sultan fail
def both(target: &mutable vector int, source: &vector int): void {
    append(*target, length(*source));
}

def main(): int {
    let values: mutable vector int = [1];
    both(&mutable values, &values);
    return 0;
}
```

```text
error[SULTANC-BORROW-CONFLICT]: borrow conflict — The conflicting borrowed value is `values`. The value is borrowed for reading, so it cannot be borrowed mutably.
  --> main.sn:6:5..6:42
   |
 6 |     let values: mutable vector int = [1];
   |     ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
   |
```

Why forbid this? `both` adds to `target` while reading `source`. If they are the same vector, adding an
element may move the vector's contents to a larger place in memory, and `source` would then point at
memory that was given back. This kind of bug, called a *data race* when threads are involved, is hard to
find because it depends on timing and sizes. The rule makes it impossible.

Any number of read-only references can exist at the same time, since reading cannot disturb other
readers.

**Second: a reference cannot outlive the value it points at.** Here a function returns a reference to
its own local variable:

```sultan fail
def dangle(): &string {
    let s: string = "hello";
    return &s;
}

def main(): int {
    let r: &string = dangle();
    return 0;
}
```

```text
error[SULTANC-BORROW-REFERENCE-ESCAPE]: reference escapes its valid scope — The escaping reference is `s`. A reference outlives the value it points to.
  --> main.sn:2:5..2:29
   |
 2 |     let s: string = "hello";
   |     ^^^^^^^^^^^^^^^^^^^^^^^^
   |
```

`s` is freed when `dangle` returns, so the reference would point at freed memory: a *dangling*
reference. The compiler refuses it. The fix is to return the `string` itself, moving it to the caller.

## You cannot move out of a borrow

A reference lends a value; it does not give it away. So you cannot take an owning value out of
something you only borrowed:

```sultan fail
def first(names: &vector string): string {
    return (*names)[0];
}

def main(): int {
    let names: vector string = ["Huda"];
    let f: string = first(&names);
    return 0;
}
```

```text
error[SULTANC-OWNERSHIP-MOVE-OUT-OF-REFERENCE]: cannot move an owned value out of borrowed storage — The value moved out of borrowed storage is `names`. An owned value is moved out of borrowed storage.
  --> main.sn:2:12..2:23
   |
 2 |     return (*names)[0];
   |            ^^^^^^^^^^^
   |
```

Returning `(*names)[0]` would move the first string out of the caller's vector, leaving a hole in it.
Return a copy instead:

```sultan
import std.io.println;

def first(names: &vector string): string {
    return clone(&(*names)[0]);
}

def main(): int {
    let names: vector string = ["Huda", "Sara"];
    let f: string = first(&names);
    println(f);
    println(clone(&names[1]));
    return 0;
}
```

```text
Huda
Sara
```

## Printing a value you still need

Now the `println` problem from the last section has a solution. `println` takes its string by value, so
give it a copy, and keep the original:

```sultan
import std.io.println;

def main(): int {
    let greeting: string = "hello";
    println(clone(&greeting));
    println(greeting);
    return 0;
}
```

```text
hello
hello
```

The last use can move the value, since nothing needs it afterwards.

## Summary

- `&T` borrows a value to read it; `&mutable T` borrows it to change it. `*` reaches the value.
- A mutable reference cannot exist alongside any other reference to the same value.
- A reference cannot outlive its value.
- Nothing can be moved out of a borrowed value; return or store a copy.
