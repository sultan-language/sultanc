# Contributor Guide

SultanC is a self-hosted compiler with strict subsystem boundaries. A change should extend the
subsystem that owns the relevant fact and should not add a parallel implementation. This guide is the
entry point for contributors: it covers the repository layout, how to build the compiler for
development, the constraints that the bootstrap puts on compiler sources, how to test a change, and how
the documentation is written.

## Repository layout

| Path | Contents |
| --- | --- |
| `compiler/` | The compiler, written in SultanC (see below) |
| `library/std/` | The standard library, including the allocator and the OS layers |
| `bootstrap/` | The C Stage0 seed compiler |
| `bootstrap-wasm/` | The WebAssembly seed, its C translator and WASI host |
| `cmake/`, `build.sh` | Stage configuration and the build entry point |
| `tests/` | Qualification suites: `ar/`, `en/` and the `runner/` |
| `tools/` | `tests/` (CI entry and checks), `generate/` (registries and generators), `audit/` |
| `examples/` | Programs in English (`en/`) and Arabic (`ar/`) |
| `docs/` | Documentation: `guide/`, `reference/`, `internals/`, `contributing/`, `history/` |
| `.github/` | CI: `workflows/ci.yml` with one script per host family in `ci/` |

Inside `compiler/`:

| Path | Responsibility |
| --- | --- |
| `compiler/frontend/**` | Parsing and semantic analysis |
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

`./build.sh` builds the compiler into `build/sultanc` (`build/sultanc.exe` on Windows). All stage policy
lives in `cmake/`, and `./build.sh --help` lists the options. Three routes provide the first compiler;
[Bootstrap](../internals/bootstrap.md) describes them in full.

```sh
./build.sh                       # C Stage0 -> Stage1 -> Stage2 (Native-only): build/sultanc
./build.sh --enable-llvm         # Stage2 that also contains the LLVM backend
./build.sh --bootstrap=wasm      # Stage1 from the WebAssembly seed (needs only a C compiler)
./build.sh --bootstrap=existing  # one self-host step from the compiler in build/sultanc
```

For day-to-day work, build once with the C or wasm route and then use `--bootstrap=existing`: it skips
Stage0 and the seed translation and builds the new Stage2 with the compiler you already have. Use the
C or wasm route on a new system, when the existing compiler is too old for the sources (for example,
when the sources use new syntax or primitives), and to verify a release.

An older bundled compiler binary is not evidence that the current source passes; build the compiler
from the source under test.
Windows has no Native backend, so build there with `./build.sh --bootstrap=c --enable-llvm
--backend=llvm`.

Self-host convergence, in which Stage3 and Stage4 must be byte-identical, is the check that the compiler
reproduces itself:

```sh
./build.sh verify                        # add --enable-llvm for LLVM and cross-backend convergence
./build.sh verify --bootstrap=wasm
```

Run the compiler with the current directory at the repository root.

## Constraints from the bootstrap

The C Stage0 (`bootstrap/`) compiles `compiler/**` and the std modules that the compiler imports, so
those sources stay within the language Stage0 implements. It supports `؟`, `إنهيار` ending its path,
top-level `دع` constants and variables, `اخرج_من_حلقة`, `تابع_حلقة` and `لكل`, calls and other
expressions inside vector literals, literal and range patterns at any depth, floats and the float casts,
the wrapping intrinsics, pop, truncate, capacity and reserve, and members of call results. It still
lacks the following, and compiler and std sources must avoid them:

- the generic copy `استنسخ`; use the per-type copy functions, for example `انسخ_نص(&t)`;
- reference returns, and references stored in record literals or assigned to mutable variables;
- closures, `وقت_الترجمة`, annotations, tuples, slices, selective imports and import aliases;
- vector literals as expressions (an argument, a returned value, a field, an element, as in
  `f([a, b])`); a `let`-bound literal is fine;
- a deferred-initialization local assigned more than once;
- a function named like a type (`من` is a keyword);
- nested constructor patterns beyond one level of exhaustiveness, and alternatives that bind names;
- the forms `(**x)` and `بدّل(*r, x)`, which do not parse; write `(*(*x))` and `بدّل((*r), x)`.

The full list and the reasons are in [Ownership and Destruction](../reference/ownership.md#constraints-on-compiler-sources).
A test can pin a contract against Stage0 with `// مرحلة0: نعم` ([Tests](tests.md)), and the `llvm` CI job
builds the compiler through Stage0.

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
committed seed to be exactly what the sources rebuild. Refresh it in the same change:

```sh
cc -o build-bootstrap bootstrap-wasm/bootstrap.c
./build-bootstrap --update-seed
./build-bootstrap --out=build/bootstrap-check --verify --check-seed
```

A seed refresh is a reviewed change of its own. See [Bootstrap](../internals/bootstrap.md) for the seed
check and the per-host CI jobs.

## Examples

Examples live in `examples/en` and `examples/ar` as token-for-token pairs. See [Examples](examples.md)
for how to run, add and check them.

## Documentation

The documentation describes what the compiler and language are today, in the tree
`docs/{guide,reference,internals,contributing,history}`. [`docs/README.md`](../README.md) is the index.
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
  is valid for the current compiler; check one with `sultanc check file.sn` before committing it. Pages
  about syntax show the English and the Arabic form, in one consistent order.
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
- [Documentation index](../README.md)
