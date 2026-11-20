[العربية](../../ar/internals/bootstrap.md) | **English**

# Bootstrap

SultanC is self-hosted: the compiler is written in SultanC, so building it from source needs a
first compiler. This page describes the three routes that provide that first compiler (the C
Stage0, the committed WebAssembly seed, and an existing compiler), how `build.sh` and `cmake/`
chain the stages, how the seed is checked and updated, and how CI proves each route on each host.
It is written for people who build, verify or change the bootstrap.

## Stages

```text
Stage0   the C seed compiler (bootstrap/)             needs a C compiler and an LLVM SDK
Stage1   the first SultanC compiler, built by one of the routes below
Stage2   Stage1 compiles compiler/main.sn             the compiler that build.sh exports
Stage3   Stage2 compiles compiler/main.sn
Stage4   Stage3 compiles compiler/main.sn             Stage3 and Stage4 must be byte-identical
```

All stage policy lives in `cmake/`; `build.sh` only invokes CMake. Stage1 is always a host
executable. Stage2 is the production compiler, Native-only unless `--enable-llvm` is given
([Compiler Architecture](compiler-architecture.md), "Compiler-build capabilities"). Stage1 builds
Stage2 directly with SultanC's own backend; there is no intermediate C build of the compiler.

## Routes to Stage1

| | `--bootstrap=c` (default) | `--bootstrap=wasm` | `--bootstrap=existing` |
|---|---|---|---|
| Stage1 from | the C Stage0 in `bootstrap/` | the seed in `bootstrap-wasm/` | an already-built compiler (`--with=PATH`, default `build/sultanc`) |
| Needs | C compiler and an LLVM SDK | C compiler (an LLVM SDK only with `--enable-llvm`) | the existing compiler (an LLVM SDK only with `--enable-llvm`) |
| CMake tree | `build/bootstrap/cmake` | `build/bootstrap/cmake-wasm` | `build/bootstrap/cmake-existing` |
| Stage1 | LLVM-enabled | Native-only | whatever the existing compiler contains |
| `verify` (Native-only) | native and cross-backend convergence | native convergence (Stage1 has no LLVM backend) | native convergence |
| `stage0` | builds Stage0 | not applicable (fails with a message) | not applicable (fails with a message) |

The C route and the wasm route build a compiler from nothing but sources and are the reference
routes. Use them on a new system, when the existing compiler is too old for the sources (new syntax
or primitives), and to verify a release. The existing route is the fast development route, as Go
uses `GOROOT_BOOTSTRAP`: it skips Stage0 and the seed translation and takes one self-host step from
the existing compiler to the new Stage2.

### build.sh

```sh
./build.sh                                  # Stage2 -> build/sultanc (C route, Native-only)
./build.sh --enable-llvm                    # Stage2 that also contains the LLVM backend
./build.sh verify                           # Stage2, then Stage3 == Stage4
./build.sh verify --enable-llvm             # adds LLVM and cross-backend convergence
./build.sh bootstrap                        # Stage0 + Stage1 only
./build.sh stage0                           # Stage0 only

./build.sh --bootstrap=wasm                 # Stage2 from the seed (needs only a C compiler)
./build.sh verify --bootstrap=wasm          # + Stage3 == Stage4
./build.sh bootstrap --bootstrap=wasm       # Stage1 only -> build/bootstrap/sultanc-stage1

./build.sh --bootstrap=existing             # one self-host step from build/sultanc
./build.sh --bootstrap=existing --with=/path/to/sultanc
```

The first argument may name a stage (`stage0`, `stage1`, `stage2`, `bootstrap`, `verify`); the
default is `stage2`. The other options are:

| Option | Meaning |
|---|---|
| `--enable-llvm` | include the LLVM backend in the built compiler (`SULTANC_COMPILER_LLVM`); with `--bootstrap=c`, Stage0 and Stage1 always use LLVM |
| `--backend=native\|llvm` | backend that Stage1 uses to generate Stage2 (`SULTANC_STAGE2_BACKEND`, default `native`) |
| `--bootstrap=c\|wasm\|existing` | the route (`SULTANC_BOOTSTRAP`) |
| `--with=PATH` | the existing compiler of `--bootstrap=existing` (`SULTANC_EXISTING_COMPILER`) |
| `--target=TRIPLE` | target of Stage2; Stage1 always runs on the build host |
| `OUTPUT` | a path for the requested stage's product |

