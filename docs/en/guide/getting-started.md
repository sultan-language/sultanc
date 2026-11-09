[العربية](../../ar/guide/getting-started.md) | **English**

# Getting Started

This guide takes you from a source checkout to a running program. It covers building the compiler,
writing a first program, compiling, interpreting and checking it,
cross-compiling, the LLVM backend, and the language and format of diagnostics. It assumes you
are comfortable with a command line and a statically typed language such as Rust, Go or C.

## Building the compiler

SultanC is self-hosted: the compiler is written in SultanC and builds itself in stages. A build starts
from a small seed (Stage0), uses it to build Stage1, and uses Stage1 to build the final compiler, Stage2.
`build.sh` drives all stages through CMake.

```sh
git clone https://github.com/sultan-language/sultanc.git
cd sultanc
./build.sh
```

The finished compiler is `build/sultanc`. The stage products stay under `build/bootstrap/cmake/bin`
(`cmake-wasm/bin` for the WebAssembly seed). Copy `build/sultanc` anywhere on your `PATH`, or run it in place.

The seed can come from three places, chosen with `--bootstrap`:

| Route | Command | Needs |
|---|---|---|
| C seed (default) | `./build.sh` | a C compiler and LLVM |
| WebAssembly seed | `./build.sh --bootstrap=wasm` | only a C compiler |
| An existing compiler | `./build.sh --bootstrap=existing` | a built `build/sultanc`, or one named with `--with=PATH` |

The existing-compiler route performs a single self-hosting step and takes about a minute, which makes it
the fast loop during development. Use the C or WebAssembly route on a new system, when the existing compiler is
too old for the sources, and when verifying a release.

