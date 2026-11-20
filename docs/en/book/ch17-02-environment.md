[العربية](../../ar/book/ch17-02-environment.md) | **English**

# The Environment

Every program starts with an *environment*: its command line, a set of named variables, and a current
directory. Chapter 12 used the first two; here they are together.

## The command line

`arguments()`, from `std.process.arguments`, returns the whole command line as a vector of strings, with
the program itself at position 0. Chapter 12 wrote the same function by hand from `argument` and
`argument_count`:

```sultan
import std.io.println;
import std.text.number;
import std.process.arguments;

def main(): int {
    let all: vector string = arguments();
    println(int_to_text(length(all) - 1));
    return 0;
}
```

```text
0
```

Started without arguments, the program prints 0.

## Variables

`get_environment_variable(&name)` reads a variable. It returns `result option string`: an error if the
environment cannot be read, `none` if the variable is not set, and `some(value)` otherwise:

```sultan
import std.io.println;
import std.env.environment;

def main(): int {
    match get_environment_variable(&"PATH") with
    | result.ok(option.some(_)) -> { println("PATH is set"); }
    | result.ok(option.none) -> { println("PATH is not set"); }
    | result.err(_) -> { println("the environment could not be read"); };
    return 0;
}
```

```text
PATH is set
```

Variables are the usual way to give a program settings that should not be typed on every command line:
where to keep its data, which language to speak, whether to print more detail.

## The current directory

`current_directory()` returns the folder that relative paths are resolved from, and
`temporary_directory()` the system's folder for temporary files. Both return a `result`:

```sultan
import std.io.println;
import std.env.environment;

def main(): int {
    match current_directory() with
    | result.ok(_) -> { println("the current directory is known"); }
    | result.err(code) -> { return code; };
    return 0;
}
```

```text
the current directory is known
```
