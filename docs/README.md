# SultanC Documentation

The root [README](../README.md) covers installation, basic usage, and the public language overview. The documents here describe the compiler architecture and the language mechanisms that need more detail than the project homepage.

## Reference

| Document | Scope |
| --- | --- |
| [MSIR and code generation](MSIR_CODEGEN_ARCHITECTURE.md) | Verified IR, native lowering, targets, object emission, and the interpreter boundary |
| [Compiler architecture](SULTANC_KERNEL_ARCHITECTURE.md) | Frontend, semantic ownership, runtime, backend, bootstrap, and repository boundaries |
| [Safety model](SULTAN_SAFETY_FLOW.md) | Move, borrow, lifetime, escape, and control-flow state tracking |
| [Type system](TYPE_SYSTEM.md) | Canonical type identity, builtin types, structural types, and compatibility |
| [Arabic terminology](ARABIC_TERMINOLOGY.md) | Canonical Arabic compiler and language vocabulary |

Contributor workflow and repository rules are documented in [CONTRIBUTING.md](../CONTRIBUTING.md).

## Source of truth

Documentation describes the current architecture; the implementation remains authoritative. When a compiler contract changes, update the relevant document with the source change rather than adding a separate status or handoff document.
