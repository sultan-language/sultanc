# Ownership and Destruction

SultanC gives every owning value a single owner and releases it automatically when the owner goes out of scope.
This page specifies the ownership rule that the compiler enforces, explicit copying with `استنسخ` / `clone`,
automatic destruction and its allocator, the vector primitives built on it, closure environments, and the limits
that remain. It is for programmers who need the exact rules and for contributors working on the safety analysis
and MSIR lowering.

## Owning and plain types

Memory classification (`compiler/frontend/memory`) marks a type as owning (`يتطلب_تحريرا`) when it holds text, a
vector, a box, or an aggregate member that does. Every other type is plain, and reading a plain value copies it.
Numbers, characters and booleans are plain, and so are records made only of plain members. A `string`, a `vector`
and any record that contains one are owning.

## The ownership rule

The safety analysis, SSF (`compiler/frontend/ssf`), enforces single ownership of owning values, with Rust's E0507 as
the model. Using an owning value moves it, and the old name cannot be used again. A function that takes its
parameter by value consumes the argument; a function that takes `&T` (`&T`) or `&mutable T` (`&متغير T`) only
borrows it.

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

```sultan
استورد std.io.println;
استورد std.text.number;

نوع حساب = { المالك: نص, الرصيد: عدد };

عرف أودع(الحساب: &متغير حساب, المقدار: عدد): فراغ {
    (*الحساب).الرصيد := (*الحساب).الرصيد + المقدار;
}

عرف أغلق(الحساب: حساب): نص {
    أرجع استنسخ(&الحساب.المالك);
}

عرف الرئيسية(): عدد {
    دع الحساب: متغير حساب = { المالك: "ليلى", الرصيد: 100 };
    أودع(&متغير الحساب, 50);
    اطبع_سطر(عدد_إلى_نص(الحساب.الرصيد));
    اطبع_سطر(أغلق(الحساب));
    أرجع 0;
}
```

After `close(account)` the value has moved, and a later use of `account` is `SULTANC-OWNERSHIP-USE-AFTER-MOVE`. The
diagnostic points at the original owner and at the move.

The rule has these parts:

- **No moves out of borrowed storage** (`SULTANC-OWNERSHIP-MOVE-OUT-OF-REFERENCE`). A use that would move an owning
  value out of borrowed storage is rejected. That covers a dereference `*r`, a projection through a reference such
  as `(*r).f` or `(*r)[i]`, and a borrowed view.
- **Borrowed views are not moves.** An immutable `دع x: T = *r;` (or a projection through `r`), and a pattern
  binding whose scrutinee is borrowed storage, read the borrowed storage in place. A view is never destroyed and
  cannot be moved.
- **A view ends when the storage it reads is written.** The writes are an assignment through its reference;
  `ألحق`, `بدّل`, `أزل_الأخير` or `اقتطع` through it; and passing the reference, or an exclusive reference local, as a
  `&متغير` argument. Using a view after it ends is reported as use after move
  (`SULTANC-OWNERSHIP-USE-AFTER-MOVE`). Rust ends a borrow at the conflicting write (NLL) in the same way.
- **Boxes.** Moving an owning value out of an owned box consumes the box. Reading a plain value through a box or a
  reference never moves.
- **Borrows conflict.** A write borrow cannot coexist with another borrow of the same value
  (`SULTANC-BORROW-CONFLICT`).

### Generic templates: inferred plain bounds

A generic template is checked before substitution, with every type parameter treated as owning. Some templates copy
a value of exactly a type parameter `P` out of borrowed storage, for example a map lookup that returns the stored
value. Such a template gets an inferred plain bound on `P`:

1. SSF re-analyzes the function with `P` plain (`قيد_معامل_نوع_عاديا`, `compiler/frontend/memory/semantic_pipeline.sn`).
2. The bound propagates through the generic calls recorded in the call facts. When `F<P>` is instantiated with a
   caller's parameter `Q`, `Q` gets the bound too (`انشر_قيود_عادية_لتدفق`).
3. Every call that instantiates a bound parameter with an owning type is rejected at that call
   (`تحقق_قيود_عادية_لتدفق`).

Rust states the same requirement explicitly as `T: Copy`. `map_get<K, i32>` is accepted. `map_get<K, text>` is
rejected at the call, because the lookup would copy owned text out of the map.

