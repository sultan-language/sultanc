[العربية](../../ar/book/ch05-02-example-records.md) | **English**

# An Example Program Using Records

To see why records help, let's write a program that computes the area of a rectangle, and improve it in
three steps.

Make a new project called `rectangles`:

```console
$ supax new rectangles
$ cd rectangles
```

## First version: two variables

<sub>Listing 5-1: the area from a separate width and height. Filename: src/main.sn</sub>

```sultan
import std.io.println;
import std.text.number;
import std.text.transform;

def area(width: int, height: int): int {
    return width * height;
}

def main(): int {
    let width: int = 30;
    let height: int = 50;
    println(concat3(&"The area of the rectangle is ", &int_to_text(area(width, height)), &" square pixels."));
    return 0;
}
```

```text
The area of the rectangle is 1500 square pixels.
```

It works, but `area` has two parameters that are related, and nothing in the program says so. Is the
first one the width or the height? Every call must get the order right, and the compiler cannot help,
because both are `int`.

## Second version: a tuple

<sub>Listing 5-2: the width and height in one tuple. Filename: src/main.sn</sub>

```sultan
import std.io.println;
import std.text.number;
import std.text.transform;

def area(dimensions: (int, int)): int {
    match dimensions with
    | (width, height) -> { return width * height; };
}

def main(): int {
    let rect: (int, int) = (30, 50);
    println(concat3(&"The area of the rectangle is ", &int_to_text(area(rect)), &" square pixels."));
    return 0;
}
```

```text
The area of the rectangle is 1500 square pixels.
```

Better in one way: the two numbers travel together as one value. Worse in another: the parts have no
names. The program has to remember that the first is the width, and anyone reading `(30, 50)` has to
guess.

## Third version: a record

<sub>Listing 5-3: a `Rectangle` record. Filename: src/main.sn</sub>

```sultan
import std.io.println;
import std.text.number;
import std.text.transform;

type Rectangle = { width: int, height: int };

def area(rectangle: &Rectangle): int {
    return rectangle.width * rectangle.height;
}

def main(): int {
    let rect: Rectangle = { width: 30, height: 50 };
    println(concat3(&"The area of the rectangle is ", &int_to_text(area(&rect)), &" square pixels."));
    return 0;
}
```

```text
The area of the rectangle is 1500 square pixels.
```

Now the type says what the value is, every part has a name, and `area` says in its signature that it
computes the area of a `Rectangle`. Nobody can pass the width and height the wrong way round, because
nobody passes them separately.

`area` takes `&Rectangle`: it only reads the rectangle, so it borrows it, and `main` keeps it. Inside,
`rectangle.width` reaches the field through the reference; the next section explains how.

`concat3`, from `std.text.transform`, joins three strings, as `concat` joins two.

## Printing a rectangle

A `Rectangle` cannot be passed to `println`, which only prints strings. To show one, write a function
that builds its text:

<sub>Listing 5-4: a function that describes a `Rectangle`. Filename: src/main.sn</sub>

```sultan
import std.io.println;
import std.text.number;
import std.text.transform;

type Rectangle = { width: int, height: int };

def describe(rectangle: &Rectangle): string {
    let text: string = concat3(&int_to_text(rectangle.width), &" x ", &int_to_text(rectangle.height));
    return text;
}

def main(): int {
    let rect: Rectangle = { width: 30, height: 50 };
    println(describe(&rect));
    return 0;
}
```

```text
30 x 50
```

Functions like `describe` and `area`, which take a record and work with its fields, are the subject of
the next section.
