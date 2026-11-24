[العربية](../../ar/book/ch12-01-accepting-arguments.md) | **English**

# Accepting Command Line Arguments

Create the project:

```console
$ supax new finder
$ cd finder
```

The program will be run with two *arguments*, the word to look for and the file to look in. With Supax,
the arguments for the program come after `--`, so that Supax does not read them as its own:

```console
$ supax run -- needle haystack.txt
```

## Reading the arguments

`std.process.argument` has `argument(i)`, which returns the argument at position `i` as an
`option string`: `some(text)`, or `none` when there is no argument at that position.
`std.process.argument_count` has `argument_count()`, the number of arguments. Start by printing them
all:

<sub>Listing 12-1: printing every argument. Filename: src/main.sn</sub>

```sultan ignore
import std.io.println;
import std.process.argument;
import std.process.argument_count;

def main(): int {
    let count: uint = argument_count();
    for (let i: mutable uint = 0; i < count; i := i + 1) {
        match argument(i) with
        | option.some(text) -> { println(text); }
        | option.none -> { };
    }
    return 0;
}
```

```console
$ supax run -- needle haystack.txt
   Compiling finder v0.1.0 (/projects/finder)
    Finished in 0.11s → build/bin/finder
     Running build/bin/finder
/projects/finder/build/bin/finder
needle
haystack.txt
```

Position 0 holds the path of the program itself; the arguments you typed start at position 1.

## Saving the arguments in variables

The program needs two arguments, and should say how to use it when they are missing. Each comes out of
its `option` with a `match`; a missing one prints a usage line and ends the program with status 2, the
usual status for a program called the wrong way:

<sub>Listing 12-2: the query and the file path. Filename: src/main.sn</sub>

```sultan ignore
import std.io.println;
import std.process.argument;
import std.text.transform;

def main(): int {
    let query: string;
    match argument(1) with
    | option.some(text) -> { query := text; }
    | option.none -> { println("usage: finder QUERY FILE"); return 2; };
    let file_path: string;
    match argument(2) with
    | option.some(text) -> { file_path := text; }
    | option.none -> { println("usage: finder QUERY FILE"); return 2; };

    println(concat(&"Searching for ", &query));
    println(concat(&"In file ", &file_path));
    return 0;
}
```

This uses the "declare now, give the value later" pattern from chapter 3: `query` gets its value in one
branch, and the other branch leaves the function.
