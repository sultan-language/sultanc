[العربية](../../ar/book/ch06-02-match.md) | **English**

# The `match` Statement

`match` compares a value with a list of patterns, and runs the branch of the first one that matches.
You have seen it on numbers, text, results and enums. This section collects what patterns can do.

## What a pattern can be

| Pattern | Matches |
| --- | --- |
| `5`, `'a'`, `"yes"` | that exact value |
| `range 1..9`, `range 'a'..'z'` | any value from 1 to 9 (from `'a'` to `'z'`), both ends included |
| `range 100..`, `range ..0` | any value from 100 up, any value up to 0 |
| `name` | anything, and names it for the branch |
| `_` | anything, without naming it |
| `A \| B` | anything one of the patterns `A`, `B` matches; both bind the same names |
| `Shape.Circle(radius)` | that enum case, naming what it carries |
| `some(x)`, `none`, `ok(x)`, `err(e)` | the cases of `option` and `result`, written without their type |
| `(x, 0)` | a tuple, part by part |
| `(A \| B)` | the pattern inside: parentheses group |
| `{ width: w, height: h }` | a record, field by field; fields left out match anything |
| `{ width }` | a record: `width` binds the field of that name; the other fields match anything |
| `[]`, `[a, b]` | a vector or slice of exactly that many elements |
| `[first, rest _]`, `[rest _, last]`, `[first, rest tail]` | a vector or slice of at least that many elements; `rest` stands for the others |
| `pattern as name` | what `pattern` matches, and names the whole value |

After the pattern, an arm may have a guard: `| pattern if condition -> { … }`.

Patterns nest: anything inside a case, a tuple or a record is itself a pattern. This `match` looks at a
tuple of a number and an `option`, and inside the `option` at a range:

```sultan
import std.io.println;

def describe(pair: (int, option int)): string {
    match pair with
    | (0, none) -> { return "zero and nothing"; }
    | (_, some(range 1..9)) -> { return "a small number"; }
    | (_, some(_)) -> { return "another number"; }
    | (_, none) -> { return "nothing"; };
}

def main(): int {
    let a: (int, option int) = (0, none);
    let b: (int, option int) = (3, some(4));
    let c: (int, option int) = (3, some(40));
    println(describe(a));
    println(describe(b));
    println(describe(c));
    return 0;
}
```

```text
zero and nothing
a small number
another number
```

The branches are tried in order. `(3, some(4))` matches both the second and the third pattern;
the second one is first, so it wins. Put the specific patterns before the general ones.

## `some`, `none`, `ok` and `err`

