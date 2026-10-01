# MSIR and Native Code Generation

MSIR is SultanC's target-neutral intermediate representation. The frontend decides source-language semantics before lowering; native backends and the interpreter consume verified MSIR rather than reinterpreting the AST.

## Pipeline

```text
source
  |
  v
lexer / parser
  |
  v
module + name + type + pattern + safety analysis
  |
  v
semantic lowering
  |
  v
MSIR
  |
  v
MSIR verifier
  |
  +--------------------+
  |                    |
  v                    v
native backend       interpreter
  |
  v
instruction selection
  |
  v
register allocation / frame lowering
  |
  v
machine encoding
  |
  v
Mach-O or ELF object
  |
  v
native executable
```

The verifier is the boundary between semantic lowering and execution/code generation. An optimization that changes MSIR must produce a program that verifies again before it reaches a backend or the interpreter.

## What MSIR owns

MSIR owns the target-neutral execution model used after semantic analysis. The model includes:

- typed SSA-style values;
- functions, blocks, block parameters, and control flow;
- integer and floating-point operations;
- calls and external calls;
- places used for load/store semantics;
- function and global identities;
- constants and mutable module-level globals;
- runtime primitive identities;
- operations needed by the interpreter and native backends.

MSIR carries canonical type IDs. It does not duplicate the frontend type system or target-specific physical layout.

### Constants and globals

Immutable top-level scalar constants are substituted during lowering and do not require writable runtime storage. Mutable module-level values are represented by MSIR globals with a canonical `GlobalID`, type, initializer, and writability flag.

Native output materializes their layout once using the target data model. Backends refer to the same global identity; object writers decide how that identity is represented in Mach-O or ELF.

## Target boundary

Target selection is separate from source-language semantics. A resolved target describes the architecture and platform contracts required by native code generation, including:

- architecture;
- operating system and environment;
- data model;
- ABI;
- object format;
- runtime provider;
- register rules;
- executable finalization.

The registered native target families are:

| Target | Architecture | ABI | Object format |
| --- | --- | --- | --- |
| `arm64-darwin` | AArch64 | Darwin ARM64 | Mach-O |
| `x86_64-linux` | x86-64 | System V AMD64 | ELF |

Support maturity is recorded by the target registry and is not inferred by the backend.

## Backend responsibilities

Architecture backends own machine-specific code generation:

1. **Instruction selection** maps MSIR operations to architecture-level instructions.
2. **Allocation constraints** describe register classes, clobbers, fixed registers, and operand requirements.
3. **Register allocation and frame lowering** assign physical locations and construct the final frame.
4. **Encoding** produces machine bytes and symbolic relocation requests.

Backends do not decide operating-system policy, object format, runtime selection, or target maturity. Those decisions belong to the target layer.

## ABI and layout

Physical layout and calling convention are distinct concerns.

Layout answers questions such as:

- size and alignment;
- field offsets;
- representation of aggregate values.

The ABI answers questions such as:

- argument and return placement;
- call-preserved and call-clobbered registers;
- stack-call requirements;
- architecture/platform calling convention details.

Code generation queries these owners instead of maintaining backend-local copies of their policy.

## Object emission

The object layer serializes already-selected machine code and data. It owns:

- sections;
- symbols;
- relocation encoding;
- object-format metadata;
- Mach-O or ELF serialization.

Architecture encoders emit relocation intent. The object writer maps that intent to the format-specific relocation.

For mutable globals, the shared native-output model provides initialized bytes, alignment, and the canonical global identity. Mach-O and ELF writers place the data and create the required symbols without introducing a second global model.

## ARM64 / Mach-O

The ARM64 path lowers symbolic addresses through ARM64 address materialization and Mach-O relocations. Writable global data is emitted through the Mach-O data-section path and remains separate from executable text.

## x86-64 / ELF

The x86-64 path distinguishes call relocations from ordinary PC-relative address materialization. Calls use the appropriate PLT-oriented relocation path when required; data/function addresses use PC-relative address relocations. Writable global data is emitted in ELF data sections and resolved into writable executable mappings by finalization.

## Runtime calls

Runtime primitives are identified independently of their platform implementation. The target runtime provider maps those identities to the implementation available for the selected target.

This keeps low-level facilities such as system services, memory operations, networking, synchronization, and other runtime mechanisms out of frontend semantics and out of bootstrap C.

## Interpreter

The interpreter executes verified MSIR. It shares the same semantic program model as native code generation and must not become an alternate parser/type/safety implementation.

Host interaction is provided through interpreter runtime/host adapters. Platform-specific operating-system behavior should remain behind those adapters rather than being copied into the MSIR operation dispatcher.

## Extension rule

A new backend or execution engine should begin at verified MSIR. It may provide its own instruction selection or execution mechanism, but it must consume the same semantic decisions, type identities, runtime contracts, and verified control-flow model as the existing consumers.
