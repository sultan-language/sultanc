[العربية](../../ar/book/ch05-01-defining-records.md) | **English**

# Defining Records

`type` declares a new type. A record type lists its *fields* between braces, each with a name and a
type:

```sultan ignore
type User = {
    name: string,
    email: string,
    sign_in_count: int,
    active: bool
};
```

A record value is written the same way, with a value for each field. A field is read with a dot:

```sultan
import std.io.println;
import std.text.number;

type User = {
    name: string,
    email: string,
    sign_in_count: int,
    active: bool
};

def main(): int {
    let user: User = {
        name: "Layla",
        email: "layla@example.com",
        sign_in_count: 1,
        active: true
    };
    println(user.email);
    println(int_to_text(user.sign_in_count));
    return 0;
}
```

```text
layla@example.com
1
```

The fields of a value may be given in any order; the names say which value goes where. But every field
must be given. A record is never half filled:

```sultan fail
type Point = { x: int, y: int };

def main(): int {
    let p: Point = { x: 1 };
    return p.x;
}
```

```text
error[SULTANC-TYPE-MISSING-RECORD-FIELD]: required record field is missing — The missing record field is `y`.
  --> main.sn:4:5..4:29
   |
 4 |     let p: Point = { x: 1 };
   |     ^^^^^^^^^^^^^^^^^^^^^^^^
   |
```

## Changing a field

If the variable is `mutable`, its fields can be changed with `:=`. The whole record is either mutable or
not; there are no separately mutable fields:

```sultan
import std.io.println;
import std.text.number;

type Counter = { label: string, count: int };

def main(): int {
    let visits: mutable Counter = { label: "visits", count: 0 };
    visits.count := visits.count + 1;
    visits.count := visits.count + 1;
    println(visits.label);
    println(int_to_text(visits.count));
    return 0;
}
```

```text
visits
2
```

## Records inside records

A field can have any type: a number, a string, a vector, or another record:

```sultan
import std.io.println;
import std.text.number;

type Point = { x: int, y: int };
type Line = { start: Point, end: Point, labels: vector string };

def main(): int {
    let line: Line = {
        start: { x: 0, y: 0 },
        end: { x: 6, y: 8 },
        labels: ["north", "east"]
    };
    println(int_to_text(line.end.x - line.start.x));
    println(line.labels[1]);
    return 0;
}
```

```text
6
east
```

`line.end.x` reads from left to right: the field `end` of `line`, then its field `x`.

## Records and ownership

A record owns its fields. If all of them are plain, the record is plain and is copied, as you saw in
chapter 4. If any field is owning, like the `string` fields of `User`, the record is owning: assigning
it moves it, and when its owner goes out of scope, all its fields are freed with it.

`println(user.email)` above only borrows the field, because `println` takes `&string`, so `user`
stays whole. Moving an owning field out, as `let email: string = user.email;` would, is refused: it
would leave the record with a hole. Copy it with `clone(&user.email)` when you need a string of your
own. Reading a plain field, like `user.sign_in_count`, copies it and needs nothing special.

## Another name for a type

`type` can also give an existing type a second name. The new name is the same type, but it says what
the value is for:

```sultan
import std.io.println;
import std.text.number;

type Celsius = int;
type Grid = vector vector int;

def main(): int {
    let freezing: Celsius = 0;
    let boiling: Celsius = 100;
    println(int_to_text(boiling - freezing));
    let grid: Grid = [[1, 2, 3], [4, 5, 6]];
    println(int_to_text(grid[1][2]));
    return 0;
}
```

```text
100
6
```

Because `Celsius` is only another name for `int`, a `Celsius` and an `int` mix freely. A name like this
documents; it does not protect. To make a type that cannot be mixed up with another, wrap it in a
record: `type Celsius = { degrees: int };`.

## Naming

By convention, the examples in this book name their own types in `PascalCase`, each word starting with a
capital letter (`User`, `Point`), and fields in `snake_case`, like variables. The standard library names
its types in `snake_case` (`text_builder`). Either works; choose one per project.
