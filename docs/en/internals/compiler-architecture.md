[العربية](../../ar/internals/compiler-architecture.md) | **English**

# Compiler Architecture

SultanC is a self-hosted compiler organized around explicit ownership of compiler facts. A
subsystem defines a concept once, and every other subsystem queries or consumes that result
instead of reconstructing it. This page describes the pipeline from source to executable, the
source layout of `compiler/`, and the rules that keep the subsystems separate. It is written for
people who change the compiler.

## Pipeline

```text
source
  -> tokens                      compiler/frontend/lexer
  -> AST                         compiler/frontend/parser, ast, desugar, annotations
  -> modules and names           compiler/frontend/module_loading, names
  -> types and patterns          compiler/frontend/types, pattern
  -> memory and safety (SSF)     compiler/frontend/memory, ssf
  -> semantic lowering           compiler/msir/lowering
  -> MSIR + verifier             compiler/msir
  -> code generation             compiler/backend (Native or LLVM provider), or the interpreter
  -> object files                compiler/backend/object
  -> linker                      compiler/link
  -> executable
```

`compiler/driver` orchestrates the whole sequence for a request (`check`, `run`, or a build with
`--emit=exe|obj|shared|reloc|module|asm|llvm-ir`). The entry point `compiler/main.sn` only calls
the driver's process entry.

Each stage replaces syntax-level information with a semantic identity. English and Arabic source
spellings converge during lexing: later layers operate on canonical token, symbol and type
identities and have no language-specific semantic paths.

## Source layout

| Path | Responsibility |
| --- | --- |
| `compiler/source` | Source files, spans, source maps and provenance |
| `compiler/frontend/lexer` | Tokens, keyword and identifier policy, lexical processing |
| `compiler/frontend/parser`, `ast` | Syntax and AST construction |
| `compiler/frontend/desugar` | Syntax-level rewrites such as `?` propagation and the entry result |
| `compiler/frontend/annotations` | Declaration annotations (decorators) |
| `compiler/frontend/module_loading` | Module discovery, logical module identity and source location |
| `compiler/frontend/names` | Symbols, scopes, declaration identity, visibility and lookup |
| `compiler/frontend/types` | Semantic type identity, compatibility, call signatures, unsafe-operation and thread-capture checks |
| `compiler/frontend/pattern` | Pattern semantics, exhaustiveness and reachability |
| `compiler/frontend/memory` | Memory classification and ownership properties |
| `compiler/frontend/ssf` | Move, borrow, lifetime and control-flow safety state |
| `compiler/frontend/semantic_pipeline.sn` | Runs the frontend stages in order and collects their diagnostics |
| `compiler/msir` | The verified, target-neutral execution representation, its lowering, optimizer, promotion and verifier |
| `compiler/backend/target` | Target identity, registry and platform/backend composition, runtime query contexts |
| `compiler/backend/layout`, `abi` | Physical type layout; calling conventions and ABI classification |
| `compiler/backend/codegen` | Architecture-neutral selection, allocation and liveness machinery |
| `compiler/backend/architectures` | Instruction selection and machine encoding per architecture |
| `compiler/backend/provider` | The `native` and `llvm` code-generation providers |
| `compiler/backend/object` | Mach-O, ELF and WebAssembly object serialization, DWARF production |
| `compiler/link` | The built-in linkers (ELF, Mach-O, COFF, WebAssembly, SPIR-V) and archive reading |
| `compiler/runtime` | Runtime primitive and service contracts |
| `compiler/interpreter` | Execution of verified MSIR |
| `compiler/debuginfo` | Debug-information model shared by the providers |
| `compiler/diagnostics`, `compiler/ispeak` | Diagnostic identities, records and every user-facing string |
| `compiler/driver` | CLI, compilation requests, package manifests, link plans |
| `library/std` | The user-facing standard library |
| `bootstrap`, `bootstrap-wasm`, `cmake` | The seed compilers and the stage build ([Bootstrap](bootstrap.md)) |
| `tests`, `tools` | Qualification suites, generators and audits |

## Frontend

### Names and modules

