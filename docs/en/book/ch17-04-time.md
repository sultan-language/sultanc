[العربية](../../ar/book/ch17-04-time.md) | **English**

# Time

## Two clocks

`std.time.time` has two clocks, for two different questions:

- `system_time_ns()` answers *what time is it?*: the number of nanoseconds since 1 January 1970 (UTC).
  It is what chapter 2 used as a seed. It can jump: the user, or the network, may set the clock.
- `monotonic_time_ns()` answers *how long did that take?*: nanoseconds since some fixed moment. It only
  ever moves forward, so the difference between two readings is a true duration.

`sleep_ns(n)` pauses the thread for at least `n` nanoseconds. Here the program measures a pause:

```sultan
import std.io.println;
import std.time.time;

def now(): int {
    match monotonic_time_ns() with
    | result.ok(ns) -> { return ns; }
    | result.err(_) -> { return 0; };
}

def main(): int {
    let start: int = now();
    match sleep_ns(50000000) with
    | result.ok(_) -> { }
    | result.err(_) -> { return 1; };
    let elapsed_ms: int = (now() - start) / 1000000;
    if (elapsed_ms >= 50) {
        println("slept for at least 50 ms");
    };
    return 0;
}
```

```text
slept for at least 50 ms
```

Measure durations with the monotonic clock, never with the system clock: if the system clock is set back
by an hour while you measure, the difference becomes negative.

## Dates

`std.time.calendar` answers questions about the calendar:

```sultan
import std.io.println;
import std.text.number;
import std.time.calendar;

def main(): int {
    println(uint_to_text(days_in_month(2028, 2)));
    if (is_leap_year(2028)) {
        println("2028 is a leap year");
    };
    return 0;
}
```

```text
29
2028 is a leap year
```

`std.time` also has durations, dates, times of day, time zones and ISO 8601 text; the `examples/en/time`
folder has a program for each.

## Summary

- Files: `write_file`, `read_file`, `rename`, `remove_file`; folders: `create_dir_all`, `list_directory`,
  `remove_tree`. Relative paths start from the current directory.
- The environment: `arguments()`, `get_environment_variable`, `current_directory`.
- Other programs: `new_command`, `command_arg`, `command_status`; arguments pass without a shell.
- Time: `system_time_ns` for the date, `monotonic_time_ns` for durations, `sleep_ns` to wait.
