[العربية](../../ar/contributing/README.md) | **English**

# Contributor Guide

SultanC is a self-hosted compiler with strict subsystem boundaries. A change should extend the
subsystem that owns the relevant fact and should not add a parallel implementation. This guide is the
entry point for contributors: it covers the repository layout, how to build the compiler for
development, what the seed means for the compiler's sources, how to test a change, and how
the documentation is written.

## Repository layout

| Path | Contents |
| --- | --- |
| `compiler/` | The compiler, written in SultanC (see below) |
| `library/std/` | The standard library, including the allocator and the OS layers |
| `bootstrap/` | The frozen C Stage0 (`FROZEN` pins the commit it compiles; `chain.sh`, `chain.ps1`: the release check) |
| `bootstrap-wasm/` | The WebAssembly seed, its C translator and WASI host |
| `build.sh`, `tools/build/` | The build entry point and the build driver, written in SultanC |
| `tests/` | Qualification suites: `ar/`, `en/` and the `runner/` |
| `tools/` | `build/` (the build driver), `tests/` (CI entry and checks), `generate/` (registries and generators), `audit/` |
| `examples/` | Programs in English (`en/`) and Arabic (`ar/`) |
| `docs/` | Documentation: `book/`, `reference/`, `internals/`, `contributing/`, `history/` |
| `.github/` | CI: `workflows/ci.yml` (every job, in tiers), `promote.yml` (main → beta → master), one script per host family in `ci/` |

Inside `compiler/`:

| Path | Responsibility |
| --- | --- |
| `compiler/frontend/**` | Parsing and semantic analysis |
| `compiler/format/**` | The source formatter (`sultanc fmt`, and the library the language server calls) |
| `compiler/msir/**` | The verified, target-neutral IR |
| `compiler/backend/**` | Targets, ABI and layout, native code generation, object output |
| `compiler/link/**` | The built-in ELF, Mach-O, COFF, WebAssembly and SPIR-V linkers |
| `compiler/ispeak/**`, `compiler/diagnostics/**` | Every user-facing string (Arabic and English) and the diagnostic identities |
| `compiler/runtime/**` | Compiler and runtime contracts |
| `compiler/interpreter/**` | Execution of verified MSIR |
| `compiler/driver/**` | CLI, compilation requests, link plans |

[Compiler Architecture](../internals/compiler-architecture.md) describes how these fit together.

Before changing behavior, identify which subsystem owns the fact. Avoid hardcoded target policy,
duplicate semantic tables, and fixes in generated files when an upstream registry owns the data. The
registries are under `tools/generate/registry`, and the generated files they produce carry a
"generated" header.

### Performance invariants

The compiler compiles itself, so its own performance is part of the build.

- Canonical owners such as programs, type, name and source registries, provenance, debug registries,
  and target and query contexts are never copied for read access. Borrow them and read through the
  reference.
- Mutable builders mutate their owned vectors and tables in place. Do not copy a growing container,
  append to or patch it, and assign the copy back.
- Stable IDs are direct lookup keys. Do not replace an existing ID or index relationship with a linear
  scan.
- Read-only queries do not rebuild, clone or rescan global state to answer facts already owned
  elsewhere. Cache expensive derived analyses under their canonical owner and invalidate them only when
  that owner changes.
- Full-program or full-module scans, and full verification between microscopic transformations, need a
  correctness reason. Prefer indexed lookup, worklists and explicit pipeline verification boundaries
  to verification after every step.
- Control-flow snapshots may copy local flow state when semantics require it. They do not snapshot
  global compiler owners.

## Building for development

`./build.sh` builds the compiler into `build/sultanc`, and `./build.sh --help` lists the options. Each
build is one compile of `compiler/main.sn` (at `-O2`) by a previous compiler: `build/sultanc` when it
exists, otherwise Stage1 from the committed WebAssembly seed, which needs only a C compiler. The last
line printed is `build: compiler: build/sultanc`; `--verbose` shows each command. Without a shell,
`cc -o build-bootstrap bootstrap-wasm/bootstrap.c && ./build-bootstrap` builds the same `build/sultanc`.
[Bootstrap](../internals/bootstrap.md) describes the roots and the policy in full.

```sh
./build.sh                       # one step from build/sultanc (the first time: from the seed)
./build.sh --enable-llvm         # a compiler that also contains the LLVM backend
./build.sh --bootstrap=wasm      # start from the seed again (an older build/sultanc cannot build the sources)
./build.sh --with=/path/sultanc  # one step from another compiler
./build.sh verify                # the self-host fixpoint: S2 == S3 (add --enable-llvm for LLVM)
```