Products stay in each build tree (`bin/`), and the requested stage is copied to `build/sultanc`
(`build/sultanc.exe` on Windows) for Stage2 or to `build/bootstrap/` for Stage0 and Stage1.
`LLVM_CONFIG=/path/to/llvm-config`, or `-DLLVM_DIR` through CMake, selects the LLVM SDK. CMake can be
driven directly:

```sh
cmake -S cmake -B build/bootstrap/cmake -DSULTANC_REQUESTED_STAGE=stage2
cmake --build build/bootstrap/cmake --target sultanc-stage2
```

Stage1 may use any backend it contains for Stage2, but the Native backend has no Windows target. A
Windows Stage2 therefore needs the C route with LLVM:
`./build.sh --bootstrap=c --enable-llvm --backend=llvm`. The wasm route's Stage1 runs on Windows but
yields Stage1 only there. The existing route does not build an LLVM-enabled Windows Stage2.

For an LLVM-enabled Stage2, the wasm and existing routes pass the selected libLLVM (resolved path)
to Stage1 explicitly. The C route's Stage1 records the same path itself, so every route produces
identical Stage2 bytes.

The existing compiler is copied into the build tree when CMake configures, so the Stage2 that the
route exports to `build/sultanc` never overwrites the compiler that is producing it. The next
configure takes the new `build/sultanc` as its existing compiler.

## Convergence

`verify` checks that the compiler reproduces itself. Stage2 builds Stage3, Stage3 builds Stage4, and
the two must be byte-identical. Every executed compiler targets the build host, and the steps run one
at a time because each self-build of the compiler needs several gigabytes of memory.

- On a host with a Native backend, native convergence is always checked.
- With an LLVM-enabled Stage2, LLVM convergence and cross-backend equality (an LLVM-generated
  compiler building a native one) are checked as well.
- With a Native-only Stage2, the cross-backend check uses a Native-only Stage3 generated by Stage1's
  LLVM backend. It is skipped on the wasm route, whose Stage1 has no LLVM backend, and on the
  existing route, which does not assume the existing compiler has one.
- Windows has no Native backend, so Windows `verify` needs an LLVM-enabled compiler.

## The C Stage0

`bootstrap/` holds a compiler for the SultanC language written in C11. It lexes, parses and analyzes
SultanC sources and emits objects through the LLVM C API (`bootstrap/src/llvm`), so building it needs
an LLVM SDK. Stage0 compiles `compiler/**` and the std modules that the compiler imports into the
Stage1 object, which CMake links with the host C linker driver.

Stage0 is the seed, not a second production implementation. It is compiled with warnings as errors
(`SULTANC_STAGE0_WARNINGS_AS_ERRORS`), is CodeQL-scanned in CI, and has its own drop elaboration so
that the Stage1 it builds releases its working data ([Safety Flow](safety-flow.md)). Because it
compiles the compiler, Stage0's language limits constrain the compiler and std sources; see
[Contributing](../contributing/README.md) and [Ownership and Destruction](../reference/ownership.md).

## The WebAssembly seed

The seed bootstrap commits a WebAssembly build of the compiler and translates it to C, so building
SultanC needs only a C compiler.

```text
seed/sultanc.wasm.zst --wasm2c.c (+ zstd_decoder.h)--> C --cc + wasi.c--> sultanc-stage1
sultanc-stage1 --compiler/main.sn--> sultanc-stage2 (native) --> Stage3 == Stage4
```

`bootstrap-wasm/` holds the seed and its C sources only; the CMake rules are in `cmake/`.