The cases of `option` and `result` are written without their type, in values and in patterns: `some(x)`, `none`,
`ok(x)` and `err(e)` (in Arabic `موجود(س)`, `لا_شيء`, `نجح(س)` and `خطأ(خ)`). The type that is expected there (a
parameter, a declared variable, a function's return type, the matched value) says which `option` or `result` it is.
`none` needs no `()`; `none()` and the qualified forms (`option.some(x)`, `result.err(e)`) still work. Every other
enum's cases keep their type: `Shape.Circle(r)`.

Where no type is expected, as for the value a `match` looks at, `some(x)` says its own: it is the `option` of `x`'s
type, so `match some(5) with | some(n) -> … | none -> …` works. `none`, `ok(x)` and `err(e)` do not say the whole
type (which value `none` lacks, which error `ok` could have been), so there they are an error
(`SULTANC-TYPE-PRELUDE-CONSTRUCTOR-UNTYPED`): give the value a type first, as in `let found: option int = none;`.

A declaration with one of these names shadows it in expressions: after `let none: int = 0;`, `none` is that local,
and a function named `some` is called as written. In a pattern the bare forms are always the cases of `option` and
`result`, never new names. In Arabic `خطأ` alone is the boolean false; `خطأ(e)` is the error case.

## Alternatives

`A | B` matches what either alternative matches. Alternatives can be any patterns, at the top of an arm or inside
another pattern:

```sultan
import std.io.println;
import std.text.number;

type Shape = | Circle of int | Square of int | Dot;

def size(shape: Shape): int {
    match shape with
    | Shape.Circle(0) | Shape.Dot -> { return 0; }
    | Shape.Circle(n) | Shape.Square(n) -> { return n; };
}

def on_axis(point: (int, int)): int {
    match point with
    | (0, v) | (v, 0) -> { return v; }
    | _ -> { return -1; };
}

def main(): int {
    println(int_to_text(size(Shape.Dot())));
    println(int_to_text(size(Shape.Square(4))));
    println(int_to_text(on_axis((0, 7))));
    return 0;
}
```

```text
0
4
7
```

Whichever alternative matches, the branch sees the same names, so every alternative must bind the same names, each
with the same type. `| (0, x) | (y, 0)` is `SULTANC-PATTERN-ALTERNATIVE-BINDINGS` (`x` is not bound in every
alternative), and `| Value.Number(x) | Value.Word(x)` with an `int` and a `string` payload is
`SULTANC-PATTERN-ALTERNATIVE-TYPES`. An alternative that the arms before it, and the alternatives before it, already
cover never matches; like an arm that never runs, it is reported with a warning (`SULTANC-PATTERN-UNREACHABLE`). An
or-pattern nested inside a pattern is checked against its own earlier alternatives: in `| (1 | 2 | 1, y)` the second
`1` is reported. Whether the earlier arms cover a nested alternative is not checked: after `| some(1)`, the
`1` of `| some(1 | 2)` is not reported.

## Guards

An arm may add a condition after its pattern: `| pattern if condition -> { … }`. The arm is chosen only when the
pattern matches and the condition is true; otherwise matching goes on with the next arm. The condition sees the
pattern's names, and a guard applies to all the alternatives of the arm's pattern:

```sultan
import std.io.println;

def classify(value: int): string {
    match value with
    | x if x > 100 -> { return "large"; }
    | x if x > 5 -> { return "medium"; }
    | 0 -> { return "zero"; }
    | _ -> { return "small"; };
}

def main(): int {
    println(classify(500));
    println(classify(9));
    println(classify(2));
    return 0;
}
```

```text
large
medium
small
```

The compiler cannot know when a condition holds, so a guarded arm does not count toward exhaustiveness: the arms
after it must still cover every value. Here the last two arms do.

While the guard runs, the pattern's names are borrowed: the guard reads them and cannot move them (moving one is
`SULTANC-OWNERSHIP-MOVE-OUT-OF-REFERENCE`). Nothing has left the matched value yet, so when the guard is false the next
arm sees all of it; owned parts move into the names only when the arm is chosen.

A guard cannot return early: `?` is accepted only at the end of a statement's right-hand side, so `| v if check(v)? ->`
is `SULTANC-PARSE-SYNTAX` at the `?`. Compute the value in a statement first, or match on it.

## Naming the whole value: `as`

`pattern as name` matches what `pattern` matches and also names the whole value. `as` binds the loosest, so
`Hand.Rock | Hand.Paper as h` names the value whichever alternative matched; parentheses group a pattern,
`(Shape.Circle(_) as c, n)`:

```sultan
import std.io.println;
import std.text.number;

type Shape = | Circle of int | Square of int;

def area(shape: Shape): int {
    match shape with
    | Shape.Circle(r) -> { return 3 * r * r; }
    | Shape.Square(n) -> { return n * n; };
}

def describe(shape: Shape): int {
    match shape with
    | Shape.Circle(r) as circle -> { return r + area(circle); }
    | Shape.Square(_) as square -> { return area(square); };
}

def main(): int {
    println(int_to_text(describe(Shape.Circle(2))));
    return 0;
}
```

```text
14
```

When the value owns memory, a name for the whole of it owns all of it. If the pattern inside also binds a part that
owns memory (`| Named.Two(a, _) as all`), both names would own the same storage. Such an arm borrows the matched
value instead: every name of it whose type owns memory is a borrowed view, and nothing moves out of the value, which
its owner keeps. The arm can read the names; moving one of them is `SULTANC-OWNERSHIP-MOVE-OUT-OF-REFERENCE`. Plain
parts (numbers, characters, booleans) are always copied.

## Records: only the fields you need

A record pattern names the fields it looks at; the fields it leaves out match anything. A field written alone binds a
name spelled as the field: `{ width, height }` is `{ width: width, height: height }`, and the forms mix,
`{ width, height: 0 }`:

```sultan
import std.io.println;
import std.text.number;

type Size = { width: int, height: int, depth: int };

def area(size: Size): int {
    match size with
    | { width, height: 0 } -> { return width; }
    | { width, height } -> { return width * height; };
}

def main(): int {
    let box: Size = { width: 3, height: 4, depth: 5 };
    println(int_to_text(area(box)));
    return 0;
}
```

```text
12
```

## Vectors and slices

A pattern in brackets matches a vector or a slice by its length and its elements. `[]` matches an empty one and
`[a, b]` exactly two elements. Inside the brackets, the word `rest` followed by a name or `_` stands for the elements
between the ones written, at most once:

