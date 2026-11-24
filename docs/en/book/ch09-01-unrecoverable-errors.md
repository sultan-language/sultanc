[العربية](../../ar/book/ch09-01-unrecoverable-errors.md) | **English**

# Unrecoverable Errors with `panic`

Sometimes a program reaches a state that should be impossible. When that happens, the honest thing is to
stop, and say where and why.

## `panic`

`panic(message)`, from `std.process.panic`, stops the program with a message:

```sultan exit=1
import std.io.println;
import std.process.panic;

def main(): int {
    println("starting");
    panic("the configuration is missing");
    println("this line never runs");
    return 0;
}
```

The program prints what it printed before the panic on standard output:

```text
starting
```

and the reason on *standard error*, the separate output for problems, then exits with status 1:

```text stderr
error[SULTANC-RUNTIME-PROGRAM-STOPPED]: the program stopped
the configuration is missing
```

The first line is written by the standard library, in the language of the environment the program runs
in. The second is your message. Because it goes to standard error, a user who redirects the program's
output to a file still sees it.

## Assertions

`assert(condition)`, from `std.debug.assert`, stops the program when a condition is false.
`assert_with_message(condition, &message)` does the same with your explanation. An assertion states
something that must be true, and checks it every time the program runs:

```sultan
import std.io.println;
import std.debug.assert;

def factorial(n: int): int {
    if (n <= 1) {
        return 1;
    };
    return n * factorial(n - 1);
}

def main(): int {
    assert(factorial(5) == 120);
    let message: string = "factorial(6) must be 720";
    assert_with_message(factorial(6) == 720, &message);
    println("all checks passed");
    return 0;
}
```

```text
all checks passed
```

When one fails, the program stops and says where:

```sultan exit=1
import std.debug.assert;

def main(): int {
    let balance: int = -5;
    assert(balance >= 0);
    return 0;
}
```

```text stderr
error[SULTANC-RUNTIME-PROGRAM-STOPPED]: the program stopped — An assertion failed.
  --> main.sn:5:5
```

`main.sn:5:5` is the file, line and column of the failed assertion.

## Stops you did not write

You met a third kind in chapter 3: integer overflow, division by zero, and a position past the end of a
vector stop the program too. These are checks the language makes on every such operation, because
continuing with a wrong number or with memory that is not yours is never right. They stop the program
the way `panic` does, with a message that says what went wrong and where:

```sultan exit=1
import std.io.println;
import std.text.number;

def average(total: int, count: int): int {
    return total / count;
}

def main(): int {
    println(int_to_text(average(10, 0)));
    return 0;
}
```

```text stderr
error[SULTANC-RUNTIME-PROGRAM-STOPPED]: the program stopped
division by zero at main.sn:5:12
```

`main.sn:5:12` is where `total / count` starts. An overflow reads `integer overflow at …`, and a
position past the end `index 5 out of range for length 3 at …`.
