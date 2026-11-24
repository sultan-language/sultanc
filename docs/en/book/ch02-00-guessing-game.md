[العربية](../../ar/book/ch02-00-guessing-game.md) | **English**

# Programming a Guessing Game

This chapter builds a whole program at once. The program picks a whole number between 1 and 100 and
asks you to guess it. After each guess it says whether the guess is too small or too big, and it
ends when you guess right.

On the way you meet `let`, `match`, `result`, `option`, references, loops and the standard library.
Each gets only a short explanation here; the following chapters cover them fully. The goal of this
chapter is to see how the pieces work together in a real program, and to practise reading what the
compiler tells you.

## Setting up the project

Create a project with Supax, in your `projects` folder:

```console
$ supax new guessing_game
created package "guessing_game"
$ cd guessing_game
```

Supax has written the same starting program as in chapter 1. Run it to check that everything works:

```console
$ supax run
   Compiling guessing_game v0.1.0 (/projects/guessing_game)
    Finished in 0.19s → build/bin/guessing_game
     Running build/bin/guessing_game
Hello from SultanC
```

## Reading a guess

The first part of the game asks for a guess, reads it, and prints it back. Replace the contents of
`src/main.sn` with Listing 2-1.

<sub>Listing 2-1: reading a guess from the user and printing it. Filename: src/main.sn</sub>

```sultan stdin=42\n
import std.io.println;
import std.io.read_line;
import std.text.transform;

def main(): int {
    println("Guess the number!");
    println("Please input your guess.");

    let guess: string;
    match read_line() with
    | result.ok(line) -> { guess := trim(&line); }
    | result.err(_) -> {
        println("Failed to read line");
        return 1;
    };

    println(concat(&"You guessed: ", &guess));
    return 0;
}
```

Run it, type `42` and press Enter:

```console
$ supax run
   Compiling guessing_game v0.1.0 (/projects/guessing_game)
    Finished in 0.16s → build/bin/guessing_game
     Running build/bin/guessing_game
Guess the number!
Please input your guess.
42
You guessed: 42
```

Much is happening in these lines. Let's go through them.

### Imports

```sultan ignore
import std.io.println;
import std.io.read_line;
import std.text.transform;
```

The program uses three standard library modules: `println` to print, `read_line` to read a line that
the user types, and `std.text.transform` for `trim` and `concat`, two functions that work on text.

### Storing a value: `let`

```sultan ignore
    let guess: string;
```

`let` creates a *variable*: a name for a value. This line creates a variable named `guess` whose type
is `string`, a piece of text. In SultanC the type of a variable is always written, after a colon.

This `let` has no value yet. The value comes a few lines later, from what the user types. The compiler
checks that every path through the program gives `guess` a value before the program uses it, and that
it gets a value only once. A variable whose value never changes is the default in SultanC; chapter 3
shows how to make one that changes.

### Reading a line, and handling failure

```sultan ignore
    match read_line() with
    | result.ok(line) -> { guess := trim(&line); }
    | result.err(_) -> {
        println("Failed to read line");
        return 1;
    };
```

`read_line()` reads one line from the program's input: the keyboard, here. Reading can fail, for
example if the input was closed. So `read_line` does not return the text directly. It returns a
`result`, a value that is one of two things:

- `result.ok(…)` holds the text, if reading worked;
- `result.err(…)` holds a description of the error, if it did not.

`match` looks at a value and runs the branch that fits it. Each branch starts with `|`, then a
*pattern*, then `->` and the code to run.

- The pattern `result.ok(line)` matches a successful read, and gives the text inside it the name
  `line`. The branch assigns a value to `guess` with `:=`.
- The pattern `result.err(_)` matches a failure. `_` means "anything; I don't need its value". This
  branch prints a message and returns 1 from `main`, so the program ends with a status that means
  failure.

The compiler does not let you forget the second branch. A `match` must handle every possible value, so
a failure cannot go unnoticed. The whole `match` ends with `;`, like every statement.

### Removing the new line: `trim` and `&`

