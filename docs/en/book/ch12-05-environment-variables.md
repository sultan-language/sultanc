[العربية](../../ar/book/ch12-05-environment-variables.md) | **English**

# Working with Environment Variables

Some users want the search to ignore the difference between capital and small letters. An option like
this, one that a user sets once and keeps, is often given as an *environment variable*: a named setting
that the shell passes to every program it starts.

## A second search function

First the library, test first. `to_ascii_lowercase`, from `std.text.transform`, turns the Latin capital
letters of a text into small ones; comparing the small forms ignores the difference. The settings get a
third field, `ignore_case`, which `build_config` receives from its caller. Here is the whole library:

<sub>Listing 12-9: searching without regard to case. Filename: src/search.sn</sub>

```sultan test
import std.debug.assert;
import std.text.search;
import std.text.transform;

public type Config = { query: string, file_path: string, ignore_case: bool };

public def build_config(args: &vector string, ignore_case: bool): result Config, string {
    if (length(*args) < 3) {
        return result.err("usage: finder QUERY FILE");
    };
    let config: Config = {
        query: clone(&(*args)[1]),
        file_path: clone(&(*args)[2]),
        ignore_case: ignore_case
    };
    return result.ok(config);
}

public def search(query: &string, contents: &string): vector string {
    let found: mutable vector string = [];
    let all: vector string = lines(contents);
    for (let i: mutable int = 0; i < length(all); i := i + 1) {
        if (contains(&all[i], query)) {
            append(found, clone(&all[i]));
        };
    }
    return found;
}

public def search_case_insensitive(query: &string, contents: &string): vector string {
    let wanted: string = to_ascii_lowercase(query);
    let found: mutable vector string = [];
    let all: vector string = lines(contents);
    for (let i: mutable int = 0; i < length(all); i := i + 1) {
        if (contains(&to_ascii_lowercase(&all[i]), &wanted)) {
            append(found, clone(&all[i]));
        };
    }
    return found;
}

@@test
def one_result(): void {
    let contents: string = "Slow and steady.\nsafe, fast, productive.\nPick three.";
    let found: vector string = search(&"fast", &contents);
    assert(length(found) == 1);
    assert(found[0] == "safe, fast, productive.");
}

@@test
def case_sensitive(): void {
    let contents: string = "Fast cars.\nsafe, fast, productive.\nFASTER still.";
    let found: vector string = search(&"fast", &contents);
    assert(length(found) == 1);
}

@@test
def case_insensitive(): void {
    let contents: string = "Fast cars.\nsafe, fast, productive.\nFASTER still.";
    let found: vector string = search_case_insensitive(&"fast", &contents);
    assert(length(found) == 3);
}

@@test
def too_few_arguments(): void {
    let args: vector string = ["finder", "fast"];
    match build_config(&args, false) with
    | result.ok(_) -> { assert(false); }
    | result.err(_) -> { };
}
```

```text
PASS one_result
PASS case_sensitive
PASS case_insensitive
PASS too_few_arguments

test result: ok. passed: 4; failed: 0; filtered out: 0
```

`case_sensitive` checks that `search` still respects case, and `case_insensitive` that the new function
does not. `too_few_arguments` checks that `build_config` refuses a vector without a file name.

`to_ascii_lowercase` changes only the Latin letters A to Z. Arabic has no capital letters, so Arabic text
is searched the same way by both functions.

## Reading the environment variable

`get_environment_variable(&name)`, from `std.env.environment`, reads a variable. The variable may not be
set, and reading the environment may fail, so it returns a `result` that holds an `option`. Only
whether it is set matters here:

```sultan ignore
def ignore_case_requested(): bool {
    match get_environment_variable(&"IGNORE_CASE") with
    | result.ok(option.some(_)) -> { return true; }
    | result.ok(option.none) -> { return false; }
    | result.err(_) -> { return false; };
}
```

The pattern `result.ok(option.some(_))` matches two levels at once: an `ok` that holds a `some`.

`main` passes the answer to `build_config`, and `run` chooses the search function by the setting:

```sultan ignore
    let found: vector string;
    if (config.ignore_case) {
        found := search_case_insensitive(&config.query, &contents);
    } else {
        found := search(&config.query, &contents);
    };
```

```console
$ supax run -- ask poem.txt
The river does not ask the stone
$ IGNORE_CASE=1 supax run -- ask poem.txt
The river does not ask the stone
Ask the stone, it will say nothing.
```

The `Compiling`, `Finished` and `Running` lines are left out of these outputs. `IGNORE_CASE=1` before
the command sets the variable for that one command, in the shells of Linux and macOS.