## Copies

`استنسخ(&x)` / `clone(&x)` is a compiler primitive (`هوية_بدائي_مترجم.استنساخ`). Its result is an independent deep
copy of the referenced type (`فئة_ناتج_بدائي.قيمة_المرجع`). Expression lowering emits the primitive, and
`اخفض_غراء_الملكية_مسير` (`compiler/msir/lowering/ownership_glue.sn`) rewrites it into per-type clone glue:
ordinary MSIR functions generated once per concrete type, as rustc generates shims. Native, LLVM and the
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
- Copy-modify-write-back updates in place. SSF state operations, the diagnostic collection, MSIR renumbering and the
  JSON parser work this way.
- Taking ownership out of a slot swaps an empty value in with `بدّل`. The HTTP client connection, TLS stream state
  and secrets, CSV fields, PKCS#12 results, monomorphization views and MSIR terminators work this way.
- Destructuring patterns transfer owned parts, as in `| نتيجة.نجح({ البايتات: البايتات, … })`.
- A genuinely independent value is copied with its per-type function. Branch snapshots of flow and lowering state,
  values returned by public std query APIs, and generic instantiation work this way.

## Destruction

Destruction is modeled on Rust's MIR drop elaboration and Swift's OSSA. One target-neutral MSIR implementation
(`compiler/msir/lowering/drops.sn`, `ownership_glue.sn`) is consumed uniformly by the Native backends (x86_64,
ARM64, wasm32), by LLVM and by the interpreter. All three drive the same `تحرير` primitive and the same per-type
drop-glue MSIR functions; none has its own notion of when to destroy a value.

1. **Ownership facts for lowering** (`compiler/frontend/ssf/ownership_facts.sn`). SSF records, per function, the
   byte offset of every local-name use that moves the local's value out, and which locals are borrowed views.
   Lowering never re-derives move or view semantics; it only consumes these facts. A `طابق` arm that binds an
   owning value by move out of an owned, non-reference scrutinee also moves the scrutinee's root local, so the
   scrutinee cannot be read again afterward.
2. **A drop frame per function** (`drops.sn`). Lowering gives every function a prologue block that, for each owner
   local in declaration order, allocates storage plus a boolean drop flag. The flag starts true on declaration,
   parameter binding or match-arm binding, and is cleared on a recorded move, an explicit release or a closure
   capture. Every `return`, explicit or by falling off the end, routes through one shared exit block that drops
   owners in reverse declaration order, gated by their flags, before the real return. A loop `break` or `continue`
   likewise drops the locals going out of scope before branching. A drop is the `تحرير` primitive, rewritten into the
   type's glue.
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
accumulated. This holds with and without an explicit `حرر` beside the automatic drop, which does not double-free.

The SSF symbol-position index (`compiler/frontend/ssf/local_state.sn`, `فهرس_مواضع_رموز_تدفق`) is one flat, sorted
vector copied in a single allocation, because under single ownership two aliasing owners would be a double free.

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

`أزل_الأخير(م)` / `pop(v)` and `اقتطع(م, ن)` / `truncate(v, n)` are compiler primitives modeled on Rust's
`Vec::pop` and `Vec::truncate`.

- **pop** moves the last element out and returns it as `اختياري.موجود` (`option.some`). An empty vector gives
  `اختياري.لا_شيء`. The element is moved, not copied: the vector stops owning it and the caller owns it, so nothing
  is dropped.
