[العربية](../../ar/book/ch03-04-comments.md) | **English**

# Comments

A comment is a note for the people who read the code. The compiler ignores it.

A comment starts with `//` and runs to the end of the line:

```sultan
import std.io.println;

// Prints the opening hours of the shop.
def main(): int {
    println("Open from 9 to 5"); // weekdays only
    return 0;
}
```

```text
Open from 9 to 5
```

A comment can stand on its own line, usually above the code it explains, or after code on the same
line. A comment longer than one line is several lines that each start with `//`:

```sultan ignore
// The tax is computed on the price after the discount,
// as the law of the country of sale requires.
```

A good comment says *why* the code is written as it is: the reason for a choice, the rule it follows,
the case it guards against. What the code does, the code itself should say, through clear names.

SultanC has only line comments. There is no form that starts and ends inside a line.