| File | Role |
|---|---|
| `bootstrap-wasm/seed/sultanc.wasm.zst` | the seed: the compiler built for `wasm32-wasi`, Native-only (LLVM provider bound to `unavailable.sn`), compressed with `zstd -19` |
| `bootstrap-wasm/wasm2c.c`, `zstd_decoder.h` | translator from the seed to C99, with an in-tree zstd decoder (MIT) |
| `bootstrap-wasm/wasi.c` | the WASI host that the translated seed links with (POSIX and Windows) |
| `bootstrap-wasm/bootstrap.c` | the standalone driver: seed to Stage1 to Stage2, plus `--verify`, `--check-seed` and `--update-seed` |
| `cmake/modules/WasmSeed.cmake` | the `sultanc-wasm2c` translator and the translated Stage1 |
| `cmake/modules/Stage1Wasm.cmake` | the wasm route of the main build and the `sultanc-check-seed` target |
| `cmake/scripts/CheckSeed.cmake` | the comparison behind `sultanc-check-seed` |

The seed is about 3.2 MB compressed and the module about 12 MB. The main build with
`-DSULTANC_BOOTSTRAP=wasm` provides `sultanc-stage1`, `sultanc-stage2`, `sultanc-verify` and
`sultanc-check-seed`. The seed check uses Stage2 when it runs on the host and otherwise Stage1
itself (Windows, and hosts without a Native backend). For a cross build, pass
`-DSULTANC_WASM2C=<sultanc-wasm2c built for the build machine>`.

### Standalone driver

From the repository root, `bootstrap.c` needs only a C compiler with gcc-style options:

```sh
cc -o build-bootstrap bootstrap-wasm/bootstrap.c
./build-bootstrap --verify            # -> build/bootstrap-wasm/sultanc-stage2
```

On Windows with MSVC, use the main build:

```sh
cmake -S cmake -B build/bootstrap/cmake-wasm -DSULTANC_BOOTSTRAP=wasm -DSULTANC_REQUESTED_STAGE=stage1
cmake --build build/bootstrap/cmake-wasm --target sultanc-stage1
cmake --build build/bootstrap/cmake-wasm --target sultanc-check-seed
```

| Option | Meaning |
|---|---|
| `--cc=CC`, `--cflags=FLAGS` | C compiler and flags (default `$CC` or `cc`, `$CFLAGS` or `-std=c99 -O2`) |
| `--jobs=N` | parallel C compilations (default 4) |
| `--out=DIR` | output directory (default `build/bootstrap-wasm`) |
| `--target=TRIPLE` | target of Stage2 (default: this host) |
| `--stage1-only` | stop after Stage1 |
| `--verify` | build Stage3 and Stage4 and require Stage3 == Stage4 |
| `--check-seed` | rebuild the seed with Stage2 and compare it with the committed seed |
| `--update-seed` | rebuild, compress (`zstd -19`, needs the `zstd` command) and replace the committed seed |

### Seed check and update policy

The seed is used as committed. Nothing pins it by checksum, and no hash files sit beside it. `wasm2c`
translates whatever valid seed it is given and rejects a malformed zstd frame or a malformed module
with a diagnostic, never a crash. Git history and review keep the seed honest, and the seed check
shows whether it still matches the sources.

`--check-seed` (and the CMake target `sultanc-check-seed`) rebuilds the module from the current
sources with Stage2, decompresses the committed seed with the in-tree decoder and compares the two
byte for byte. It reports whether they are equal. When they differ, `bootstrap.c` exits with status 3
and the CMake target fails.

`--update-seed` rebuilds the module, compresses it with `zstd -19`, decodes the result again with the
in-tree decoder to confirm that it gives back the same module, and only then replaces
`seed/sultanc.wasm.zst`. It writes nothing else.

The policy follows from CI, which requires the committed seed to be exactly what the current sources
rebuild. A change to the compiler or std sources that changes the seed module is therefore
accompanied by a refreshed seed in the same reviewed change:

```sh
cc -o build-bootstrap bootstrap-wasm/bootstrap.c
./build-bootstrap --update-seed
./build-bootstrap --out=build/bootstrap-check --verify --check-seed    # must pass cleanly
```

### Components

**`wasm2c.c`** translates WebAssembly to C99.

- Accepted input is what SultanC's wasm32 backend can emit (`compiler/backend/architectures/wasm32/`):
  the integer MVP and the sign-extension operators; f32 and f64 constants, loads, stores, `+ - * /`,
  abs, neg, copysign, comparisons and reinterpretation; `memory.copy` and `memory.fill`; one funcref
  table and one 32-bit memory; globals, active segments and a start function. Anything else is
  rejected with the function and offset.
