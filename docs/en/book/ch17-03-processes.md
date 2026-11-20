[العربية](../../ar/book/ch17-03-processes.md) | **English**

# Running Other Programs

A program can start other programs, wait for them, and read how they ended. `std.process.command` builds
a *command*: the program to run and its arguments. `command_status` runs it, waits for it to finish,
and returns its *exit status*:

```sultan
import std.io.println;
import std.text.number;
import std.text.transform;
import std.process.command;
import std.process.child;

def main(): int {
    let echo: mutable command = new_command(&"echo");
    command_arg(&mutable echo, &"hello from a child process");
    match command_status(&echo) with
    | result.ok(exit_status.exited(code)) -> { println(concat(&"exit status ", &int_to_text(code))); }
    | result.ok(_) -> { println("stopped by a signal"); }
    | result.err(_) -> { println("could not run echo"); return 1; };
    return 0;
}
```

```text
hello from a child process
exit status 0
```

The new program, the *child process*, inherits the parent's standard output, so its line appears before
the parent's.

`new_command` takes the name of the program; a name without a folder, like `echo`, is looked up in the
folders listed in the `PATH` variable. `command_arg` adds one argument; each argument is passed as it
is, with no shell in between, so spaces and quotes in an argument need no escaping and cannot be
misread.

An exit status is an enum: `exit_status.exited(code)` when the program ended by itself with a status,
or another case when the system stopped it with a signal. `exit_success(&status)` says whether it ended
with 0.

`std.process.command` can also give the child its own input, capture its output into a vector of bytes
instead of passing it through, set its environment and its current directory, and start it without
waiting, to run beside the parent. The `examples/en/process` folder of the repository has a program for
each.