- **truncate** keeps the first `n` elements (`ن: طبيعي`, Rust's `usize`) and drops the others, first to last,
  through their drop glue. An `n` at or past the length changes nothing.

```sultan
def main(): int {
    let v: mutable vector string = ["a", "b", "c"];
    let last: option string = pop(v);
    truncate(v, 1);
    return length(v) + capacity(v);
}
```

```sultan
عرف الرئيسية(): عدد {
    دع م: متغير متجه نص = ["a", "b", "c"];
    دع الأخير: اختياري نص = أزل_الأخير(م);
    اقتطع(م, 1);
    أرجع الطول(م) + سعة(م);
}
```

Like `ألحق`, both write their first argument, which must be a mutable place: a local, a field, `*r` through a
`&متغير`, or an element. The type checker rejects any other with `SULTANC-TYPE-MISMATCH`. SSF checks the write as it
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

**An emptied vector keeps its buffer**, as Rust's `Vec` does. Truncate, pop to empty and
`امسح_المتجه` / `clear_vector` store the new length, zero included, and keep the capacity, so the next appends reuse
the buffer. Drop glue releases a vector's buffer when its capacity is above zero, not when its length is; a vector
that never allocated has capacity zero. A loop that fills and empties vectors of text, nested vectors and closures
10^5 times leaves the allocator's live bytes unchanged (`tests/*/language/surface/vector_pop_rss_flat.sn`).

**At compile time**, evaluation runs the same MSIR in the interpreter. It allows primitives that change a value the
evaluation owns (append, the length store) and releases of the interpreter's own storage, so `وقت_الترجمة` code can
build, pop and truncate vectors. Observable effects, such as machine memory and the host environment, stay
prohibited.

The C Stage0 lowers the same primitives on its vector header (`bootstrap/src/llvm/llvm_builtins.c`), so the compiler
and the std modules it imports may use them. `std.text.number` pops its digits, and `std.collections.vector` builds
Rust's `retain`, `dedup_by`, `remove`, `swap_remove`, `insert` and `clear` on them.

### Capacity

The vector header is three pointer-sized words: data, length and capacity (`compiler/backend/layout/layout_resolution.sn`).
The layout is the same in x86_64, arm64, wasm32 (32-bit words) and LLVM (pointer-sized on 32-bit targets too).
Capacity is how many elements the buffer holds before an append reallocates it. A vector literal and every append
grow it by one policy (`compiler/runtime/vector_memory_policy.sn`: minimum 1, doubling), so a literal of 3 elements
has capacity 4 in every backend.

- `سعة(م)` / `capacity(v)` reads the header's capacity word as an `عدد`, as `الطول` / `length` and Go's `cap` do.
- `احجز_سعة(م, ن)` / `reserve_exact(v, n)` (Rust's `Vec::reserve_exact`) makes room for `ن` more elements. When
  `length + ن` exceeds the capacity, the buffer is reallocated to exactly that many elements through the same
  allocation service and overflow traps as append's growth: x86_64 and arm64 use the append instruction in its
  reserve form, and wasm32 and LLVM use the same guarded realloc. It writes its first argument, a mutable place, as
  append does, and SSF checks it as a write.
- `std.collections.vector` builds on them: `متجه_بسعة` / `with_capacity` (Rust `Vec::with_capacity`),
  `احجز_في_المتجه` / `reserve` (Rust `Vec::reserve`; when it grows, it grows to at least twice the capacity, so
  repeated reserves stay amortized) and `قلص_المتجه` / `shrink_to_fit` (the elements move in order into a buffer of
  exactly their count, and an empty vector releases its buffer).

The interpreter keeps the same capacity in each buffer (`مخزن_مفسر.السعة`): the growth policy's capacity when a
buffer is made from elements, grown by append, raised by reserve and kept by truncate. `capacity` therefore answers
as it does in an executable. Element 0 of an emptied buffer names the buffer's start, which drop glue releases.
`tests/*/stdlib/behavior/vector_capacity.sn` compares native and interpreter results, and
`vector_capacity_rss_flat.sn` checks that 10^5 rounds of `with_capacity`, `reserve`, `truncate(0)` and reuse, and of
pop to empty and `shrink_to_fit`, leave the allocator's live bytes unchanged.

## Closure environments

A closure value is the address of a heap environment. Closures stay copyable, because the frontend classifies
function types as plain, so the environment is reference counted instead of uniquely owned. The model is Swift's
retain and release of closure contexts, expressed through the same MSIR ownership glue as every other owned value.

**Layout** (`خطط_بيئة_إغلاق_للاختيار`, `compiler/backend/target/selection_query_context.sn`, shared by x86_64, arm64,
wasm32 and LLVM). A 24-byte header of three 8-byte slots holds the code pointer, the reference count and the
destructor's code pointer, followed by the aligned captured values. A construction (`بناء_إغلاق`) sets the count to
one and stores the destructor. Every function value is an environment, a named function used as a value included.

**Ownership in MSIR.** `يملك_تخزينا_للغراء` counts a function type as owning one reference, so owners of function
values, and of aggregates and vectors holding them, are dropped like any other owner. A type that holds closure
references and nothing else (`هل_نوع_معدود_عادي_مسير`: a function type, or an aggregate of plain members and
function values) is plain for the frontend, so SSF records no move of it. Lowering therefore keeps one reference per
owner (`drops.sn`, "Counted values"):

- Every place that takes ownership receives its own reference. These places are a store into a local or place, an
  argument (parameters own their value), a return, an aggregate member or constructor payload, an appended element,
  a closure capture, and a counted match binding. A fresh value (a call result, a closure construction, a move SSF
  recorded) is transferred, and any other read is copied with the `احتفاظ_بقيمة` primitive
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

**Closure bodies.** A closure body is checked and lowered as its own function body, as rustc borrow-checks a closure
as a separate MIR body. SSF analyzes it with a fresh flow state (`حلل_جسم_إغلاق_لتدفق`,
`compiler/frontend/ssf/semantic_pipeline.sn`). Its parameters are owned locals, its captures (plain values the
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
lowering follows Rust's rules for temporaries (`هل_أصل_مؤقت_للخفض`, `أسقط_بقية_مؤقت_خفض`,
`compiler/msir/lowering/drops.sn`):

- An owning member read from a temporary is moved out, and the temporary's other owning members are destroyed. Rust
  destroys them at the end of the enclosing statement; nothing can name the temporary after the read, so destroying
  them right after it is equivalent.
- A plain member is copied, and the whole temporary is destroyed.
- An element of a temporary vector is copied (a vector does not let an element move out, Rust's E0507), and the
  vector is destroyed.
- A member of a temporary is itself fresh: a counted one (a closure) is transferred rather than retained, and a
  fresh callee is dropped after the call. Match scrutinees, the reader primitives (`الطول`, `استنسخ`) and slice views
  treat it as they treat the call results.

`tests/*/language/surface/temporary_members.sn` (native and interpreter agree) and `temporary_members_rss_flat.sn`
(live bytes flat over 10^5 rounds) cover them.

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
- **Patterns.** Nested constructor patterns count as exhaustive one level deep (`نتيجة.نجح(اختياري.لا_شيء)` beside
  `نتيجة.نجح(اختياري.موجود(س))`), and deeper nesting needs a wildcard arm. Alternatives may not bind names.
- **Not in Stage0's language:** closures (`دالة(…) -> …` literals), `وقت_الترجمة`, annotations (`@…`), tuples,
  slices (`[] T`), selective imports (`من … استورد`), import aliases, `main` returning a result, and reading a field
  of a temporary record more than one member deep when it owns memory.
- **Syntax.** `(**x)` does not parse, so write `(*(*x))`. `بدّل(*r, x)` must be written `بدّل((*r), x)`.

Stage0 also compiles what the self-hosted frontend added after it was written, with the same semantics: `؟`;
`إنهيار` ending its path; module constants and module variables (`دع` at the top level, folded at compile time;
their static data keeps capacity 0); `اخرج_من_حلقة`, `تابع_حلقة` and `لكل`; any expression in a vector literal and
after `صندوق`; literal, range and alternative patterns at any depth; float literals, float arithmetic and the float
casts (`llvm.fpto[su]i.sat`); the wrapping intrinsics; `سعة`, `احجز_سعة`, `أزل_الأخير`, `اقتطع`; `حرر`; members,
elements and calls of call results and calls through function-valued fields; keyword segments in import paths
(`std.math.float`); `>>>`; the std package's Arabic names (`المكتبة.أخطاء.failure`); and Unicode separators.
Unreachable match arms are accepted, and the self-hosted compiler warns about them.

## Limitations

- **Views derived from returned references.** A view held through a reference local derived from an exclusive
  parameter is not ended when that parameter is later passed as `&متغير`. Only views whose holder is the parameter
  itself are ended. Stage0 forbids reference returns, so compiler sources cannot create such a local; user programs
  compiled by Stage2 can.
- **Closures.** SSF admits only plain captures. A closure that needs an owned value takes it as a parameter, which
  the closure body drops, or captures a closure value, which is counted and dropped by the environment's destructor.

## See also

- [Type system](type-system.md)
- [Safety model](../internals/safety-flow.md)
- [MSIR and code generation](../internals/msir-and-codegen.md)
- [Bootstrap](../internals/bootstrap.md)
- [Language tour](../guide/language-tour.md)