By default the finished compiler contains only the Native backend and has no dependency on libLLVM. Add
`--enable-llvm` to build the LLVM backend into it (see [The LLVM backend](#the-llvm-backend)). The C seed
always uses LLVM, so LLVM 19 to 23 must be installed for the default route. On macOS with Homebrew, point the build at
it first:

```sh
export LLVM_CONFIG="$(brew --prefix llvm)/bin/llvm-config"
```

Select another LLVM with `LLVM_CONFIG=/path/to/llvm-config`, or with `-DLLVM_DIR` for a direct CMake build.

The remaining `build.sh` arguments name a stage or a check:

| Argument | Effect |
|---|---|
| `stage0`, `stage1`, `stage2` | Build up to that stage. |
| `bootstrap` | Build Stage0 and Stage1 only. |
| `verify` | Build Stage2, then check that Stage3 and Stage4 are byte-for-byte identical. With `--enable-llvm` it also checks LLVM convergence. |
| `--target=TRIPLE` | Build for another target. |
| `--backend=native\|llvm` | Backend Stage1 uses to generate Stage2 (default `native`). |
| `OUTPUT` | Where to copy the finished compiler. |

Windows has no Native backend. Build there with `./build.sh --bootstrap=c --enable-llvm --backend=llvm`.

### Where the compiler looks for std

The compiler takes the first of these that holds a `Supax.toml` naming the package `std`:

1. `$SULTANC_STD`, the library directory itself (when set, it is used even if it is wrong, so the error names it);
2. `library/` in the current directory (the repository root);
3. next to the compiler: `<bin>/library`, `<bin>/../library` (as `build/sultanc` in a checkout) and
   `<bin>/../lib/sultanc/library` (an install prefix, `<prefix>/bin/sultanc`).

A project's own `library/` directory is not mistaken for std. A source file may be named by a relative path
(`..` included) or an absolute one; relative sources under the working directory are rooted there, others at
their common directory. On Windows, `\` and `/` both separate path components and drive letters are roots.

## A first program

SultanC reads source files with the extension `.sn`. The entry function is `main` in English and `الرئيسية`
in Arabic. It returns an `int`, which becomes the process exit status. The two spellings are
interchangeable at every point, and a file may mix them.

```sultan
import std.io.println;
import std.text.number;

def main(): int {
    println("Hello from SultanC");
    let answer: int = 6 * 7;
    println(int_to_text(answer));
    return 0;
}
```

The first two lines import std modules. `std.io.println` provides `println` and `std.text.number`
provides `int_to_text`. Statements end with a semicolon, and a block that is used as a statement,
such as an `if`, ends with one too. The program is in the repository as `examples/en/basics/hello.sn`, and its Arabic version as
`examples/en/basics/hello.sn`.

## Compiling and running

Pass the source file and, optionally, an output path:

```sh
sultanc examples/en/basics/hello.sn -o hello
./hello
```

```text
Hello from SultanC
42
```

Without `-o` the executable takes the base name of the source file. The default output is a native executable
for the host. Other products are selected with `--emit`:

| Value | Product |
|---|---|
| `exe` | executable (default) |
| `obj` | relocatable object; `-c` is the short form |
| `asm` | assembly text |
| `llvm-ir` | optimized LLVM IR text (LLVM backend) |

`shared`, `reloc` and `module` are also accepted. Whether a target and backend can produce them varies; the
compiler reports `SULTANC-LINK-OUTPUT-UNSUPPORTED` when they cannot.

To link a C library, pass `-l name` for `libname` and `-L dir` to add a directory to the search before the
standard locations.

## Running with the interpreter

`sultanc run` executes a program without producing a file:

```sh
sultanc run examples/en/basics/hello.sn
```

The interpreter runs the same verified intermediate representation (MSIR) and the same std sources that the
compiler uses, so a program behaves the same under `run` and as an executable. Arguments after the source file
are passed to the program unchanged. It is the quickest way to try a snippet, and it runs on Linux, macOS,
Windows and FreeBSD. See [The interpreter](../internals/interpreter.md) for how it works.

## Checking without building

`sultanc check` parses, resolves names, type-checks and runs the ownership and borrow analysis, and stops
before code generation:

```sh
sultanc check src/main.sn
sultanc check --target wasm32-wasi src/main.sn
```

It exits with status 0 when the program is accepted and 1 when diagnostics report an error. With `--target`
it checks for that target, which matters for code that depends on the platform. Without it the host is assumed.

## Cross-compiling

`--target` (`-t`) selects the target triple, and the compiler produces an executable for it with no further
setup:

```sh
sultanc --target=aarch64-linux-gnu examples/en/basics/hello.sn -o hello-arm
sultanc --target=x86_64-windows-msvc examples/en/basics/hello.sn -o hello.exe
sultanc --target=wasm32-wasi examples/en/basics/hello.sn -o hello.wasm
```

`sultanc targets` lists every registered target with its architecture, operating system, object format and
level of support. The compiler has its own linkers for ELF on x86_64 and aarch64 Linux, Mach-O, COFF and
WebAssembly. Other Linux ELF targets and FreeBSD are linked with `ld.lld`, which you can override with the
`SULTANC_LINKER` environment variable. Executables on Linux and FreeBSD are static and use no libc. The
[Targets](../reference/targets.md) reference says what each target supports today.

## The LLVM backend

Two code generators exist. The Native backend generates x86_64, arm64 and wasm32 code directly. The LLVM
backend covers every target and optimizes at O2. `sultanc targets` prints the backends your compiler contains.
Select one with `-b` or `--backend`:

```sh
sultanc --backend=llvm examples/en/basics/hello.sn -o hello
sultanc --backend=llvm --target=riscv64-linux-gnu examples/en/basics/hello.sn -o hello-rv
sultanc --backend=llvm --emit=llvm-ir examples/en/basics/hello.sn -o hello.ll
```

A compiler built without `--enable-llvm` has only the Native backend. See
[MSIR and code generation](../internals/msir-and-codegen.md) for how both backends consume the same IR.

## Diagnostics

Every diagnostic has a stable identity of the form `SULTANC-<AREA>-<NAME>`, a message, and the source
locations involved. Diagnostics are written in Arabic by default. `--lang` chooses the language:
`ar`, `en`, or `auto`, which follows the host locale.

```sh
sultanc --lang=en check bad.sn
```

```text
error[SULTANC-TYPE-MISMATCH]: expected `int`, found `string` — The value does not match the declared type of its destination.
  --> bad.sn:2:18..2:24
   |
 2 |     let x: int = "text";
   |                  ^^^^^^
   |
```

Quoted names are wrapped in Unicode bidirectional isolates so that a Latin identifier inside Arabic text keeps its direction.
`--diagnostic-format` selects how diagnostics are written:

| Format | Output |
|---|---|
| `human` | the annotated source excerpt shown above (default) |
| `short` | one line per diagnostic: `file:line:column: severity[ID]: message [reason]` |
| `json` | one JSON object per diagnostic, for editors and tools |

A JSON diagnostic carries the schema version, the language, the identity, the severity, the message, the primary
span with file, byte offsets, line and column, and the arguments of the message in both languages. `--color`
accepts `auto`, `always` and `never`.

`sultanc explain` describes a diagnostic identity, in the language chosen by `--lang`:

```sh
sultanc --lang=en explain SULTANC-OWNERSHIP-USE-AFTER-MOVE
```

```text
SULTANC-OWNERSHIP-USE-AFTER-MOVE

Value used after move. Find the move in the related spans.
```

A program that ends on an error, a panic or a failed assertion prints its own frame on standard error, in
the language of the program's environment (`SULTANC_LANG`, then `LC_ALL`, `LC_MESSAGES` and `LANG`, then the
`--lang` it was compiled with). [Errors](../reference/errors.md#runtime-failures-of-a-program) describes that path.

## Where to go next

- [Language tour](language-tour.md) walks through the language with short examples.
- The `examples/` directory holds runnable programs in both languages, grouped by topic.
- [Type system](../reference/type-system.md), [Ownership](../reference/ownership.md),
  [Errors](../reference/errors.md), [Compile-time evaluation](../reference/comptime.md) and
  [Modules and imports](../reference/modules-and-imports.md) specify the language.
- [Arabic terminology](../reference/arabic-terminology.md) lists the canonical Arabic vocabulary.
- [Compiler architecture](../internals/compiler-architecture.md) explains how the compiler is organized.

## See also

- [Targets](../reference/targets.md)
- [Interpreter](../internals/interpreter.md)
- [Contributing: examples](../contributing/examples.md)
