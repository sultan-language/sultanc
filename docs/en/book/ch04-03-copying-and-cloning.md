[العربية](../../ar/book/ch04-03-copying-and-cloning.md) | **English**

# Copying and Cloning

Sometimes you really need two independent values: one to keep and one to change, or one to give away
while you go on using the other. `clone` makes them.

## `clone`

`clone(&value)` returns a complete, independent copy of any value, however deep: a vector of records of
strings is copied all the way down. Changing the copy never changes the original:

```sultan
import std.io.println;
import std.text.number;

def main(): int {
    let original: vector int = [1, 2, 3];
    let copy: mutable vector int = clone(&original);
    append(copy, 4);
    println(int_to_text(length(original)));
    println(int_to_text(length(copy)));
    return 0;
}
```

```text
3
4
```

`clone` takes a reference, because making a copy only reads the original.

A clone costs time and memory in proportion to the size of the value. That is why SultanC never clones
by itself: each `clone` in the code is a visible decision to pay that cost. Borrow when you can, and
clone when you need a second value.

## Plain values are copied

Plain values do not need `clone`: assigning or passing them always copies, because the copy is as cheap
as the original. A record whose fields are all plain is plain too:

```sultan
import std.io.println;
import std.text.number;

type Point = { x: int, y: int };

def main(): int {
    let a: Point = { x: 1, y: 2 };
    let b: Point = a;
    println(int_to_text(a.x + b.y));
    return 0;
}
```

```text
3
```

Records come in [chapter 5](ch05-00-records.md). Add one `string` field to `Point` and it becomes owning:
assigning it would move it, as with any string.

## When memory is freed

To put the rules together: an owning value is freed when its owner goes out of scope, at the end of the
block where the owner was declared, unless it was moved somewhere else before. If it was moved, the new
owner frees it at the end of its own scope. A value returned from a function is freed by whoever ends up
owning it.

The compiler tracks every value through every path of the program, and places the freeing code for you.
The result:

- memory is never used after it is freed, because a moved-from or freed value cannot be named;
- memory is never freed twice, because each value has one owner;
- memory is not forgotten, because every owner goes out of scope somewhere.

## Habits that make ownership easy

- Take parameters as `&T`, unless the function needs to keep the value. A function that reads text
  takes `&string`.
- Take `&mutable T` when the function changes the caller's value.
- Take `T` itself when the function stores the value, or builds something new from it and returns that.
- To give a value you still need to a function that keeps it, `clone` it. Printing only borrows.
- When the compiler says *use after move*, look at the place it marks *value moved here*. That is where
  you gave the value away: borrow there, or clone there.

## Summary

- Plain values (numbers, `bool`, `char`, and records and tuples of them) are copied.
- Owning values (`string`, `vector`, and anything containing them) move, and have one owner at a time.
- `clone(&value)` makes an independent copy, at a visible cost.
- Memory is freed exactly once, when the last owner's scope ends.

The next chapter groups values together into records of your own.
