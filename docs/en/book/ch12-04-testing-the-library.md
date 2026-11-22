[العربية](../../ar/book/ch12-04-testing-the-library.md) | **English**

# Developing the Library with Tests

The search itself is the heart of the program. This section writes it the other way round from usual:
the test first, then the code. This way of working, *test-driven development*, has three steps:

1. Write a test that fails, because the code it tests does not exist or does not work yet. Run it, and
   watch it fail for the reason you expect.
2. Write just enough code to make the test pass.
3. Improve the code, running the tests to check that it still passes.

## A failing test

The test says what `search` should do: given a query and a text, return the lines that contain the
query. Add it to `src/search.sn`, with a `search` that returns nothing yet:

<sub>Listing 12-6: a test, and a `search` that does not work yet. Filename: src/search.sn</sub>

```sultan ignore
import std.debug.assert;

public def search(query: &string, contents: &string): vector string {
    let found: vector string = [];
    return found;
}

@@test
def one_result(): void {
    let contents: string = "Slow and steady.\nsafe, fast, productive.\nPick three.";
    let found: vector string = search(&"fast", &contents);
    assert(length(found) == 1);
    assert(found[0] == "safe, fast, productive.");
}
```

```console
$ supax test --lib
     Running lib src/finder.sn
FAIL one_result: exit status 1
    --> /projects/finder/src/search.sn:12:5
    note: the test is declared at /projects/finder/src/search.sn:9:5
    error[SULTANC-RUNTIME-PROGRAM-STOPPED]: the program stopped — An assertion failed.

test result: FAILED. passed: 0; failed: 1; filtered out: 0
error: test failed; to rerun pass "--lib"
```

It fails at line 12, the check that exactly one line was found. That is the failure we expected.

## Making the test pass

`search` goes through the lines of `contents`, and keeps those that contain `query`. `lines`, from
`std.text.transform`, splits a text into its lines, and `contains`, from `std.text.search`, says whether
one text contains another:

<sub>Listing 12-7: a working `search`. Filename: src/search.sn</sub>

```sultan test
import std.debug.assert;
import std.text.search;
import std.text.transform;

public type Config = { query: string, file_path: string };

public def build_config(args: &vector string): result Config, string {
    if (length(*args) < 3) {
        return result.err("usage: finder QUERY FILE");
    };
    let config: Config = { query: clone(&(*args)[1]), file_path: clone(&(*args)[2]) };
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

@@test
def one_result(): void {
    let contents: string = "Slow and steady.\nsafe, fast, productive.\nPick three.";
    let found: vector string = search(&"fast", &contents);
    assert(length(found) == 1);
    assert(found[0] == "safe, fast, productive.");
}
```

```text
PASS one_result

test result: ok. passed: 1; failed: 0; filtered out: 0
```

The lines in `found` are copies, made with `clone`: `all` is a local vector that is freed when `search`
returns, and the returned vector must own its strings.

## Using `search` in `run`

`run` now prints the lines that `search` found, instead of the whole text:

<sub>Listing 12-8: `run` with the search. Filename: src/main.sn</sub>

```sultan ignore
def run(config: &Config): result int, string {
    let contents: string;
    match read_file(&config.file_path) with
    | result.ok(text) -> { contents := text; }
    | result.err(_) -> { return result.err(concat(&"could not read ", &config.file_path)); };
    let found: vector string = search(&config.query, &contents);
    for (let i: mutable int = 0; i < length(found); i := i + 1) {
        println(found[i]);
    }
    return result.ok(0);
}
```

```console
$ supax run -- stone poem.txt
   Compiling finder v0.1.0 (/projects/finder)
    Finished in 0.30s → build/bin/finder
     Running build/bin/finder
The river does not ask the stone
Ask the stone, it will say nothing.
```

The program works. Try `ask`: only the first line matches, because the last one has `Ask` with a
capital letter. The next section adds an option for that.
