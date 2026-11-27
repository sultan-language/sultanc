[العربية](../../ar/book/ch08-01-vectors.md) | **English**

# Vectors

A vector, `vector T`, holds values of one type `T`, in order, and can grow. `vector int` holds integers,
`vector string` holds strings, `vector Point` holds records.

## Creating a vector and reading from it

A vector is written as its values in square brackets; an empty one is `[]`. `v[i]` reads the value at
position `i`, starting from 0, and `length(v)` gives the number of values:

```sultan
import std.io.println;
import std.text.number;

def main(): int {
    let primes: vector int = [2, 3, 5, 7, 11];
    println(int_to_text(primes[0]));
    println(int_to_text(primes[length(primes) - 1]));
    println(int_to_text(length(primes)));
    return 0;
}
```

```text
2
11
5
```

The last value is at `length(v) - 1`. What about a position that does not exist? When the position is
written in the code, the compiler sees the problem:

```sultan fail
import std.io.println;
import std.text.number;

def main(): int {
    let primes: vector int = [2, 3, 5];
    println(int_to_text(primes[10]));
    return 0;
}
```

```text
error[SULTANC-SAFETY-INDEX-OUT-OF-BOUNDS]: index is outside the valid bounds — The out-of-bounds index is 10. The length is 3.
  --> main.sn:6:32..6:34
   |
 6 |     println(int_to_text(primes[10]));
   |                                ^^
   |
```

Usually the position is computed while the program runs, and the compiler cannot know it. Then the
program checks it when it reads, and stops if it is past the end, as it does on an overflow:

```sultan
import std.io.println;
import std.text.number;

def value_at(values: &vector int, position: int): int {
    return (*values)[position];
}

def main(): int {
    let primes: vector int = [2, 3, 5];
    println(int_to_text(value_at(&primes, 10)));
    return 0;
}
```

```console
$ ./primes
error[SULTANC-RUNTIME-PROGRAM-STOPPED]: the program stopped
index 10 out of range for length 3 at main.sn:5:12
```

The message gives the position, the length of the vector, and the place in the source that read it.

Reading past the end of a vector is one of the oldest and most dangerous bugs in systems programming:
the program reads whatever memory follows the vector, and goes on with garbage, or with someone else's
data. A SultanC program never reads memory outside the vector. When you do not know whether a position
exists, compare it with `length` first.

## Adding and changing values

A vector that changes must be `mutable`. `append` adds a value at the end, and `v[i] := value` replaces
the value at a position:

```sultan
import std.io.println;
import std.text.number;

def main(): int {
    let scores: mutable vector int = [];
    append(scores, 70);
    append(scores, 95);
    append(scores, 82);
    scores[0] := 75;
    println(int_to_text(length(scores)));
    println(int_to_text(scores[0]));
    return 0;
}
```

```text
3
75
```

`[]` alone says nothing about what the vector will hold; the declared type, `vector int`, does.

## Going through every value

A `for` loop over the positions visits every value in order:

```sultan
import std.io.println;
import std.text.number;

def main(): int {
    let prices: vector int = [120, 45, 300, 15];
    let total: mutable int = 0;
    let highest: mutable int = prices[0];
    for (let i: mutable int = 0; i < length(prices); i := i + 1) {
        total := total + prices[i];
        if (prices[i] > highest) {
            highest := prices[i];
        };
    }
    println(int_to_text(total));
    println(int_to_text(highest));
    return 0;
}
```

```text
480
300
```

## Vectors and functions

A vector owns its values, so the ownership rules apply. A function that reads a vector borrows it,
`&vector T`, and reads it through `*`:

```sultan
import std.io.println;
import std.text.number;

def average(values: &vector int): int {
    let total: mutable int = 0;
    for (let i: mutable int = 0; i < length(*values); i := i + 1) {
        total := total + (*values)[i];
    }
    return total / length(*values);
}

def main(): int {
    let marks: vector int = [80, 90, 100];
    println(int_to_text(average(&marks)));
    println(int_to_text(length(marks)));
    return 0;
}
```

```text
90
3
```

A function that changes a vector takes `&mutable vector T`:

```sultan
import std.io.println;
import std.text.number;

def double_all(values: &mutable vector int): void {
    for (let i: mutable int = 0; i < length(*values); i := i + 1) {
        (*values)[i] := (*values)[i] * 2;
    }
}

def main(): int {
    let numbers: mutable vector int = [1, 2, 3];
    double_all(&mutable numbers);
    println(int_to_text(numbers[2]));
    return 0;
}
```

```text
6
```

## Parts of a vector: slices

`values[a..b]` is the part of a vector from position `a` through position `b`, both ends included: a *slice* of
type `[] T`. It is a view of the vector's own values, not a copy. Either end may be left open: `values[2..]` runs to
the last value, `values[..1]` starts at the first, and `values[..]` is all of it. `values[a..a - 1]` is the empty
slice. A function that only reads takes `[] T`, and a vector or any slice of one can be passed to it:

