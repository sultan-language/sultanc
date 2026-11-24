[العربية](../../ar/book/ch19-00-targets.md) | **English**

# Targets and Cross-Compiling

A compiled program runs on one kind of machine: one processor architecture and one operating system.
That pair is the program's *target*. By default, `sultanc` compiles for the machine it runs on, the
*host*. With `--target`, it compiles for any other target it knows, from the same computer, with
nothing else to install. This is *cross-compiling*.

## The targets

`sultanc targets` lists every target the compiler knows. The first lines:

```console
$ sultanc targets
Target                   Architecture  OS           Object       Support
arm64-darwin             aarch64       Darwin       Mach-O       self-host
x86_64-linux             x86_64        Linux        ELF          self-host
aarch64-linux-gnu        aarch64       Linux        ELF          cross
aarch64-windows-msvc     aarch64       Windows      COFF         cross
aarch64-android          aarch64       Android      ELF          cross
aarch64-freebsd          aarch64       FreeBSD      ELF          cross
x86_64-darwin            x86_64        Darwin       Mach-O       cross
x86_64-windows-msvc      x86_64        Windows      COFF         cross
x86_64-android           x86_64        Android      ELF          cross
x86_64-freebsd           x86_64        FreeBSD      ELF          cross
...
wasm32-wasi              wasm32        WASI         WebAssembly  cross
...
```

A target's name says its architecture, then its operating system: `aarch64-linux-gnu` is 64-bit ARM
running Linux. *Object* is the file format of the program the system loads: ELF on Linux and FreeBSD,
Mach-O on macOS, PE/COFF on Windows. *self-host* targets are those the compiler itself is built and
tested on; *cross* targets are those it compiles for.

## Compiling for another target

```console
$ sultanc --target=aarch64-linux-gnu main.sn -o hello-arm
$ sultanc --target=x86_64-windows-msvc main.sn -o hello.exe
$ sultanc --target=arm64-darwin main.sn -o hello-mac
$ sultanc --target=wasm32-wasi main.sn -o hello.wasm
```

The `file` command shows what each one is:

```console
$ file hello-arm hello.exe hello-mac hello.wasm
hello-arm:  ELF 64-bit LSB executable, ARM aarch64, version 1 (SYSV), statically linked, with debug_info, not stripped
hello.exe:  PE32+ executable (console) x86-64, for MS Windows, 7 sections
hello-mac:  Mach-O 64-bit arm64 executable, flags:<|DYLDLINK|PIE>
hello.wasm: WebAssembly (wasm) binary module version 0x1 (MVP)
```

The compiler has its own linkers for these formats, so no other tools are involved. Programs for Linux
and FreeBSD are *statically linked* and use no C library: copy the file to a machine of that target, and
it runs.

`wasm32-wasi` is WebAssembly with WASI, the standard set of system functions for WebAssembly programs
outside a browser. A `.wasm` program runs in any WASI runtime, on any machine.

## Backends

The part of the compiler that produces machine code is its *backend*, and there are two. The Native
backend, SultanC's own, produces x86_64, arm64 and wasm32 code. The LLVM backend covers every target in
the list. `sultanc version` lists the backends your compiler contains, and a target that needs a missing
backend says so:

```console
$ sultanc --target=riscv64-linux-gnu main.sn -o hello-rv
error[SULTANC-BACKEND-UNAVAILABLE]: the selected code-generation backend is not available in this compiler — The backend is unavailable for target `riscv64-linux-gnu`. LLVM backend is not included in this compiler.
```

A compiler built with `--enable-llvm` (chapter 1) has both. `--backend=llvm` chooses LLVM where both
could serve, for example to use its optimizations.

## Cross-compiling a package

Supax takes the same option. The programs go to a folder named after the target:

```console
$ supax build --target=aarch64-linux-gnu
   Compiling app v0.1.0 (/projects/app)
    Finished in 0.17s → build/bin/aarch64-linux-gnu/app
```

## Code that depends on the target

The standard library hides most differences between systems: the same `read_file` works everywhere.
When a program itself must know where it runs, `sultanc check --target=TARGET` type-checks it for that
target without building, which catches a call that does not exist there. The
[Targets](../reference/targets.md) reference lists what every target supports today.
