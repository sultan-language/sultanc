# Compiler Architecture

SultanC is organized around explicit ownership of compiler facts. A subsystem defines a concept once; other subsystems query or consume that result rather than reconstructing it independently.

## Repository structure

| Path | Responsibility |
| --- | --- |
| `compiler/frontend/source` | Source files and source locations |
| `compiler/frontend/lexer` | Tokens and lexical processing |
| `compiler/frontend/parser` | Syntax and AST construction |
| `compiler/frontend/module_loading` | Module discovery and loading |
| `compiler/frontend/names` | Symbols, scopes, declaration identity, and lookup |
| `compiler/frontend/types` | Semantic type identity and compatibility |
| `compiler/frontend/pattern` | Pattern semantics and coverage |
| `compiler/frontend/memory` | Memory classification and ownership properties |
| `compiler/frontend/ssf` | Move, borrow, lifetime, and control-flow safety state |
| `compiler/msir` | Verified target-neutral execution representation |
| `compiler/backend/target` | Target identity and platform/backend composition |
| `compiler/backend/layout` | Physical type layout |
| `compiler/backend/abi` | Calling convention and ABI classification |
| `compiler/backend/architectures` | Instruction selection and machine encoding |
| `compiler/backend/object` | Mach-O and ELF serialization |
| `compiler/runtime` | Runtime primitive/service contracts |
| `compiler/interpreter` | Verified-MSIR execution |
| `library/std` | User-facing standard library |
| `bootstrap` | Minimal seed used to obtain the self-hosted compiler |
| `sultanc_lsp` | Language Server Protocol implementation |
| `editors` | Editor integration and packaging |
| `tests` | Regression and qualification suites |

## Frontend

The frontend progressively replaces syntax-level information with semantic identities.

```text
source
  -> tokens
  -> AST
  -> module/name resolution
  -> type + pattern analysis
  -> memory/safety analysis
  -> semantic lowering
  -> MSIR
```

English and Arabic source spellings converge before semantic analysis. Downstream compiler layers operate on canonical token, symbol, and type identities rather than maintaining language-specific semantic paths.

### Names and modules

The name system owns declaration identity, scopes, visibility, and symbol resolution. Module loading owns the relationship between logical modules and source files. Consumers use the resolved symbol/module identities rather than repeating lookup rules.

### Types

The type system owns canonical semantic type identity, compatibility, conversion classification, and type-shape queries. Layout and ABI are consumers of type information; they do not become alternate type systems.

### Patterns

Pattern analysis validates pattern meaning and coverage before lowering. MSIR receives the result of that analysis rather than source-pattern syntax requiring backend interpretation.

### Memory and safety

Memory classification determines properties such as whether a type owns resources. SSF uses those facts to track value state, moves, borrows, escapes, and control-flow merges. See [Safety Model](SULTAN_SAFETY_FLOW.md).

## MSIR

MSIR is the common execution representation for native code generation and interpretation. It contains target-neutral operations and canonical IDs, then passes through a verifier before execution/code generation.

The detailed contract is documented in [MSIR and Native Code Generation](MSIR_CODEGEN_ARCHITECTURE.md).

## Target and backend separation

The target layer composes platform facts. It owns:

- architecture selection;
- operating system/environment identity;
- ABI selection;
- object format;
- runtime provider;
- register-rule identity;
- backend/finalizer selection;
- target support metadata.

Architecture backends own machine code generation. They select and encode instructions and expose architecture-specific register constraints, but they do not decide Darwin/Linux policy or choose Mach-O/ELF independently.

This separation allows the same architecture to support more than one platform without embedding operating-system assumptions into instruction selection.

## Runtime boundary

`compiler/runtime` defines compiler-visible low-level runtime contracts. Target runtime providers implement those contracts for a platform.

User-facing APIs belong in `library/std`. A platform mechanism required by generated code may live in the runtime/Target layer; a normal library abstraction built on top of that mechanism belongs in the standard library.

The same rule applies to networking, DNS, synchronization, process/system services, and similar facilities.

## Bootstrap boundary

Bootstrap exists only to produce the first self-hosted compiler stage. It is not a second production implementation of SultanC.

Production semantics and runtime behavior should not be moved into C or LLVM simply because the seed can express them more easily. Once Stage1 exists, the main compiler architecture remains authoritative.

## Generated surfaces

Some language surfaces are generated from registries under `tools/generate/registry`. The registry is the source of truth; generated compiler/bootstrap mirrors are derived outputs and should not be edited as independent policy owners.

## Diagnostics

Semantic subsystems report structured diagnostic identities and source locations. Presentation can then select language/format without changing the semantic result. Arabic and English diagnostics therefore describe the same compiler event rather than separate error systems.

## LSP boundary

The LSP is a protocol and editor-semantic consumer. It should consume compiler facts and standard-library functionality rather than reproducing parser, type, filesystem, JSON, URI, or Unicode policy in parallel.

## Change discipline

When changing the compiler:

1. locate the subsystem that owns the fact;
2. change that owner;
3. update consumers through existing contracts;
4. remove obsolete paths after migration;
5. add focused qualification for the changed behavior.

Parallel semantic implementations, hardcoded target defaults, and permanent compatibility copies are avoided because they create conflicting sources of truth.
