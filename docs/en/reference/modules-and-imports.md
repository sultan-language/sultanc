[العربية](../../ar/reference/modules-and-imports.md) | **English**

# Modules and Imports

This page specifies how SultanC finds, names and connects source modules: the import forms, how an import path
resolves to a file, how package manifests contribute root names, and the diagnostics for each failure. It is
for programmers who split a program across files and for contributors working on module loading.

## Modules

Every `.sn` file is a module. A module is identified by a canonical key, the pair (root, normalized logical path).
The root is the package or library tree the file belongs to, and the logical path is the dotted name of the file
below that root, without the `.sn` extension. The file `shapes/area.sn` in a package has the logical path
`shapes.area`. Identity never depends on the filesystem path used to reach the file, on the spelling of the root,
or on the import form: every import of one file produces the same module key, the same declarations and the same
symbols. Module loading owns only this canonical identity; it does not infer identity from file names, which
belong to the source locator.

A declaration is visible to other modules only when it is marked `public`. Anything else is private to its
module, and a use from outside is `SULTANC-NAME-PRIVATE`.

```sultan
public type Rectangle = { width: int, height: int };

public def area(shape: &Rectangle): int {
    return (*shape).width * (*shape).height;
}

def helper(): int { return 0; }
```

Only `Rectangle` and `area` are visible to an importer of this file.

## Import forms

`import` is spelled `استورد` in Arabic, and `from … import` is `من … استورد`. Either spelling may appear in any
file, whatever language the rest of the file is written in.

### Quoted relative import

```sultan
import "geometry.sn";
```

A quoted path names a file relative to the logical directory of the importing module. It stays inside the same
root: a path that climbs out of the root (`../x.sn`) is rejected, and so is an absolute path.

### Named import

```sultan
import std.text.number;
import util.shapes.area;
```