The name system owns declaration identity, scopes, visibility and symbol resolution. Module
loading owns the relation between logical modules and source files. Consumers use resolved
symbol and module identities and do not repeat lookup rules.

### Types

The type system owns canonical type identity, compatibility, conversion classification and
type-shape queries. Layout and ABI consume type information; they are not alternative type
systems.

### Patterns

Pattern analysis validates pattern meaning and coverage before lowering. MSIR receives the
result of that analysis, not source-pattern syntax that a backend would have to interpret.

### Memory and safety

Memory classification determines properties such as whether a type owns resources. SSF uses
those facts to track value state, moves, borrows, escapes and control-flow merges. See
[Safety Flow](safety-flow.md).

## MSIR

MSIR is the common execution representation for native code generation and interpretation. It
holds target-neutral operations and canonical IDs and passes through a verifier before any
consumer sees it. The contract is documented in [MSIR and Native Code Generation](msir-and-codegen.md).

## Targets and backends

The target layer composes platform facts. It owns the architecture, the operating system and
environment, the ABI, the object format, the runtime provider, register-rule identity,
backend and finalizer selection, and target support metadata.

Architecture backends own machine-code generation. They select and encode instructions and
expose architecture-specific register constraints. They do not decide operating-system policy or
choose Mach-O or ELF; that separation lets one architecture serve more than one platform without
embedding operating-system assumptions into instruction selection.

Linkers follow object formats, not code-generation providers. `compiler/link/provider.sn` maps
a target's object format to its linker family, and every supported output of those families is
produced by SultanC's own linker with no C runtime. ELF executables that the built-in ELF linker
does not lay out go to a system `ld.lld` (`$SULTANC_LINKER`, `compiler/driver/external_linker.sn`);
see [Targets](../reference/targets.md) for the per-target status.

## Backend providers and compiler-build capabilities

Five facts about backends are easy to conflate. Each has one owner and one decision point.

| Fact | Owner | Decided by |
| --- | --- | --- |
| Backend that generates the current program | Driver request; default policy `مزود_الخلفية_المفضل` (`compiler/backend/target/backend_provider_query_context.sn`) | `--backend`, or the target default when absent |
| Backends included in a compiler executable | LLVM provider inclusion boundary, logical module `compiler.backend.provider.llvm.llvm_codegen` | the build of that compiler (`--module-source`, `SULTANC_COMPILER_LLVM`) |
| Targets a backend supports | Target, `يدعم_مزود_الخلفية` | the registered target |
| External libraries a produced program needs | Linker, `compiler/link/dependency/llvm.sn` | undefined LLVM C API symbols of the linked objects |
| LLVM available to the running compiler | `compiler/backend/provider/llvm/toolchain.sn`, reached only through the inclusion boundary | the running compiler's own host loader |

The native provider is part of every SultanC compiler. The LLVM provider is a compiler-build
capability: the target of a compiler executable says nothing about which backends that
executable should contain. A Native-generated `arm64-darwin` compiler built by a WASI-hosted
compiler, for example, must not reference the LLVM C API, because its link would then need a
Darwin `libLLVM` that the WASI host cannot provide
(`SULTANC-LINK-LLVM-LIBRARY-MISSING`).

### The inclusion boundary

The logical module `compiler.backend.provider.llvm.llvm_codegen` is the only way the driver
reaches LLVM. It exposes code generation (`ولد_مخرج_بمزود_إل_إل_في_إم`), the inclusion fact
(`مزود_إل_إل_في_إم_مضمن`) and the host libLLVM location (`موقع_مكتبة_إل_إل_في_إم_المضيف`). The
build of the compiler chooses its source:

- `compiler/backend/provider/llvm/llvm_codegen.sn`, the default source, is the full provider. It
  imports the LLVM lowering, emission, debug-information and toolchain modules. Stage0 always
  compiles this source.
- `compiler/backend/provider/llvm/unavailable.sn` has the same entry points and no LLVM module
  or LLVM C API reference. It is bound with
  `--module-source=compiler.backend.provider.llvm.llvm_codegen=compiler/backend/provider/llvm/unavailable.sn`.

