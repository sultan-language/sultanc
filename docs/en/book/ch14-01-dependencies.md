[العربية](../../ar/book/ch14-01-dependencies.md) | **English**

# Dependencies

A *dependency* is another package whose library your package uses. Dependencies are listed in the
`[dependencies]` section of `Supax.toml`, and come from one of three places.

## A package in a folder

The simplest source is a folder on your own disk: a package you are writing next to this one. Here is a
small library package, `greeting`, with one module:

<sub>Filename: ../greeting/src/words.sn</sub>

```sultan ignore
import std.text.transform;

public def hello(name: &string): string {
    return concat3(&"Hello, ", name, &"!");
}
```

Its manifest has `[lib] path = "src"`, and its root file `src/greeting.sn` imports `greeting.words`, as
in chapter 11. A second package, `app`, adds it with `supax add`:

```console
$ supax add greeting --path ../greeting
       Added greeting = { path = "../greeting" }
```

`supax add` wrote the dependency into `Supax.toml`:

```toml
[dependencies]
greeting = { path = "../greeting" }
```

`app` imports the module by its full name, starting with the package's name:

<sub>Filename: src/main.sn</sub>

```sultan ignore
import std.io.println;
import greeting.words;

def main(): int {
    println(hello(&"Noor"));
    return 0;
}
```

```console
$ supax run
   Compiling app v0.1.0 (/projects/app)
    Finished in 0.16s → build/bin/app
     Running build/bin/app
Hello, Noor!
```

## A package from the index

Published packages are listed in the *package index*, a git repository at
`https://github.com/sultan-language/index`. Each version in it points to a commit of the package's own
repository and a checksum of its contents. `supax add NAME@VERSION` looks the package up and adds it;
without `@VERSION`, it takes the newest release:

```console
$ supax add nahr@0.1.0-dev
    Fetching https://github.com/sultan-language/index
    Fetching https://github.com/supax0/nahr.git
       Added nahr = "0.1.0-dev"
```

Nahr is a web framework written in SultanC. Its modules are now importable as `nahr.…`.

A version in `Supax.toml` is a *requirement*, not one exact version. `"1.2"` means any version from 1.2.0
up to, but not including, 2.0.0: versions that, by the rules of *semantic versioning*, add features and
fix bugs without breaking code written for 1.2.

## A package from git

A package that is not in the index can come straight from its git repository, at a tag or commit:

```toml
[dependencies]
json = { git = "https://github.com/someone/sn-json", tag = "v1.2.0" }
```

`supax add json --git URL --tag v1.2.0` writes the same line.

## Seeing the dependencies

`supax tree` shows what the package depends on, including the dependencies of dependencies:

```console
$ supax tree
app v0.1.0
  - greeting v0.2.0 (../greeting)
```

`supax remove NAME` removes a dependency again.

## Several packages in one repository

A large project often has several packages that depend on each other: a library, a program, a tool.
A *workspace* keeps them in one repository, with one lock file and one `build` folder. Its root
`Supax.toml` lists the members:

```toml
[workspace]
members = ["app", "libs/*"]
```

`"libs/*"` means every folder under `libs` that has a `Supax.toml`. Commands run at the root act on
every member; `-p NAME` chooses one.
