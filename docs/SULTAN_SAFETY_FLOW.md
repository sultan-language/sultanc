# Sultan Safety Flow

Sultan Safety Flow (SSF) is the frontend analysis responsible for value state, moves, borrows, lifetime provenance, and escape checks. It operates after the compiler knows the relevant names and types and before MSIR lowering commits the program to execution semantics.

## Value state

SSF tracks whether a local value is available for use. Relevant states include initialized, uninitialized, moved, released, and conservative merged states produced by control flow.

A read, move, release, or mutation is checked against the current state before lowering. If different reachable paths disagree, the merged state remains conservative; a value moved on one path cannot be treated as definitely available afterward.

## Ownership

Ownership classification comes from the memory/type semantic layer. SSF consumes that classification rather than maintaining its own list of owned types.

Owned values are move-aware. Moving a value transfers its usable state to the destination and prevents later use through the previous owner unless the language operation establishes a new valid value.

Owned aggregates are handled as aggregate ownership units. Partial moves that would create unresolved destruction obligations are rejected rather than being silently accepted.

## Borrowing

The core borrowing rule is:

```text
many shared readers
or
one mutable writer
```

A mutable borrow conflicts with any live shared or mutable borrow of the same rooted storage. A shared borrow conflicts with a live mutable borrow.

Borrow records retain source information so a diagnostic can identify both the requested operation and the earlier borrow that makes it illegal.

## Lifetime provenance

References, slices, and aggregate values containing views retain provenance through assignments, parameters, calls, and returns.

A returned view may refer to storage whose lifetime is valid for the caller, such as an allowed parameter-origin reference. A view into function-local storage may not escape the function. The same check applies recursively when an aggregate contains a reference-like value.

## Control flow

Branches and loops snapshot and merge SSF state.

```text
entry state
   |
   +--> branch A --+
   |               |
   +--> branch B --+--> merged state
```

Only facts valid on every reachable incoming path remain definite after the merge. This applies to initialization, moves, borrow state, and other tracked safety facts.

## Containers and reallocation

Operations that can relocate storage must respect active borrows into that storage. For example, a vector cannot grow in a way that invalidates a live reference to one of its elements.

The container/runtime layer owns growth and storage mechanics; SSF owns whether the operation is legal while references are live.

## Unsafe operations

Operations that intentionally bypass normal safe-memory guarantees require the language's unsafe context. The type and safety layers still validate the surrounding operation shape; `unsafe` changes which operations are permitted, not the ownership of unrelated semantic rules.

## Diagnostics

Safety failures are emitted through the normal structured diagnostic system. The diagnostic identity is independent of whether the user requests Arabic or English presentation.

Typical failures include:

- use of an uninitialized value;
- use after move or release;
- conflicting borrows;
- lifetime escape;
- unsafe raw-pointer operations outside an unsafe context;
- invalid bounds-sensitive operations;
- illegal foreign/unsafe operations;
- partial movement of an owned aggregate.

## Relationship to MSIR

SSF is a semantic analysis, not a backend pass. Once lowering produces verified MSIR, native backends and the interpreter consume the already-decided legality of moves, borrows, and lifetimes. They do not rerun source-level borrow analysis.
