<div align="center">
<img src="https://raw.githubusercontent.com/sultan-language/sultanc-vscode/main/icons/sultanc-logo.svg" alt="SultanC" width="100">

[العربية](../../README.md) | **English**

# SultanC

**A self-hosted systems programming language, written in Arabic.**

<p>
<a href="https://github.com/sultan-language/sultanc/actions/workflows/ci.yml"><img src="https://github.com/sultan-language/sultanc/actions/workflows/ci.yml/badge.svg" alt="CI"></a>
</p>
<p>
<img src="https://img.shields.io/badge/self--hosted-yes-1a7f37?style=for-the-badge&labelColor=0d1117" alt="Self-hosted">
<img src="https://img.shields.io/badge/Linux-x86__64%20%7C%20arm64%20%7C%20+8-1a7f37?style=for-the-badge&labelColor=0d1117&logo=linux&logoColor=white" alt="Linux">
<img src="https://img.shields.io/badge/macOS-arm64%20%7C%20x86__64-1a7f37?style=for-the-badge&labelColor=0d1117&logo=apple&logoColor=white" alt="macOS">
<img src="https://img.shields.io/badge/Windows-x64%20%7C%20arm64-1a7f37?style=for-the-badge&labelColor=0d1117" alt="Windows">
<img src="https://img.shields.io/badge/FreeBSD%20%7C%20Android%20%7C%20WASI-yes-1a7f37?style=for-the-badge&labelColor=0d1117" alt="FreeBSD, Android, WASI">
<img src="https://img.shields.io/badge/source-Arabic%20%2B%20English-8957e5?style=for-the-badge&labelColor=0d1117" alt="Language">
<img src="https://img.shields.io/badge/license-MIT-8b949e?style=for-the-badge&labelColor=0d1117" alt="License">
</p>