`--module-source=<module>=<file>` is a driver option of the canonical logical environment. The
module is resolved exactly as an import from the root module spells it, and the file is recorded
as that module key's explicit source location (`أضف_موقع_مصدر_وحدة`), which the source locator
consults before deriving a path. The option replaces the source of an existing non-std module
only. An unknown module (`SULTANC-MODULE-NOT-FOUND`), a second binding or a direct input
(`SULTANC-MODULE-SOURCE-CONFLICT`), a missing file (`SULTANC-IO-INPUT-NOT-FOUND`) and a file that
is not `.sn` (`SULTANC-CLI-UNKNOWN-INPUT-TYPE`) are rejected before any module loads. The option
does not select a backend, a target or a library, and nothing in the compiler depends on the
file name of the program being compiled.

Inclusion is ordinary module reachability in the canonical logical environment. The two sources
of the boundary are alternatives for one module key and never coexist in a program. No
condition namespace was added for it because Stage0 accepts only `الهدف.<field>` predicates
over its own seven target fields and parses every arm of every file it loads, so a new
predicate in any module it loads would stop the bootstrap. An explicit module source is
invisible to Stage0, which compiles the default source.

### Availability rules

An explicit backend is used only if the running compiler includes it and the registered target
admits it.

- `--backend=llvm` on a compiler built without LLVM fails before any module is loaded with
  `SULTANC-BACKEND-UNAVAILABLE` (reason `llvm.backend-not-included`).
- A target that the native backend does not implement (`يدعم_مزود_الخلفية`) fails with the same
  diagnostic. Native-only compilers do not claim universal native coverage.
- With no `--backend`, the target default policy chooses Native where it is implemented and
  LLVM otherwise. On a compiler without LLVM the second case is `SULTANC-BACKEND-UNAVAILABLE`.
  No request ever falls back from one backend to the other.
- `sultanc version` prints the included backends (`backends: native` or `backends: native llvm`).

### Host LLVM and target LLVM

The libLLVM that the running compiler is linked with (`toolchain.sn`: `/proc/self/maps` on
Linux, `dladdr` on Darwin) is a host fact. The driver records it in the link plan only for
outputs whose target is the compiler's registered host target, and a compiler without LLVM has
none. Whether an output needs libLLVM is decided by the linker from the output's own undefined
LLVM C API symbols, never from the producer backend:

- a Native-only compiler has no such symbols and gets no libLLVM dependency;
- an LLVM-enabled compiler for another target, wasm32-wasi included, needs that target's libLLVM
  as an explicit link input or stops with `SULTANC-LINK-LLVM-LIBRARY-MISSING`;
- an arbitrary program that calls the LLVM C API keeps that requirement whichever backend
  generated it.

Unresolved symbols are never ignored. No compiler primitive exposes facts of the compiling
toolchain.

### Stages and self-hosting

Stage0 has no capability input and builds the default, LLVM-enabled Stage1, which
`cmake/modules/Stage1.cmake` links with `SultanC::LLVM`. The CMake option
`SULTANC_COMPILER_LLVM` (default `OFF`; `./build.sh --enable-llvm` sets it `ON`) selects the
capability of every self-hosted stage, so a built compiler never depends on libLLVM unless its
build asks for LLVM.

`cmake/CMakeLists.txt` turns `OFF` into `SULTANC_COMPILER_CAPABILITY_ARGS`, the `--module-source`
binding. `Stage2.cmake` passes it when Stage1 generates Stage2 with `SULTANC_STAGE2_BACKEND`, and
`Verify.cmake` passes it to every Stage3 and Stage4 build, so the capability survives
self-hosting. A Native-only verify checks native Stage3 and Stage4 convergence and cross-backend
equality through an LLVM-generated Native-only Stage3 produced by Stage1; LLVM convergence needs
`ON`. On Windows a Native-only compiler cannot regenerate a Windows compiler, so Windows verify
requires `ON`.

Direct invocations of an existing compiler on `compiler/main.sn` compile the on-disk,
LLVM-enabled source, because that is the source Stage0 must compile. They pass the binding for a
Native-only result. A WASI-hosted compiler carries whatever capability its build chose: to build
one, bind the boundary to `unavailable.sn` (a WASI host has no libLLVM to link), and pass the
binding again whenever that compiler builds another Native-only compiler.

