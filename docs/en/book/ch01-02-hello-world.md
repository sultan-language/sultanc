[العربية](../../ar/book/ch01-02-hello-world.md) | **English**

# Hello, World!

Make a folder for the programs in this book, and a folder inside it for this one:

```console
$ mkdir -p projects/hello_world
$ cd projects/hello_world
```

Create a file named `main.sn`. SultanC source files end in `.sn`. Put this program in it:

<sub>Filename: main.sn</sub>

```sultan
import std.io.println;

def main(): int {
    println("Hello, world!");
    return 0;
}
```

Compile it, then run the program that the compiler made:

```console
$ sultanc main.sn -o hello
$ ./hello
```

```text
Hello, world!
```

On Windows, name the program `hello.exe` and run it with `.\hello.exe`.

If you see `Hello, world!`, you have written and run a SultanC program.

## Anatomy of the program

```sultan ignore
def main(): int {

}
```

This defines a function named `main`. `def` starts a function definition. The empty parentheses mean
`main` takes no input. `: int` says what the function gives back: an `int`, a whole number. The body
of the function goes between the braces `{` and `}`.

`main` is special: it is where every program starts. When `main` finishes, the number it returns
becomes the program's exit status. That is how a program tells the system that called it whether it
succeeded: 0 means success, and any other number means something went wrong.

```sultan ignore
    println("Hello, world!");
```

This line calls the function `println`, which prints a piece of text followed by a new line. The text
is a string literal: characters between double quotes. The line ends with a semicolon `;`. In SultanC,
every statement ends with one.

```sultan ignore
    return 0;
```

`return` ends the function and gives back a value, here 0: the program succeeded.

```sultan ignore
import std.io.println;
```

`println` is not built into the language. It is part of the standard library, `std`, the collection of
modules that comes with the compiler. `std.io.println` is the module that provides it: `io` holds input
and output. An `import` line makes a module's public names usable in your file. Each program in this
book starts with the imports it needs.

## When something is wrong

The compiler checks the whole program before it produces anything. When it finds a mistake, it says
what is wrong, where, and often how to fix it. Remove the semicolon after `println(…)` and compile
again:

```sultan fail
import std.io.println;

def main(): int {
    println("Hello, world!")
    return 0;
}
```

```text
error[SULTANC-PARSE-SYNTAX]: syntax error: expected `;`, found `return` — The parser expected a different token at this location.
  --> main.sn:4:29
   |
 4 |     println("Hello, world!")
   |                             ^ expected here
   |
  ::: main.sn:5:5..5:11
   |
 5 |     return 0;
   |     ^^^^^^ parser discovered the problem here
   |
  = help: insert `;` after this construct
```

Every message has the same parts:

- `error` and, in brackets, the message's identity, `SULTANC-PARSE-SYNTAX`. The identity never changes,
  so you can search for it. `sultanc explain SULTANC-PARSE-SYNTAX` describes it.
- What is wrong, then a dash and a longer explanation.
- `-->` and the place: file, line and column. `main.sn:4:29` is line 4, column 29.
- The line of code itself, with `^` under the exact spot.
- Sometimes a second place, after `:::`. Here it is where the compiler noticed the problem: it only
  knew the semicolon was missing when it reached `return`.
- `help`, when the compiler knows the fix.

Put the semicolon back and remove the `import` line instead:

```sultan fail
def main(): int {
    println("Hello, world!");
    return 0;
}
```

```text
error[SULTANC-NAME-UNKNOWN]: unknown name: `println` — The required name was not found during resolution.
  --> main.sn:2:5..2:12
   |
 2 |     println("Hello, world!");
   |     ^^^^^^^
   |
```

Without the import, `println` is a name the compiler does not know. `2:5..2:12` is a range: from line
2, column 5, to line 2, column 12.

The compiler writes its messages in Arabic by default. To get them in English, add `--lang=en`:

```console
$ sultanc --lang=en main.sn -o hello
```

## Compiling and running are two steps

`sultanc main.sn -o hello` produced a file, `hello`, that runs on its own. You can give it to someone
who does not have SultanC installed, and it runs on their computer if it has the same operating system
and processor. This is what *compiling ahead of time* means: all the checking and translating happens
once, before the program runs.

To try a program without making a file, use `sultanc run`:

```console
$ sultanc run main.sn
Hello, world!
```

`sultanc run` executes the program in the compiler's interpreter. It runs the same checked program, so
it behaves the same as the compiled one.
