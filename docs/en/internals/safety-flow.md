[العربية](../../ar/internals/safety-flow.md) | **English**

# Safety Flow

Sultan Safety Flow (SSF) names the rules of value state, moves, borrows, lifetime provenance and
escapes. They are decided by the ownership legality analysis on initial MSIR, after lowering and
before drop elaboration (`compiler/msir/ssf/ownership_legality.sn`, `compiler/msir/ssf/borrow_flow.sn`,
`compiler/msir/ssf/template_legality.sn`; [Ownership Analysis](ownership-analysis.md) describes how).
The frontend analysis that once decided them, on the syntax tree (`compiler/frontend/ssf`), is
gone; below, "SSF" is those rules as the analysis enforces them. This page describes what they
decide, how their results reach destruction, and where `unsafe` is required. It is written for
compiler contributors.

## Value state

SSF tracks whether a local value is available for use. The states are initialized, uninitialized,
moved, released, and the conservative merged states that control flow produces.

A read, move, release or mutation is checked against the current state before lowering. When
reachable paths disagree, the merged state stays conservative: a value moved on one path is not
definitely available afterwards.

## Ownership

Ownership classification comes from the memory and type layer (`compiler/frontend/memory`). SSF
consumes that classification and keeps no list of owned types of its own.

Owned values are move-aware. A move transfers the usable state to the destination and prevents later
use through the previous owner, unless a language operation establishes a new valid value. Owned
aggregates are handled as aggregate ownership units. A partial move that would leave an unresolved
destruction obligation is rejected rather than silently accepted.

## Borrowing

The core rule is many shared readers or one mutable writer.

```text
many shared readers
or
one mutable writer
```

A mutable borrow conflicts with any live shared or mutable borrow of the same rooted storage, and a
shared borrow conflicts with a live mutable borrow. A borrow is live from where it is taken to the
last use of the local that holds it, on any path that follows (a use in a later loop iteration, a
reborrow or a copy that is still alive, and the destruction of a holder that owns storage count): it
does not last to the end of the holder's block ([Borrow regions](ownership-analysis.md#borrow-regions)).
Borrow records keep source information, so a diagnostic can name both the requested operation and the
earlier borrow that makes it illegal.

## Lifetime provenance

References, slices and aggregate values that contain views keep their provenance through
assignments, parameters, calls and returns. A returned view may refer to storage that is valid for
the caller, such as an allowed parameter-origin reference. A view into function-local storage may
not escape the function. The same check applies recursively when an aggregate contains a
reference-like value.

## Control flow

Branches and loops snapshot and merge SSF state.

```text
entry state
   |
   +--> branch A --+
   |               |
   +--> branch B --+--> merged state
```

