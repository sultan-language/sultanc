[العربية](../../ar/book/ch17-01-files.md) | **English**

# Files and Folders

## Writing and reading a file

`write_file(&path, &text)` creates a file, or replaces it, with the given text. `read_file(&path)` reads a
whole file as text. `lines`, from `std.text.transform`, splits the text into its lines:

```sultan
import std.io.println;
import std.text.number;
import std.text.transform;
import std.fs.write_file;
import std.fs.read_file;
import std.fs.mutation;

def main(): int {
    let path: string = "shopping.txt";
    let list: string = "bread\nmilk\ndates\n";
    match write_file(&path, &list) with
    | result.err(code) -> { return code; }
    | result.ok(_) -> { };
    match read_file(&path) with
    | result.ok(text) -> {
        let items: vector string = lines(&text);
        for (let i: mutable int = 0; i < length(items); i := i + 1) {
            println(concat3(&int_to_text(i + 1), &". ", &items[i]));
        }
    }
    | result.err(code) -> { return code; };
    match remove_file(&path) with
    | result.ok(_) -> { }
    | result.err(code) -> { return code; };
    return 0;
}
```

```text
1. bread
2. milk
3. dates
```

On an error, this program returns the operating system's error code as its own exit status, which is a
quick way to report it in a small tool.

A relative path such as `shopping.txt` is relative to the *current directory*: the folder the program was
started from, not the folder where the program file is.

## Folders

`create_dir_all` creates a folder together with any folders above it that are missing. `try_exists` says
whether a path exists, `rename` moves or renames a file or folder, and `remove_tree` deletes a folder with
everything inside it:

```sultan
import std.io.println;
import std.text.number;
import std.fs.walk;
import std.fs.directory;
import std.fs.write_file;
import std.fs.mutation;

def main(): int {
    let folder: string = "reports/2026";
    match create_dir_all(&folder) with
    | result.err(code) -> { return code; }
    | result.ok(_) -> { };
    let draft: string = "reports/2026/draft.txt";
    let final: string = "reports/2026/final.txt";
    let text: string = "total: 42\n";
    match write_file(&draft, &text) with
    | result.err(code) -> { return code; }
    | result.ok(_) -> { };
    match rename(&draft, &final) with
    | result.err(code) -> { return code; }
    | result.ok(_) -> { };
    match try_exists(&draft) with
    | result.ok(true) -> { println("the draft is still there"); }
    | result.ok(false) -> { println("the draft was renamed"); }
    | result.err(code) -> { return code; };
    match list_directory(&folder) with
    | result.ok(entries) -> { println(int_to_text(length(entries))); }
    | result.err(code) -> { return code; };
    let top: string = "reports";
    match remove_tree(&top) with
    | result.ok(_) -> { println("removed"); }
    | result.err(code) -> { return code; };
    return 0;
}
```

```text
the draft was renamed
1
removed
```

`list_directory` returns the entries of one folder, and `walk` the entries of a folder and everything
under it.

`std.fs` has more: file information such as size and modification time, permissions, links, temporary
files and file locks. The `examples/en/fs` folder of the repository has a program for each.