When you press Enter, the line you typed ends with a new line character. `read_line` keeps it: the text
it returned for `42` is `"42\n"`, where `\n` is how a new line is written inside a string. `trim` gives
back the text without spaces and new lines at its start and end, so `guess` holds `"42"`.

`trim(&line)` passes `&line`, not `line`. `&` makes a *reference*: it lets `trim` read `line` without
taking it. Chapter 4 explains why this matters: in SultanC each value has one owner, and a reference
lets a function use a value that still belongs to someone else.

### Printing the guess

```sultan ignore
    println(concat(&"You guessed: ", &guess));
```

`concat` joins two strings into a new one. Both arguments are references, because `concat` only reads
them. The new string goes straight to `println`.

## Generating a secret number

Next the game needs a number to guess, a different one each time. A program that picks numbers
"at random" uses a *pseudorandom number generator*: a calculation that produces a sequence of numbers
which look random. The sequence depends on a starting value called the *seed*. The same seed always
gives the same sequence, so the program needs a seed that changes every time it runs. The current time,
in nanoseconds, does that.

Listing 2-2 adds both.

<sub>Listing 2-2: generating a secret number. Filename: src/main.sn</sub>

```sultan stdin=50\n
import std.io.println;
import std.io.read_line;
import std.text.number;
import std.text.transform;
import std.random.prng;
import std.time.time;

def main(): int {
    println("Guess the number!");

    let seed: uint;
    match system_time_ns() with
    | result.ok(nanoseconds) -> { seed := nanoseconds as uint; }
    | result.err(_) -> { seed := 1; };
    let generator: mutable prng = new_prng(seed);
    let secret_number: uint;
    match prng_range(&mutable generator, 1, 101) with
    | option.some(n) -> { secret_number := n; }
    | option.none -> { return 1; };

    println(concat(&"The secret number is: ", &uint_to_text(secret_number)));

    println("Please input your guess.");
    let guess: string;
    match read_line() with
    | result.ok(line) -> { guess := trim(&line); }
    | result.err(_) -> {
        println("Failed to read line");
        return 1;
    };

    println(concat(&"You guessed: ", &guess));
    return 0;
}
```

Run it a few times. The secret number changes each time:

```console
$ supax run
   Compiling guessing_game v0.1.0 (/projects/guessing_game)
    Finished in 0.19s → build/bin/guessing_game
     Running build/bin/guessing_game
Guess the number!
The secret number is: 28
Please input your guess.
50
You guessed: 50
```

### The seed

```sultan ignore
    let seed: uint;
    match system_time_ns() with
    | result.ok(nanoseconds) -> { seed := nanoseconds as uint; }
    | result.err(_) -> { seed := 1; };
```

`system_time_ns()`, from `std.time.time`, returns the current time as the number of nanoseconds since
1 January 1970. Asking the system for the time can fail, so it also returns a `result`. If it fails,
the program uses 1 as the seed: the game still works, with the same secret number every time.

`nanoseconds` is an `int`, which can be negative. The generator wants a `uint`: an unsigned integer,
which is never negative. `as uint` converts one to the other. SultanC never converts between number
types by itself; you always write the conversion.

### The generator

```sultan ignore
    let generator: mutable prng = new_prng(seed);
```

`new_prng`, from `std.random.prng`, creates a generator of type `prng` from the seed. The type is
written `mutable prng` because the generator changes each time it produces a number: it moves to the
next number in its sequence. A variable can only change if its type says `mutable`.

This generator is fine for games and simulations. It is not suitable for passwords or encryption keys,
because anyone who knows the seed can compute its numbers.

### Picking the number

```sultan ignore
    let secret_number: uint;
    match prng_range(&mutable generator, 1, 101) with
    | option.some(n) -> { secret_number := n; }
    | option.none -> { return 1; };
```

`prng_range(&mutable generator, 1, 101)` produces a number from 1 up to, but not including, 101: so
from 1 to 100. It takes `&mutable generator`, a reference that is allowed to change what it refers to.
Writing `&mutable` at the call shows anyone reading the code that this call changes `generator`.