Only facts valid on every reachable incoming path stay definite after a merge. This covers
initialization, moves, borrow state and the other tracked safety facts
(the joins of the analysis's lattices, `ادمج_حالة_شرعية_مسير` and `ادمج_حالتي_استعارات_مسير`).

## Containers and reallocation

Operations that can relocate storage must respect active borrows into that storage. A vector, for
example, cannot grow in a way that invalidates a live reference to one of its elements. The
container and runtime layer owns growth and storage mechanics; SSF owns whether the operation is
legal while references are live.

## Storage reclamation

SSF decides when a value is moved, released or dead. Destruction is decided on MSIR from what
lowering states about each owner ([Ownership Analysis](ownership-analysis.md)).

1. Lowering decides which locals are borrowed views, on the MSIR that reads their values
   (`قيمة_مستعارة_عروض_خفض`), and makes no owner of them. Which reads of an owner move its value out is derived on MSIR
   by drop elaboration, from the uses of each load of the owner's place.
2. Every function has a drop frame (`compiler/msir/lowering/drops.sn`): each owner has a place in
   the function's entry block, and lowering states its initialization, every move or release, and
   a destruction obligation at every scope exit (returns through one exit block, `break`,
   `continue`, block ends) in reverse declaration order. Temporary match scrutinees and arm-local
   values are owners too.
3. Drop elaboration (`compiler/msir/drop_elaboration.sn`) runs on verified initial MSIR. An
   initialization dataflow over the control-flow graph decides at each obligation whether the owner
   holds its value on every path (an unconditional destruction), on none (the obligation is
   removed) or on some (a drop flag, the only kind left).
4. A destruction is the `تحرير` primitive, rewritten into per-type drop glue
   (`compiler/msir/lowering/ownership_glue.sn`). Vectors drop their elements and release the buffer,
   tagged values drop the payload of their constructor, and boxes drop the payload and release the
   cell. Native, LLVM and the interpreter run the same glue, on finalized MSIR only.
5. The std allocator (`library/std/os/allocation.sn`) reuses released blocks, merges free neighbours
   and returns large free ranges to the operating system.

Closure environments are reference counted and released through the same glue; the model is
described in [Ownership and Destruction](../reference/ownership.md).

Reading an owning value through a reference (`*r`) is not a copy. It is a move out of borrowed
storage, which SSF rejects (`SULTANC-OWNERSHIP-MOVE-OUT-OF-REFERENCE`), or an immutable borrowed
view that is never destroyed. An independent value is made with an explicit copy (`clone`, or a
per-type copy function in compiler sources), so no two owners share a buffer and releasing one never
frees storage that another still reads.

The Stage0 seed (`bootstrap/**`) has its own drop elaboration, so the Stage1 compiler it builds also
releases its working data.

## Unsafe operations

Operations that intentionally bypass the normal memory-safety guarantees require the language's
unsafe context. The type and safety layers still validate the surrounding operation shape. `unsafe`
changes which operations are permitted; it does not relax unrelated semantic rules.

`unsafe` is reserved for operations whose memory safety the compiler cannot guarantee,
not for operations that merely reach the operating system.

| Operation | Needs `unsafe` | Why |
|---|---|---|
| Dereferencing or indexing a raw pointer (`*p`, `p[i]`) | yes | the pointer only claims the memory |
| Converting a raw pointer to a reference (`p as &T`) | yes | creates a checked reference from an unchecked address |
| Inline assembly (`asm`) | yes | arbitrary machine code |
| Raw kernel call (`raw_kernel_call`) | yes | bypasses every typed OS interface: the kernel reads and writes through unchecked pointers, lengths and handles |
| C call at an address (`raw_c_call`) | yes | trusts an address and a signature that nothing checks; only the interpreter's foreign-call bridge uses it ([Interpreter](interpreter.md)) |
| Any use of a writable global whose type is not a scalar (a vector, text, a record) | yes | any function may change it while another holds a reference into it (an append can move its buffer), which SSF, tracking locals, cannot see. A writable scalar global (a counter, a flag, a machine word) is read and written by value and needs no unsafe scope |
| std filesystem, networking, process, time and environment APIs | no | std's implementations uphold SultanC's guarantees and hold the unsafe scopes internally (`library/std/os/platform/**`) |
| Extern (foreign) functions | no | declared platform boundaries, audited in [Platform Dependencies](../reference/platform-dependencies.md) |

Raw kernel calls and pointer operations are therefore `unsafe`, while std's filesystem and
networking APIs are safe, and no ordinary module can make a raw kernel call. Inside std each
raw kernel call sits in its own small `unsafe { ... }` block, next to the code that validates its
arguments, so the unsafe surface stays explicit and reviewable.

## Diagnostics

Safety failures are emitted through the structured diagnostic system. The diagnostic identity does
not depend on whether the user requests Arabic or English presentation. The typical failures are:

- use of an uninitialized value;
- use after move or release;
- conflicting borrows;
- lifetime escape;
- unsafe raw-pointer operations outside an unsafe context;
- invalid bounds-sensitive operations;
- illegal foreign or unsafe operations;
- partial movement of an owned aggregate.

## Relationship to MSIR

SSF is a semantic analysis, not a backend pass. In a compilation, the legality of moves and
initialization in functions declared without type parameters is decided on verified initial MSIR,
by the ownership legality analysis ([Ownership Analysis](ownership-analysis.md#legality)), which
states the same diagnostics: SSF still evaluates these rules, to stop each function's analysis at
its first error and for tooling, which runs no MSIR, but leaves their diagnostics to that analysis.
Generic templates, borrows, lifetimes and escapes are SSF's alone. The native backends and the
interpreter consume the already-decided legality; they do not rerun source-level borrow analysis.

## See also

- [Ownership and Destruction](../reference/ownership.md)
- [Ownership Analysis](ownership-analysis.md)
- [MSIR and Native Code Generation](msir-and-codegen.md)
- [Compiler Architecture](compiler-architecture.md)