[Install](#install) &nbsp;·&nbsp; [Language](#language) &nbsp;·&nbsp; [Standard library](#standard-library) &nbsp;·&nbsp; [Targets](#targets) &nbsp;·&nbsp; [Documentation](#documentation)

</div>

<br>

SultanC is a native systems language with ownership-aware safety, algebraic data types, pattern matching and threads. Its keywords exist in Arabic and English as equal, interchangeable surfaces of the same syntax. The compiler is written in SultanC and compiles itself: every commit on main passes a byte-for-byte self-host fixpoint check (S2 == S3).

<br>

<table>
<tr><th width="50%">English</th><th width="50%">العربية</th></tr>
<tr>
<td valign="top">

```
def main(): int {
    let x: int = 40;
    let y: int = 2;
    return x + y;
}
```

</td>
<td valign="top" dir="rtl">

```
عرف الرئيسية(): عدد {
    ليكن س: عدد = 40;
    ليكن ص: عدد = 2;
    أرجع س + ص;
}
```

</td>
</tr>
</table>

Same AST, same semantics, same diagnostics: only the keywords and identifiers change.

Why does SultanC exist? Read [its story](story.md).

## Install

### Download a release

Each [release](https://github.com/sultan-language/sultanc/releases) has an archive per platform with
`sultanc`, `supax` (the package manager), `sultanc-analyzer` (the language server) and the standard library:
`sultanc-VERSION-TARGET.tar.gz` for `x86_64-linux`, `aarch64-linux-gnu`, `arm64-darwin` and `x86_64-darwin`,
and `sultanc-VERSION-x86_64-windows-msvc.zip`. Check it against the release's `SHA256SUMS.txt`, unpack it
and add its `bin/` to `PATH`:

```sh
version=0.1.2 target=x86_64-linux
base=https://github.com/sultan-language/sultanc/releases/download/v$version
curl -fL -O "$base/sultanc-$version-$target.tar.gz" -O "$base/SHA256SUMS.txt"
sha256sum -c SHA256SUMS.txt --ignore-missing    # macOS: shasum -a 256 -c SHA256SUMS.txt --ignore-missing
tar -xzf "sultanc-$version-$target.tar.gz"
export PATH="$PWD/sultanc-$version-$target/bin:$PATH"
sultanc --version
```

On Windows, compare `Get-FileHash sultanc-VERSION-x86_64-windows-msvc.zip` with its line in `SHA256SUMS.txt`,
unpack the zip and add its `bin` folder to your user `Path`. Keep `library/` next to `bin/`: the compiler
reads the standard library from there. `INSTALL.txt` in the archive has the same steps, and
[Releases](contributing/releases.md) says what a release holds.

### Build from source

```sh
git clone https://github.com/sultan-language/sultanc.git && cd sultanc
./build.sh                          # needs only a C compiler; the result is build/sultanc
sudo cp build/sultanc /usr/local/bin/sultanc
```

Without a shell, the same build needs only the C compiler:

```sh
cc -o build-bootstrap bootstrap-wasm/bootstrap.c && ./build-bootstrap
```

On Windows: `cl /Fe:build-bootstrap.exe bootstrap-wasm/bootstrap.c` (or `gcc -o build-bootstrap.exe
bootstrap-wasm/bootstrap.c`), then `build-bootstrap.exe`.

Both end with the compiler at `build/sultanc` (`build\sultanc.exe` on Windows) and print it as their last
line: `build: compiler: build/sultanc`.

| Step | Command | Needs |
|---|---|---|
| Build, or rebuild after a change | `./build.sh` | a C compiler; later builds start from `build/sultanc` and skip the build when `compiler/` did not change |
| Check | `./build.sh verify` | the compiler rebuilds itself byte for byte (S2 == S3) |
| With the LLVM backend inside | `./build.sh --enable-llvm` | LLVM 19–23 |
| Maintainers | `./build.sh seed-update`, `./build.sh seed-check`, `./build.sh chain` | see [Bootstrap](internals/bootstrap.md) |

> `--enable-llvm` finds LLVM through `llvm-config` (Homebrew's on macOS included); `LLVM_CONFIG=/path/to/llvm-config` selects another.
> Windows, Linux on ARM and macOS on Intel have no Native backend: there `build/sultanc` is the seed compiler, which targets x86_64-linux, arm64-darwin and wasm32-wasi; see [Bootstrap](internals/bootstrap.md) for the compilers built for those hosts.

```sh
sultanc main.sn -o app && ./app
```

## Language

- **Ownership, borrowing and move checking**, verified statically before code generation
- **Algebraic data types and pattern matching**, with exhaustiveness checking
- **Generics** and **compile-time evaluation** (`comptime`)
- **Errors as values**: `result` values, `?` propagation, `panic`
- **Threads and synchronization**: spawn/join, atomics, `Arc`, `Mutex`, `Condvar`, `Once`, channels, with a compile-time check on what a thread may capture
- **Arabic and English** as equal source surfaces, including every diagnostic
- **A verified intermediate representation (MSIR)**: every pass goes through a checker before reaching a backend
- **Native output**: ELF, Mach-O, PE/COFF and WebAssembly, with the compiler's own linkers; no libc needed on Linux and FreeBSD

```sh
sultanc check main.sn                       # type-check only
sultanc run main.sn                         # run in the interpreter, no compilation
sultanc --backend=llvm main.sn -o app       # LLVM backend (O2)
sultanc --target=aarch64-linux-gnu main.sn  # cross-compile
sultanc --lang=en check main.sn             # English diagnostics (Arabic by default)
sultanc --diagnostic-format=json check main.sn
sultanc targets                             # list every target
```

The interpreter runs the same MSIR and the same standard library as the compiler, on Linux, macOS, Windows and FreeBSD.

## Standard library

| Area | Highlights |
|---|---|
| Text and formatting | UTF-8 text, Unicode segmentation and normalization, `std.fmt` (format strings checked at compile time) |
| Collections | vectors with capacity, maps, sets, sorting |
| Files and processes | files, directories, links, file locks, streaming child processes, environment |
| Networking | TCP, UDP, Unix sockets, DNS, TLS 1.3, HTTP/1.1 and HTTP/2 (client and server) |
| Data formats | JSON, TOML, CSV, zip, tar |
| Compression | gzip/deflate, zstd, xz/LZMA2 |
| Crypto and hashing | SHA-2, X25519, Ed25519, ChaCha20-Poly1305, AES-GCM, XXH64, CRC |
| Concurrency | `std.thread`, `std.sync` (atomics, Arc, Mutex, RwLock, Condvar, Once, channels) |

See [std coverage](reference/std-coverage.md) for the full list.

## Targets

| OS | Architectures |
|---|---|
| Linux | x86_64, aarch64, riscv64, loongarch64, ppc64le, s390x, mips64el, sparcv9, i686, armv7 |
| macOS | arm64, x86_64 |
| Windows | x64, arm64 |
| FreeBSD | x86_64, aarch64, riscv64 |
| Android | x86_64, aarch64, armv7 |
| WebAssembly | WASI (wasm32); freestanding wasm32/wasm64 |

Native backends for x86_64 and arm64; every other target goes through LLVM. See [targets](reference/targets.md).

## Documentation

| | |
|---|---|
| [Architecture & MSIR](internals/msir-and-codegen.md) | How source becomes native code |
| [Kernel architecture](internals/compiler-architecture.md) | Compiler internals |
| [Bootstrap](internals/bootstrap.md) | The WebAssembly seed, the frozen C Stage0 and the build driver |
| [The compilation cache](internals/compile-cache.md) | What `sultanc` caches and how |
| [Safety flow](internals/safety-flow.md) | Ownership and borrow checking |
| [Ownership analysis](internals/ownership-analysis.md) | Ownership in initial MSIR and drop elaboration |
| [Type system](reference/type-system.md) | Types, inference, generics, casts |
| [Ownership and destruction](reference/ownership.md) | Moves, drops, temporaries, capacity |
| [Errors](reference/errors.md) | Results, `?`, panics, localized messages |
| [Concurrency](reference/concurrency.md) | Threads, atomics, synchronization |
| [Formatting](reference/formatting.md) | `std.fmt` |
| [Interpreter](internals/interpreter.md) | `sultanc run` |
| [Platform dependencies](reference/platform-dependencies.md) | What std needs from each OS |
| [Arabic terminology](reference/arabic-terminology.md) | Canonical Arabic compiler vocabulary |
| [Examples](contributing/examples.md) · [Tests](contributing/tests.md) | `examples/` and the test runner |

## Status

Self-hosting on `x86_64-linux` and `arm64-darwin`, verified by a byte-identical self-host fixpoint from the WebAssembly seed, and by the frozen C Stage0's chain building the same compiler. CI builds on Linux, macOS and Windows (x64 and arm64).

## License

[MIT License](../../LICENSE)