- Imports are bound by name, and each signature is checked against what `wasi.c` implements.
- Stack slots become typed C locals by static stack translation. Every instruction is type-checked,
  so a malformed module or a translator bug fails at translation time.
- Memory accesses in the generated C are bounds-checked. Arithmetic uses only defined C: no signed
  overflow and no implementation-defined shifts or conversions. Big-endian hosts are handled with
  byte swaps.
- Every call between module functions counts against a fixed limit of 100000 nested calls and checks
  the native stack bound that the host set (`w2c_limit_native_stack`). Deep recursion traps with
  `call stack exhausted (more than 100000 nested calls)`, or `(native stack)` on a host whose stack
  is smaller than the limit needs, instead of crashing. The guest's own shadow stack (8 MiB in linear
  memory) traps as an out-of-bounds access when it overflows first, as on wasmtime.
- Output is deterministic and sharded for parallel compilation. The utility mode
  `--decompress FILE.zst OUT` serves the build drivers and the seed check.

**`zstd_decoder.h`** is a Zstandard (RFC 8878) decoder written from the specification for this
project, with no reference-implementation code. It supports all block, literal and sequence
encodings, multiple and skippable frames, and verifies the XXH64 content checksum. It does not
support dictionaries.

**`wasi.c`** implements the 25 WASI Preview 1 functions that std can import, on one portable layer
with two OS backends.

- The POSIX backend covers Linux, macOS, the BSDs, illumos, Haiku and Cygwin.
- The Win32 backend converts UTF-8 and UTF-16 for paths, arguments and the environment, and handles
  handles, Windows clocks and random, UTF-8 console output, and drive-letter and UNC paths. It needs
  only kernel32 at link time.

Common to both: the preopens are `.` as fd 3 and, on POSIX hosts, `/` as fd 4. Every guest pointer is
bounds-checked (`EFAULT`). `fd_readdir` fills its buffer as WASI requires: cookie 0 reads the whole
directory once, sorts it by name (bytewise) and caches it on the descriptor, and later cookies read
from the cache, so listings are the same on every host and file system and a listing read in many
calls costs one scan. The guest runs on a known native stack: Windows uses a thread with a 256 MiB
stack reservation (64 MiB on 32-bit), Linux raises the main thread's soft stack limit to 64 MiB where
the hard limit allows (as the native compiler does), and other POSIX hosts use the main thread's
limit. The bound is passed to the translated code's call check.

**`bootstrap.c`** drives the build. POSIX uses `fork`; Windows uses `CreateProcess` and `cmd`. WASI
Preview 1 cannot set the executable bit, so the driver runs `chmod 0755` on its outputs.

## Hosts

| Host | Stage1 (a working SultanC compiler) | Stage2, verify and seed check |
|---|---|---|
| Linux x86_64 | yes | yes |
| macOS arm64 | yes | yes |
| Linux aarch64, i686, s390x, riscv64 and others; FreeBSD, NetBSD, OpenBSD, illumos | yes | no: SultanC has no Native backend for these, and a host-native Stage2 needs an LLVM-enabled compiler |
| Windows x64 and arm64 (MinGW, clang, MSVC through CMake) | yes | not from the seed, for the same reason; the main build stops with the supported path. A Windows Stage2 comes from the C route with LLVM: `./build.sh --bootstrap=c --enable-llvm --backend=llvm` |

On hosts without a Native backend the seed check rebuilds the seed with Stage1. Stage1 on any host
can compile SultanC programs for x86_64-linux, arm64-darwin and wasm32-wasi, including building the
compiler itself for those targets.

## CI

`.github/workflows/ci.yml` proves every route on every host where it can run. One script serves each
host family: `.github/ci/linux.sh` and `.github/ci/macos.sh` (POSIX sh) and
`.github/ci/windows.ps1` (PowerShell). There are three jobs, each over the six hosts linux-x86_64,
linux-arm64, macos-arm64, macos-x86_64, windows-x86_64 and windows-arm64, except `seed-update`.

