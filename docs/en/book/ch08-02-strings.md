[العربية](../../ar/book/ch08-02-strings.md) | **English**

# Text, Bytes and Characters

A `string` holds text of any length, in any script. Text looks simple, and is not: a program has to know
what a character is, how it is stored, and how many bytes it takes. This section shows how SultanC
answers those questions, and the everyday functions for working with text.

The functions come from modules under `std.text`:

| Module | What it has |
| --- | --- |
| `std.text.transform` | `concat`, `concat3`, `join`, `split`, `trim`, `replace`, `lines` |
| `std.text.number` | `int_to_text`, `uint_to_text`, `parse_int`, `parse_uint` |
| `std.text.float` | `f64_to_string`, `parse_f64` |
| `std.text.search` | `contains`, `starts_with`, `ends_with`, `find`, `is_empty`, `char_count` |
| `std.text.compare` | `texts_equal`, `compare_texts_lexicographically` |
| `std.text.substring` | `substring` |
| `std.text.builder` | `text_builder`, for building long text piece by piece |

## Writing text

A string literal goes between double quotes. Inside it, a backslash starts an *escape*, a way to write a
character that cannot be typed directly:

| Escape | Means |
| --- | --- |
| `\n` | a new line |
| `\t` | a tab |
| `\"` | a double quote |
| `\\` | a backslash |

Text can also be written between `«` and `»`. Nothing inside these is an escape, and a `"` needs none,
which suits text full of quotes or backslashes:

```sultan
import std.io.println;

def main(): int {
    println("first line\nsecond line");
    println("a \"quoted\" word, and a backslash: \\");
    println(«no escapes here: \t "quotes" stay as they are»);
    return 0;
}
```

```text
first line
second line
a "quoted" word, and a backslash: \
no escapes here: \t "quotes" stay as they are
```

## Text owns its contents

A `string` is an owning value: its characters live on the heap. Everything from chapter 4 applies.
Passing a string to a function that takes `string` moves it, so functions that only read text take
`&string`. `println` is one of them: a `string` passed where `&string` is expected is borrowed for the
call, so `println(greeting)` leaves `greeting` usable.

## Joining text

`+` adds numbers only. To join text, use `concat` for two pieces, `concat3` for three, and `join` for a
vector of pieces with a separator between them:

```sultan
import std.io.println;
import std.text.transform;

def main(): int {
    let first: string = "Layla";
    let last: string = "Hassan";
    println(concat3(&first, &" ", &last));
    let words: vector string = ["red", "green", "blue"];
    println(join(&words, &", "));
    println(first);
    return 0;
}
```

```text
Layla Hassan
red, green, blue
Layla
```

They take their arguments by reference and return a new string, so `first` and `last` are still usable
afterwards.

## Numbers in text, and text in numbers

`int_to_text` and `uint_to_text` write a number as decimal text, and `f64_to_string` a floating-point
number. In the other direction, `parse_int` reads a number from text. Not every text is a number, so it
returns a `result`:

```sultan
import std.io.println;
import std.text.number;

def main(): int {
    match parse_int(&"-42") with
    | result.ok(number) -> { println(int_to_text(number * 2)); }
    | result.err(_) -> { println("not a number"); };
    match parse_int(&"4x2") with
    | result.ok(number) -> { println(int_to_text(number)); }
    | result.err(_) -> { println("not a number"); };
    return 0;
}
```

```text
-84
not a number
```

## Comparing text

`==` and `!=` compare two strings by their contents: they are equal when they hold the same characters.
Neither side is moved, so a string stays usable after a comparison. For their dictionary order,
`compare_texts_lexicographically`, from `std.text.compare`, gives a negative number if the first comes
first, zero if they are equal, a positive number otherwise. `<` and `>` are not defined for text:

```sultan
import std.io.println;
import std.text.compare;

def main(): int {
    let answer: string = "yes";
    if (answer == "yes") {
        println("confirmed");
    };
    if (compare_texts_lexicographically(&"apple", &"banana") < 0) {
        println("apple comes first");
    };
    return 0;
}
```

