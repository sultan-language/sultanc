[العربية](../../ar/book/ch12-06-writing-to-stderr.md) | **English**

# Writing Errors to Standard Error

A program has two outputs. *Standard output* is for its results; *standard error* is for messages about
problems. When a user sends the results to a file with `>`, only standard output goes there, and the
messages still appear on the screen.

`finder` prints its error messages with `println`, to standard output. Send the output to a file, with a
file that does not exist:

```console
$ supax run -- stone missing.txt > found.txt
$ cat found.txt
could not read missing.txt
```

The message went into the file, where the user expected search results, and nothing appeared on the
screen. The fix is `eprintln`, from `std.io.println` like `println`, which prints a line to standard
error. Here is the final `main.sn`:

<sub>Listing 12-10: the final program. Filename: src/main.sn</sub>

```sultan ignore
import std.io.println;
import std.process.argument;
import std.process.argument_count;
import std.fs.read_file;
import std.env.environment;
import std.text.transform;
import finder.search;

def arguments(): vector string {
    let all: mutable vector string = [];
    for (let i: mutable uint = 0; i < argument_count(); i := i + 1) {
        match argument(i) with
        | option.some(text) -> { append(all, text); }
        | option.none -> { };
    }
    return all;
}

def ignore_case_requested(): bool {
    match get_environment_variable(&"IGNORE_CASE") with
    | result.ok(option.some(_)) -> { return true; }
    | result.ok(option.none) -> { return false; }
    | result.err(_) -> { return false; };
}

def run(config: &Config): result int, string {
    let contents: string;
    match read_file(&config.file_path) with
    | result.ok(text) -> { contents := text; }
    | result.err(_) -> { return result.err(concat(&"could not read ", &config.file_path)); };
    let found: vector string;
    if (config.ignore_case) {
        found := search_case_insensitive(&config.query, &contents);
    } else {
        found := search(&config.query, &contents);
    };
    for (let i: mutable int = 0; i < length(found); i := i + 1) {
        println(found[i]);
    }
    return result.ok(0);
}

def main(): int {
    let args: vector string = arguments();
    let config: Config;
    match build_config(&args, ignore_case_requested()) with
    | result.ok(built) -> { config := built; }
    | result.err(message) -> {
        eprintln(message);
        return 2;
    };
    match run(&config) with
    | result.ok(_) -> { return 0; }
    | result.err(message) -> {
        eprintln(message);
        return 1;
    };
}
```

```console
$ supax run -- stone missing.txt > found.txt
could not read missing.txt
$ cat found.txt
$ supax run -- stone poem.txt > found.txt
$ cat found.txt
The river does not ask the stone
Ask the stone, it will say nothing.
```

The error now appears on the screen and the file stays empty; the results go to the file.

## Summary

This chapter used ideas from the whole book so far: records, `result`, ownership, modules, tests, and the
standard library's text, file, process and environment modules. It also showed a structure that works
for most command line programs:

- the library does the work, in functions that take values and return values, and is tested;
- `main` collects the input, calls the library, and turns errors into messages on standard error and an
  exit status;
- every function that can fail returns a `result`, and failures are handled in one place.

The next chapter makes output look right: numbers with a fixed number of digits, columns that line up,
and text built from a template.
