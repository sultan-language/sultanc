[العربية](../../ar/reference/ownership.md) | **English**

# Ownership and Destruction

SultanC gives every owning value a single owner and releases it automatically when the owner goes out of scope.
This page specifies the ownership rule that the compiler enforces, explicit copying with `clone`,
automatic destruction and its allocator, the vector primitives built on it, closure environments, and the limits
that remain. It is for programmers who need the exact rules and for contributors working on the safety analysis
and MSIR lowering.

## Owning and plain types

Memory classification (`compiler/frontend/memory`) marks a type as owning (`يتطلب_تحريرا`) when it holds text, a
vector, a box, or an aggregate member that does. Every other type is plain, and reading a plain value copies it.
Numbers, characters and booleans are plain, and so are records made only of plain members. A `string`, a `vector`
and any record that contains one are owning.

## The ownership rule

The ownership legality analysis on MSIR (`compiler/msir/ssf/ownership_legality.sn`) enforces single ownership of owning values. Using an owning value moves it, and the old name cannot be used again. A function that takes its
parameter by value consumes the argument; a function that takes `&T` or `&mutable T` only
borrows it. Operands are evaluated left to right, and a move takes effect where its operand is
evaluated: a later operand of the same statement cannot use the moved value
(`consume(x) + length(x)` is rejected with `SULTANC-OWNERSHIP-USE-AFTER-MOVE`), while an earlier one
can (`length(x) + consume(x)`).

```sultan
import std.io.println;
import std.text.number;

type Account = { owner: string, balance: int };

def deposit(account: &mutable Account, amount: int): void {
    (*account).balance := (*account).balance + amount;
}

def close(account: Account): string {
    return clone(&account.owner);
}

def main(): int {
    let account: mutable Account = { owner: "Layla", balance: 100 };
    deposit(&mutable account, 50);
    println(int_to_text(account.balance));
    println(close(account));
    return 0;
}
```

After `close(account)` the value has moved, and a later use of `account` is `SULTANC-OWNERSHIP-USE-AFTER-MOVE`. The
diagnostic points at the original owner and at the move.

The rule has these parts:

- **No moves out of borrowed storage** (`SULTANC-OWNERSHIP-MOVE-OUT-OF-REFERENCE`). A use that would move an owning
  value out of borrowed storage is rejected. That covers a dereference `*r`, a projection through a reference such
  as `(*r).f` or `(*r)[i]`, and a borrowed view.
- **Borrowed views are not moves.** An immutable `let x: T = *r;` (or a projection through `r`), and a pattern
  binding whose scrutinee is borrowed storage, read the borrowed storage in place. A view is never destroyed and
  cannot be moved.
- **A view ends when the storage it reads is written.** The writes are an assignment through its reference;
  `append`, `swap`, `pop` or `truncate` through it; and passing the reference, or an exclusive reference local, as a
  `&mutable` argument. Using a view after it ends is reported as use after move
  (`SULTANC-OWNERSHIP-USE-AFTER-MOVE`): the borrow ends at the conflicting write.
- **Boxes.** Moving an owning value out of an owned box consumes the box. Reading a plain value through a box or a
  reference never moves.