**`seed`** builds through the wasm route with only a C compiler and no LLVM.

| Host | What runs |
|---|---|
| linux-x86_64, macos-arm64 (Native backend) | `./build.sh verify --bootstrap=wasm` (seed to Stage1 to Stage2, Stage3 == Stage4), then `sultanc-check-seed` (Stage2 rebuilds the committed seed byte for byte), then the test suite |
| linux-arm64, macos-x86_64 (no Native backend) | `./build.sh bootstrap --bootstrap=wasm`, `sultanc-stage1 version`, then `sultanc-check-seed` (Stage1 rebuilds the seed) |
| windows (x64 and arm64) | Stage1 only, through every C toolchain: MSVC (`cl`) through CMake, clang-cl through CMake, and on x64 also MinGW gcc with `bootstrap.c`. The MSVC Stage1 rebuilds the committed seed, which must match byte for byte |

**`seed-update`** (linux-x86_64) runs `bootstrap.c --update-seed` and then a clean `--verify
--check-seed` from the updated seed, so the next seed update is known to work.

**`llvm`** uses the C route against LLVM 22.1.8, pinned by version and file name and cached by the
workflow. It builds an LLVM-enabled Stage2, checks convergence and runs the test suite.

| Host | What runs |
|---|---|
| linux-x86_64, macos-arm64 | `./build.sh verify --enable-llvm` (Native, LLVM and Native == LLVM convergence), then the test suite |
| linux-arm64, macos-x86_64 | Stage2 generated by LLVM (`./build.sh --enable-llvm --backend=llvm`), then an LLVM Stage3 and Stage4 built with libLLVM as a link input must be byte-identical, then the test suite |
| windows | `sultanc-verify` with the C route, `SULTANC_COMPILER_LLVM=ON` and `SULTANC_STAGE2_BACKEND=llvm` (an LLVM-generated Stage2 and LLVM Stage3 == Stage4), then the test suite under Git for Windows' bash |

The LLVM SDK is provisioned by each script. Linux links a monolithic `libLLVM` from the upstream
release's component archives. macOS arm64 does the same, while macOS x86_64 builds the pinned source,
which takes hours once and is then cached. Windows builds `LLVM-C.dll` from the pinned source with the
pinned archive's clang-cl. The test suite is `tools/tests/ci_run.sh` ([Tests](../contributing/tests.md)).

A separate workflow, `.github/workflows/security.yml`, runs CodeQL over the C Stage0 in `bootstrap/`.
It does not analyze `.sn` sources.

## Qualification of the seed toolchain

The translator, the WASI host and the zstd decoder are checked, outside the compiler's own tests,
against reference implementations.

- Conformance: a module that exercises every accepted operator on edge values, all load and store
  widths and the control-flow constructs gives results identical to wasmtime. Divide by zero,
  overflow, out-of-bounds access, unreachable and bad indirect calls trap the same way.
- Cross builds: the translated Stage1 builds the full compiler byte-identical to the native one on
  32-bit, aarch64 and big-endian (s390x) Linux under qemu-user, and on Windows x64 under wine.
- The zstd decoder matches the `zstd` tool across level and option combinations, piped input with no
  content size, multiple and skippable frames and empty input, and survives corrupted inputs without a
  crash.
- The sources compile warning-free with `-Wall -Wextra -pedantic` on gcc, clang and MinGW, and the
  generated code carries an `inline` fallback for older MSVC, for which CMake sets `/w /bigobj` on the
  generated sources.

## Limitations

- The committed seed must equal what the current sources rebuild, so a source change that alters the
  module needs a seed update in the same change.
- Stage2 from the seed exists only on hosts with a Native backend (see "Hosts").
- `wasm2c` accepts only the subset of WebAssembly that SultanC's own wasm32 backend emits, and the
  zstd decoder has no dictionary support.
- A wasm-route Stage1 has no LLVM backend, so LLVM-generated stages need the C route.

## See also

- [Compiler Architecture](compiler-architecture.md)
- [Contributor guide](../contributing/README.md)
- [Tests](../contributing/tests.md)
- [Ownership and Destruction](../reference/ownership.md)
