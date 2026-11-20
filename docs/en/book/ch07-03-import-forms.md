[العربية](../../ar/book/ch07-03-import-forms.md) | **English**

# The Three Import Forms

An import makes another module's public names usable. Modules of the standard library, and of packages,
are named by a *path* with dots between its parts: `std.text.number` is the module `number` in the
folder `text` of the package `std`.

There are three forms of import. They differ in how you write the imported names afterwards:

| Import | Then write |
| --- | --- |
| `import std.text.number;` | `int_to_text(x)`: every public name of the module, directly |
| `import std.text.number as numbers;` | `numbers.int_to_text(x)`: the names after a prefix you choose |
| `from std.text import trim, concat;` | `trim(…)`: only the names listed, from any module of the folder `std.text` |

```sultan
import std.text.number as numbers;
from std.io import println;

def main(): int {
    println(numbers.int_to_text(2026));
    return 0;
}
```

```text
2026
```

The first form is the most common, and the one this book uses. A prefix helps when two modules have
names that look alike, or when you want the reader to see where a function comes from at every call.
`from … import` names a folder of modules and brings in only the names you list, from whichever module of
the folder declares them. It is the way out when two imported modules declare the same name: list the
one you mean.

## Importing a file by its path

A program of a few files, outside any package, can import a file by its path in quotes, as in the last
section:

```sultan ignore
import "geometry.sn";
import "shapes/circle.sn";
```

The path is relative to the file that writes the import: `"shapes/circle.sn"` is a file in a folder
`shapes` beside it. In a package, prefer named imports: they do not change when a file moves to
another folder of the same package.

## The same module, in Arabic and in English

Every module of the standard library has an Arabic name too, and an import can use either. The Arabic
name of `std` is `المكتبة`, so these two lines import the same module:

```sultan ignore
import std.text.number;
استورد المكتبة.نصوص.أعداد;
```

The names inside a module have both spellings as well: `int_to_text` and `عدد_إلى_نص` are one function.
A program may use whichever it likes, and mix them. The Arabic edition of this book writes everything in
Arabic.

[Modules and imports](../reference/modules-and-imports.md), in the reference, describes exactly how the
compiler finds a module from its path.
