[العربية](../../ar/book/ch01-03-hello-supax.md) | **English**

# Hello, Supax!

`sultanc` is enough for a single file. Real programs have several files, tests, and packages written by
other people. Supax manages all of that, and the rest of this book uses it.

## Creating a project

Go back to your `projects` folder and create a new project:

```console
$ cd ..
$ supax new hello_supax --lang=en
created package "hello_supax"
$ cd hello_supax
```

`--lang=en` asks for English messages and an English starting program. Without it, Supax writes both in
Arabic. To make English the default for every command, set the environment variable once:
`export SUPAX_LANG=en`. The outputs in this book are made that way.

Supax made a folder with three files:

```text
hello_supax/
├── .gitignore
├── Supax.toml
└── src/
    └── main.sn
```

<sub>Filename: Supax.toml</sub>

```toml
[package]
name = "hello_supax"
version = "0.1.0"

[dependencies]
```

`Supax.toml` describes the project. It is written in TOML, a simple format for settings. `[package]`
gives the project's name and version. `[dependencies]` lists the packages the project uses; there are
none yet.

<sub>Filename: src/main.sn</sub>

```sultan
import std.io.println;

def main(): int {
    println("Hello from SultanC");
    return 0;
}
```

```text
Hello from SultanC
```

The code lives in `src`; the project's folder keeps the settings and anything that is not code.

`.gitignore` contains one line, `build/`, so that Git does not store the files that the build makes.

## Building and running

```console
$ supax build
   Compiling hello_supax v0.1.0 (/projects/hello_supax)
    Finished in 0.20s → build/bin/hello_supax
$ ./build/bin/hello_supax
Hello from SultanC
```

`supax build` compiles the project, and the program goes in `build/bin`, under the project's name.
`supax run` builds and runs in one step:

```console
$ supax run
   Compiling hello_supax v0.1.0 (/projects/hello_supax)
    Finished in 0.14s → build/bin/hello_supax
     Running build/bin/hello_supax
Hello from SultanC
```

`supax check` checks the program without producing anything, which is faster while you write code:

```console
$ supax check
    Checking hello_supax v0.1.0 (/projects/hello_supax)
    Finished in 0.05s
```

## Summary

- `./build.sh` builds the compiler from the seed in the repository; `tools/install.sh` in Supax puts
  `sultanc` and `supax` on your `PATH`.
- A program starts in `main`, and the number `main` returns is the exit status.
- `import` makes a standard library module's names available; every statement ends with `;`.
- An error message names its identity, its place, and often the fix.
- `sultanc file.sn -o name` compiles; `sultanc run file.sn` runs without a file.
- `supax new`, `supax build`, `supax run` and `supax check` manage a project.

In the next chapter you write a complete program: a game.