A named import is a dotted path of at least two components that ends at a module. The first component decides the
root. When it is a registered root spelling (the standard library, the importing package, a dependency, or any of
their declared names; see [Roots](#roots)), the rest of the path is resolved under that root. When it is not, the
complete path is resolved under the importer's own project root. A path is never retried relative to the importing
module's directory, and a lookup that failed under a registered root is never retried against the project tree.
A path of a single component is an invalid reference (`SULTANC-IMPORT-INVALID-REFERENCE`).

Importing a module brings every public name it declares into scope, unqualified. Importing the same module twice
is harmless. If two imported modules declare the same public name and the program uses it, the use is ambiguous
(`SULTANC-IMPORT-AMBIGUOUS`); give the imports aliases or select the names you need.

### Selective import

```sultan
from std.text import int_to_text, contains;
from std.io import println;
```

A selective import names a directory and lists the declarations to bring in; only the listed names enter scope.
Name resolution searches the public declarations of the modules in that directory. A listed name that does not
exist or is not public is `SULTANC-NAME-UNKNOWN`, and a name declared in more than one module of the directory is
`SULTANC-IMPORT-AMBIGUOUS`. Module loading only selects the directory and its canonical identity; deciding whether a
name exists, is public or is ambiguous belongs to name resolution. The same form works on directories of the
project as on those of the library.

### Aliased import

```sultan
import std.text.number as numbers;
import std.io.println;

def main(): int {
    println(numbers.int_to_text(42));
    return 0;
}
```

`as` changes only the local binding. The names of the module are reachable through the alias as a
qualifier (`numbers.int_to_text`), and are no longer in scope unqualified. The alias never creates a new module,
symbol or source identity. The alias works on quoted and named imports alike, which is how two modules that declare
the same name are used together. A qualified name cannot be written in a type position.

## Roots

A root is a tree of modules with a set of spellings that select it. Several spellings can select one root, and
they all produce the same module keys.

The standard library is one root with the spellings `std` and `المكتبة`. Its subdirectories carry per-language
spellings as well, so `std.io.println`, `المكتبة.دخ.println` and `std.entree_sortie.println` name one module. The
mapping comes from the `[imports-alias]` table of `library/Supax.toml`:

| Directory | Arabic | Directory | Arabic |
|---|---|---|---|
| `io` | `دخ` | `net` | `شبكة` |
| `fs` | `ملفات` | `http` | `ويب` |
| `text` | `نصوص` | `debug` | `تصحيح` |
| `chars` | `حروف` | `meta` | `انعكاس` |
| `bytes` | `بايتات` | `error` | `أخطاء` |
| `encoding` | `ترميز` | `fmt` | `تنسيق` |
| `memory` | `ذاكرة` | | |

The table also lists French and Spanish spellings for some directories. The file names inside a directory
(`println`, `number`) are not translated; they stay as written in both languages. Directories without an entry,
such as `std.collections`, have a single spelling.

The compiler finds the standard library through `library/Supax.toml`, relative to the working directory.

A package contributes its own roots: its `name`, every spelling in its `[package.names]` table, and, for a
dependency, the key under which the importer lists it. The project can therefore import itself by name
(`import app.helper;`), and a dependency by the name it was given.

## Package manifests

A package is a directory with a manifest named `Supax.toml`. The name is matched exactly, including case, against
the directory entry, so a file called `supax.toml` is not a manifest. (The repository root carries a lowercase
`supax.toml` with build settings; it is not read as a package manifest.) When compiling a source file, the driver
looks for the manifest in the file's directory and then in each parent directory, and uses the first it finds. All
source files named in one invocation must belong to the same package. Without a manifest, the project root is the
working directory.

```toml
[package]
name = "app"
version = "0.1.0"

[package.names]
ar = "تطبيق"

[imports-alias]
shapes = { en = "shapes", ar = "أشكال" }

[dependencies]
util = { path = "../util" }

[lib]
path = "src"
```

| Table | Content |
|---|---|
| `[package]` | `name` is required. `version`, `authors`, `description`, `homepage`, `repository`, `readme`, `keywords` and `license` are optional descriptive fields, and `metadata` is kept as data. `edition`, `sultanc-version` and `build` are recognized but unsupported. |
| `[package.names]` | Maps a language tag to a spelling of the package's root name, so one package is importable under several names. A spelling must be a valid root name. |
| `[imports-alias]` | Maps a directory of the package to per-language spellings, as `library/Supax.toml` does for std. Each key is a canonical path and each value a table from language tag to spelling. |
| `[dependencies]` | Each entry has the form `name = { path = "relative/dir" }`. The path is relative to the manifest and names a directory with its own `Supax.toml`. No other field is accepted. |
| `[dev-dependencies]` | Must be empty. |
| `[lib]` | `path` locates the package's source root, relative to the manifest. When it names a directory, modules resolve beneath it. When it names a file, that file is the package's root module, and each of the package's root spellings names it. Without `[lib]`, the manifest directory is the root. |

Any other top-level table is rejected, as is an unknown field inside `[package]` or `[lib]`. A package that is
used as a dependency must declare a `[lib]` table ("path dependency does not declare an importable [lib]
target"). Dependency cycles are rejected ("package dependency cycle detected"). Both are reported under
`SULTANC-PACKAGE-MANIFEST-INVALID`. A dependency sees the standard library under the same spellings as the
project, so std is importable everywhere.

With this manifest, the module `src/shapes/area.sn` of the package is imported as `app.shapes.area`,
`تطبيق.shapes.area` or, using the directory alias, `تطبيق.أشكال.area`. If `../util` declares `name = "util"` and
`[lib] path = "src"`, its module `src/shapes/area.sn` is imported as `util.shapes.area`.

## Diagnostics

| Identity | Meaning |
|---|---|
| `SULTANC-MODULE-NOT-FOUND` | No source exists for the selected root and path. |
| `SULTANC-MODULE-INVALID-LOGICAL-PATH` | A component of a source path is not a valid module name. |
| `SULTANC-IMPORT-INVALID-REFERENCE` | The import has the wrong shape, such as a single-component path. |
| `SULTANC-IMPORT-CYCLE` | Modules import each other in a cycle. |
| `SULTANC-IMPORT-AMBIGUOUS` | Two imports provide the same name, or a selective import matches several modules. |
| `SULTANC-NAME-UNKNOWN` | A selected name does not exist in the imported scope. |
| `SULTANC-NAME-PRIVATE` | The name exists but is not `public`. |
| `SULTANC-PACKAGE-MANIFEST-INVALID` | A manifest is missing, unreadable, malformed or contains an unsupported key. |

A missing project module and a missing module under a registered library root both fail as an absent source for the
root that was already selected.

## Explicit module sources

`--module-source=<module>=<file>` changes only where the source of an existing module is read from. The dotted
`<module>` is resolved from the root module exactly as a named import, so every import of that module keeps the
same canonical key; the file becomes the explicit source location of that key in the logical environment, which the
source locator consults before deriving a path. The trusted standard-library root, direct inputs, unknown modules
and repeated bindings are rejected. Builds use it to select compiler-build capabilities (see
[Compiler-build capabilities](../internals/compiler-architecture.md#backend-providers-and-compiler-build-capabilities)).

## Limitations

- A qualified name from an aliased import cannot appear in a type position.
- A selective import lists plain names; renaming a name with `as` in the list is not supported.
- A single-component named import is rejected.

## See also

- [Getting started](../guide/getting-started.md)
- [Language tour](../guide/language-tour.md)
- [Compiler architecture](../internals/compiler-architecture.md)
- [Arabic terminology](arabic-terminology.md)
