[العربية](../../ar/book/appendix-02-operators.md) | **English**

# B. Operators and Symbols

The operators and symbols of SultanC, with the chapter that explains each. They are the same in Arabic
and English programs; only `?` has an Arabic spelling too, `؟`.

## Operators

| Operator | Example | Meaning | Chapter |
| --- | --- | --- | --- |
| `+` `-` `*` `/` | `a + b` | arithmetic; integer `/` rounds toward zero | [3.2](ch03-02-data-types.md) |
| `%` | `a % b` | remainder, with the sign of `a` | [3.2](ch03-02-data-types.md) |
| `-` | `-a` | negation | [3.2](ch03-02-data-types.md) |
| `==` `!=` | `a == b` | equal, not equal | [3.2](ch03-02-data-types.md) |
| `<` `<=` `>` `>=` | `a < b` | order comparisons | [3.2](ch03-02-data-types.md) |
| `&&` `\|\|` | `a && b` | and, or; the right side is evaluated only when needed | [3.2](ch03-02-data-types.md) |
| `!` | `!a` | not | [3.2](ch03-02-data-types.md) |
| `&` `\|` `^` | `a & b` | bitwise and, or, exclusive or | [3.2](ch03-02-data-types.md) |
| `~` | `~a` | bitwise not | [3.2](ch03-02-data-types.md) |
| `<<` | `a << n` | shift left | [3.2](ch03-02-data-types.md) |
| `>>` | `a >> n` | logical shift right: fills with zeros | [3.2](ch03-02-data-types.md) |
| `>>>` | `a >>> n` | arithmetic shift right: keeps the sign | [3.2](ch03-02-data-types.md) |
| `as` | `a as u8` | number conversion | [3.2](ch03-02-data-types.md) |
| `:=` | `x := 5;` | assignment to a `mutable` variable | [3.1](ch03-01-variables-and-mutability.md) |
| `&` | `&x` | a reference that reads | [4.2](ch04-02-references-and-borrowing.md) |
| `&mutable` | `&mutable x` | a reference that may change | [4.2](ch04-02-references-and-borrowing.md) |
| `*` | `*r` | the value a reference or pointer points at | [4.2](ch04-02-references-and-borrowing.md) |
| `?` | `f()?` | gives the value of `ok`, or returns the error | [9.2](ch09-02-recoverable-errors.md) |

## Symbols

| Symbol | Example | Meaning | Chapter |
| --- | --- | --- | --- |
| `:` | `let x: int` | introduces a type | [3.1](ch03-01-variables-and-mutability.md) |
| `=` | `let x: int = 5;` | the value in a declaration | [3.1](ch03-01-variables-and-mutability.md) |
| `;` | `x := 5;` | ends a statement | [1.2](ch01-02-hello-world.md) |
| `{ }` | `{ x: 1, y: 2 }` | a block; a record type or value | [3.5](ch03-05-control-flow.md), [5.1](ch05-01-defining-records.md) |
| `( )` | `f(x)`, `(1, 2)` | a call; a tuple; grouping | [3.3](ch03-03-how-functions-work.md) |
| `[ ]` | `[1, 2]`, `v[0]` | a vector; a position | [8.1](ch08-01-vectors.md) |
| `.` | `p.x`, `std.io` | a field; a path | [5.1](ch05-01-defining-records.md), [7.3](ch07-03-import-forms.md) |
| `,` | `f(a, b)` | separates items | |
| `\|` | `\| Red \| Green` | starts an enum case or a `match` branch; "or" in a pattern | [6.1](ch06-01-defining-an-enum.md) |
| `->` | `\| 0 -> { … }` | introduces a `match` branch's block, or an anonymous function's body | [6.2](ch06-02-match.md), [10.3](ch10-03-closures.md) |
| `_` | `\| _ -> { }` | a pattern that matches anything | [3.5](ch03-05-control-flow.md) |
| `< >` | `Stack<T>` | type parameters | [10.1](ch10-01-generic-data-types.md) |
| `" "` | `"text\n"` | a string, with escapes | [8.2](ch08-02-strings.md) |
| `« »` | `«C:\files»` | a string without escapes | [8.2](ch08-02-strings.md) |
| `' '` | `'a'` | a character | [3.2](ch03-02-data-types.md) |
| `//` | `// note` | a comment, to the end of the line | [3.4](ch03-04-comments.md) |
| `@@` | `@@test` | an annotation on the next declaration | [11.1](ch11-01-writing-tests.md) |
| `0x` `0b` | `0xFF`, `0b1010` | hexadecimal and binary integers | [3.2](ch03-02-data-types.md) |
| `_` in numbers | `1_000_000` | separates digits | [3.2](ch03-02-data-types.md) |
