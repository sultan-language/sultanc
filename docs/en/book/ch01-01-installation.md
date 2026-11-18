[العربية](../../ar/book/ch01-01-installation.md) | **English**

# Installation

SultanC is built from source. The compiler is written in SultanC, so the build starts from a small seed
that is part of the repository and uses it to build the real compiler. You do not need an existing
SultanC compiler.

> **Command line notation.** Lines in this book that start with `$` are commands you type in a
> terminal; do not type the `$`. Lines without it are what the command prints.

There are two routes. Which one you use depends on your system.

| Your system | Route | What you need |
| --- | --- | --- |
| Linux on x86_64 | WebAssembly seed | Git, CMake, a C compiler |
| macOS on Apple silicon (M1 and later) | WebAssembly seed | Git, CMake, a C compiler |
| Windows, Linux on ARM, macOS on Intel, FreeBSD and others | C seed with LLVM | Git, CMake, a C compiler, LLVM 19 to 23 |

The difference is the backend: the part of the compiler that produces machine code. SultanC has its
own backend, called Native, for x86_64 Linux and Apple silicon macOS. On every other system the
compiler produces code through LLVM, so LLVM has to be installed before you build.

## Linux (x86_64)

Install the tools with your distribution's package manager:

```console
$ sudo apt install git cmake build-essential     # Debian, Ubuntu
$ sudo dnf install git cmake gcc make            # Fedora
$ sudo pacman -S git cmake base-devel            # Arch
```

Then build:

```console
$ git clone https://github.com/sultan-language/sultanc.git
$ cd sultanc
$ ./build.sh --bootstrap=wasm
```

## macOS (Apple silicon)

The C compiler comes with Apple's command line tools. CMake is available from Homebrew or from
[cmake.org](https://cmake.org/download/):

```console
$ xcode-select --install
$ brew install cmake
```

Then build with the same commands as on Linux:

```console
$ git clone https://github.com/sultan-language/sultanc.git
$ cd sultanc
$ ./build.sh --bootstrap=wasm
```

## Windows, and every other system

Install Git, CMake, a C compiler and LLVM 19 to 23, then build with the LLVM route:

```console
$ git clone https://github.com/sultan-language/sultanc.git
$ cd sultanc
$ ./build.sh --bootstrap=c --enable-llvm --backend=llvm
```

The build finds LLVM in its usual install locations. If yours is somewhere else, name its
`llvm-config` with `LLVM_CONFIG=/path/to/llvm-config ./build.sh …`.

On Windows, the LLVM that the official installer ships does not include every target or the LLVM-C
library that SultanC links against, so LLVM has to be built from its source once. The script
`.github/ci/windows.ps1` in the repository does exactly that: its `llvm-sdk` job builds LLVM, and its
`llvm-build` job builds SultanC against it.

## The result

A build takes a few minutes. When it finishes, the compiler is `build/sultanc`. Check that it runs:

```console
$ build/sultanc --lang=en version
SultanC development
backends: native
```

`--lang=en` asks for English; without it, the compiler answers in Arabic. The last line lists the backends your compiler contains: `native`, `llvm`, or both.

## Installing `sultanc` and Supax

Supax is SultanC's package manager. It creates projects, fetches the packages they depend on, and
builds, runs and tests them. Its installer also puts `sultanc` and the standard library on your
`PATH`. Clone Supax next to the compiler and run the installer, giving it the compiler's folder:

```console
$ cd ..
$ git clone https://github.com/sultan-language/supax.git
$ cd supax
$ tools/install.sh ../sultanc
```

On Windows, run `tools\install.ps1` instead. Both put `sultanc`, `supax` and the standard library in
the folder `.supax` in your home folder, and add `.supax/bin` to your `PATH`. Open a new terminal and
check:

```console
$ sultanc --lang=en version
$ supax --lang=en --help
```

## Trying SultanC without installing

The [playground](https://sultan-language.github.io/playground/) compiles and runs SultanC in your
browser. It is enough for the programs in the first chapters of this book.