The function returns an `option`: `option.some(n)` holds the number, and `option.none` means there was
none. That happens only when the range is empty, for example `prng_range(&mutable generator, 5, 5)`. It
cannot happen here, but `match` still requires the branch, and returning 1 is a reasonable answer.

`result` and `option` are similar. Use `result` when something can fail and the failure has a reason;
use `option` when a value may simply be absent.

The program prints the secret number for now, so that you can test it. The final version will not.

## Comparing the guess to the secret number

Now that there is a guess and a secret number, the program can compare them. Listing 2-3 adds a first
comparison after printing the guess. It does not compile yet.

<sub>Listing 2-3: comparing the guess with the secret number. Filename: src/main.sn</sub>

```sultan fail
import std.io.println;
import std.io.read_line;
import std.text.number;
import std.text.transform;
import std.random.prng;
import std.time.time;

def main(): int {
    println("Guess the number!");

    let seed: uint;
    match system_time_ns() with
    | result.ok(nanoseconds) -> { seed := nanoseconds as uint; }
    | result.err(_) -> { seed := 1; };
    let generator: mutable prng = new_prng(seed);
    let secret_number: uint;
    match prng_range(&mutable generator, 1, 101) with
    | option.some(n) -> { secret_number := n; }
    | option.none -> { return 1; };

    println(concat(&"The secret number is: ", &uint_to_text(secret_number)));

    println("Please input your guess.");
    let guess: string;
    match read_line() with
    | result.ok(line) -> { guess := trim(&line); }
    | result.err(_) -> {
        println("Failed to read line");
        return 1;
    };

    println(concat(&"You guessed: ", &guess));
    if (guess < secret_number) {
        println("Too small!");
    };
    return 0;
}
```

```console
$ supax build
   Compiling guessing_game v0.1.0 (/projects/guessing_game)
```

```text
error[SULTANC-TYPE-MISMATCH]: expected `string`, found `uint` — The two operands of this operator have different types.
  --> /projects/guessing_game/src/main.sn:33:17..33:30
    |
 33 |     if (guess < secret_number) {
    |                 ^^^^^^^^^^^^^
    |
```

The message says that the two sides of `<` have different types: `guess` is a `string` and
`secret_number` is a `uint`. The text `"42"` and the number 42 are different things. One is two
characters, the other a quantity, and SultanC does not guess which one you meant. To compare them, the
program must first turn the text into a number.

Listing 2-4 does that, then compares.

<sub>Listing 2-4: turning the guess into a number and comparing it. Filename: src/main.sn</sub>

```sultan stdin=50\n
import std.io.println;
import std.io.read_line;
import std.text.number;
import std.text.transform;
import std.random.prng;
import std.time.time;

def main(): int {
    println("Guess the number!");

    let seed: uint;
    match system_time_ns() with
    | result.ok(nanoseconds) -> { seed := nanoseconds as uint; }
    | result.err(_) -> { seed := 1; };
    let generator: mutable prng = new_prng(seed);
    let secret_number: uint;
    match prng_range(&mutable generator, 1, 101) with
    | option.some(n) -> { secret_number := n; }
    | option.none -> { return 1; };

    println(concat(&"The secret number is: ", &uint_to_text(secret_number)));

    println("Please input your guess.");
    let line: string;
    match read_line() with
    | result.ok(text) -> { line := text; }
    | result.err(_) -> {
        println("Failed to read line");
        return 1;
    };

    let guess: uint;
    match parse_uint(&trim(&line)) with
    | result.ok(n) -> { guess := n; }
    | result.err(_) -> {
        println("Please type a number!");
        return 1;
    };

    println(concat(&"You guessed: ", &uint_to_text(guess)));
    if (guess < secret_number) {
        println("Too small!");
    } else if (guess > secret_number) {
        println("Too big!");
    } else {
        println("You win!");
    };
    return 0;
}
```

### Turning text into a number