| Pattern | Matches |
| --- | --- |
| `[first, rest _]` | one element or more; names the first |
| `[rest _, last]` | one element or more; names the last |
| `[first, rest _, last]` | two elements or more; names both ends |
| `[first, rest tail]` | one element or more; `tail` is the slice of the others |
| `[a, rest middle, b]` | two elements or more; `middle` is the slice between the ends |
| `[a, b, rest _]` | two elements or more |

A named rest is the slice the slicing expression gives for the same elements: in `[first, rest tail]`, `tail` is
`v[1..]` ([8.1](ch08-01-vectors.md)). `rest` is a word of this place only: `[rest]` and `[rest, x]` bind a name
`rest`, and a variable may be called `rest`. A second `rest` in one pattern is `SULTANC-PARSE-PATTERN`.

```sultan
import std.io.println;
import std.text.number;

def sum(values: [] int): int {
    match values with
    | [] -> { return 0; }
    | [first, rest tail] -> { return first + sum(tail); };
}

def palindrome(text: [] u8): bool {
    match text with
    | [a, rest middle, b] -> { return a == b && palindrome(middle); }
    | _ -> { return true; };
}

def main(): int {
    let numbers: vector int = [1, 2, 3];
    println(int_to_text(sum(numbers)));
    return 0;
}
```

```text
6
```

A named rest is a slice `[] T` of the matched value's own elements: nothing is copied, whatever the elements are
(texts, records, vectors), and on a slice it is a narrower slice of the same memory. Each arm tests the length once
and reads the elements it names at fixed positions (`[rest _, last]` reads element `length - 1`), with no loop and
no bounds check.

Exhaustiveness counts lengths: `[]` and `[_, rest _]` together cover every vector, and an arm `[a]` after
`[_, rest _]` never runs. A match that misses a length names the shortest one in its diagnostic, as `[_, _, rest _]`
for "two or more" (`SULTANC-PATTERN-NON-EXHAUSTIVE`).

Elements bound out of a vector are borrowed views of it when they own memory: a vector owns its elements as a whole,
so an arm that binds owning elements borrows the vector, as an arm with `as` does above. A named rest borrows the
matched vector as a slice given a vector does: the vector cannot be moved or changed while the rest is in use, and a
slice that takes the rest's value cannot be read after the vector is destroyed (`SULTANC-BORROW-REFERENCE-ESCAPE`).
When the matched value is a temporary (a call result), the arm keeps it until the arm ends, so the rest is valid in
the whole arm.

A named rest is a read-only view. To write through it, match the vector with `&mutable`, as for any change through
a borrow: under `match &mutable v with` the named rest is a writable view of v's elements, a `mutable [] T`, while
the elements bound by name stay read-only values (assigning to one is `SULTANC-MUTABILITY-WRITE-TO-IMMUTABLE`). The
rest borrows v exclusively for as long as it is used.

```sultan
def fill_tail(v: &mutable vector int): int {
    match &mutable *v with
    | [first, rest tail] -> {
        let i: mutable int = 0;
        while (i < length(tail)) { tail[i] := first; i := i + 1; }
        return length(tail);
    }
    | [] -> { return 0; };
}

def main(): int {
    let w: mutable vector int = [9, 8, 7];
    if (fill_tail(&mutable w) != 2 || w[2] != 9) { return 1; };
    return 0;
}
```

## Every case must be handled

A `match` must have a branch for every possible value. On an enum, that means every case:

```sultan fail
type Light = | Red | Amber | Green;

def code(light: Light): int {
    match light with
    | Light.Red -> { return 1; }
    | Light.Green -> { return 3; };
}

def main(): int {
    return code(Light.Amber());
}
```

```text
error[SULTANC-PATTERN-NON-EXHAUSTIVE]: non-exhaustive pattern: `Light.Amber` not covered — Every value of the matched type must match one of the arms.
  --> main.sn:4:11..4:16
   |
 4 |     match light with
   |           ^^^^^
   |
  = help: add an arm for each case that is not covered
```

`Light.Amber` has no branch, and the message says so.

A `match` whose arms miss a case is `SULTANC-PATTERN-NON-EXHAUSTIVE`, marked at the matched value.
The headline names the cases no arm covers, up to three, then how many more
(``non-exhaustive pattern: `Shape.Dot` not covered``); an option's or a result's cases and a boolean's are
spelled in the language of the output, and `_` stands for a value of a type with no closed set of cases
(a number, a text). The fix adds an arm `| Shape.Dot -> { }` for each missing case after the last arm,
in the language of the source.