- **Borrows conflict.** A write borrow cannot coexist with another live borrow of the same value
  (`SULTANC-BORROW-CONFLICT`). A borrow is live from where it is taken to the last use of what holds it
  ([Borrow regions](#borrow-regions)).
- **No partial moves.** Moving one owning member or element out of an owned aggregate, such as `parts[1]` of an owned
  `vector string`, is `SULTANC-OWNERSHIP-PARTIAL-MOVE`. The primary span is the member that is moved, and the
  declaration of the aggregate is a related span ("declared here").
- **No use before initialization.** Reading a local declared without a value (`let x: int;`) on a path that has not
  assigned it is `SULTANC-FLOW-USE-BEFORE-INITIALIZATION`, marked at the read, with the declaration as the related
  span ("declared here").

### Borrow regions

A borrow is held by the local it is stored in (a reference, a value holding references, a slice view or a
borrowed view) and lasts while that local may still be used: from where the borrow is taken to the last use of the
holder on every path that follows, not to the end of the holder's block. Once a reference is last used, the value it
borrows may be assigned, read, moved, borrowed exclusively, appended to or popped in the same block:

```sultan
def main(): int {
    let value: mutable int = 1;
    let reader: &int = &value;
    let seen: int = *reader;
    value := 2;               // accepted: `reader` is not used again
    return seen + value;
}
```

What counts as a use of the holder:

- every read of it, and of anything computed from it, up to where that is stored in another local or returned;
- the uses of a local it is copied into, or that carries its value on (a call result built from it, a reborrow
  `&*r`, a member `&(*r).f` or a view behind it): the original borrow stays while what derives from it is alive;
- across a loop's back edge, a read in the next iteration: a reference read at the top of a loop keeps its borrow
  over the rest of the previous iteration;
- the destruction of a holder that owns storage (a vector of references, a closure, a record with an owning member
  and a reference): it keeps its borrows until it is destroyed at the end of its block, or moved.

A borrow may not outlive the owner it borrows. An owner is destroyed where its scope ends, so a reference to it
that is still read afterwards (a reference declared outside the owner's block, set inside it and read after it,
or read again in the next iteration of a loop before it is set) escapes its valid scope
(`SULTANC-BORROW-REFERENCE-ESCAPE`, marked where the owner's scope ends). Copying a plain value out through
the reference before that point is fine.

A borrow used on one branch only leaves the value free on the other branch. Giving the holder a new value, or
declaring it again in the next iteration, ends its old borrows. A value that leaves the function (a `return`, a
store into a module value, a closure's result) is still checked against every local it borrows, wherever the borrow
started (`SULTANC-BORROW-REFERENCE-ESCAPE`). The compiler decides these regions on MSIR
([Ownership analysis](../internals/ownership-analysis.md#borrow-regions)); a program they accept is the same program
on every backend and the interpreter.

### Generic templates: inferred plain bounds

A generic template is checked before substitution, with every type parameter treated as owning. Some templates copy
a value of exactly a type parameter `P` out of borrowed storage, for example a map lookup that returns the stored
value. Such a template gets an inferred plain bound on `P`:

1. The analysis lowers and analyzes the template again with `P` plain (`قيد_معامل_نوع_عاديا`, `compiler/msir/ssf/template_legality.sn`).
2. The bound propagates through the generic calls recorded in the call facts. When `F<P>` is instantiated with a
   caller's parameter `Q`, `Q` gets the bound too (`انشر_قيود_عادية_لتدفق`).
3. Every call that instantiates a bound parameter with an owning type is rejected at that call
   (`تحقق_قيود_عادية_لتدفق`).

The bound is inferred, never written. `map_get<K, i32>` is accepted. `map_get<K, text>` is
rejected at the call, because the lookup would copy owned text out of the map.

The rejection (`SULTANC-OWNERSHIP-MOVE-OUT-OF-REFERENCE`) is marked at the call and names the function, its type
parameter and the owning type: ``btree_map_keys` copies values of `م` out of borrowed storage, so `م` must be a
plain type; `string` owns memory`, with the reason and a help that points to a variant that copies each value. The
function is named in the language of the message (its English alias in English). For keys that own memory, std has
those variants: `btree_map_keys_cloned` (`مفاتيح_خريطة_شجرة_بي_منسوخة`) returns a copy of each key in order, and
`map_keys_cloned_with` (`مفاتيح_الخريطة_بناسخ`) copies each key with the function it is given (`copy_text` for text
keys; std.collections.map is compiled by the C Stage0, which has no `clone`).

## Copies

`clone(&x)` is a compiler primitive (`هوية_بدائي_مترجم.استنساخ`). Its result is an independent deep
copy of the referenced type (`فئة_ناتج_بدائي.قيمة_المرجع`). Expression lowering emits the primitive, and
`اخفض_غراء_الملكية_مسير` (`compiler/msir/lowering/ownership_glue.sn`) rewrites it into per-type clone glue:
ordinary MSIR functions generated once per concrete type. Native, LLVM and the
interpreter run the same glue.

| Type | Clone glue |
|---|---|
| Vector | a new empty vector, then an append of a copy of each element |
| Text | the same as a byte vector, then ownership transfer to text |
| Record or tuple | copy each member and reassemble |
| Enum, option or result | dispatch on the tag and copy the payloads into the same constructor |
| Box | a new box holding a copy of the payload |
| Plain type | the load alone |

Compiler and std sources are first compiled by the Stage0 seed, which has no copy primitive (see
[Constraints on compiler sources](#constraints-on-compiler-sources)). They make independent copies with ordinary
per-type functions, deep and identical in every stage:

- `انسخ_بايتات`, `انسخ_نص`, `انسخ_اختيار_نص` and `انسخ_اختيار_بايتات` (`library/std/text/from_bytes.sn`) for bytes
  and text;
- one `انسخ_مملوكا_<Type>` function per named owning type, placed beside the type, for example
  `انسخ_مملوكا_نوع_مقنن` in `compiler/frontend/types/type_model.sn`;
- `انسخ_مملوكا_متجه_<Type>` for vectors of a named type;
- a few private structural helpers (`انسخ_مملوكا__…`) where a type has no named owner.

Where the ownership rule rejects a site, the fix follows the intent of the code:

- Read-only access borrows. Registry queries match the registry slot in place, and JSON stringification takes
  references.
- Copy-modify-write-back updates in place. The ownership analysis's state operations, the diagnostic collection, MSIR renumbering and the
  JSON parser work this way.
- Taking ownership out of a slot swaps an empty value in with `swap`. The HTTP client connection, TLS stream state
  and secrets, CSV fields, PKCS#12 results, monomorphization views and MSIR terminators work this way.
- Destructuring patterns transfer owned parts, as in `| result.ok({ bytes: bytes, … })`.
- A genuinely independent value is copied with its per-type function. Branch snapshots of flow and lowering state,
  values returned by public std query APIs, and generic instantiation work this way.

## Destruction

One target-neutral MSIR implementation (`compiler/msir/lowering/drops.sn`, `compiler/msir/drop_elaboration.sn`,
`ownership_glue.sn`) is consumed uniformly by the Native backends (x86_64, ARM64, wasm32), by LLVM and by the
interpreter. All three receive the same finalized MSIR, with the same `تحرير` primitive rewritten into the same
per-type drop-glue MSIR functions; none has its own notion of when to destroy a value. The stages are described
for compiler contributors in [Ownership Analysis](../internals/ownership-analysis.md).

1. **Moves and views.** The ownership legality analysis on MSIR decides which uses are legal. Which reads of an owner move its value out is derived on
   MSIR by drop elaboration, from the uses of each load of the owner's place (a call argument, a store, a return,
   an aggregate member and the other uses that take ownership; [Ownership Analysis](../internals/ownership-analysis.md)).
   Lowering decides which locals are borrowed views, on the MSIR that reads the value each is given
   (`قيمة_مستعارة_عروض_خفض`, `compiler/msir/lowering/borrow_events.sn`), and makes no owner of them. A `match` arm that binds an
   owning value by move out of an owned, non-reference scrutinee also moves the scrutinee's root local, so the
   scrutinee cannot be read again afterward.
2. **A drop frame per function** (`drops.sn`). Lowering gives every function a prologue block that allocates a
   place for each owner local in declaration order, and states each owner's ownership on that place in initial
   MSIR: its initialization on declaration, parameter binding or match-arm binding, its give-up on a recorded move,
   an explicit release or a closure capture, and a destruction obligation where it leaves scope. Every `return`,
   explicit or by falling off the end, routes through one shared exit block that holds an obligation for every
   owner in reverse declaration order, before the real return. A loop `break` or `continue` likewise states the
   obligations of the locals going out of scope before branching.
   **Drop elaboration** (`drop_elaboration.sn`) then resolves the obligations of verified initial MSIR from an
   initialization dataflow over the control-flow graph: an owner that holds its value on every path reaching an
   obligation is destroyed unconditionally, one that holds none there leaves nothing, and only one that holds it on
   some paths gets a boolean drop flag. A destruction is the `تحرير` primitive, rewritten into the type's glue.
3. **Drop glue** (`ownership_glue.sn`, mirroring clone glue). One `(T) -> ()` MSIR function exists per owning type.
   Aggregates drop their owning members. Tagged values dispatch on the tag and drop the payload. Vectors drop their
   elements and then release the buffer. Text transfers its buffer to a byte vector and reuses the vector's drop
   body. Boxes drop the payload and then release the box cell. All release calls go through the canonical allocator
   provider (`مزودات_تخصيص_الذاكرة.التحرير`), which adapts a reference argument (a vector or text buffer's first
   element) the way it adapts a box payload.
4. **The allocator** (`library/std/os/allocation.sn`) reuses what destruction frees. Small blocks, up to 4 KiB, are
   reused per size. Large blocks are split to fit a request and merged with free large neighbors when released, and a
   released block that reaches the top of the heap lowers it. On native targets the interior pages of free large
   blocks, and the committed pages well above the top, are returned to the operating system. A block at the top grows
   in place, and a block followed by a free large block grows into it. Releasing an address inside the heap that is
   not a live block (a double release, an interior pointer) traps. An address outside the heap, such as static
   storage holding a text literal passed as an owned value, is ignored.

A loop that allocates and drops an owning record 10^5 times, discarding the result each iteration, holds a flat
resident set of about 300 KB regardless of the iteration count: heap blocks are released and reused rather than
accumulated. This holds with and without an explicit `release` beside the automatic drop, which does not double-free.

The ownership analysis's per-function facts (`compiler/msir/ssf/ownership_legality.sn`) are flat vectors indexed by
value and slot, each built once per function, because under single ownership two aliasing owners would be a double free.

### Memory use of the compiler

Destruction keeps the compiler's own working set small. Peak resident memory for a self-compile of
`compiler/main.sn` on x86_64-linux (Native-only, `--emit=exe`, measured with `/usr/bin/time -f %M`):

| Compiler | Built by | Peak RSS |
|---|---|---|
| Stage1 | the C Stage0 (`bootstrap/**`) | 0.92 GB |
| Stage2 | Stage1 | 0.78 GB |

Stage0 elaborates drops as well (`bootstrap/src/llvm/llvm_aggregates.c`, `llvm_values.c`, `llvm_patterns.c`): it
destroys owned locals at scope exits, arm-local values and temporary match scrutinees. Stage1 therefore releases its
own working data, and its peak stays within about 15% of Stage2's.

## Vector primitives

### Pop and truncate

`pop(v)` and `truncate(v, n)` are compiler primitives.

- **pop** moves the last element out and returns it as `option.some`. An empty vector gives
  `option.none`. The element is moved, not copied: the vector stops owning it and the caller owns it, so nothing
  is dropped.
- **truncate** keeps the first `n` elements (`n: uint`) and drops the others, first to last,
  through their drop glue. An `n` at or past the length changes nothing.

```sultan
def main(): int {
    let v: mutable vector string = ["a", "b", "c"];
    let last: option string = pop(v);
    truncate(v, 1);
    return length(v) + capacity(v);
}
```

Like `append`, both write their first argument, which must be a mutable place: a local, a field, `*r` through a
`&mutable`, or an element. The type checker rejects any other with `SULTANC-TYPE-MISMATCH`. The ownership analysis checks the write as it
checks an append: it conflicts with a live borrow of the vector (`SULTANC-BORROW-CONFLICT`) and ends a view read
through the same reference, so a later use is `SULTANC-OWNERSHIP-USE-AFTER-MOVE`. The result of pop is a fresh owned
value.

**Lowering** (`اخفض_إزالة_أو_اقتطاع_مسير`, `compiler/msir/lowering/expressions.sn`) emits ordinary MSIR on the vector's
place. Pop loads the header, reads the length, branches, reads the last element by index extraction and builds the
option in a join block. Truncate loops from `n` to the length, emitting the release primitive on each owning element,
which the ownership-glue pass turns into the element type's drop glue, so text, nested vectors, records, boxes and
closure references are each destroyed exactly once. One internal primitive, `ضبط_طول_متجه`, stores the new length: x86_64 and arm64
store a register into the header's length word, wasm32 an `i32.store`, LLVM a store at the pointer-sized length
width (32-bit targets included), and the interpreter shortens its buffer, copying a static literal's buffer first as
append does.

**An emptied vector keeps its buffer**. Truncate, pop to empty and
`clear_vector` store the new length, zero included, and keep the capacity, so the next appends reuse
the buffer. Drop glue releases a vector's buffer when its capacity is above zero, not when its length is; a vector
that never allocated has capacity zero. A loop that fills and empties vectors of text, nested vectors and closures
10^5 times leaves the allocator's live bytes unchanged (`tests/*/language/surface/vector_pop_rss_flat.sn`).

**At compile time**, evaluation runs the same MSIR in the interpreter. It allows primitives that change a value the
evaluation owns (append, the length store) and releases of the interpreter's own storage, so `comptime` code can
build, pop and truncate vectors. Observable effects, such as machine memory and the host environment, stay
prohibited.

The C Stage0 lowers the same primitives on its vector header (`bootstrap/src/llvm/llvm_builtins.c`), so the compiler
and the std modules it imports may use them. `std.text.number` pops its digits, and `std.collections.vector` builds
its retain, deduplication, remove, swap-remove, insert and clear operations on them.

### Capacity

The vector header is three pointer-sized words: data, length and capacity (`compiler/backend/layout/layout_resolution.sn`).
The layout is the same in x86_64, arm64, wasm32 (32-bit words) and LLVM (pointer-sized on 32-bit targets too).
Capacity is how many elements the buffer holds before an append reallocates it. A vector literal and every append
grow it by one policy (`compiler/runtime/vector_memory_policy.sn`: minimum 1, doubling), so a literal of 3 elements
has capacity 4 in every backend.

- `capacity(v)` reads the header's capacity word as an `int`, as `length` reads the length.
- `reserve_exact(v, n)` makes room for `n` more elements. When
  `length + n` exceeds the capacity, the buffer is reallocated to exactly that many elements through the same
  allocation service and overflow traps as append's growth: x86_64 and arm64 use the append instruction in its
  reserve form, and wasm32 and LLVM use the same guarded realloc. It writes its first argument, a mutable place, as
  append does, and the ownership analysis checks it as a write.
- `std.collections.vector` builds on them: `with_capacity`,
  `reserve` (when it grows, it grows to at least twice the capacity, so
  repeated reserves stay amortized) and `shrink_to_fit` (the elements move in order into a buffer of
  exactly their count, and an empty vector releases its buffer).

The interpreter keeps the same capacity in each buffer (`مخزن_مفسر.السعة`): the growth policy's capacity when a
buffer is made from elements, grown by append, raised by reserve and kept by truncate. `capacity` therefore answers
as it does in an executable. Element 0 of an emptied buffer names the buffer's start, which drop glue releases.
`tests/*/stdlib/behavior/vector_capacity.sn` compares native and interpreter results, and
`vector_capacity_rss_flat.sn` checks that 10^5 rounds of `with_capacity`, `reserve`, `truncate(0)` and reuse, and of
pop to empty and `shrink_to_fit`, leave the allocator's live bytes unchanged.

## Closure environments

A closure value is the address of a heap environment. Closures stay copyable, because the frontend classifies
function types as plain, so the environment is reference counted instead of uniquely owned. Its retain and release
are expressed through the same MSIR ownership glue as every other owned value.

**Layout** (`خطط_بيئة_إغلاق_للاختيار`, `compiler/backend/target/selection_query_context.sn`, shared by x86_64, arm64,
wasm32 and LLVM). A 24-byte header of three 8-byte slots holds the code pointer, the reference count and the
destructor's code pointer, followed by the aligned captured values. A construction (`بناء_إغلاق`) sets the count to
one and stores the destructor. Every function value is an environment, a named function used as a value included.

**Ownership in MSIR.** `يملك_تخزينا_للغراء` counts a function type as owning one reference, so owners of function
values, and of aggregates and vectors holding them, are dropped like any other owner. A type that holds closure
references and nothing else (`هل_نوع_معدود_عادي_مسير`: a function type, or an aggregate of plain members and
function values) is plain for the frontend, so no read of it moves it. Lowering therefore keeps one reference per
owner (`drops.sn`, "Counted values"):

- Every place that takes ownership receives its own reference. These places are a store into a local or place, an
  argument (parameters own their value), a return, an aggregate member or constructor payload, an appended element,
  a closure capture, and a counted match binding. A fresh value (a call result, a closure construction, a moving read)
  is transferred, and any other read is copied with the `احتفاظ_بقيمة` primitive
  (`خفض_تعبير_مملوكا_إلى_مسير`, `احتفظ_بقراءة_معدودة_خفض`).
- A fresh callee (`make()(x)`) is dropped after the call.

**Glue** (`ownership_glue.sn`). `احتفاظ_بقيمة` becomes the type's copy glue, and the copy glue of a function type
counts one more reference (`زيادة_مراجع_إغلاق`). The drop glue of a function type is:

```text
if (closure-release(f)) { closure-destructor(f)(); release(f) }
```

`إنقاص_مراجع_إغلاق` counts one fewer reference and answers whether none is left. `مدمر_بيئة_إغلاق` points the
environment's code slot at its destructor and answers the environment as a `() -> ()` closure, so the ordinary
indirect call runs the destructor with that environment. The destructor of a closure-created function `F` is a glue
function with `F`'s capture types and no parameters: its capture extractions read the dying environment, and it
drops the owned captures (closure values a closure captured). Environments that own nothing share one empty
destructor. The glue pass assigns a destructor to every construction (`بناء_إغلاق_مسير.المدمر`) before any backend
sees the program. These three primitives are the only backend work. Each native backend, LLVM and wasm lowers them in
a few instructions on the header. The interpreter implements the same three primitives
(`compiler/interpreter/execution/primitives.sn`) over its closure handles, counting references as the native header
does.

**Closure bodies.** A closure body is checked and lowered as its own function body. The ownership analysis checks it where it is constructed, as its own function with the parent's
locals it captures (`compiler/msir/ssf/ownership_legality.sn`). Its parameters are owned locals, its captures (plain values the
environment holds) are initialized locals, and the body's value is returned. A body that uses a moved parameter is
therefore rejected (`SULTANC-OWNERSHIP-USE-AFTER-MOVE`), and the moves it makes are recorded. Lowering gives the body
the same drop frame as a function (`اخفض_إغلاق_إلى_مسير`, `compiler/msir/lowering/expressions.sn`): the parameters it
does not consume, function values included, are dropped when it returns, and captures are not, because the
environment's destructor owns them. `tests/*/language/surface/closure_drop_rss_flat.sn` keeps the allocator's live
bytes flat over 10^5 rounds.

**Classification cost.** Lowering asks whether a type owns storage, with and without closures, at every store,
argument and drop. The registry caches the answer per TypeID (`تصنيفات_ملكية_مسير`, filled once before function
bodies are lowered by `املأ_تصنيفات_ملكية_مسير`, `ownership_glue.sn`) instead of walking the type's structure on
every question.

## Temporaries

A call result is a temporary that no local owns, and so is a vector literal used as an expression (`[a, b][i]`,
`length([a, b])`; see [Type system](type-system.md#vector-literal-expressions)). Field, tuple, index and call
suffixes apply to it (`make().name`, `make().inner.n`, `words()[1]`, `make().callback(3)`, `adder(1)(2)`), and
lowering destroys temporaries by these rules (`هل_أصل_مؤقت_للخفض`, `أسقط_بقية_مؤقت_خفض`,
`compiler/msir/lowering/drops.sn`):

- An owning member read from a temporary is moved out, and the temporary's other owning members are destroyed right
  after the read: nothing can name the temporary after it.
- A plain member is copied, and the whole temporary is destroyed.
- An element of a temporary vector is copied (a vector does not let an element move out), and the
  vector is destroyed.
- A member of a temporary is itself fresh: a counted one (a closure) is transferred rather than retained, and a
  fresh callee is dropped after the call. Match scrutinees, the reader primitives (`length`, `clone`) and slice views
  treat it as they treat the call results.

`tests/*/language/surface/temporary_members.sn` (native and interpreter agree) and `temporary_members_rss_flat.sn`
(live bytes flat over 10^5 rounds) cover them.

### Borrowing a temporary

`&` of a literal (an integer, float, bool, char or text literal, or a negated number) creates a temporary that lives
to the end of the enclosing statement, in every expression position: a call argument whatever the call returns
(`larger(&15, &20)`), a constructor argument (`[fmt_arg.text(&"total")]` passed to `format`), a record field, an
element. This is the one rule for `&` on a literal. A value kept past
the statement (a `let` binding or initializer, an assignment, a returned value) must not carry the borrow:
`let r: &int = &5;`, `let args: vector fmt_arg = [fmt_arg.text(&"total")];` and `return &5;` are
`SULTANC-TYPE-ADDRESS-OF-NON-LVALUE` at the literal (`أول_عنوان_حرفي_محمول`,
`compiler/frontend/types/semantic_pipeline.sn`). A value whose type holds no reference carries no borrow, so
`let n: int = *larger(&15, &20);` is accepted.

`&` of another value that has no place (a vector or record literal, a call result) is accepted as a call argument
when the call's result holds no reference, so the borrow cannot leave the call. MSIR lowering stores the value in a
slot the block owns and borrows the slot (`اخفض_عنوان_مؤقت_إلى_مسير`, `compiler/msir/lowering/expressions.sn`).
`tests/{ar,en}/language/surface/literal_address_temporaries.sn`, `temporary_address_arguments.sn`,
`reject_literal_address_kept.sn`, `reject_literal_address_returned.sn` and `reject_temporary_address_escape.sn`
cover it.

## Implicit dereference and borrow

### Field access through a reference

For a reference `r: &T` or `r: &mutable T` to a record, `r.f` means `(*r).f`. A reference to a
reference (`&&T`) is dereferenced once per level. The ownership rules are those of `(*r).f`: reading a plain field
copies it; moving an owning field out of a shared reference is `SULTANC-OWNERSHIP-MOVE-OUT-OF-REFERENCE`;
`r.f := v` and `&mutable r.f` need every reference on the way to be `&mutable`, otherwise they are
`SULTANC-MUTABILITY-WRITE-TO-IMMUTABLE`. A borrow `&r.f` borrows the referent, not the local `r`, so a function may return
`&p.name` for a parameter `p: &Person`.

The Type pass types `r.f` with the rule and records the base expression (`موضع_ضمني`,
`compiler/frontend/types/expression_types.sn`). Before the later phases read the body, the frontend rebuilds it with
the dereferences written out and types it again (`compiler/frontend/desugar/implicit_places.sn`), so patterns, MSIR
lowering and the ownership analysis see `(*r).f` and nothing else changes. `tests/{ar,en}/language/ownership/reference_field_access.sn`
and `reject_reference_field_move.sn` cover it.

### Text arguments for `&string` parameters

`print`, `println`, `eprint` and `eprintln` take `&string`, so they borrow their arguments. At a call, an
argument of type `string` (a place, a literal or any other text value) passed to a parameter of type `&string` is
borrowed for the call, as if written `&arg`, so `let t: string = "x"; println(t); println(t);` prints twice and
`t` stays usable. `println(&t)` and `println("x")` work as well. The rule is for text only: a value of any other
type passed to a reference parameter still needs `&` (`SULTANC-CALL-ARGUMENT-TYPE-MISMATCH` with the help to borrow
it). A literal or call result becomes a temporary as `&"x"` does (see the temporaries rule in
[Type system](type-system.md)).

The Type pass accepts the argument by typing it as `string` and records it; the frontend writes the `&` out with the
dereferences above. The C Stage0 seed has no such rule, so std and the compiler sources pass `&text` explicitly.
`tests/{ar,en}/language/ownership/text_argument_borrow.sn` and `reject_implicit_borrow_other_types.sn` cover it.

## Type registry access

The registry owns every `نوع_مقنن`. Readers resolve an identity with `معرف_نوع_موجود_عند` or `معرف_نوع_أساس_عند`,
then match `(*سجل).الأنواع[id]` in place. Named-type structure is reached through a plain handle,
`موضع_تعريف_بنية`, which is a template slot or an instance slot, and `حقيقة_بنية_عند` reads facts from the borrowed
definition. Generic instantiation (`ضمن_مثيلات_نوع_داخل`, `حل_نوع_مسمى_بحجج`) mutates the registry, so it works on
copies of the template and arguments it needs.

## Constraints on compiler sources

Stage0 (`bootstrap/**`, owned by the bootstrap maintainers) compiles `compiler/**` and the std modules it imports,
so those sources stay within Stage0's language. The constraints, with the reasons, are:

- **No copy primitive, no type reflection, no traits.** Copies are hand-written functions.
- **No reference returns.** Stage0 rejects any function that returns a reference
  (`SULTANC-BORROW-REFERENCE-ESCAPE`), even one derived from a parameter, including inside an option. Accessors
  therefore return identities or handles, and callers borrow the slot. References cannot be stored in record
  literals or assigned to mutable locals.
- **Patterns.** Nested constructor patterns count as exhaustive one level deep (`result.ok(option.none)` beside
  `result.ok(option.some(x))`), and deeper nesting needs a wildcard arm. Alternatives may not bind names.
- **Not in Stage0's language:** closures (`fn(…) -> …` literals), `comptime`, annotations (`@…`), tuples,
  slices (`[] T`), selective imports (`from … import`), import aliases, `main` returning a result, and reading a field
  of a temporary record more than one member deep when it owns memory.
- **Syntax.** `(**x)` does not parse, so write `(*(*x))`. `swap(*r, x)` must be written `swap((*r), x)`.

Stage0 also compiles what the self-hosted frontend added after it was written, with the same semantics: `?`;
`panic` ending its path; module constants and module variables (`let` at the top level, folded at compile time;
their static data keeps capacity 0); `break`, `continue` and `for`; any expression in a vector literal and
after `box`; literal, range and alternative patterns at any depth; float literals, float arithmetic and the float
casts (`llvm.fpto[su]i.sat`); the wrapping intrinsics; `capacity`, `reserve_exact`, `pop`, `truncate`; `release`; members,
elements and calls of call results and calls through function-valued fields; keyword segments in import paths
(`std.math.float`); `>>>`; the std package's Arabic names (`المكتبة.أخطاء.failure`); and Unicode separators.
Unreachable match arms are accepted, and the self-hosted compiler warns about them.

## Limitations

- **Views derived from returned references.** A view held through a reference local derived from an exclusive
  parameter is not ended when that parameter is later passed as `&mutable`. Only views whose holder is the parameter
  itself are ended. Stage0 forbids reference returns, so compiler sources cannot create such a local; user programs
  compiled by Stage2 can.
- **Closures.** The ownership analysis admits only plain captures. A closure that needs an owned value takes it as a parameter, which
  the closure body drops, or captures a closure value, which is counted and dropped by the environment's destructor.

## See also

- [Type system](type-system.md)
- [Safety model](../internals/safety-flow.md)
- [MSIR and code generation](../internals/msir-and-codegen.md)
- [Bootstrap](../internals/bootstrap.md)
- [The SultanC Book](../book/ch00-00-introduction.md)
