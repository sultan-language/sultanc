# Type System

SultanC uses one canonical semantic type system for Arabic, English, and mixed source. Surface spellings are normalized before downstream semantic consumers see a type identity.

## Canonical identity

```text
source spelling
    |
    v
token / AST type
    |
    v
type resolution
    |
    v
canonical Type ID
```

For example, English and Arabic spellings of a builtin integer resolve to the same semantic type ID. Backends, MSIR, safety analysis, and tooling do not receive separate Arabic and English type systems.

Builtin definitions are generated from the canonical type registry under:

```text
tools/generate/registry/type_registry.def
```

Generated frontend/bootstrap mirrors are derived from that registry.

## Builtin types

The active builtin families include:

- `any`, `bool`, `string`, `char`, `int`, `uint`, `void`;
- signed integers: `i8`, `i16`, `i32`, `i64`;
- unsigned integers: `u8`, `u16`, `u32`, `u64`;
- floating-point values: `f32`, `f64`.

Canonical Arabic spellings are listed in [Arabic Terminology](ARABIC_TERMINOLOGY.md).

## Composite types

The semantic type model also represents the supported composite forms, including:

- tuples;
- boxes;
- option and result values;
- functions;
- pointers;
- shared and mutable references;
- slices;
- vectors;
- mutable wrappers;
- named struct/enum/alias types.

Named struct and enum identity is tied to resolved declaration identity rather than textual spelling alone.

## Type-system responsibilities

The type layer owns:

- canonical type identity;
- builtin metadata;
- type construction/interning;
- named-type resolution;
- equality and compatibility;
- conversion classification;
- scalar/signedness queries;
- integer-literal fit checks;
- semantic type shape exposed to downstream consumers.

Consumers ask these APIs for type facts instead of reconstructing them from AST syntax or source spelling.

## What the type system does not own

Type identity is distinct from physical representation and machine calling convention.

The type system does not decide:

- source tokenization or grammar;
- module/name lookup;
- borrow/lifetime legality;
- target selection;
- physical field offsets or alignment;
- ABI argument/return locations;
- register allocation;
- object format or instruction encoding.

Those facts belong to their respective frontend, layout, target, ABI, and backend layers.

## Layout and ABI consumers

After semantic type resolution, target-aware layers can query the type shape and combine it with the selected target data model.

```text
semantic Type ID
     |
     +--> safety / semantic queries
     |
     +--> layout + target data model
                 |
                 +--> ABI
                 +--> native code generation
```

This keeps the meaning of a type target-neutral while allowing its physical representation to vary by target where required.