This check is what makes enums safe to change. When you add a case to an enum, every `match` on it that
does not handle the new case stops compiling, and each one is a place in your program that needs a
decision about the new case.

`_` can catch all remaining cases in one branch. Use it when the remaining cases really are all the same
to you. When they are not, list them: a `_` would also catch a case added later, silently, without the
compiler pointing at it.

## Matching through a reference

To match the value a reference points at, match `*reference`. This is the usual way to look at a value
you borrowed:

```sultan
import std.io.println;
import std.text.number;
import std.text.transform;

type Shape = | Circle of int | Square of int;

def describe(shape: &Shape): string {
    match *shape with
    | Shape.Circle(r) -> { return concat(&"circle of radius ", &int_to_text(r)); }
    | Shape.Square(s) -> { return concat(&"square of side ", &int_to_text(s)); };
}

def main(): int {
    let s: Shape = Shape.Circle(3);
    println(describe(&s));
    return 0;
}
```

```text
circle of radius 3
```

## A branch is a block

Each branch of a `match` statement runs a block of statements. When a branch has nothing to do, its block
can be empty, `{ }`.

## A match that gives a value

`if (c) a else b` used as a value ([3.5](ch03-05-control-flow.md)) is this kind of `match` on the condition.

A `match` can also stand wherever a value is expected: after the `=` of a `let`, after `:=`, after `return`,
as an argument, as an operand, in a condition, in the body of a closure. Its value is the value of the branch
that matches, and a branch is then `| pattern -> value`:

```sultan
import std.io.println;
import std.text.number;

type Light = | Red | Amber | Green;

def label(light: Light): string {
    return match light with
    | Light.Red -> "stop"
    | Light.Amber -> "wait"
    | Light.Green -> "go";
}

def main(): int {
    println(label(Light.Amber()));
    let seconds: int = 10 * match Light.Red() with | Light.Green -> 0 | _ -> 3;
    println(int_to_text(seconds));
    return 0;
}
```

```text
wait
30
```

A branch that needs statements first writes them in braces. The expression at the end of the braces, with
no `;` after it, is the branch's value:

```sultan
import std.io.println;
import std.text.number;

def score(word: string): int {
    return match length(word) with
    | 0 -> 0
    | n -> {
        let size: int = n as int;
        size * size
    };
}

def main(): int {
    println(int_to_text(score("four")));
    return 0;
}
```

```text
16
```

A branch that does not finish needs no value: its block returns, leaves or continues a loop, or panics.

```sultan
import std.io.println;
import std.text.number;

def doubled_or_zero(value: option int): int {
    let found: int = match value with
    | some(v) -> v
    | none -> { return 0; };
    return found * 2;
}

def main(): int {
    println(int_to_text(doubled_or_zero(some(21))));
    println(int_to_text(doubled_or_zero(none)));
    return 0;
}
```

```text
42
0
```

The rules of a `match` that gives a value:

- Every branch gives a value of one type. When the place of the `match` expects a type (the `let`'s type, the
  parameter's, the function's return type), that is the type, and `none`, number literals and record
  or vector literals in the branches take it, as they would there. Without one, the first branch that gives a
  value decides the type, and a later branch of another type is `SULTANC-TYPE-MISMATCH` at that branch.
- At least one branch gives a value (`SULTANC-TYPE-MATCH-WITHOUT-VALUE`). A branch without a value that can run
  to its end is `SULTANC-FLOW-MATCH-ARM-WITHOUT-VALUE`, marked at the branch.
- Every case is handled, as in the statement (`SULTANC-PATTERN-NON-EXHAUSTIVE`).
- A branch's pattern is any pattern the statement takes, with a guard too: `| Shape.Circle(r) if r > 10 -> 3`.
  The guard borrows the pattern's names, as in the statement.
- The branch's value moves into the value of the `match`. A part of the matched value that the pattern names
  moves out of it, as in the statement, and a value moved in one branch only stays with its owner on the other
  paths.
- The `match` runs where the source has it: what comes before it in the expression runs first, what comes
  after it runs after the branch.
- A branch value ends at the next `|`: write `(a | b)` for a bitwise or, and put a closure or a further
  `match` in parentheses unless it is in the last branch.
- `?` cannot be used inside the branches. A `?` after the last branch applies to the value of the whole
  `match`.
- A statement that starts with `match` is the `match` statement, whose branches are blocks. `if … else` is a
  statement only.
