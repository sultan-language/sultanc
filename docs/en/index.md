[العربية](../ar/index.md) | **English**

# SultanC Documentation

This is the documentation for SultanC, a self-hosted systems programming language whose keywords exist in
Arabic and English as two surfaces of the same syntax. The pages are grouped by purpose: the guide
teaches the language, the reference specifies it, the internals describe how the compiler works, and the
contributing pages explain how to change it.

The [README](README.md) gives a short overview of the project and its build routes.

## Guide

| Page | What it covers |
| --- | --- |
| [Getting started](guide/getting-started.md) | Building the compiler, the first program, compiling, running and checking |
| [Language tour](guide/language-tour.md) | The language by example: types, functions, control flow, records, enums, pattern matching, ownership, errors and modules |

## Reference

| Page | What it covers |
| --- | --- |
| [Type system](reference/type-system.md) | Built-in and structural types, type identity, generics, numeric casts and wrapping arithmetic |
| [Ownership and destruction](reference/ownership.md) | Moves, borrows, copies, drop glue, temporaries, vector capacity and the allocator |
| [Errors](reference/errors.md) | Results, propagation with `?`, `panic`, `std.error`, runtime failures and localized messages |
| [Compile-time evaluation](reference/comptime.md) | `comptime`, declaration annotations and compile-time assertions |
| [Modules and imports](reference/modules-and-imports.md) | Import forms, packages and module identity |
| [Concurrency](reference/concurrency.md) | Threads, atomics, `Arc`, `Mutex`, `RwLock`, `Condvar`, `Once`, channels and the capture check |
| [Formatting](reference/formatting.md) | `std.fmt`: format strings, arguments, custom formatters and the compile-time check |
| [Standard library coverage](reference/std-coverage.md) | What std provides, module by module, compared with Rust and Go |
| [Targets](reference/targets.md) | Supported operating systems and architectures, and which backend serves each |
| [Platform dependencies](reference/platform-dependencies.md) | Every system function std imports, and what std implements itself |
| [Arabic terminology](reference/arabic-terminology.md) | The canonical Arabic vocabulary of the language, std and diagnostics |

## Internals

| Page | What it covers |
| --- | --- |
| [Compiler architecture](internals/compiler-architecture.md) | The pipeline from source to executable and the layout of `compiler/` |
| [MSIR and code generation](internals/msir-and-codegen.md) | The verified intermediate representation, the native and LLVM backends, and the linkers |
| [Safety flow](internals/safety-flow.md) | How moves, borrows, lifetimes and escapes are checked |
| [Interpreter](internals/interpreter.md) | `sultanc run`: execution model, memory model and host calls |
| [Bootstrap](internals/bootstrap.md) | The C Stage0, the WebAssembly seed, the build routes and how CI proves each one |
| [TLS and cryptography](internals/tls.md) | The TLS 1.3 implementation in std and its cryptographic primitives |
| [iSpeak](internals/ispeak/README.md) | The message layer behind every diagnostic, in Arabic and English, and its [context contracts](internals/ispeak/context-contracts.md) |

## Contributing

| Page | What it covers |
| --- | --- |
| [Contributor guide](contributing/README.md) | Repository layout, development builds, the rules for compiler sources and documentation style |
| [Tests](contributing/tests.md) | Running the test suite, test directives and adding tests |
| [Examples](contributing/examples.md) | The example programs and how they are organized |

## History

These pages are records of past decisions and migrations. They describe the project as it was when they
were written; the guide, reference and internals describe it as it is.

| Page | What it records |
| --- | --- |
| [Byte character literals](history/byte-literal-proposal.md) | A language proposal that has not been adopted |
| [iSpeak history](history/ispeak-history.md) | How the message layer was introduced |
| [iSpeak reason audit](history/ispeak-semantic-audit.md) | A review of every contextual-reason key |
| [Embedded runtime retirement](history/embedded-runtime-retirement.md) | Moving the compiler's embedded runtime into std |
| [OS migration inventory](history/os-migration-inventory.md) | Moving the operating-system layer into std |

## Keeping the documentation current

The implementation is authoritative. When a change alters a contract described here, update the page in
the same change rather than adding a separate status note. Style and conventions are in the
[contributor guide](contributing/README.md).