```sultan
import std.io.println;
import std.text.number;

def total(values: [] int): int {
    let sum: mutable int = 0;
    for (let i: mutable int = 0; i < length(values); i := i + 1) {
        sum := sum + values[i];
    }
    return sum;
}

def main(): int {
    let marks: vector int = [10, 20, 30, 40, 50];
    println(int_to_text(total(marks[1..3])));
    println(int_to_text(total(marks[3..])));
    println(int_to_text(total(marks)));
    return 0;
}
```

```text
90
90
150
```

A slice is read-only. To change part of a vector through a slice, ask for it with `&mutable`, as for any change
through a borrow: `&mutable values[2..]` is a writable view, of type `mutable [] T`, and `&mutable values` is a
writable view of the whole vector where a `mutable [] T` is expected:

```sultan
import std.io.println;
import std.text.number;

def fill(part: mutable [] int, value: int): void {
    for (let i: mutable int = 0; i < length(part); i := i + 1) {
        part[i] := value;
    }
}

def main(): int {
    let numbers: mutable vector int = [1, 2, 3, 4, 5];
    fill(&mutable numbers[3..], 0);
    println(int_to_text(numbers[3]));
    fill(&mutable numbers, 7);
    println(int_to_text(numbers[0]));
    return 0;
}
```

```text
0
7
```

Two writable views of one vector at once come from `split_mutable(&mutable values, i)`: the values before position
`i` and those from `i` on. A tuple `let` ([chapter 3](ch03-02-data-types.md#taking-a-tuple-apart)) names the two
halves, and each half is written on its own:

```sultan
def main(): int {
    let numbers: mutable vector int = [1, 2, 3, 4];
    let (left, right): (mutable [] int, mutable [] int) = split_mutable(&mutable numbers, 2);
    left[0] := right[1];
    right[0] := 0;
    return numbers[0] + numbers[2];
}
```

The program returns 4. `mutable [] int` makes each half a writable view; written `[] int`, a half is read-only. While
either half is in use, `numbers` is not used another way; once neither is, it is, as `return` does here. `i` may be
anything from 0 to the length; any other value stops the program, as a position outside the vector does. A `match`
takes the pair apart too: `match split_mutable(&mutable numbers, 2) with | (left, right) -> { … }`.

A slice borrows its vector. While a writable view is in use, the vector is not read, changed or viewed another way;
read-only slices can be used side by side. A slice cannot outlive its vector. A position outside the vector stops
the program with the bound that was wrong, for example `slice end 5 out of range for length 5 at main.sn:4:28`.

## Vectors of owning values

A vector of strings works the same way, with one rule from chapter 4: taking a string out of the vector
would move it out and leave a hole. Printing one does not take it, because `println` only borrows its
text:

```sultan
import std.io.println;

def main(): int {
    let names: mutable vector string = ["Huda", "Adam"];
    append(names, "Zaid");
    for (let i: mutable int = 0; i < length(names); i := i + 1) {
        println(names[i]);
    }
    return 0;
}
```

```text
Huda
Adam
Zaid
```

To keep a string of your own, or to pass one to a function that keeps it, copy it with
`clone(&names[i])`. When the vector goes out of scope, it frees all its strings with it.

## Sorting, removing and inserting

`std.collections.sort` sorts vectors and searches sorted ones; `std.collections.vector` has the other
everyday operations:

| Function | What it does |
| --- | --- |
| `sort_int(&mutable v)`, `sort_uint`, `sort_texts` | sorts from smallest to largest |
| `binary_search_int(&v, value)` | finds a value in a sorted vector: `some(position)` or `none` |
| `remove_at(&mutable v, i)` | removes the value at `i` and returns `some(value)`, or `none` past the end |
| `insert_at(&mutable v, i, value)` | inserts a value at `i`, moving the later ones along |
| `reverse_vector(&mutable v)` | reverses the order |
| `clear_vector(&mutable v)` | removes every value |

```sultan
import std.io.println;
import std.text.number;
import std.text.transform;
import std.collections.sort;
import std.collections.vector;

def main(): int {
    let numbers: mutable vector int = [42, 7, 19, 3];
    sort_int(&mutable numbers);
    println(int_to_text(numbers[0]));
    match remove_at(&mutable numbers, 0) with
    | option.some(smallest) -> { println(int_to_text(smallest)); }
    | option.none -> { println("empty"); };
    println(int_to_text(length(numbers)));
    let cities: mutable vector string = ["Riyadh", "Cairo", "Amman"];
    sort_texts(&mutable cities);
    println(join(&cities, &", "));
    return 0;
}
```

```text
3
3
3
Amman, Cairo, Riyadh
```

`remove_at` returns an `option`, because the position may not exist. `join`, from `std.text.transform`,
joins a vector of strings with a separator between them.

## Vectors of vectors

The values of a vector can be vectors themselves, which makes a table: `grid[row][column]`:

```sultan
import std.io.println;
import std.text.number;

def main(): int {
    let grid: vector vector int = [[1, 2, 3], [4, 5, 6]];
    println(int_to_text(grid[1][2]));
    println(int_to_text(length(grid[0])));
    return 0;
}
```

```text
6
3
```
