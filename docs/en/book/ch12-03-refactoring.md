[العربية](../../ar/book/ch12-03-refactoring.md) | **English**

# Refactoring for Modularity and Error Handling

Four things in Listing 12-3 will get in the way as the program grows:

1. `main` does all the work. Parsing the arguments, reading the file and printing are separate jobs, and
   a function that does one job is easier to read and to change.
2. `query` and `file_path` are separate variables, although they are the program's settings and belong
   together.
3. Every failure is handled on the spot, with its own message and `return`, in the middle of the work.
4. Nothing can be tested: `main` cannot be called from a test, and it reads the real command line.

The usual structure for a command line program answers all four:

- the work lives in a **library**, as functions that take values and return values, so tests can call
  them;
- `main` stays small: it collects the arguments, calls the library, and turns errors into messages and
  exit statuses.

## A record for the settings

The settings become a record, built by a function that checks the arguments and returns a `result`. It
goes in the library, in the file `src/search.sn`, which makes it the module `finder.search`:

<sub>Listing 12-4: the settings and their checks. Filename: src/search.sn</sub>

```sultan ignore
public type Config = { query: string, file_path: string };

public def build_config(args: &vector string): result Config, string {
    if (length(*args) < 3) {
        return result.err("usage: finder QUERY FILE");
    };
    let config: Config = { query: clone(&(*args)[1]), file_path: clone(&(*args)[2]) };
    return result.ok(config);
}
```

`build_config` takes the arguments as a borrowed vector of strings, so that a test can pass any vector
it likes. It copies the two strings it needs with `clone`, because the vector belongs to the caller.

Make `src` a library in `Supax.toml`, and give it a root file that imports the module, as in chapter 11:

<sub>Filename: Supax.toml</sub>

```toml
[package]
name = "finder"
version = "0.1.0"

[lib]
path = "src"

[dependencies]
```

<sub>Filename: src/finder.sn</sub>

```sultan ignore
import finder.search;
```

## A small `main`

`main` now has three steps: collect the arguments into a vector, build the settings, and run. `run`
returns a `result` too, so `main` handles every failure in the same place, at the end:

<sub>Listing 12-5: `main` after the refactoring. Filename: src/main.sn</sub>

```sultan ignore
import std.io.println;
import std.process.argument;
import std.process.argument_count;
import std.fs.read_file;
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

def run(config: &Config): result int, string {
    match read_file(&config.file_path) with
    | result.ok(contents) -> {
        println(contents);
        return result.ok(0);
    }
    | result.err(_) -> { return result.err(concat(&"could not read ", &config.file_path)); };
}

def main(): int {
    let args: vector string = arguments();
    let config: Config;
    match build_config(&args) with
    | result.ok(built) -> { config := built; }
    | result.err(message) -> {
        println(message);
        return 2;
    };
    match run(&config) with
    | result.ok(_) -> { return 0; }
    | result.err(message) -> {
        println(message);
        return 1;
    };
}
```

`run` reads the file into `contents`, and returns an error with a message when it cannot. It prints the
whole text for now; the next section makes it search.

The program does exactly what it did before, but every part now has one job, and the part that matters
most, the search, will live where tests can reach it.
