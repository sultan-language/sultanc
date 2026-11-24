[العربية](../../ar/book/ch06-02-match.md) | **English**

# The `match` Statement

`match` compares a value with a list of patterns, and runs the branch of the first one that matches.
You have seen it on numbers, text, results and enums. This section collects what patterns can do.

## What a pattern can be

| Pattern | Matches |
| --- | --- |
| `5`, `'a'`, `"yes"` | that exact value |
| `range 1 to 9` | any value from 1 to 9, both included |
| `name` | anything, and names it for the branch |
| `_` | anything, without naming it |
| `A \| B` | anything `A` or `B` matches |
| `Shape.Circle(radius)` | that enum case, naming what it carries |
| `(x, 0)` | a tuple, part by part |
| `{ width: w, height: h }` | a record, field by field |

Patterns nest: anything inside a case, a tuple or a record is itself a pattern. This `match` looks at a
tuple of a number and an `option`, and inside the `option` at a range:

```sultan
import std.io.println;

def describe(pair: (int, option int)): string {
    match pair with
    | (0, option.none) -> { return "zero and nothing"; }
    | (_, option.some(range 1 to 9)) -> { return "a small number"; }
    | (_, option.some(_)) -> { return "another number"; }
    | (_, option.none) -> { return "nothing"; };
}

def main(): int {
    let a: (int, option int) = (0, option.none());
    let b: (int, option int) = (3, option.some(4));
    let c: (int, option int) = (3, option.some(40));
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

The branches are tried in order. `(3, option.some(4))` matches both the second and the third pattern;
the second one is first, so it wins. Put the specific patterns before the general ones.

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

Each branch runs a block of statements. When a branch has nothing to do, its block can be empty, `{ }`.
The branch blocks are not values: `match` decides which statements run. To get a value out of a
`match`, assign a variable in each branch, as chapter 2 did with `guess`, or `return` from the branch.