```sultan ignore
    let guess: uint;
    match parse_uint(&trim(&line)) with
    | result.ok(n) -> { guess := n; }
    | result.err(_) -> {
        println("Please type a number!");
        return 1;
    };
```

`parse_uint`, from `std.text.number`, reads a `uint` from text. The user can type anything, and
`"hello"` is not a number, so `parse_uint` returns a `result` as well: `result.ok(n)` with the number, or
`result.err(…)` if the text is not one.

The line read from the input is now called `line`, and `guess` is the number. `trim(&line)` produces the
trimmed text, and `&` passes a reference to it on to `parse_uint`.

To print the guess, `uint_to_text` turns the number back into text.

### Comparing

```sultan ignore
    if (guess < secret_number) {
        println("Too small!");
    } else if (guess > secret_number) {
        println("Too big!");
    } else {
        println("You win!");
    };
```

`if` runs its block when the condition in parentheses is true. `else if` tries another condition when
the first one was false, and `else` runs when none were true. Exactly one of the three messages is
printed. Like `match`, the whole `if` is a statement and ends with `;`.

Try it with a number, and with text that is not a number:

```console
$ supax run
     Running build/bin/guessing_game
Guess the number!
The secret number is: 83
Please input your guess.
50
You guessed: 50
Too small!
$ supax run
     Running build/bin/guessing_game
Guess the number!
The secret number is: 82
Please input your guess.
abc
Please type a number!
```

From here on, the `Compiling` and `Finished` lines are left out of the outputs.

## Allowing several guesses with a loop

One guess is not much of a game. `while (true) { … }` repeats its block forever, so Listing 2-5 puts
everything from the question to the comparison inside one.

<sub>Listing 2-5: repeating the question in a loop. Filename: src/main.sn</sub>

```sultan stdin=50\nquit\n
import std.io.println;
import std.io.read_line;
import std.text.number;
import std.text.transform;
import std.random.prng;
import std.time.time;

def main(): int {
    println("Guess the number!");

    let seed: uint;
    match system_time_ns() with
    | result.ok(nanoseconds) -> { seed := nanoseconds as uint; }
    | result.err(_) -> { seed := 1; };
    let generator: mutable prng = new_prng(seed);
    let secret_number: uint;
    match prng_range(&mutable generator, 1, 101) with
    | option.some(n) -> { secret_number := n; }
    | option.none -> { return 1; };

    println(concat(&"The secret number is: ", &uint_to_text(secret_number)));

    while (true) {
        println("Please input your guess.");
        let line: string;
        match read_line() with
        | result.ok(text) -> { line := text; }
        | result.err(_) -> {
            println("Failed to read line");
            return 1;
        };

        let guess: uint;
        match parse_uint(&trim(&line)) with
        | result.ok(n) -> { guess := n; }
        | result.err(_) -> {
            println("Please type a number!");
            return 1;
        };

        println(concat(&"You guessed: ", &uint_to_text(guess)));
        if (guess < secret_number) {
            println("Too small!");
        } else if (guess > secret_number) {
            println("Too big!");
        } else {
            println("You win!");
        };
    }
    return 0;
}
```

The variables `line` and `guess` are now declared inside the loop, so each time around they are new
variables that get one value each.

```console
$ supax run
     Running build/bin/guessing_game
Guess the number!
The secret number is: 65
Please input your guess.
53
You guessed: 53
Too small!
Please input your guess.
80
You guessed: 80
Too big!
Please input your guess.
69
You guessed: 69
Too big!
Please input your guess.
61
You guessed: 61
Too small!
Please input your guess.
65
You guessed: 65
You win!
Please input your guess.
quit
Please type a number!
```

The program has two problems. It asks again after you win, and the only way out is to type something
that is not a number, which ends the program as a failure.

## Quitting after a correct guess, and ignoring bad input

Listing 2-6 fixes both, and removes the line that prints the secret number.

<sub>Listing 2-6: the complete game. Filename: src/main.sn</sub>