```sh
./build.sh                  # Native-only Stage2 (default)
./build.sh --enable-llvm    # full Stage2 (Native + LLVM)

# Full compiler (Native + LLVM), generated by either backend
cmake -S cmake -B build/full -DSULTANC_COMPILER_LLVM=ON -DSULTANC_STAGE2_BACKEND=native
cmake --build build/full --target sultanc-stage2

# Native-only compiler, generated by either backend
cmake -S cmake -B build/native-only -DSULTANC_STAGE2_BACKEND=llvm
cmake --build build/native-only --target sultanc-stage2      # or sultanc-verify

# Directly with an existing compiler
NATIVE_ONLY=--module-source=compiler.backend.provider.llvm.llvm_codegen=compiler/backend/provider/llvm/unavailable.sn
./build/sultanc compiler/main.sn --backend=native "$NATIVE_ONLY" -o build/sultanc-native-only
./build/sultanc compiler/main.sn --target=wasm32-wasi "$NATIVE_ONLY" -o build/sultanc.wasm
wasmtime run --dir=. build/sultanc.wasm --backend=native compiler/main.sn \
    --target=arm64-darwin "$NATIVE_ONLY" -o build/sultanc-arm64-darwin
```

### Comparison with other compilers

rustc includes its LLVM backend only with the `llvm` Cargo feature (`rustc_interface/Cargo.toml`,
`llvm = ['dep:rustc_codegen_llvm']`, and `get_codegen_backend` in `rustc_interface/src/util.rs`),
and `rustc_codegen_cranelift` is an alternative backend selected by name. SultanC does not adopt
dynamic backend loading. LLVM's own CMake (`llvm/docs/CMake.rst`, `LLVM_TARGETS_TO_BUILD`)
makes the target components of a libLLVM a property of that library's build, which is why target
support of the LLVM backend stays a target and libLLVM fact and not a compiler-build capability.

## Runtime boundary

`compiler/runtime` defines compiler-visible low-level runtime contracts. Target runtime
providers implement those contracts for a platform. User-facing APIs belong in `library/std`. A
platform mechanism that generated code requires may live in the runtime or target layer, while a
library abstraction built on top of it belongs in the standard library. The same rule applies to
networking, DNS, synchronization, process and system services, and similar facilities.

## Bootstrap boundary

Bootstrap exists only to produce the first self-hosted compiler stage. The C Stage0 and the
WebAssembly seed are not second production implementations of SultanC. Production semantics and
runtime behavior are not moved into C or LLVM because the seed could express them more easily;
once Stage1 exists, the main compiler architecture is authoritative. The routes, the seed and the
constraints they put on compiler sources are described in [Bootstrap](bootstrap.md).

## Generated surfaces

Some language surfaces are generated from registries under `tools/generate/registry`
(`token_registry.def`, `type_registry.def`, `identifier_policy_registry.def`, and the system-call
tables). The registry is the source of truth. Generated compiler and bootstrap mirrors are derived
outputs and are never edited as independent policy owners.

## Diagnostics

Semantic subsystems report structured diagnostic identities and source locations. Presentation
selects language and format afterwards, so Arabic and English diagnostics describe the same
compiler event and are not separate error systems. See [iSpeak](ispeak/README.md).

## LSP boundary

A language server is a protocol and editor-semantic consumer. It consumes compiler facts and
standard-library functionality and does not reproduce parser, type, filesystem, JSON, URI or
Unicode policy in parallel.

## Change discipline

When changing the compiler:

1. locate the subsystem that owns the fact;
2. change that owner;
3. update consumers through existing contracts;
4. remove obsolete paths after migration;
5. add focused qualification for the changed behavior.

Parallel semantic implementations, hardcoded target defaults and permanent compatibility copies
are avoided because they create conflicting sources of truth.

## See also

- [MSIR and Native Code Generation](msir-and-codegen.md)
- [Safety Flow](safety-flow.md)
- [Interpreter](interpreter.md)
- [Bootstrap](bootstrap.md)
- [Targets](../reference/targets.md)
- [Contributor guide](../contributing/README.md)