```text
confirmed
apple comes first
```

`std.text.compare` also has `texts_equal(&a, &b)`, the same test as `==` written as a function, for
code that needs the comparison as a function value.

A `match` on a string, as in chapter 3, also compares texts.

## Bytes and characters

How long is `"سلطان"`? It depends on what you count.

SultanC stores text in *UTF-8*, the encoding the web and most systems use. In UTF-8, a Latin letter takes
one byte, an Arabic letter two, and some characters, like many emoji, four. `length` counts bytes, the
storage the text takes. `char_count` counts characters:

```sultan
import std.io.println;
import std.text.number;
import std.text.search;

def main(): int {
    let english: string = "Sultan";
    let arabic: string = "سلطان";
    println(int_to_text(length(english)));
    println(int_to_text(length(arabic)));
    println(uint_to_text(char_count(&arabic)));
    return 0;
}
```

```text
6
10
5
```

Positions in a string are byte positions too. `text[0]` is the first byte, of type `u8`, not the first
character:

```sultan
import std.io.println;
import std.text.number;

def main(): int {
    let arabic: string = "سلطان";
    let first: u8 = arabic[0];
    println(uint_to_text(first as uint));
    return 0;
}
```

```text
216
```

216 is the first of the two bytes of `س`, not a character at all. That is why SultanC does not give a
string's positions as characters: finding the tenth character means reading the nine before it, since
each may have a different width, and a position that looked cheap would quietly cost a walk through the
text. Byte positions are always cheap, and the functions that work with characters, like `char_count`,
say so in their names.

## Searching

```sultan
import std.io.println;
import std.text.number;
import std.text.search;

def main(): int {
    let sentence: string = "the quick brown fox";
    if (contains(&sentence, &"quick")) { println("has quick"); };
    if (starts_with(&sentence, &"the")) { println("starts with the"); };
    if (ends_with(&sentence, &"fox")) { println("ends with fox"); };
    match find(&sentence, &"brown") with
    | option.some(position) -> { println(uint_to_text(position)); }
    | option.none -> { println("not found"); };
    return 0;
}
```

```text
has quick
starts with the
ends with fox
10
```

`find` returns `some` with the byte position of the first match, or `none`.

## Cutting text apart

`trim` removes white space from both ends: spaces, tabs, new lines and the other Unicode space
characters. `split` cuts the text at every separator and returns a vector of the pieces. `replace` swaps
every occurrence of one text for another. `substring` takes the part that starts at a byte position and
has a byte length, and returns an `option`, which is empty when the range goes past the end:

```sultan
import std.io.println;
import std.text.number;
import std.text.transform;
import std.text.substring;

def main(): int {
    let line: string = "  red, green, blue  ";
    let clean: string = trim(&line);
    let colors: vector string = split(&clean, &", ");
    println(int_to_text(length(colors)));
    println(colors[1]);
    println(replace(&clean, &"green", &"gold"));
    match substring(&clean, 5, 5) with
    | option.some(part) -> { println(part); }
    | option.none -> { println("out of range"); };
    return 0;
}
```

```text
3
green
red, gold, blue
green
```

## Building long text

Joining pieces one by one with `concat` copies everything gathered so far at each step. For text built
from many pieces, use a *text builder*, which collects the pieces and makes one string at the end:

```sultan
import std.io.println;
import std.text.builder;
import std.text.number;

def main(): int {
    let builder: mutable text_builder = new_text_builder();
    for (let i: mutable int = 1; i <= 5; i := i + 1) {
        let added: bool = text_builder_append_text(&mutable builder, &int_to_text(i));
        if (i < 5) {
            let comma: bool = text_builder_append_text(&mutable builder, &", ");
        };
    }
    println(text_builder_text(&builder));
    return 0;
}
```

```text
1, 2, 3, 4, 5
```

The append functions return `false` if memory runs out; this example ignores that, by storing the
answers in variables it does not use.
