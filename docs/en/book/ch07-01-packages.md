[العربية](../../ar/book/ch07-01-packages.md) | **English**

# Packages

You have created packages since chapter 1: `supax new` makes one. This section looks at what a package
contains.

```console
$ supax new shop
created package "shop"
$ cd shop
```

```text
shop/
├── .gitignore
├── Supax.toml
└── src/
    └── main.sn
```

`Supax.toml` is the package's *manifest*: its name, its version, and the packages it depends on. Every
package has one, at the top of its folder. The folder `src` holds the package's code.

## Programs and the library

A package can contain two kinds of code:

- **programs**, each a file with a `main` function, which Supax builds into an executable;
- a **library**, modules that the package's programs, its tests and other packages can import.

`src/main.sn` is the package's main program, named after the package. More programs go in `src/bin`: a
file `src/bin/report.sn` becomes a program named `report`.

To make the modules in `src` a library that can be imported by name, add a `[lib]` section to the
manifest:

<sub>Filename: Supax.toml</sub>

```toml
[package]
name = "shop"
version = "0.1.0"

[lib]
path = "src"

[dependencies]
```

From then on, a file under `src` is a module named after the package and its path: `src/prices/tax.sn`
is the module `shop.prices.tax`. Section 7.4 builds such a library step by step.

## Building several programs

With `src/main.sn` and `src/bin/report.sn`, `supax build` builds both:

```console
$ supax build
   Compiling shop v0.1.0 (/projects/shop)
    Finished in 0.27s → build/bin/shop, build/bin/report
```

and `supax run --bin report` runs one of them:

```console
$ supax run --bin report
   Compiling shop v0.1.0 (/projects/shop)
    Finished in 0.14s → build/bin/report
     Running build/bin/report
1150
```

## Package names

A package's name is how other code refers to it: its library's modules all start with the name. Use
lowercase words joined by `_`, or Arabic words: `shop`, `guessing_game`, `متجر`. The name in
`Supax.toml` is what counts, not the folder's name.

## The commands

| Command | What it does |
| --- | --- |
| `supax new NAME` | creates a package in a new folder |
| `supax init` | creates a package in the current folder |
| `supax build` | builds every program into `build/bin/` |
| `supax run [--bin NAME]` | builds, then runs a program |
| `supax check` | checks the package without building |
| `supax test` | runs the tests ([chapter 11](ch11-00-testing.md)) |
| `supax clean` | deletes `build/` |

Every command also has an Arabic name, listed by `supax --help`: `supax ابن` is `supax build`, and
`supax شغل` is `supax run`.

Dependencies on other packages are the subject of [chapter 14](ch14-00-more-about-supax.md).