The default build records the hash of the sources under `compiler/` and the build options next to the
compiler (`build/sultanc.stamp`) and does nothing when neither changed, so a change to `library/`,
`tests/`, `docs/` or `examples/` costs no compiler build. The compiler reads std from `library/` for every
program it compiles; its own copy of std follows on its next build (`--rebuild` forces one).

An older bundled compiler binary is not evidence that the current source passes; build the compiler
from the source under test. A compiler for a host without a Native backend (Windows, Linux on arm64,
macOS on x86_64) is generated by LLVM; see [Bootstrap](../internals/bootstrap.md), "Hosts".

`verify` builds S2 from the seed's Stage1 and S3 from S2: the two must be byte-identical. The seed is
current by policy, so S2 is already the fixpoint, and the check needs two compiles.

Run the compiler with the current directory at the repository root.

## The seed and the compiler's sources

The compiler's sources build with the previous compiler: the committed WebAssembly seed's Stage1 on a
cold runner ([Bootstrap](../internals/bootstrap.md), "Roots and policy"). A compiler source may use any
feature that compiler has. A new feature reaches `compiler/` in three commits: it lands unused by the
compiler, a seed-bump commit regenerates the seed (`./build.sh seed-update`), and then the compiler may
use it.

The frozen C Stage0 (`bootstrap/`) no longer compiles the current compiler, so the Stage0 subset that
[Ownership and Destruction](../reference/ownership.md#constraints-on-compiler-sources) records no longer
binds the compiler and std sources. Existing sources keep their shape; new code may use what the seed
supports. A test can still pin a contract against Stage0 with `// مرحلة0: نعم` ([Tests](tests.md)), and
the release check builds the pinned commit's compiler through Stage0 (`bootstrap/chain.sh`).

## Tests

Run focused tests for the subsystem you change, then the suites that CI runs.

```sh
bash tools/tests/ci_run.sh               # tests/runner + iSpeak suite; build/tests-result.json
bash tools/tests/ispeak_test.sh          # iSpeak suite alone (TARGET=, SC= to change)
```

`ci_run.sh` exits non-zero on any failure, on a suite that does not build or exits non-zero, and on a
missing or malformed summary. It reports skipped and compile-only tests apart from passes. [Tests](tests.md)
explains the runner's directives and how to add a test, including the Arabic and English pairs.

A change to the compiler or std sources changes the WebAssembly seed module, and CI requires the
committed seed to be exactly what the sources rebuild when the change lands on `main`. Refresh it with the
change:

```sh
./build.sh seed-update
./build.sh verify && ./build.sh seed-check
```

A seed refresh is a reviewed change of its own. See [Bootstrap](../internals/bootstrap.md) for the seed
check and the CI tiers.

## Examples

Examples live in `examples/en` and `examples/ar` as token-for-token pairs. See [Examples](examples.md)
for how to run, add and check them.

## Documentation

The documentation describes what the compiler and language are today, in the tree
`docs/{guide,reference,internals,contributing,history}`. [`docs/README.md`](../index.md) is the index.
Update the relevant page in the same change as a public compiler contract changes. Development history
belongs in version control and in `docs/history/`; do not add handoff or status Markdown files to the
public tree. The implementation remains authoritative when a page and the source disagree.

Pages follow these conventions.

- Each page opens with a title in Title Case and one to three sentences saying what it covers and who it
  is for. Sections follow in order: concepts, usage, details and edge cases, limits, and a "See also"
  list of relative links. Headings are `##` and `###` in sentence case, without numbers.
- The prose is plain and precise, in the present tense and the active voice. It avoids filler, marketing
  words, hedging and project-history remarks such as "previously" or "now". Lists and tables are used
  only for content that is list-shaped or tabular.
- Code blocks carry a language tag: `sultan` for SultanC sources and `sh` for shell. Every SultanC snippet
  is valid for the current compiler; check one with `sultanc check file.sn` before committing it. Each
  edition shows SultanC code in its own language: English pages in English, Arabic pages in Arabic. Only a
  block whose purpose is to compare the two surfaces shows both.
- Arabic terms follow [Arabic terminology](../reference/arabic-terminology.md) and the current spellings
  (`طالما`, `اختياري` / `موجود` / `لا_شيء`, `طبيعي`, `ط8` to `ط64`, `الرئيسية`, `إنهيار`, `؟`). English
  uses `main`, `uint`, and `option` / `some` / `none`.
- Links between pages are relative. Source references are code spans such as
  `compiler/msir/msir_model.sn`.

## See also

- [Compiler Architecture](../internals/compiler-architecture.md)
- [Bootstrap](../internals/bootstrap.md)
- [Tests](tests.md)
- [Examples](examples.md)
- [Releases](releases.md)
- [Documentation index](../index.md)
