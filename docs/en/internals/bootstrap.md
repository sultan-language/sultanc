[العربية](../../ar/internals/bootstrap.md) | **English**

# Bootstrap

SultanC is self-hosted: the compiler is written in SultanC, so building it from source needs a
previous compiler. This page describes where that compiler comes from (the roots), the policies that
keep the roots honest (N-1, the seed bump, the fixpoint), how `build.sh` builds the compiler, how the
frozen C Stage0 checks the seed, and how CI proves each part on each host. It is written for people who
build, verify or change the bootstrap.

## Build, check, maintain

**Build.** `./build.sh`. Without a shell, with only a C compiler:
`cc -o build-bootstrap bootstrap-wasm/bootstrap.c && ./build-bootstrap`. Either way the result is
`build/sultanc` (`build\sultanc.exe` on Windows), and the last line printed is
`build: compiler: build/sultanc`. The stages are intermediates under `build/bootstrap/`; a Stage1 that
`./build-bootstrap` left there is reused by `./build.sh`. On a host without a Native backend
`build/sultanc` is Stage1, the seed compiler, and the build says so in one line. `--verbose` shows each
command; without it a command's output is shown only when it fails, with the command.

**Check.** `./build.sh verify` (`./build-bootstrap --verify` without a shell): the compiler builds itself
again and the two are identical ([The fixpoint](#the-fixpoint)).

**Maintainers.** `./build.sh seed-update` (the seed bump), `./build.sh seed-check` (the committed seed is
what the sources build) and `./build.sh chain` (the release check of the frozen C Stage0); see
[Seed check and update](#seed-check-and-update) and [The frozen C Stage0](#the-frozen-c-stage0).

## Roots and policy

Mature self-hosting toolchains build each compiler with the previous one, keep one portable root in the
tree that changes only by an explicit commit, and check the self-host fixpoint when a change is promoted.
SultanC follows the same rules.

**The committed seed is the only root in the tree.** `bootstrap-wasm/seed/sultanc.wasm.zst` is the
compiler built for `wasm32-wasi`. `bootstrap-wasm/` translates it to C, so any C compiler turns it into a
working compiler (Stage1) on any host. No compiler is ever downloaded.

**N-1.** Every build of the compiler is one compile of `compiler/main.sn` at `-O2` by a previous compiler:
`build/sultanc`, the compiler CI cached for the base commit, or Stage1 from the seed. The compiler sources
must always build with the seed's Stage1. CI proves it on a cold runner, where the seed is the only
previous compiler.

**The seed bump.** A new language feature or primitive reaches the compiler's own sources in three
commits: the feature lands, unused by `compiler/`; a seed-bump commit regenerates the seed
(`./build.sh seed-update`), so the seed's Stage1 has the feature; then `compiler/` may use it. A change
that uses a feature the seed lacks fails the N-1 build.

**The seed is current.** CI requires the committed seed to be exactly what the current sources build
(`./build.sh seed-check`) when a commit lands on `main` and nightly, so a change to the compiler or std
sources that changes the seed module carries its regenerated seed. Then Stage1 from the seed is the
current compiler, and two compiles prove the fixpoint: Stage1 builds S2, S2 builds S3, and S2 must equal S3
byte for byte.

**The frozen C Stage0.** `bootstrap/` holds a compiler for the SultanC language written in C11. It is
frozen: its sources do not change. It compiles the compiler of one pinned commit
(`bootstrap/FROZEN`), and the release check climbs from there to the current sources and requires the
result to equal what the seed builds (see [The frozen C Stage0](#the-frozen-c-stage0)). It is not on any
build path a change takes.

**No Stage0 subset.** Because the C Stage0 no longer compiles the current compiler, the compiler and std
sources are no longer limited to the language Stage0 implements. The constraints that list in
[Ownership and Destruction](../reference/ownership.md#constraints-on-compiler-sources) are lifted: a
compiler source may use any feature the seed's Stage1 has (the seed bump above). Existing sources are not
rewritten for that; new code may use what the seed supports.

## Stages

```text
Stage1   the seed translated to C and compiled: a working compiler on any host (Native-only)
S2       a previous compiler builds compiler/main.sn          build/sultanc
S3       S2 builds compiler/main.sn                           S2 == S3: the fixpoint
```

Stage1 is always a host executable. S2 is the production compiler, Native-only unless `--enable-llvm` is
given ([Compiler Architecture](compiler-architecture.md), "Compiler-build capabilities"). Every compile of
the compiler passes `-O2` explicitly: without `-O` the compiler optimizes nothing
([The `sultanc` command](../reference/command-line.md#optimization-levels)).

## build.sh

`build.sh` is a thin POSIX script: it gets a first compiler from the seed when there is none (Stage1, built
with `bootstrap.c` and `$CC` or `cc`), builds the build driver with it and runs the driver with the same
arguments. The driver, `tools/build/` (`main.sn`, `steps.sn`, `chain.sn`, `support.sn`), is written in
SultanC; it is built again only when its sources or the compiler that builds it changed
(`build/bootstrap/sultanc-build.stamp`). On a host without a Native backend the driver needs a compiler that
has LLVM (`build/sultanc` or `--with`); with Stage1 alone, `build.sh` still runs `bootstrap` and
`seed-check` itself.

```sh
./build.sh                       # one step from build/sultanc (else from the seed) -> build/sultanc
./build.sh --enable-llvm         # the same, with the LLVM backend inside
./build.sh verify                # Stage1 -> S2 -> S3, S2 == S3
./build.sh verify --enable-llvm  # adds the LLVM fixpoint and cross-backend equality
./build.sh bootstrap             # Stage1 only -> build/bootstrap/sultanc-stage1
./build.sh seed-check            # the committed seed equals what the sources build
./build.sh seed-update           # the seed bump: replace the committed seed
./build.sh --with=/path/sultanc  # one step from another compiler
./build.sh --target=TRIPLE       # a compiler for another target -> build/cross/TRIPLE/
./build.sh chain                 # the release check of the frozen C Stage0 (see below)
```

| Option | Meaning |
|---|---|
| `--enable-llvm` | include the LLVM backend in the built compiler; it links libLLVM, found through `LLVM_CONFIG` (or the first `llvm-config`, versions 23 to 19, Homebrew's included) or named by `SULTANC_LLVM_LIBRARY` |
| `--with=PATH` | the previous compiler of the step |
| `--bootstrap=wasm` | start from the seed even when `build/sultanc` exists; `--bootstrap=existing` requires `build/sultanc` or `--with` |
| `--backend=native\|llvm` | the backend that generates the compiler (default `native`; `llvm` needs a previous compiler that has it) |
| `--target=TRIPLE` | build the compiler for TRIPLE |
| `--release-version=VERSION` | the version the built compiler reports (a generated `compiler.version` module) |
| `--rebuild` | build even when nothing changed |
| `--jobs=N`, `-jN` | parallel C compilations of Stage1 and of the C Stage0 (default: the processors) |
| `--verbose` | show each command as it runs |
| `OUTPUT` | also copy the built compiler there |

The default build is the developer loop. It records the hash of the files under `compiler/` and the build
options in `build/sultanc.stamp`, and does nothing when neither changed: a change to `library/`, `tests/`,
`docs/` or `examples/` costs no compiler build (the compiler reads std from `library/` for every program;
its own copy of std follows on its next build). The previous compiler is copied before the step, so the
new `build/sultanc` never overwrites the compiler that builds it. Stage1 (`build/bootstrap/seed/`) is
translated again only when the seed or `bootstrap.c` changed: `build/bootstrap/seed/seed.stamp` holds the
SHA-256 of the lines `<SHA-256>  <path>` of the two files, which `build.sh`, the driver and `bootstrap.c`
all write and check, so the Stage1 a `./build-bootstrap` run left is reused. On a host without a Native
backend and with no compiler that has LLVM, `./build.sh` copies Stage1 to `build/sultanc` (and marks it in
`build/sultanc.stamp`, so the next run refreshes it from the seed).

`SULTANC_COMPILE_TIMEOUT` (seconds) bounds each compile, as CI sets it: a compile that runs longer is
reported as hung. The developer loop's compiles use the compilation cache
([The compilation cache](compile-cache.md)); `verify`, `seed-check`, `seed-update` and `chain` turn it off
(`SULTANC_CACHE=off`) for every compiler they run.

## The fixpoint

`verify` checks that the compiler reproduces itself. Stage1 from the seed (or `--with`) builds S2, S2 builds
S3, and the two must be byte-identical. With a current seed Stage1 is the current compiler, so S2 is already
the fixpoint; a seed that predates the sources fails here (and in `seed-check`). Every compiler runs on the
build host, and the compiles run one at a time because each needs several gigabytes of memory.

With `--enable-llvm` on a host with a Native backend, verify continues: S3 builds an LLVM-generated
compiler (`llvm-a`), which builds itself again (`llvm-b`, the LLVM fixpoint `llvm-a == llvm-b`), and
`llvm-a` builds a native compiler that must equal S3 (cross-backend equality). A host without a Native
backend verifies with `--backend=llvm --with=PATH`.

## The frozen C Stage0

`bootstrap/` lexes, parses and analyzes SultanC sources and emits objects through the LLVM C API
(`bootstrap/src/llvm`), so building it needs an LLVM SDK. It has its own drop elaboration, so the compiler it
builds releases its working data ([Safety Flow](safety-flow.md)). It is compiled with warnings as errors and
CodeQL-scanned (`.github/workflows/security.yml`). Its sources are frozen; only `bootstrap/FROZEN` and the
chain scripts beside them change.

`bootstrap/FROZEN` names the commits the chain climbs:

```text
stage0 9d2642f514f2f5c1347afb57222a9e961fd02bd8   the commit whose compiler Stage0 compiles (tagged)
step   <commit>                                     optional: built by the compiler of the line before
```

`./build.sh chain` (the build driver, `tools/build/chain.sn`, on a host with a Native backend),
`bootstrap/chain.sh` (any POSIX host: before the chain a host without a Native backend has only Stage1,
which cannot build the driver) and `bootstrap/chain.ps1` (Windows, from an MSVC developer environment with
`LLVM_SDK`) run the release check, the same steps in each:

1. build the C Stage0 against the LLVM SDK;
2. Stage0 compiles the `stage0` commit's compiler (exported from the git history), linked with libLLVM;
3. each `step` commit's compiler is built by the compiler of the line before;
4. the chain's last compiler builds the current sources (A, LLVM-enabled), and A builds them again,
   Native-only, for the comparison target (D);
5. Stage1 from the seed builds the same compiler (W), and D must equal W byte for byte.

The comparison target is the host where it has a Native backend and `x86_64-linux` elsewhere: the seed's
Stage1 is Native-only, and it builds a compiler for a Native target on any host. Two independent roots,
the C Stage0 and the WebAssembly seed, so give one compiler (a diverse double-compile). On a host without
a Native backend, A (`build/chain/sultanc-a`) is also the from-source LLVM-enabled compiler for the host,
which the release check's test run uses.

A step is added when the current sources use a feature the chain's last compiler lacks: the commit that
bumped the seed for that feature is a good step, since its compiler has the feature and its own sources do
not use it yet. `sh bootstrap/chain.sh --stage0-only` builds Stage0 alone, as CodeQL does.

## The WebAssembly seed

The seed bootstrap commits a WebAssembly build of the compiler and translates it to C, so building
SultanC needs only a C compiler.

```text
seed/sultanc.wasm.zst --wasm2c/--> C (10 files) --cc + runtime/--> sultanc-stage1
sultanc-stage1 --compiler/main.sn--> S2 --compiler/main.sn--> S3 == S2
```

| File | Role |
|---|---|
| `bootstrap-wasm/seed/sultanc.wasm.zst` | the seed: the compiler built for `wasm32-wasi` at `-O2`, Native-only (LLVM provider bound to `unavailable.sn`), compressed with `zstd -19` |
| `bootstrap-wasm/wasm2c/` | `sultanc-wasm2c`, the translator from the seed to C99: `stream.c` (module bytes, LEB128, loading, with the in-tree zstd decoder `zstd_decoder.h`, MIT), `module.c` (module model, parser, what is accepted), `funcgen.c` (one function body to C), `output.c` (the generated files), `main.c` |
| `bootstrap-wasm/runtime/` | what the translated seed links with: `w2c.h` and `w2c.c` (memory, checked integer operations, traps, call depth) and `wasi.c` (the WASI host, POSIX and Windows) |
| `bootstrap-wasm/bootstrap.c` | the standalone driver: seed to Stage1 to S2, copied to `build/sultanc`, plus `--verify`, `--check-seed` and `--update-seed`; `build.sh` uses it for Stage1 |

The seed is about 4.9 MB compressed and the module about 24 MB (of which 12 MB is DWARF, which the
translation ignores).

### Standalone driver

From the repository root, `bootstrap.c` needs only a C compiler, with gcc-style options or MSVC-style
ones (`cl`, `clang-cl`, chosen by the compiler's name or `--msvc`):

```sh
cc -o build-bootstrap bootstrap-wasm/bootstrap.c
./build-bootstrap                     # -> build/sultanc
./build-bootstrap --verify            # the same, and S2 == S3
```

```bat
cl /Fe:build-bootstrap.exe bootstrap-wasm/bootstrap.c
build-bootstrap.exe --cc=cl           # -> build\sultanc.exe, the seed compiler (no Native backend)
```

A normal run prints one line per phase and the result:

```text
build: translating the seed to C (bootstrap-wasm/seed/sultanc.wasm.zst)
build: compiling C (11 files)
build: building the compiler from compiler/ with the seed compiler
build: compiler: build/sultanc
```

On a host without a Native backend the run stops after Stage1 and says so before the result:
`build: no native backend for this host; build/sultanc is the seed compiler (targets x86_64-linux,
arm64-darwin, wasm32-wasi)`.

| Option | Meaning |
|---|---|
| `--cc=CC`, `--cflags=FLAGS` | C compiler and flags (default `$CC` or `cc`, `$CFLAGS` or `-std=c99 -O2`; `/nologo /O2` for an MSVC-style compiler) |
| `--msvc` | the C compiler takes MSVC-style options |
| `--jobs=N` | parallel C compilations (default 4) |
| `--output=PATH` | where the compiler goes (default `build/sultanc`; `build/cross/TRIPLE/sultanc` for another `--target`) |
| `--out=DIR` | directory of the intermediates (default `build/bootstrap/seed`, the one `build.sh` uses) |
| `--target=TRIPLE` | target of Stage2 (default: this host) |
| `--verbose` | show each command as it runs, and its output |
| `--stage1-only` | stop after Stage1; it is copied to `--output` only when that is given (`build/sultanc` is left alone) |
| `--verify` | build Stage3 with Stage2 and require Stage2 == Stage3 |
| `--check-seed` | rebuild the seed with Stage2 and compare it with the committed seed |
| `--update-seed` | rebuild, compress (`zstd -19`, needs the `zstd` command) and replace the committed seed |

### Seed check and update

The seed is used as committed. Nothing pins it by checksum, and no hash files sit beside it. `wasm2c`
translates whatever valid seed it is given and rejects a malformed zstd frame or a malformed module
with a diagnostic, never a crash. Git history and review keep the seed honest, the seed check shows
whether it matches the sources, and the frozen C Stage0's chain shows that it builds the compiler an
independent root builds.

`./build.sh seed-check` rebuilds the module from the current sources (Native-only, `wasm32-wasi`, `-O2`)
with `build/sultanc` when it is built from these sources, else with an S2 it builds from the seed (Stage1
itself on a host without a Native backend), decompresses the committed seed with the in-tree decoder and
compares the two byte for byte. It exits with status 3 when they differ.

`./build.sh seed-update` (or `bootstrap.c --update-seed`) rebuilds the module, compresses it with
`zstd -19`, decodes the result again with the in-tree decoder to confirm that it gives back the same
module, and only then replaces `seed/sultanc.wasm.zst`. It writes nothing else. A seed bump is a commit
of its own:

```sh
./build.sh seed-update
./build.sh verify && ./build.sh seed-check      # from the new seed: must pass cleanly
```

### Components

**`wasm2c/`** translates WebAssembly to C99.

- Accepted input is what SultanC's wasm32 backend can emit (`compiler/backend/architectures/wasm32/`):
  the integer MVP and the sign-extension operators; f32 and f64 constants, loads, stores, `+ - * /`,
  abs, neg, copysign, comparisons and reinterpretation; `memory.copy` and `memory.fill`; one funcref
  table and one 32-bit memory; globals, active segments and a start function. Anything else is
  rejected with the function and offset.
- Imports are bound by name, and each signature is checked against what `wasi.c` implements.
- Stack slots become typed C locals by static stack translation. Every instruction is type-checked,
  so a malformed module or a translator bug fails at translation time.
- The C is written to be read. An operand stays an expression until it must be stored, so
  `local.get 3; i32.const 16; i32.add; local.set 4` is `v4 = v3 + 16u;`. An expression is stored
  before a local it reads changes, and one that can trap or reads memory or a global is evaluated
  before the next side effect, so evaluation order and traps are exactly the module's. An integer
  operation or conversion on constants is folded to its constant (division, which can trap, is not).
  Float arithmetic stays one operation per statement.
- Control flow keeps its shape: an `if` is C's `if`/`else` (`else if` for chains, a guard
  `if (!c) { ... }` first when the else-branch is the shorter one and never falls through), a loop is
  a labeled block, and a `block` is a label after its end. Indentation shows the ifs and loops.
- Memory accesses in the generated C are bounds-checked. Arithmetic uses only defined C: no signed
  overflow and no implementation-defined shifts or conversions. Big-endian hosts are handled with
  byte swaps.
- Every translated function counts its frame (`w2c_enter`, `w2c_leave` in `w2c.h`) against a fixed
  limit of 100000 nested calls and checks the native stack bound that the host set
  (`w2c_limit_native_stack`). Deep recursion traps with
  `call stack exhausted (more than 100000 nested calls)`, or `(native stack)` on a host whose stack
  is smaller than the limit needs, instead of crashing. The guest's own shadow stack (8 MiB in linear
  memory) traps as an out-of-bounds access when it overflows first, as on wasmtime.
- Output is deterministic: `sultanc.h` (prototypes), `sultanc-module.c` (memory size, globals, the
  indirect-call table, data, `w2c_run`) and 8 function units `sultanc-1.c` to `sultanc-8.c`,
  balanced by code size so they compile in parallel (`--units=N`). The utility mode
  `--decompress FILE.zst OUT` serves the build drivers and the seed check.

**`zstd_decoder.h`** is a Zstandard (RFC 8878) decoder written from the specification for this
project, with no reference-implementation code. It supports all block, literal and sequence
encodings, multiple and skippable frames, and verifies the XXH64 content checksum. It does not
support dictionaries.

**`runtime/w2c.h`, `w2c.c`** are the runtime the generated C is written against: bounds-checked
little-endian loads and stores, the integer operations whose C form needs care (multiplication,
signed division and shifts without signed overflow), `memory.grow`, `memory.copy` and
`memory.fill`, traps and the call-depth guard. They are ordinary source files, reviewed like any
other.

**`runtime/wasi.c`** implements the 25 WASI Preview 1 functions that std can import, on one portable layer
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

| Host | Stage1 (a working SultanC compiler) | A host compiler |
|---|---|---|
| Linux x86_64 | yes | S2 from the seed; `verify` and the seed check run here |
| macOS arm64 | yes | S2 from the seed; `verify` and the seed check run here |
| Linux aarch64, macOS x86_64, Windows x64 and arm64 | yes (Windows: MSVC, clang-cl, MinGW) | no Native backend: a Native-only compiler cross-built on a Native host by an LLVM-enabled compiler (`./build.sh --target=TRIPLE --backend=llvm --with=PATH`, nightly), or the LLVM-enabled compiler of the frozen C Stage0's chain (release check) |
| Linux i686, s390x, riscv64 and others; FreeBSD, NetBSD, OpenBSD, illumos | yes | as the row above: cross-built, or an LLVM-enabled compiler with `--backend=llvm --with=PATH` |

On hosts without a Native backend the seed check rebuilds the seed with Stage1. Stage1 on any host can
compile SultanC programs for x86_64-linux, arm64-darwin and wasm32-wasi, including building the compiler
itself for those targets. There `./build.sh` (with no compiler that has LLVM) and `./build-bootstrap` leave
Stage1 at `build/sultanc` and say so in one line.

## CI

The CI tiers, in short:

| Tier | Trigger | Bootstrap work |
|---|---|---|
| pr | pull request | one build from the previous compiler (the base commit's cached compiler, else Stage1 from the seed), then the tests |
| main | push to `main` | Stage1 → S2 → S3, S2 == S3, the seed check, the tests; S2 is cached for pull requests |
| full | nightly, qualification | the seed route on all six hosts, the seed update, the LLVM fixpoint and cross-backend equality, the compilers cross-built for the hosts without a Native backend, run there |
| release | monthly, `v*` tags, by hand (`chain.yml`) | the frozen C Stage0 chain on all six hosts, equal to the seed route's compiler; the tests with the chain's compiler where no seed-route compiler exists |

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
  generated code carries an `inline` fallback for older MSVC, for which `bootstrap.c --msvc` compiles the
  generated sources with `/w /bigobj`.

## Limitations

- The committed seed must equal what the current sources rebuild, so a source change that alters the
  module needs a seed bump in the same change (CI checks it when the change lands).
- A host compiler from the seed exists only on hosts with a Native backend (see "Hosts").
- `wasm2c` accepts only the subset of WebAssembly that SultanC's own wasm32 backend emits, and the
  zstd decoder has no dictionary support.
- The frozen C Stage0 compiles only the pinned commit's compiler; the chain needs a `step` whenever the
  current sources outgrow the chain's last compiler.

## See also

- [Compiler Architecture](compiler-architecture.md)
- [Contributor guide](../contributing/README.md)
- [Tests](../contributing/tests.md)
