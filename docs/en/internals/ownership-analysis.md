[العربية](../../ar/internals/ownership-analysis.md) | **English**

# Ownership Analysis and Drop Elaboration

This page describes how SultanC decides ownership legality and destruction, and how the result
reaches every execution consumer as finalized MSIR. It is written for compiler contributors and
complements [Safety Flow](safety-flow.md), which describes the rules the analysis enforces, and
[Ownership and Destruction](../reference/ownership.md), which describes the language semantics.

One ownership analyzer operates on MSIR, in three parts over one shared dataflow engine
(`احسب_حالات_دخول_شبكة_مسير`, `compiler/msir/drop_elaboration.sn`):

- **Lifetime analysis** decides where each borrow ends: at the last use of the local that holds it,
  computed backward on MSIR ([Borrow regions](#borrow-regions)).
- **Ownership checking** decides the legality of moves, initialization, borrows (against the borrows
  live at each point) and escapes, for every function and every generic template, in a compilation and
  in a tooling request alike ([Legality](#legality)).
- **Destruction elaboration** turns the obligations into the destruction finalized MSIR performs, after
  the checking and on the same dataflow ([Drop elaboration](#drop-elaboration)); it is the one
  destruction mechanism.

The frontend checks no ownership rule; it states the analyzer's errors as diagnostics.

## The previous path

Before this change, ownership was decided and applied in three places:

1. **SSF in the frontend** (`compiler/frontend/ssf`) walked each function's syntax tree, tracked
   the state of every local (initialized, moved, released, possibly one of them after a merge),
   borrows and their holders, and rejected illegal programs. It also recorded ownership facts for
   lowering (`ownership_facts.sn`): the source position of every local-name use that moves the
   local out, and which locals are borrowed views.
2. **MSIR lowering** (`compiler/msir/lowering/drops.sn`) read those facts while it lowered each
   function and inserted destruction directly: a boolean drop flag place for every owner, stores
   that set and cleared the flags, and a conditional branch around every scope-exit destruction.
3. **Ownership glue** (`compiler/msir/lowering/ownership_glue.sn`) rewrote every release primitive
   into the drop glue of the value's type, inside lowering, before the program was first verified.

Destruction policy was therefore fixed while the source was lowered, from facts keyed by source
positions, and no verified MSIR ever existed before the flags did.

## The pipeline now

```text
source
  |
  v
parsing, names, types, patterns          (frontend)
  |
  v
memory classification of types           (frontend: owning, plain, borrowed)
  |
  v
lowering to initial MSIR                 (every local a place, ownership operations,
  |                                       no drop flags, the source of every use,
  |                                       borrowed views decided on the MSIR of their values;
  |                                       then each generic template, for the analysis only)
  |
  v
structural verification of initial MSIR  (تحقق_برنامج_مسير_أولي)
  |
  v
ownership legality analysis              (lifetimes: where each borrow ends, from liveness;
  |                                       moves, initialization, borrows, escapes, on the
  |                                       same dataflow)
  |
  v
drop elaboration                         (initialization dataflow on the CFG)
  |
  v
ownership glue                           (type-specific destruction and copies)
  |
  v
verification of finalized MSIR           (تحقق_برنامج_مسير: no ownership operation left)
  |
  v
compile-time evaluation, verification, runtime checks, optimization, verification
  |
  v
Native (x86_64, ARM64, wasm32), LLVM, interpreter
```

The driver (`compiler/driver/compiler_driver.sn`, `نفذ_سياق_تجميع`) owns the sequence. Lowering
returns initial MSIR; the driver verifies it, runs the ownership legality analysis
(`حلل_شرعية_ملكية_برنامج_مسير`, `compiler/msir/ssf/ownership_legality.sn`), which rejects the program
with its diagnostics if any use is illegal, then calls `أتمم_ملكية_مسير_مخفض`
(`compiler/msir/semantic_lowering.sn`), which runs drop elaboration and then ownership glue, and
verifies the finalized program with the canonical verifier before compile-time evaluation reads it.
Initial MSIR and finalized MSIR are two stages of the same representation: the same program value,
the same places, values, blocks and block arguments. Nothing between the two verifications other
than drop elaboration and glue receives the program, and nothing after the second one can receive
an ownership operation.

## Who owns what

| Fact | Owner |
|---|---|
| Name resolution, type identity, type checking, pattern semantics | frontend |
| Ownership class of a type (owning, plain, counted, borrowed) | memory classification (`compiler/frontend/memory`), cached per type for lowering and glue (`املأ_تصنيفات_ملكية_مسير`) |
| Inferred plain bounds of generic type parameters | the analysis of generic templates (`compiler/msir/ssf/template_legality.sn`), recorded in lowering's copy of the memory facts (`قيد_معامل_نوع_عاديا`); instantiations with an owning type are rejected at the call |
| Where each borrow ends | the lifetime analysis on initial MSIR (`compiler/msir/ssf/borrow_liveness.sn`), from the liveness of the local that holds it |
| Legality of moves, initialization, borrows, lifetimes, escapes | the ownership legality analysis on initial MSIR (`compiler/msir/ssf/ownership_legality.sn`), for functions, closures, compile-time blocks and generic templates |
| Which locals are borrowed views | lowering, from the MSIR that computes the value a local is given (`قيمة_مستعارة_عروض_خفض`, `compiler/msir/lowering/borrow_events.sn`) |
| How an ownership error is stated | the frontend's diagnostics builder (`تشخيصات_خطأ_تدفق_سلامة_بالرموز`, `compiler/frontend/semantic_pipeline.sn`); the error type is `compiler/frontend/ownership_errors.sn` |
| The identity of a source local in MSIR | its place, which names the local's symbol (`مكان_محلي_مسير.المحلي`), declared by `تصريح` |
| What the language does with each owner (initialize, give up, scope exit) | lowering, as MSIR ownership operations (`compiler/msir/lowering/drops.sn`) |
| Where each owner holds a value, and the destruction that follows | drop elaboration (`compiler/msir/drop_elaboration.sn`) |
| How a value of a given type is destroyed or copied | ownership glue (`compiler/msir/lowering/ownership_glue.sn`) |
| That finalized MSIR holds no ownership operation | the canonical verifier (`compiler/msir/msir_verifier.sn`) |

## Source locals in initial MSIR

Every source local is an MSIR place: a `let` (the variables of a `for` header included), a
parameter, a match binding, a closure parameter and a closure capture. Lowering gives each one its
place where it is bound (`compiler/msir/lowering/drops.sn`, `ثبّت_محلي_خفض`):

- **Identity.** The place instruction (`مكان_محلي_مسير`) names the local's symbol in `المحلي`, and
  the text form shows it (`place.local mutable symbol N`). The symbol is the local's identity, not
  a source span. A place lowering makes for itself (a temporary owner, a drop flag) names none.
- **Where the place is.** An owner's place is in the function's entry block (the prologue), as
  before. Every other local's place is in the block the prologue enters (the drop frame's
  `الخانات`). Each place is therefore allocated once per call and dominates every use. Code
  without a drop frame (a compile-time block) puts the place in the current block.
- **Declaration.** Where the declaration executes, lowering states `تصريح` (`local.declare`) on the
  place: from there the place holds no value. This is the explicit uninitialized state, and a loop
  that declares a local restates it on every iteration. Its debug location is the declaration's:
  the declared name for a `let` and a parameter, the enclosing statement for a match binding, a
  closure parameter and a capture.
- **Reads and writes.** A value the local is bound with (an argument, a matched part, a capture) is
  stored right after `تصريح`, and an owner is then initialized (`تهيئة`). Every read is a load of
  the place and every assignment a store. Lowering tracks neither a value nor an initialization
  state for a local: a read before any store is a load of a place that holds none, and lowering
  does not reject it. Branches, merges and loop heads pass no block argument for a local; they
  share its place.
- **Provenance.** The origin of `تصريح`'s debug location is the declaration's span. The name
  context maps the symbol to its declaration span (`نطاق_تصريح_رمز_عند`), which is the span a
  diagnostic reports as "declared here".
- **Debug data.** Every local's place is recorded with its symbol (`مكان_محلي_تصحيح_مسير`, owners
  included). The slot of a local that is not an owner is bound to its debug variable; an owner's
  slot is recorded but not bound, since it holds a value only while the owner does. At -O0 the
  slot of a scalar local is its variable's only location (`اجعل_الخانات_مواقع_المحليات_تصحيح_مسير`,
  `msir_promotion.sn`): a value an assignment also bound to the variable would go stale once the
  slot is written through a reference.
- **Code shape.** Above -O0, promotion (`compiler/msir/msir_promotion.sn`) turns these places back
  into values: a scalar whatever its stores, any other Type when it is stored once before its loads.
  A member read projects a local's place only once the local lives in storage anyway: an owner, a
  local whose address or member was taken, or a local alive across control flow
  (`ربط_محلي_خفض.مخزن`). A local read only whole keeps the whole load that promotion replaces by
  its stored value. At -O0, where no pass runs, every local stays in its frame slot.

Lowering still rejects programs it cannot represent (`خطأ_خفض_مسير`, `compiler/msir/lowering/model.sn`):
an unresolved Type or name binding, a missing semantic fact, an unsupported statement, expression
or external function, inconsistent external or allocator or panic-handler declarations, a
compile-time evaluation that is not constant or traps, an opaque Type argument a generic instance
needs the layout of, an operator a Type does not define, and builder failures. None of them is
about initialization or moves: a program with a use before initialization or a use after a move
still lowers to initial MSIR. When a program cannot be lowered to run, its ownership is analyzed
on the analysis-only lowering ([Tooling](#tooling-and-programs-that-cannot-run)), and its ownership
errors, if any, are the ones reported.

The kernel-call memory proof of the verifier accepts a typed reference read from a source local's
place, as it accepts one from a parameter or a block parameter (`هل_مرجع_نطاق_نداء_موثوق`,
`compiler/msir/msir_verifier.sn`): a reference parameter now reaches a call through its place.

## Ownership in initial MSIR

Initial MSIR keeps the distinctions destruction needs in the canonical model, extended by one
operation (`compiler/msir/msir_model.sn`, `ملكية_مكان_مسير`):

- **Owner storage** is an ordinary function-local place (`مكان_محلي`). Every owner — a local, a
  parameter, a match binding or a temporary that owns its value — has one, allocated in the
  function's entry block. There is no second storage identity and no separate ownership
  representation.
- **Ownership operations** name a function-local place and define no value. The first names any
  source local's place, the others an owner place:
  - `تصريح` (printed `local.declare`): the local's declaration (above).
  - `تهيئة` (printed `owner.initialize`): the place now holds a value its owner must destroy. It
    follows the store that initialized or assigned it.
  - `تسليم` (`owner.give_up`): the value left the owner. Drop elaboration derives it for every
    read that moves the value out (below); lowering states it only where it takes a value apart
    itself: a closure capture, a match arm that moves parts of the scrutinee out, and
    `return local.field`.
  - `إسقاط` (`owner.drop`): a potential destruction obligation. Lowering states one at every scope
    exit (block end, `break`, `continue`), before every assignment (for the old value), and in the
    function's exit block for every owner.
- **Reads and writes** of storage are the existing loads and stores. **Copies** that keep the
  source owner are explicit: the copy primitive, rewritten into clone glue, and the retain of a
  counted value (a function value or an aggregate of plain members and function values).
- **References** are the existing place-to-reference operation; **field and index projections**
  are the existing field and index places. **Arguments, returns and captures** are calls, the
  return terminator (through the exit block, which holds the obligations) and closure
  constructions.
- **Values without an owner place** (a temporary no local owns, the old value of an assigned
  field, a discarded call result) are destroyed unconditionally where lowering knows their last
  use; they are not obligations.

The structural verifier checks that an ownership operation names a function-local place it is
dominated by and defines no value (`ملكية_على_غير_موضع_محلي`), and that finalized MSIR holds none
(`ملكية_بعد_التفصيل`).

## Drop elaboration

`فصل_إسقاط_برنامج_مسير` processes each function independently.

1. **Owners.** Every place named by an ownership operation other than `تصريح` is an owner,
   numbered in first-seen order. `تصريح` changes no state here (an owner's scope exit already
   left it holding none) and, like every ownership operation, leaves no instruction; a function
   without owners only loses its declarations.
1. **Moves derived from MSIR.** A read of an owner is a load of its place. The read moves the
   value out when the loaded value reaches a use that takes ownership: a store (but not one into
   the place of a source local that is no owner: a borrowed view initialized from the value leaves
   it to its owner), an argument of a
   call of a SultanC function, an external function or a function value (parameters own their
   values), a member of an aggregate, a vector, a tagged value or a box, a closure capture, a
   block argument, a returned value, or the owning argument of the three primitives that consume
   one (release, the appended element of append, leak). A retype of an owned sequence and a payload
   read out of an owned box pass the question back to their source, and a derived value that owns
   nothing (a vector viewed as a slice, a plain payload) takes nothing. Any other use reads the
   value in place. Each move becomes `تسليم` right after its load, unless no use in the load's
   own block takes the value: then the move happens at each use that does, on the paths that
   reach one. A match arm that binds the whole scrutinee takes it where the arm is entered, so the
   arms that do not bind it leave it to its owner, which destroys it
   (`tests/*/language/ownership/match_scrutinee_moves.sn`). An exchange of two places
   (`بدّل`) moves nothing: lowering states `تهيئة` for both owners after it.
2. **Initialization dataflow.** For each owner and each block entry the pass computes one of four
   states: unreached, holds no value, holds a value, holds a value on some paths only. The entry
   block starts with every owner holding no value. `تهيئة` sets "holds a value"; `تسليم` and
   `إسقاط` set "holds no value". A join takes the least state above both inputs, so "holds a value"
   joined with "holds no value" is "on some paths only". A worklist re-processes a block whenever
   its entry state changes. The lattice has height three, so loops converge: a back edge that
   carries "moved" into a loop head that was entered with "holds a value" changes the head once,
   to "on some paths only", and no further.
3. **Flags only where needed.** An owner needs a run-time flag only if one of its obligations is
   reached in the "on some paths only" state. Its flag is a boolean place allocated at the start
   of the entry block and initialized false.
4. **Rewriting.** Each block is rebuilt with the state reaching its entry:
   - an obligation reached holding a value becomes an unconditional load and release;
   - an obligation reached holding no value is dead and removed;
   - an obligation reached on some paths only becomes `if flag { flag = false; release(load place) }`,
     which splits the block; the original terminator ends the last part;
   - for flagged owners, `تهيئة` stores true and `تسليم` stores false; for the others they leave
     no instruction;
   - instructions in unreachable blocks lose their ownership operations.

Every replacement keeps the debug location of the operation it replaces. A value is released only
where the dataflow says its owner holds it, and the state becomes "holds no value" at the release,
so a moved or released value is never destroyed, and no value is destroyed twice. Partially moved
aggregates keep the existing semantics: a match arm that moves parts out of an owned scrutinee
gives up the scrutinee (`تسليم`) and lowering destroys the parts no binding takes, on entry to the
arm; a `return local.field` gives up the local and destroys its other owning fields.

### Moves out of a box

Moving the payload out of a box an owner holds (`append(items, *boxed)`, `take(*boxed)`, or
`take(**outer)` for a box held in a box) moves the box: its owner gives it up. The box's own cell
must still be released, without destroying the payload, which now belongs elsewhere. Right after
each such payload load, the derivation emits the release primitive on a reference to the box's
payload place. A reference owns nothing, so ownership glue leaves that release as it is: it is the
raw storage release the backends and the interpreter adapt, the same one a box's drop glue ends
with after destroying the payload. No second destruction mechanism exists. The release happens
exactly where the move happens, so only on the paths that move (a conditional move, a loop
iteration, a branch that returns); on the other paths the owner still holds the box and destroys it
whole, payload included. A payload read without a move (`length(*boxed)`), and a borrowed view
initialized from it (`let seen: string = *boxed;`), move nothing: the store into the view's place
is not a move, and the owner destroys the box at its scope exit
(`tests/*/language/ownership/box_payload_moves.sn`).

## Legality

The ownership legality analysis (`compiler/msir/ssf/ownership_legality.sn`,
`حلل_شرعية_ملكية_برنامج_مسير`) decides the legality of moves and initialization on verified initial
MSIR, before drop elaboration, for every function declared without type parameters, with its
closures and compile-time blocks, and for every generic template ([Generic templates](#generic-templates)).
It reports, for each function, the first use the language rejects:

- a read of a local that holds no value on some path (`SULTANC-FLOW-USE-BEFORE-INITIALIZATION`),
  that was moved out on some path (`SULTANC-OWNERSHIP-USE-AFTER-MOVE`) or released
  (`SULTANC-OWNERSHIP-RESOURCE-RELEASED`, also for a second release);
- a move of an owned value out of borrowed storage: a borrowed view, a dereference, a member reached
  through one, a module value (`SULTANC-OWNERSHIP-MOVE-OUT-OF-REFERENCE`, also for releasing what the
  function does not own);
- a move of one owned member or element out of an owned local (`SULTANC-OWNERSHIP-PARTIAL-MOVE`);
- a second assignment to a local declared without `mutable` (`SULTANC-MUTABILITY-WRITE-TO-IMMUTABLE`).

**One dataflow.** The state of a local at every block entry is the fixed point of drop elaboration's
initialization dataflow (`احسب_حالات_دخول_شبكة_مسير`, shared by both passes, which differ only in
the lattice and in what each block sets). The legality lattice (`ادمج_حالة_شرعية_مسير`) distinguishes
holding no value, holding one, moved out, released, and each of the first, third and fourth on some
paths only; a moved-out state also remembers where the value moved. A declaration (`تصريح`) leaves
the place holding none, a store gives it a value, and a load whose value moves out leaves it moved:
which loads move is drop elaboration's derivation (`احسب_استهلاك_قيم_دالة_مسير`), with only a move
the source states passing back from a box payload to the box. A move whose use is in a later block
than the load (a match arm binding the whole scrutinee) happens there, on that path only. Only owners
and the locals some declaration of which is not given its value right there are followed; every
other local holds a value wherever it is read.

**Provenance.** Lowering records the source of every use it emits
(`برنامج_مسير.مصادر_الاستخدامات`, `استخدام_مصدري_مسير`): the instruction (block and position), the
symbol the place is rooted at, the name as written and the whole expression. Reads of names,
members, elements and dereferences and borrows of them are recorded where the expression is
lowered (`خفض_تعبير_إلى_مسير`, `خفض_مكان_إلى_مسير`); a capture at its closure; the give-ups lowering
states itself (a match that moves its scrutinee, a returned member that moves its record) with
where the value moves; the store of an assignment to a local declared without `mutable`; and the
declaration of every local that owns its value, also in code without drop elaboration. Each
function also records the source function it was lowered from (`دوال_المصدر`). Only recorded
instructions are uses: a load lowering adds for itself (the old value an assignment destroys, the
rest of a record whose member is returned) is not one. The analysis reads these facts before any
pass changes a block, and clears them. A declaration span comes from the symbol
(`نطاق_تصريح_رمز_عند`), so every diagnostic is the one the frontend states, built by the same
function (`تشخيصات_خطأ_تدفق_سلامة_بالرموز`).

**Order.** A statement's operands are evaluated left to right, and so are the instructions that
compute them: each instruction's reads are checked against the state the instructions before it
left, and its moves, releases, captures and assignments are applied before the next one's reads.
A read after a move earlier in the same statement is therefore a use after the move:
`consume(x) + length(x)` is rejected (`SULTANC-OWNERSHIP-USE-AFTER-MOVE`), `length(x) + consume(x)`
is accepted (`tests/*/diagnostics/safety/use_after_move_same_statement.sn`,
`tests/*/language/ownership/move_after_read_same_statement.sn`). Blocks are walked in reverse
postorder, and a loop's body first with the states before it repeats, then with those of the fixed
point, so the first error is the one the source meets first. A closure's body and a compile-time
block are checked where they are constructed or called.

**Borrows.** The same walk decides borrows, lifetimes and escapes. Lowering records, where each
statement ends, what it does to borrows in the order the source states it
(`compiler/msir/lowering/borrow_events.sn`, `برنامج_مسير.أحداث_الاستعارة`, `حدث_استعارة_مسير`):
a write to a local's storage or through a reference, a change in place (append, pop, truncate,
swap, an assignment to a member), a value passed on by value, the borrows a call takes while it
runs (with whether two of them conflict), the origins an assignment or a binding gives a local (as
terms: a local's own origins, a borrow of a local, a read through a reference, a parameter's own),
a borrowed view, a value that leaves the function (a `return`, a closure's body, a store into a
module value) and a capture. Lowering records nothing where a scope ends: where a borrow ends is the
lifetime analysis's ([Borrow regions](#borrow-regions)). Lowering names only what the source says; it reads the Memory facts of a Type and the kind of a Symbol's
declaration for that, and it finds the body's borrowed views itself before lowering it (a local
declared without `mutable` first given an owning value from borrowed storage, a match binding of an
owning Type taken from a borrowed scrutinee). The borrow state of the locals
(`compiler/msir/ssf/borrow_flow.sn`) holds, for each local an event names, where a write ended it (or
an exclusive reference moved out), whether it is a borrowed view, its origins as an ordered list
(joined as the source joins them: the first edge's list, then what only the next one holds), and,
for every local that can be borrowed, the borrow each holder holds on it (shared or exclusive). It
is followed by the shared dataflow (a third lattice of `احسب_حالات_دخول_شبكة_مسير`, whose summary
for a block is its events, the ending events of the lifetime analysis included) and checked in the
walk against the borrows live at each point: a read finds a value a write ended (a use after move)
or an exclusive borrow (a conflict); a change, a move, a release or an assignment finds a borrow; a
new borrow conflicts with one held; a value leaving the function has a local among its origins
(`SULTANC-BORROW-REFERENCE-ESCAPE`). Origins outlive the borrows: a reference whose own borrow ended
still names, when it leaves the function, every local its value points into. A join of a local that
holds no value on one edge and was moved out or released on another, or moved out on one and
released on another, or borrowed exclusively on one and shared (or by another holder) on another by
holders live where the edges join, is rejected.

## Borrow regions

A borrow lasts while the local that holds it (a holder: a local an assignment gives a value that
carries a borrow, or a borrowed view) may still be used, not to the end of the holder's block. The
lifetime analysis (`compiler/msir/ssf/borrow_liveness.sn`, `أضف_نهايات_حياة_استعارات_مسير`) runs on
initial MSIR for every function with a holder, after the borrow events are decoded and before the
legality walk, and adds one event where each holder's borrows end.

**Uses.** The uses of a holder are computed from its place, not from the source:

- every instruction and terminator that reads a value derived from the place: a load, a reference or a
  projection of it, any instruction whose operand is derived (a call's result is derived from its
  arguments), and a temporary lowering stores one into once (a place that names no symbol). A value
  that reaches a join (a block parameter, a temporary stored more than once, such as what a `return`
  passes to the exit block) is derived on one incoming path only: its uses are not the holder's (they
  would keep the borrow alive through every other `return`), and it is followed only to the source
  locals it is stored into; the edge or the store that takes it there is the holder's last use;
- the uses of another source local a derived value is stored into, when that local holds borrows or
  its Type may hold a reference (a reference, a slice, a type parameter, or a member, element or
  payload that may): a copy, or a value a call returns from its argument, carries the borrow on;
- the uses of every local that reads through the holder: a borrowed view behind it, or a local whose
  origins may hold a read through it (`&*r`, `&(*r).f`, `&(*r)[i]`); the borrow a reborrow depends on
  stays while the reborrow is alive (`قراء_عبر_حوامل_حياة` follows the origins every event may give,
  whatever the path);
- the destruction obligation of a holder that owns storage (`إسقاط` of its place): drop glue runs on the
  value, so a vector of references, a closure or a record with an owning member keeps its borrows
  until it is destroyed or moved. Its borrows end right after the destruction.

A store of a whole value into the holder's place and its declaration (`تصريح`) end its liveness.

**Fixed point.** Liveness is a backward fixed point of the shared engine: its core
(`احسب_حالات_شبكة_على_خلفاء_مسير`) takes the graph as lists of successors and a set of start
blocks, so the analysis passes the predecessors, starts every block dead, and uses the lattice
`شبكة_تهيئة_مسير.حياة` (a join is live when either side is). A block's summary is, for each holder, its
first point in the block: a use (live where the block is entered) or a whole value given (dead). Loops
converge like any other dataflow: a reference read in the next iteration is live across the back edge.

**Where borrows end.** Each block is walked backward from the state at its end. A holder's borrows end
(an event of kind `صنف_انتهاء_حوامل_استعارة_مسير`, applied after the events lowering recorded at the
same position, so a statement's own events still find the borrows its last uses held):

- right after its last use, where it is not live afterwards;
- where a block is entered on which it is dead while a predecessor ends with it live (it is used on
  another successor of that predecessor);
- right after an event that gives it borrows where it is dead (a borrow nothing uses);
- right after its destruction.

A join ignores the borrows of a holder that is dead where the block is entered
(`وصف_استعارات_مسير.حية_الدخول`), so two branches whose holders die before the join merge. Everything
the legality walk checks then reads the borrow state as before: conflicts, writes and moves of
borrowed places, views a write ends, and exclusive references that move.

**A borrow may not outlive its owner.** Lowering records the destruction obligation of every owner
where its scope ends (the end of its block, a `break` or `continue` that leaves it, the end of a match
arm or of a loop's initializer) as a use of kind `صنف_استخدام_إتلاف_نطاق_مسير`, with the end of that
scope as its span; the obligation before an assignment is no such use. Where the walk meets one while a
holder still holds a borrow of the owner (it is live there), it looks forward on the control flow for a
read the source states of the holder, or of a place that carries its borrow on, before the holder is
given a whole value or declared again (`استخدام_لاحق_حامل_شرعية`). Such a read rejects the function
(`مرجع_بعد_إتلاف`, stated as `SULTANC-BORROW-REFERENCE-ESCAPE`: the primary span is the end of the
owner's scope, the owner's declaration and the later read are related spans). A holder kept alive only by
its own destruction (an owning holder destroyed right after the owner in the same scope) reads nothing,
and a plain value copied out through a reference before the owner's scope ends borrows nothing
(`tests/*/diagnostics/e2e/borrow_outlives_owner_in_order.sn`,
`tests/*/language/ownership/borrow_within_owner_scope.sn`). A move or an assignment of a borrowed owner
is a conflict, as before.

**Effect.** Programs where a borrow's last use precedes a conflicting access are accepted
(`tests/*/language/ownership/borrow_early_end.sn`, `tests/*/language/ownership/borrow_drop_paths.sn`).
A program that stays invalid is reported exactly as before: the conflict is still found where the
access is, with the same diagnostic (`tests/*/diagnostics/e2e/borrow_regions_in_order.sn` covers a
reference read in the next iteration, a reborrow, a vector of references until it is destroyed, a
reference read after a join, and an escape through a copy).

**Borrowed views.** A local declared without `mutable` whose first value, of a type that needs
releasing, is read from borrowed storage is a view of that storage, not an owner; so is a match
binding of such a type taken from a borrowed scrutinee. Whether storage is borrowed is decided on
the MSIR that computes the value, as lowering emits it (`قيمة_مستعارة_عروض_خفض`): the load of a
place reached through a reference or a box anywhere on the way to its root (members and elements,
as projected places or extracted parts, lead to their root), or rooted at another view; for a
scrutinee also a member or an element, and an immutable module value. Lowering places every such
local as an owner at its declaration; the store that makes it a view moves its place to the frame's
slots block and removes the owner's declaration use (`اجعل_المحلي_عرضا_خفض`), before anything
reads it. Closure bodies and a `for`'s own locals have no views.

**Order of the errors.** The driver states one error per rejected function, in the order of the
modules and their declarations (`رتب_دوال_الوحدات_مسير`), generic templates among them, then the
calls that break a plain bound (`رتب_أخطاء_ملكية_مسير`). A compilation whose frontend gathered
errors (declarations it set aside) is rejected with those first, then the ownership errors of what
remains: the frontend runs the analysis-only lowering for them through the analyzer the driver
registers with it (`سجل_محلل_ملكية_أدوات`). Warnings are shown only when the program is accepted.

## Generic templates

A generic template is checked before substitution: an instance with a plain type argument accepts
what the template rejects, and a template nobody instantiates is checked too
(`compiler/msir/ssf/template_legality.sn`, `حلل_قوالب_دوال_للخفض`). After the program is lowered, each
template is lowered once more for the analysis only: each type parameter is itself, an owning type
for ownership glue's classification unless a plain bound says otherwise; every call to a generic
function with a type parameter among its arguments is bound to a declaration of that function
without a body; the template's debug data and source origins stay in a scratch record. The
analysis checks it as any function (`حلل_شرعية_دوال_من_مسير`) and its functions are emptied.

A template whose first error copies a value of exactly one of its type parameters out of borrowed
storage infers a plain bound on that parameter instead: its memory facts and its cached ownership
class become plain (a type built from it keeps its class, as its memory facts do), and the template
is lowered and analyzed again. After every template, inferred bounds propagate from a generic callee
to the caller's type parameter it is instantiated with, and every call that instantiates a
plain-bound parameter with an owning type is rejected (`SULTANC-OWNERSHIP-MOVE-OUT-OF-REFERENCE`,
with the bound named at the call). Nothing of this lowering stays: the program's tables and the
type registry are restored (`أعد_سجل_أنواع_إلى_عدد`), so the types registered later keep their
identities and generated code does not change.

## Tooling and programs that cannot run

A tooling request runs the same analysis. Its program is lowered for the analysis only
(`اخفض_وحدات_دلالية_إلى_مسير` with `للتحليل`): every function that lowers, a function a partial
program cannot lower left out, a conversion lowering cannot represent kept as its operand, and no
instance, initializer, entry binding or panic handler made; then its generic templates. The
frontend's result is left as it was, its type registry included
(`حلل_ملكية_واجهة_للأدوات_مسير`, `compiler/msir/semantic_lowering.sn`). The errors join the
request's diagnostics, before the tests' own, and the uses of a local's name whose value the
analysis moves out (`اجمع_نقول_أسماء_شرعية`) are the result's moved names
(`الأسماء_المنقولة`, read by `compiler/frontend/tooling_queries.sn`). Every function the
analysis-only lowering leaves out (a declaration or body a partial program cannot lower, a generic
template it cannot lower, every function when the program cannot be lowered at all) is recorded
(`نقول_أسماء_مسير.المتروكة`, `ملكية_أدوات_مسير.المتروكة`), and the Frontend's result carries whether the
analysis ran and those functions (`ملكية_محللة`, `دوال_ملكية_متروكة`): `ملكية_دالة_محققة_واجهة` tells
tooling whether a function's ownership was checked. No diagnostic is made for a function left out, and
nothing about it is claimed checked.

An incremental tooling session (`compiler/frontend/incremental_analysis.sn`) does not analyse every
module on every request. A function's legality reads its own body (and, for a closure, its enclosing
function's, in the same module) and the signatures it calls, never another module's bodies; so the
analysis can be given the modules it covers (`حلل_ملكية_وحدات_مختارة_للأدوات_مسير`): only their bodies
are lowered, their templates' errors, left-out functions and moved names stated, and their calls checked
against plain bounds (every template is still analysed, as a covered call may instantiate a template of
any module). A request that continues a kept state analyses the kept state's modules once per kept
state (`ملكية_مخبأة_واجهة`, keyed by the state's number and module count: its modules keep their Symbol
and source IDs in every copy, and none imports a later module) and its own modules every time
(`حلل_ملكية_مرحلية_واجهة`). The result is that of analysing every module.

The frontend does not import MSIR. It holds one registered analyzer (`سجل_محلل_ملكية_أدوات`):
the driver registers it before a compilation, and `compiler/msir/ssf/ownership_tooling.sn` registers
it with a definition-time decorator in every program that loads it, as `tooling_queries.sn` does.
The same analysis-only lowering states the ownership errors of a compilation whose frontend
gathered errors, and of a program whose lowering to run fails (`مرفوض_للملكية`): its ownership
errors, if it has any, are reported instead of the lowering failure.

## Ownership glue

Glue is unchanged and remains the only provider of type-specific destruction and copying. It now
runs on elaborated MSIR: every release primitive on an owning value, whether elaboration emitted
it or the program called `حرر`, becomes a call of the value type's drop glue, and every explicit
copy a call of its clone glue. Closure constructions receive their environment destructors there.

## Execution consumers

Compile-time evaluation, the runtime-check pass, the optimization pipeline, the Native backends,
LLVM and the interpreter all receive the program only after the verification of finalized MSIR.
None of them knows ownership operations: each fails closed on one (the selectors' unsupported
operation, LLVM's unsupported MSIR operation, the interpreter's internal failure), and the verifier
guarantees none reaches them. No backend and not the interpreter checks borrows or decides
destruction.

## Removed mechanisms

- Drop flags, flag stores and conditional destruction emitted during lowering
  (`اكتب_راية_في_كتلة`, `أصدر_إسقاطا_مشروطا_في_كتلة`, the flag field of lowering's local
  bindings, the boolean Type of the drop frame).
- The prologue layout of slot and flag pairs that closing a drop frame had to parse.
- Ownership glue inside lowering, before any verification: it now runs after drop elaboration on
  verified initial MSIR.
- Locals as values in lowering's state: a local kept as an SSA value until control flow moved it
  into a place (`ثبّت_أماكن_محليات_قبل_تفرع_التحكم` emitted the place and a store), block
  parameters for locals at branch merges, match merges, loop heads, loop steps and `continue`, and
  the lowering failure on a read of a local with no value (`محلي_غير_مهيأ` and its reason
  `msir-lowering.uninitialized-local`).
- The frontend's safety flow analysis (SSF, `compiler/frontend/ssf`): its traversal of each
  function's syntax tree, its per-function state of locals and borrows, its merges of control flow
  (`control_flow_merge.sn`), the ownership facts it recorded for lowering and tooling
  (`ownership_facts.sn`: moves, borrowed views, constant module values), its inference of plain
  bounds, and the machinery that deferred its errors to the analysis, ranked them and fell back to
  its own statement of them (`ملكية_مؤجلة_تدفق`, `ملكية_معلقة_واجهة`, the merge of the driver's
  `ownership_diagnostics.sn`). Its internal error kinds and their seven reasons are gone, so the
  registry holds 915 message keys.
- Finding borrowed views in the body's syntax before lowering it (`احسب_عروض_كتلة_خفض`).
- Lexical borrow regions: the borrow event lowering recorded where the locals holding borrows left
  their scope (`سجل_نهاية_نطاق_استعارة_خفض`, kind `صنف_استعارة_نطاق_مسير`), which ended every borrow
  at the end of its holder's block. Borrows now end where their holders die
  ([Borrow regions](#borrow-regions)).
