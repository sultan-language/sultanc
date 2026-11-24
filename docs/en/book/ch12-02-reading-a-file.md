[العربية](../../ar/book/ch12-02-reading-a-file.md) | **English**

# Reading a File

Now the program needs a file to search. Create `poem.txt` at the top of the project, beside
`Supax.toml`:

<sub>Filename: poem.txt</sub>

```text
The river does not ask the stone
why it stands in the way.
It goes around, and keeps its tone,
and reaches the sea some day.
Ask the stone, it will say nothing.
```

`read_file(&path)`, from `std.fs.read_file`, reads a whole file as one `string`. Reading can fail (the
file may not exist, or may not be readable), so it returns a `result`. Its error is the operating
system's error code, an `int`.

<sub>Listing 12-3: reading the file. Filename: src/main.sn</sub>

```sultan ignore
import std.io.println;
import std.process.argument;
import std.fs.read_file;
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

    match read_file(&file_path) with
    | result.ok(contents) -> {
        println("With text:");
        println(contents);
    }
    | result.err(_) -> {
        println(concat(&"could not read ", &file_path));
        return 1;
    };
    return 0;
}
```

```console
$ supax run -- stone poem.txt
   Compiling finder v0.1.0 (/projects/finder)
    Finished in 0.22s → build/bin/finder
     Running build/bin/finder
Searching for stone
In file poem.txt
With text:
The river does not ask the stone
why it stands in the way.
It goes around, and keeps its tone,
and reaches the sea some day.
Ask the stone, it will say nothing.

$ supax run -- stone missing.txt
   Compiling finder v0.1.0 (/projects/finder)
    Finished in 0.19s → build/bin/finder
     Running build/bin/finder
Searching for stone
In file missing.txt
could not read missing.txt
```

A relative path such as `poem.txt` is relative to the folder the program is started from, which with
`supax run` is the project's folder.

The program works, but `main` now does everything: it reads the arguments, reads the file, decides what
to print, and handles every error. As it grows, that will become hard to read and impossible to test.
The next section fixes that.