```sultan stdin=foo\n50\n
import std.io.println;
import std.io.read_line;
import std.text.number;
import std.text.search;
import std.text.transform;
import std.random.prng;
import std.time.time;

def main(): int {
    println("Guess the number!");

    let seed: uint;
    match system_time_ns() with
    | result.ok(nanoseconds) -> { seed := nanoseconds as uint; }
    | result.err(_) -> { seed := 1; };
    let generator: mutable prng = new_prng(seed);
    let secret_number: uint;
    match prng_range(&mutable generator, 1, 101) with
    | option.some(n) -> { secret_number := n; }
    | option.none -> { return 1; };

    while (true) {
        println("Please input your guess.");
        let line: string;
        match read_line() with
        | result.ok(text) -> { line := text; }
        | result.err(_) -> {
            println("Failed to read line");
            return 1;
        };
        if (is_empty(&line)) {
            println("No more input.");
            return 1;
        };

        let guess: uint;
        match parse_uint(&trim(&line)) with
        | result.ok(n) -> { guess := n; }
        | result.err(_) -> {
            println("Please type a number!");
            continue;
        };

        println(concat(&"You guessed: ", &uint_to_text(guess)));
        if (guess < secret_number) {
            println("Too small!");
        } else if (guess > secret_number) {
            println("Too big!");
        } else {
            println("You win!");
            break;
        };
    }
    return 0;
}
```

Three changes:

- `break;` after `You win!` leaves the loop. After the loop comes `return 0;`, so the program ends with
  success.
- When the text is not a number, `continue;` goes straight to the next round of the loop, instead of
  ending the program. The player sees the question again.
- At the end of the input, `read_line` returns an empty string: not even a new line. That happens when
  the input comes from a file that has been read to the end, or when the user presses Ctrl-D in
  a Linux or macOS terminal. Without a check, the program would keep asking and reading nothing, forever.
  `is_empty` tells whether a string has no characters; it comes from `std.text.search`, the new import.

If you forget that import, the compiler finds the module for you:

```sultan ignore
        if (is_empty(&line)) {
```

```text
error[SULTANC-NAME-UNKNOWN]: unknown name: `is_empty` — The required name was not found during resolution.
  --> /projects/guessing_game/src/main.sn:30:13..30:21
    |
 30 |         if (is_empty(&line)) {
    |             ^^^^^^^^
    |
  = help: this name is declared `public` in a standard library module that this module does not import; import that module — The affected path is `std.text.search`.
```

Play the game:

```console
$ supax run
     Running build/bin/guessing_game
Guess the number!
Please input your guess.
foo
Please type a number!
Please input your guess.
53
You guessed: 53
Too small!
Please input your guess.
80
You guessed: 80
Too big!
Please input your guess.
69
You guessed: 69
Too big!
Please input your guess.
61
You guessed: 61
Too small!
Please input your guess.
65
You guessed: 65
Too big!
Please input your guess.
63
You guessed: 63
Too big!
Please input your guess.
62
You guessed: 62
You win!
```

You have built a complete game.

## Summary

This chapter used, briefly, much of what a SultanC program is made of:

- `let name: type` creates a variable, with its type written. A variable gets one value, which can come
  later than the `let`; a `mutable` type allows it to change.
- Functions that can fail return a `result`; functions that may have no value return an `option`.
  `match` handles every case, and the compiler checks that none is missing.
- `&` passes a reference, so a function can use a value without taking it; `&mutable` lets it change
  the value.
- Number types never convert by themselves: `as uint` converts, `parse_uint` reads a number from text,
  `uint_to_text` writes one as text.
- `if`, `else if` and `else` choose; `while` repeats; `break` leaves a loop and `continue` starts its
  next round.

The next chapters take these one at a time. Chapter 3 begins with variables, types and functions.

## Exercises

1. Count the guesses, and print the number of tries after `You win!`. You will need a variable of type
   `mutable uint`, changed with `tries := tries + 1;`.
2. Let the player choose the range: read the largest number before the game starts.
3. Print `Out of range!` when the guess is below 1 or above 100, without counting it.
