[العربية](../../ar/book/ch06-01-defining-an-enum.md) | **English**

# Defining an Enum

An enum lists its cases, each after a `|`:

```sultan ignore
type Signal = | Red | Amber | Green;
```

A case is named through its type: `Signal.Green`. To make a value of a case, call it like a function
with empty parentheses:

```sultan
import std.io.println;

type Signal = | Red | Amber | Green;

def action(signal: Signal): string {
    match signal with
    | Signal.Red -> { return "stop"; }
    | Signal.Amber -> { return "wait"; }
    | Signal.Green -> { return "go"; };
}

def main(): int {
    let now: Signal = Signal.Green();
    println(action(now));
    println(action(Signal.Red()));
    return 0;
}
```

```text
go
stop
```

`Signal.Green()` makes a value; in a `match` pattern, `Signal.Green` is written without parentheses.
`action` takes any `Signal`, whichever case it is: the cases are values of one type.

## Cases that carry data

A case can carry a value, declared with `of`. Each case may carry a different type, or nothing:

```sultan ignore
type Shape = | Circle of int | Square of int | Dot;
```

A `Circle` carries its radius, a `Square` its side, and a `Dot` nothing. The value is given when the
case is made, `Shape.Circle(2)`, and taken out in a `match`:

```sultan
import std.io.println;
import std.text.number;

type Shape = | Circle of int | Square of int | Dot;

def area(shape: &Shape): int {
    match *shape with
    | Shape.Circle(radius) -> { return 3 * radius * radius; }
    | Shape.Square(side) -> { return side * side; }
    | Shape.Dot -> { return 0; };
}

def main(): int {
    let shapes: vector Shape = [Shape.Circle(2), Shape.Square(5), Shape.Dot()];
    for (let i: mutable int = 0; i < length(shapes); i := i + 1) {
        println(int_to_text(area(&shapes[i])));
    }
    return 0;
}
```

```text
12
25
0
```

The pattern `Shape.Circle(radius)` matches a circle and names the value it carries `radius`, for the
branch to use. One `match` chooses the case and pulls out its data.

`area` takes `&Shape` and matches `*shape`, the shape the reference points at.

Compare this with the alternative: a record with a `kind` field and separate fields for radius and side,
most of them unused for any given shape. Nothing would stop a program from reading the radius of a
square. With an enum, the radius exists only inside a circle, and the only way to reach it is the
pattern that has checked it is a circle.

## A case carries one value

Each case carries one value. When it needs several, make that value a record or a tuple:

```sultan
import std.io.println;
import std.text.number;

type Size = { width: int, height: int };
type Shape = | Circle of int | Rectangle of Size;

def area(shape: &Shape): int {
    match *shape with
    | Shape.Circle(radius) -> { return 3 * radius * radius; }
    | Shape.Rectangle({ width: w, height: h }) -> { return w * h; };
}

def main(): int {
    let room: Shape = Shape.Rectangle({ width: 3, height: 4 });
    println(int_to_text(area(&room)));
    return 0;
}
```

```text
12
```

The pattern `Shape.Rectangle({ width: w, height: h })` goes one step further than before: it takes the
record apart too, and names its fields `w` and `h`.

## Enums for states

Enums are the natural way to describe something that moves through states. Each state is a case, and a
function decides the next one:

```sultan
import std.io.println;

type Order = | Placed | Paid | Shipped | Delivered;

def next(order: Order): Order {
    match order with
    | Order.Placed -> { return Order.Paid(); }
    | Order.Paid -> { return Order.Shipped(); }
    | Order.Shipped -> { return Order.Delivered(); }
    | Order.Delivered -> { return Order.Delivered(); };
}

def name(order: &Order): string {
    match *order with
    | Order.Placed -> { return "placed"; }
    | Order.Paid -> { return "paid"; }
    | Order.Shipped -> { return "shipped"; }
    | Order.Delivered -> { return "delivered"; };
}

def main(): int {
    let order: mutable Order = Order.Placed();
    for (let step: mutable int = 0; step < 4; step := step + 1) {
        println(name(&order));
        order := next(order);
    }
    return 0;
}
```

```text
placed
paid
shipped
delivered
```

An enum is plain or owning by the same rule as a record: `Order` and `Signal` carry nothing owning, so
they are plain and copied. An enum with a case that carries a `string` is owning, and moves.
