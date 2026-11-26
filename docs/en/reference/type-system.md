[العربية](../../ar/reference/type-system.md) | **English**

# Type System

SultanC has one canonical semantic type system for Arabic, English and mixed source. This page describes the
builtin and composite types, the conversion and arithmetic rules that belong to the type layer, the thread-capture
check, and the division of responsibilities between the type layer and the rest of the compiler. It is written for
programmers who need the exact rules and for contributors who touch type resolution.

## Canonical identity

Surface spellings are normalized before any downstream consumer sees a type.

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

The English and Arabic spellings of a builtin integer resolve to the same Type ID. Backends, MSIR, safety analysis
and tooling never receive separate Arabic and English type systems. Builtin definitions are generated from the
canonical type registry, `tools/generate/registry/type_registry.def`, and the frontend and bootstrap mirrors are
derived from it.

## Builtin types

| Family | English | Arabic |
|---|---|---|
| Any value | `any` | `أي` |
| Boolean | `bool` | `منطقي` |
| Text | `string` | `نص` |
| Character | `char` | `حرف` |
| Signed integer | `int` | `عدد` |
| Unsigned integer | `uint` | `طبيعي` |
| No value | `void` | `فراغ` |
| Fixed signed | `i8`, `i16`, `i32`, `i64` | `ص8`, `ص16`, `ص32`, `ص64` |
| Fixed unsigned | `u8`, `u16`, `u32`, `u64` | `ط8`, `ط16`, `ط32`, `ط64` |
| Floating point | `f32`, `f64` | `ع32`, `ع64` |

`طبيعي` is a natural number: an integer greater than or equal to zero, with zero included. The old Arabic
spellings `غير_موقع` and `غ8`…`غ64` are deprecated aliases that compile as the new ones and are reported once per
use as `SULTANC-SYNTAX-DEPRECATED-SPELLING`. [Arabic terminology](arabic-terminology.md#renamed-spellings) lists
every renamed spelling and the reason for each.

## Composite types

The semantic model represents these composite forms:

- tuples, written `(int, int)`;
- boxes;
- option and result values;
- function types, written `fn(int): int`;
- pointers, and shared and mutable references (`&T`, `&mutable T`);
- slices and vectors;
- mutable wrappers (`mutable T`);
- named record, enum and alias types, which may take type parameters.

The identity of a named record or enum is its resolved declaration, not its spelling. Generic arguments nest, and
`arc<mutex<i64>>` closes both lists with one `>>` token; the parser takes it as two
closing angle brackets. A generic call infers a type parameter from the type of the value passed, so a `mutable`
local passed for a parameter `T` binds `T` to the value's type rather than to `mutable T`
(`compiler/frontend/types/call_signature.sn`). A generic instance whose result type is `void` may bind and return
that result, as in `let n: R = f(); return n;` with `R` = void: the binding holds no value.

A generic call may also infer its type parameters from the type its destination expects. The result it then has is
checked against that destination like any other value, so `let removed: int = remove_at(&mutable values, 1);`,
whose call has the type `option int`, is `SULTANC-TYPE-MISMATCH` at the initializer (expected `int`, found
`option int`). When the value is an option or a result holding the expected type, a help says to match on it or pass
the absence or the error on with `?`.

```sultan
type Pair<A, B> = { first: A, second: B };

def main(): int {
    let pair: Pair<int, bool> = { first: 1, second: true };
    let present: option int = option.some(3);
    let nested: vector vector int = [[1], [2, 3]];
    return pair.first + length(nested);
}
```

### Option and result

`option T` holds a value or nothing. Its constructors are `some(x)` and `Some(x)`, and `none` and `None`.
`result T, E` has `ok` / `Ok` and `err` / `Err`. These names are contextual: lexically they are ordinary
identifiers, and they name the builtin type only in a type or constructor position, so a local may still be called
`option` or `result`
(`tools/generate/registry/type_registry.def`; the constructor spellings are matched in
`compiler/frontend/types/type_semantic_shape_queries.sn`).

The old Arabic spellings `اختيار` for the type and `بعض` for the constructor are deprecated aliases. They name the
same type and constructor, and each use is warned once with `SULTANC-SYNTAX-DEPRECATED-SPELLING`.

### Enum constructors

A constructor of an enum is a call: `Color.Red()` for a constructor without payloads, `Shape.Circle(2)` for one
with a payload. The constructor read as a value without its call, `let c: Color = Color.Red;`, is
`SULTANC-NAME-CONSTRUCTOR-NOT-CALLED` at `Color.Red` (kind `constructor-not-called`), not an unknown name: the
message names the constructor and says that it is called with `()`, or, for a constructor with payloads, how many
arguments it takes. A constructor without payloads also has a help and a machine-applicable fix (key
`call-constructor`) that inserts `()` after the name; `--diagnostic-format=json` writes it in `"fixes"`, and the
editor tooling reads the same suggestion. A constructor with payloads has a help only, with no fix: its arguments
are the author's to write, so no edit could be applied without asking. Tooling requests report the same
diagnostic and set aside the declaration that holds the read, as for an unknown name
(`compiler/frontend/names/semantic_pipeline.sn`, `أجل_منشئا_غير_مستدعى`).

A constructor call of constants is also a module-level value: `let c: Color = Color.Red();`,
`let s: Shape = Shape.Circle(2);` or `let o: option int = option.some(3);` at the top of a module, with
payloads that are numbers, booleans, characters, text, the null pointer, other such constructor calls,
or names of constants declared before. Such a value is immutable; each read builds it where it is
read (docs/en/internals/msir-and-codegen.md, "Constants and globals"). A `mutable` one, or one whose
arguments are not such constants, is `SULTANC-COMPTIME-NON-CONSTANT` at the declaration.

```text
error[SULTANC-NAME-CONSTRUCTOR-NOT-CALLED]: constructor `Color.Red` is called with `()`: `Color.Red()`
error[SULTANC-NAME-CONSTRUCTOR-NOT-CALLED]: constructor `Shape.Circle` takes 1 argument: `Shape.Circle(...)`
```

### Slices

A slice `[] T` is a non-owning view with the vector layout. A `vector T` value or place is accepted wherever `[] T`
is expected: as a call argument, or in a declaration or assignment of a slice. It is not accepted as a returned
value. The slice views the vector's buffer without a copy, and the vector keeps owning it
(`هل_عرض_متجه_كشريحة`, `compiler/frontend/types/compatibility.sn`). SSF treats the view as a shared borrow of the
vector's root local. A vector argument viewed as a slice is borrowed for the call, not moved, and a slice local
viewing a vector place holds a shared borrow, so the vector cannot be changed while the view is alive. MSIR retypes
the vector header with `نقل_تسلسل_مملوك`.

A slice can also view part of a vector or slice. `v[a..b]` is the slice of the elements `a` through `b`, both
included; `v[a..]` runs to the last element, `v[..b]` starts at the first, `v[..]` is all of v, and `v[a..a - 1]` is
empty. The `rest name` of a vector pattern ([The `match` statement](../book/ch06-02-match.md)) is the same kind of
slice. Either has the representation of every slice (the data pointer moved to its first element, and its length), no
copy, whatever the element type; MSIR builds it with `مقطع_تسلسل`
([MSIR and code generation](../internals/msir-and-codegen.md)). v must be a place: slicing a call result is
`SULTANC-TYPE-ADDRESS-OF-NON-LVALUE`. The bounds are integers. When the program runs, `0 <= a <= length`,
`-1 <= b < length` and `b >= a - 1` are checked, and a failed check stops the program with the start, the end or both
([Errors](errors.md#runtime-checks)); a constant end past a local of known length is
`SULTANC-SAFETY-INDEX-OUT-OF-BOUNDS` at compile time.

A slice is a read-only view: writing through it is `SULTANC-MUTABILITY-WRITE-TO-IMMUTABLE`, and a vector or slice
place, or `v[a..b]`, given to a `mutable [] T` (a parameter, a local, an assignment) without `&mutable` is
`SULTANC-MUTABILITY-VIEW-WITHOUT-MUTABLE`, whose machine-applicable fix writes `&mutable` before it. A writable view
is passed on as it is, and a literal or a call result, a buffer of its own, still becomes a `mutable [] T`. A writable view is asked for with `&mutable`: `&mutable v[a..b]` (and the open forms) is a
`mutable [] T` when v is a place the program may write, and `&mutable v` of a vector or slice is the writable view of
all of v where a `mutable [] T` is expected (a parameter, a local, an assignment; it is written out as
`&mutable v[..]`, `compiler/frontend/desugar/implicit_places.sn`). A slice borrows the place it views as `&v` does,
and a writable view as `&mutable v` does: while a writable view is live, v is not read, written or viewed another way
(`SULTANC-BORROW-CONFLICT`), read-only views coexist, and no view outlives v (`SULTANC-BORROW-REFERENCE-ESCAPE`).

`split_mutable(&mutable v, i)` (`اقسم_قابلا_للتعديل`) is the compiler's own function (a declaration of that name
shadows it) for two writable views at once: it gives the pair `([] T, [] T)` of the elements before i and from i on,
which a tuple `let` names (`let (left, right): (mutable [] T, mutable [] T) = split_mutable(&mutable v, i);`, see
"Tuple `let`") or a match takes apart (`match split_mutable(&mutable v, i) with | (left, right) -> { … }`, whose two
names are `mutable [] T`). The pair holds the exclusive borrow of v, and so does every name bound out of it, for as
long as any of them is in use; the halves do not overlap and do not conflict with each other. `0 <= i <= length` is checked when the program runs (`split at 4 out of range for length 3 at …`, kind 8).
Lowering builds the two halves with `مقطع_تسلسل` (`أصدر_قسمة_مفحوصة_للخفض`, `compiler/msir/lowering/places.sn`).

A slice declaration initialized from a slice literal (`let s: [] T = [...]`) or from a call result builds or
receives a vector, which an anonymous owner in the enclosing block destroys at the block's end, extending the
temporary's lifetime to the block. The same holds for a call result passed directly as a slice argument. A slice assigned a
literal or call result later (`s := [...]`) keeps no owner for it, so the buffer leaks instead of letting the view
outlive it.

### Text slices

A slice of text is a `&string`, the read-only borrowed text type, written with the borrow operator: `&s[a..b]`
views the bytes a through b of the text s, both included, with no copy; `&s[a..]` runs to the last byte, `&s[..b]`
starts at the first, `&s[..]` is all of s, and `&s[a..a - 1]` is empty. No new word or type is involved: `&s` is
still the borrow of all of s, and both are `&string`.

```sultan
def main(): int {
    let s: string = "hello world";
    let w: &string = &s[6..10];   // "world", no copy
    let all: &string = &s;
    if (w == "world" && length(*all) == 11) { return 0; };
    return 1;
}
```

- **Byte offsets on character boundaries.** a and b are byte offsets, integers. When the program runs, the bounds
  are checked as a vector slice's are (`0 <= a <= length`, `-1 <= b < length`, `b >= a - 1`), and then each written
  bound must fall on a character's boundary: a is the length, or byte a does not continue a UTF-8 character (it is
  not `0b10xxxxxx`); b + 1 is the length, or byte b + 1 does not continue one. A bound inside a character stops the
  program with `byte offset 1 is inside a character at …` (`إزاحة البايت 1 داخل حرف عند …`, kind 9), which
  reports a, or b + 1 for the end ([Errors](errors.md#runtime-checks)). A constant end past a text local of known
  contents is `SULTANC-SAFETY-INDEX-OUT-OF-BOUNDS` at compile time.
- **Read-only.** Writing into a slice could cut a character in two, so a slice of text is never writable:
  `&mutable s[a..b]` is `SULTANC-TYPE-TEXT-SLICE`, and so is `s[a..b]` of text without `&`, which is not a value of
  its own.
- **Through references.** The base may be reached through references: `&w[1..3]` with `w: &string` (or
  `&mutable string`) reads `&(*w)[1..3]`, a slice of a slice; the frontend writes the dereferences out
  (`compiler/frontend/desugar/implicit_places.sn`).
- **Borrowing.** A slice borrows the place it views as `&s` does
  ([Ownership](ownership.md#the-ownership-rule)): while it is live, s is not moved, assigned or changed
  (`SULTANC-BORROW-CONFLICT`), and it does not outlive s (`SULTANC-BORROW-REFERENCE-ESCAPE`). A slice of a text a
  function was given (through a `&string` parameter) may be returned. A thread does not capture one
  (`SULTANC-THREAD-CAPTURE-REFERENCE`).
- **Reading it.** Everything that reads a `&string` reads a slice: `length(*w)` (its bytes), `(*w)[i]`, `==` and `!=`
  against text, literals and other `&string` (a `&string` operand is read through its references, `w == "let"`
  compares `*w`; see [Comparison operators](#comparison-operators)), a `match *w with` on text literal patterns,
  printing (`println(w)`), formatting and std's read-only text functions, which take `&string`. `copy_text(w)`
  (`انسخ_نص`, `std.text.from_bytes`) or `clone(w)` makes an owned `string` of it.

**Representation.** A `&string` is a text header value, data, length and capacity, with the layout of every
sequence (`هل_مرجع_نص_مشترك_دلالي`, `compiler/frontend/types/type_semantic_shape_queries.sn`;
`compiler/backend/layout/layout_resolution.sn`), not the address of one. A borrow of all of a text and a slice of
part of it are therefore the same kind of value. `&s` is s's own header and `*w` is the storage of w's header: the
native backends and LLVM hold every aggregate by the address of its storage, so the MSIR of `&s` (`مكان_إلى_مرجع`)
and of `*w` (`مرجع_إلى_مكان`) is unchanged, and the header is copied where a value is stored, passed, returned or
put in an aggregate. `&s[a..b]` is the checked subslice `مقطع_تسلسل` of s's header with a `&string` result (capacity
= length), built in a frame slot as a vector slice is (`compiler/msir/lowering/places.sn`,
`أصدر_مقطعا_مفحوصا_للخفض`). Debug information describes a `&string` as it describes text. The checked interpreter
keeps a `&string` as a reference to a place; a slice is a read-only place whose value is the view
(`جذر_مكان_تشغيل.مقطع`, `compiler/interpreter/values/runtime_values.sn`). `&mutable string` stays the address of
the text. A `&string` is not a C type: a C function takes the bytes as `*u8`.
`tests/{ar,en}/language/surface/text_slices.sn`, `tests/{ar,en}/stdlib/behavior/text_slice_tokenizer.sn`,
`tests/{ar,en}/runtime/failure/text_slice_*.sn` and `tests/{ar,en}/diagnostics/e2e/text_slice_*.sn` cover it.

### Vector literal expressions

A vector literal `[a, b, c]` is an expression. It can be a call argument, a returned value, a record field, an
element of another literal (`[[1], [2, 3]]`), an indexed temporary (`[a, b][i]`) or an operand of a reader primitive
(`length([..])`). `let v: T = [..];` keeps the initializer statement (`تهيئة_متجه`), which also builds slices.

- **Typing.** Typing is bidirectional. In a `vector E` context (a parameter, a declared or
  returned type, a field, an outer literal's element) each element is checked against `E` and `[]` is a `vector E`.
  Without such a context the first element gives `E`, so `[1, 2]` is a `vector int`, and `[]` alone is
  `SULTANC-TYPE-MISMATCH` (`ابن_نوع_حرفي_متجه_داخلي`, `compiler/frontend/types/semantic_pipeline.sn`). In any other
  context the literal's own type is compared, so `f([1])` with `f(v: [] int)` is a mismatch rather than a slice view.
- **Ownership.** The same as a `let`-bound literal. Elements are evaluated left to right and moved in (SSF
  `طبق_آثار_تعبير_لتدفق`), the literal carries every borrow its elements hold (so returning `[&local]` is an
  escape), and when the literal is an argument, the borrows its elements take directly or as enum-constructor
  payloads last for the call and conflict with the call's other arguments (`ابدأ_استعارة_حجة_مؤقتة`). A borrow
  passed to an element's own function call ends with that call.
- **Lowering.** The literal becomes one `تهيئة_متجه` SSA value, a fresh owned temporary like a call result
  (`هل_تعبير_طازج_للخفض`, `هل_أصل_مؤقت_للخفض`). It is moved into an owned parameter or destination, and destroyed
  after an element or its length is read. `tests/{ar,en}/language/surface/vector_literal_expressions.sn` and
  `vector_literal_rss_flat.sn` cover it.

The C Stage0 seed does not parse vector literal expressions, so the compiler sources and the std modules the
compiler imports keep `let`-bound literals.

### Tuple literal expressions

A tuple literal `(a, b)` (two or more elements, an optional trailing comma) is an expression. It can be a returned
value (`return (a / b, a % b);`), a call argument, a record field or an element of another literal. A parenthesized
single expression `(a)` stays a grouping. `let p: T = (..);` and `p := (..);` keep the initializer statement
(`تهيئة_صف`).

- **Typing.** In a tuple context (a declared or returned type, a parameter, a field) the literal must have as many
  elements as the tuple type, and each element is checked against its element type, so `return (1, 2);` in a
  function returning `(i8, u16)` types the integers as `i8` and `u16`. Without a context each element gives its own
  type, so `(1, true)` is an `(int, bool)`. A count that differs is `SULTANC-TYPE-MISMATCH`.
- **Ownership and lowering.** The same as a `let`-bound tuple: elements are evaluated left to right and moved in, the
  literal carries every borrow its elements hold, and it lowers to one aggregate value, a fresh owned temporary like
  a record literal expression. `tests/{ar,en}/language/surface/tuple_literal_expressions.sn` covers it.

The C Stage0 seed does not parse tuple literal expressions, so the compiler sources and the std modules the compiler
imports keep `let`-bound tuples.

### Tuple `let`

`let (a, (b, _)): (A, (B, C)) = e;` takes a tuple apart: each name is a local of its element, for the rest of the
enclosing block. Only a block's statements take one (not a loop's `let`, a compile-time block or the braced arm of a
match expression).

- **Pattern.** A tuple of names, `_` and nested tuples, checked by the parser; any other pattern (a literal, a
  constructor, a range, a vector or record pattern, alternatives, `as`) tests the value and is
  `SULTANC-PARSE-LET-PATTERN`, which points at that part: such a test belongs to `match`.
- **Type.** Written, as for every `let`: a tuple with one element for each part of the pattern, and a tuple for each
  nested tuple pattern (`SULTANC-TYPE-LET-TUPLE-SHAPE` otherwise, at the written type). Each name has its element
  exactly as written, not the value's: `mutable [] T` is a writable view and `[] T` a read-only one. No other element
  (and not the whole type) may be `mutable` (`SULTANC-MUTABILITY-LET-TUPLE-MUTABLE`): a name taken out of a tuple is
  never assigned. The value is typed against the written type as a `let` initializer is, so a tuple literal takes its
  element types from it and a writable view in it needs `&mutable`. The pair `split_mutable` gives is the exception:
  its halves fit the type with or without `mutable` on each.
- **Meaning.** The parser reads the statement as the match `match e with | (a, (b, _)) -> { the rest of the block }`
  whose only arm carries the written type (`ذراع_مطابقة.نوع_ليكن`, `مطابقة_ربط_صف` in
  `compiler/frontend/parser/statements.sn`), so names, types (`ابن_أنواع_ربط_صف_داخلي`), moves, destruction and borrows
  are the match's: an owned element moves out of the value, and a borrow the value carries lasts while any name
  taken out of it is in use (docs/en/internals/ownership-analysis.md, "Match bindings").
  `tests/{ar,en}/language/surface/let_tuple.sn` covers it.

### Match expressions

`if (c) a else b` used as a value (`إذا (ش) أ وإلا ب`) is the match expression `match c with | true -> a | _ -> b`
(`حلل_إذا_تعبيرية`, `compiler/frontend/parser/statements.sn`): every rule below applies to it, and `else if` is an if
expression in the else branch. Without `else` it is `SULTANC-PARSE-IF-WITHOUT-ELSE`. Its diagnostics speak of the `if`
and its branches: no branch giving a value is `SULTANC-TYPE-IF-WITHOUT-VALUE`, a branch that can end without one
`SULTANC-FLOW-IF-BRANCH-WITHOUT-VALUE`, and branches of different types a `SULTANC-TYPE-MISMATCH` whose reason names
the two branches.

`match e with | p -> v | q -> { statements; w } | r -> { statements; }` is an expression wherever an expression is
expected ([the book](../book/ch06-02-match.md#a-match-that-gives-a-value)); a statement that starts with `match` stays
the match statement. An arm written without braces is `| p -> v`, its value parsed above the precedence of `|` (`&&`
and `||` join such values; `(a | b)`, a closure or a further match before another arm are parenthesized). A braced arm
is statements followed by its value, an expression with no `;` before the closing brace; a braced arm without one gives
no value. A guard (`| p if c -> v`) is parsed and checked as in the statement. The arms are the statement's (`ذراع_مطابقة`, `compiler/frontend/ast/statements.sn`, `مطابقة_تعبيرية`), so
patterns, their names and exhaustiveness are the statement's.

- **Typing.** The scrutinee is typed alone and each pattern against it, as in the statement. In a context with a Type
  (a declared, assigned or returned Type, a parameter, an operand, an element) each arm's value is checked against it,
  in the context's position, so `option.none()` and literals take it and a mismatch is the context's own diagnostic at
  the arm. Without one the first arm that gives a value gives the Type, and a later value of another Type is
  `SULTANC-TYPE-MISMATCH` (reason `type.match-arm-types-differ`) at that value. The Type of a value is never `mutable`.
  A `return` in an arm is checked against the return Type of the code around the match (a closure's own). At least
  one arm gives a value (`SULTANC-TYPE-MATCH-WITHOUT-VALUE`); an arm without one that can finish is rejected by MSIR
  lowering (`SULTANC-FLOW-MATCH-ARM-WITHOUT-VALUE`), as a path that misses its return is.
- **Ownership and lowering.** The match is evaluated where it stands: lowering ends the current block's code at the
  match and goes on in a new block whose parameter is the match's value (`خفض_مطابقة_تعبيرية_إلى_مسير`,
  `compiler/msir/lowering/expressions.sn`). Once the statement that holds it is lowered, the scrutinee and the arms are
  lowered as the match statement's, from the state saved at the match, in that block
  (`اخفض_أعمال_مؤجلة_إلى_مسير`, `compiler/msir/lowering/statements.sn`): an arm that runs to its end lowers its value
  as its owned result, destroys its block's locals and its bindings, and passes the value to the block. The value is a
  fresh temporary like a call result (drops.sn): a binding moves out of the scrutinee into it, a value moved in one
  arm only is a move on that path only (drop elaboration), and its origins are those of the arms' values and of the
  scrutinee (borrow_events.sn). A closure whose body holds a match closes its drop frame after the match's arms.
  `tests/{ar,en}/language/surface/match_expressions.sn` and `tests/{ar,en}/language/ownership/match_expression_moves.sn`
  cover it.

The compiler sources do not use match expressions: the C Stage0 seed does not parse them.

## Comparison operators

`==` and `!=` are defined for integers, floats, booleans, characters, text, raw pointers (by address) and enums
without payloads (by constructor). Text compares by content, byte by byte, as std's `texts_equal`
(`نصان_متساويان`) does, so `name == "Sultan"` is true for a `string` holding those bytes. An operand of type
`&string` (or a reference to one) is read through its references, so a slice compares with text, a literal or
another slice directly: `w == "let"` is `*w == "let"` ([Text slices](#text-slices)). Neither operand is moved:
a text local stays usable after the comparison, and a call result compared directly is destroyed at the end of its
block. `<`, `<=`, `>` and `>=` are defined for integers, floats and characters. Ordering text needs
`compare_texts_lexicographically`.

Any other operand type is `SULTANC-TYPE-MISMATCH` with the reason `type.operator-not-defined`, reported at the
operation and naming the operator and the type: "`>` is not defined for `Point`". A generic body may apply an
operator to its type parameter (there are no trait bounds); the check then runs when the generic is instantiated,
and the error points at the operation inside the generic with the instantiating call as a related span
(`هل_عامل_ثنائي_معرف_للنوع`,
`compiler/frontend/types/semantic_pipeline.sn`; the text loop is `اخفض_مساواة_نصين_إلى_مسير`,
`compiler/msir/lowering/expressions.sn`). `tests/{ar,en}/language/surface/text_equality.sn`,
`reject_generic_operator_not_defined.sn` and `tests/{ar,en}/diagnostics/e2e/operator_not_defined.sn` cover it.

## Opaque types

`type NAME = opaque;` (Arabic `نوع الاسم = معتم;`) declares a type with no fields and no known size, for data
whose layout only other code knows, typically a C library's handle. No value of the type itself exists in
SultanC code: only pointers to it, `*NAME` and `*mutable NAME`, which are passed to and returned from
external functions. A type alias of an opaque type is the same type. C imports declare their incomplete and
unrepresentable structs this way ([Jisr](../internals/jisr.md)).

```sultan
type Database = opaque;

def open_count(handle: *Database): int {
    return 1;
}

def main(): int {
    return 0;
}
```

## Numeric casts

`x as T` converts between scalar types. The type checker accepts a cast between two scalar types only
when a conversion joins them (`تحويل_سلمي_مرفوض`, `compiler/frontend/types/compatibility.sn`). Any other pair, such
as `bool as int`, is `SULTANC-TYPE-MISMATCH` at the cast, with the reason `type.invalid-cast`. Casts that involve
references, pointers or byte vectors are checked where they are lowered.

| From | To | Result |
|---|---|---|
| integer | integer | two's-complement truncation or extension; wraps and never traps |
| `u8` | `char` | the code point |
| `char` | an unsigned integer of 32 bits or more | the code point |
| float | integer | truncated toward zero, saturated at the type's bounds; NaN gives 0 |
| integer | float | the nearest float, ties to even |
| `f32` | `f64` | exact |
| `f64` | `f32` | the nearest `f32`, ties to even; out of range gives an infinity; a NaN stays a NaN |

A float-to-integer conversion saturates: a value below the target's range gives its minimum, a value above
gives its maximum, and a NaN gives 0, which is also WebAssembly's non-trapping `iNN.trunc_sat_fMM`. The result
is defined for every input. Integer-to-float rounding is IEEE 754 roundTiesToEven.

```sultan
def main(): int {
    let small: u8 = 200;
    let wide: u32 = (small as u32) + 100;
    let wrapped: u8 = (wide as u8);   // 300 wraps to 44
    let ratio: f64 = 7.9;
    return (ratio as int) + (wrapped as int);
}
```

All float conversions are one MSIR operation, `تحويل_عائم` (`compiler/msir/msir_model.sn`), which carries the
integer side's signedness. Each backend lowers it as follows:

- x86_64 Native uses `CVTTSD2SI` / `CVTTSS2SI` after comparing with the bounds as floats (exact powers of two), so
  the instruction only sees in-range values. A `u64` above 2^63 subtracts 2^63 and sets bit 63. The reverse
  direction uses `CVTSI2SD` / `CVTSI2SS`, with the sticky-halving sequence for a `u64` above 2^63, and
  `CVTSS2SD` / `CVTSD2SS` between float widths.
- arm64 Native uses `FCVTZS` / `FCVTZU`, which already saturate and map NaN to 0, at 32 and 64 bits. Results of 8 and
  16 bits are clamped first with `FMIN` / `FMAX`, which keep a NaN. It uses `SCVTF` / `UCVTF` and `FCVT` for the
  reverse and between widths.
- wasm32 Native uses `iNN.trunc_sat_fMM_s/u`, `fMM.convert_iNN_s/u`, `f32.demote_f64` and `f64.promote_f32`, with the
  same `min` / `max` clamp for 8 and 16 bits.
- LLVM uses `llvm.fptosi.sat` / `llvm.fptoui.sat`, `sitofp` / `uitofp`, and `fpext` / `fptrunc`. Targets with 32-bit
  registers would legalize 64-bit conversions into compiler-rt calls (`__aeabi_d2lz`, `__fixdfdi`), which no
  SultanC link provides, so `compiler/backend/provider/llvm/wide_conversion.sn` builds them from 32-bit conversions
  and exact float steps, as `wide_division.sn` does for 64-bit division. AVR and MSP430 have no FPU, so their objects
  call the usual soft-float routines for every float operation, conversions included, and the toolchain that links
  them supplies those.
- The interpreter and compile-time evaluation use IEEE 754 software arithmetic
  (`compiler/interpreter/operations/float_software.sn`). A module constant such as `let k: int = 2.5 as int;` is
  folded with the same code, and a float result is kept as its exact decimal text
  (`نص_عشري_دقيق_من_بتات`, `compiler/msir/decimal_float_bits.sn`).

The bit patterns of floats are available as `f64_to_bits`, `f64_from_bits` and the
`f32` forms in `std.math.float`, over the compiler's bit-reinterpret intrinsics.

## Wrapping arithmetic

`+`, `-`, `*` and `<<` trap on overflow. Wrapping arithmetic, the result modulo 2^N, is provided by four MSIR
operations (`جمع_ملتف`, `طرح_ملتف`, `ضرب_ملتف`, `إزاحة_يسرى_ملتفة`) that lower to the plain machine instruction on
every backend. They are reached through the compiler intrinsics `بدائي_جمع_ملتف`, `بدائي_طرح_ملتف`,
`بدائي_ضرب_ملتف`, `بدائي_إزاحة_يسرى_ملتفة` and `بدائي_إزاحة_يمنى_ملتفة`, whose two operands share one integer
type. A shift takes its amount modulo the width.

`std.math.wrapping` gives every integer type a wrapping form `wrapping_add_T` (and `sub`, `mul`,
`shl`, `shr`), a checked form `checked_add_T` that returns an `option`, and a saturating form
`saturating_add_T`.

## Float operations

Beyond `+ - * /` and the comparisons, the IEEE 754 operations below are compiler
primitives. Each is one `هوية_بدائي_مترجم.عملية_عائمة` carrying a `عملية_عائمة_بدائية`, and its operands and result
have the first operand's float type, `f32` or `f64`.

| Operation | Intrinsic | std.math.float | Result |
|---|---|---|---|
| square root | `بدائي_جذر_تربيعي_عائم` | `sqrt_f64` | correctly rounded (IEEE squareRoot); -0 gives -0, a negative operand NaN |
| absolute value | `بدائي_قيمة_مطلقة_عائمة` | `abs_f64` | clears the sign bit, also of a NaN |
| floor, ceil, trunc | `بدائي_أرضية_عائمة`, `بدائي_سقف_عائم`, `بدائي_بتر_عائم` | `floor_f64`, `ceil_f64`, `trunc_f64` | exact; a zero result keeps the operand's sign |
| round to nearest, ties to even | `بدائي_تقريب_زوجي_عائم` | `round_ties_even_f64` | exact (roundToIntegralTiesToEven) |
| minimum, maximum | `بدائي_أدنى_عائم`, `بدائي_أقصى_عائم` | `min_f64`, `max_f64` | IEEE 754-2019 minimum and maximum: a NaN operand gives a NaN, and -0 < +0 |
| copysign | `بدائي_نسخ_إشارة_عائمة` | `copysign_f64` | the first operand's magnitude with the second's sign |
| fused multiply-add | `بدائي_ضرب_جمع_مدمج_عائم` | `mul_add_f64` | a * b + c rounded once |

The `f32` forms exist as well. Every backend produces the same bits: the native backends and LLVM lower each
operation to the target's instruction (see [MSIR and code generation](../internals/msir-and-codegen.md)), and the
interpreter computes it exactly on the bits. Where a target's baseline instruction set has no integral rounding
(baseline x86-64 has no ROUNDSD) or no fused multiply-add, std.math selects an exact bit-level form by architecture
instead of the primitive, so no libm call is ever needed. The payload and sign of a NaN result
are not specified; abs and copysign only move the sign bit.

## Negation and constant arithmetic

Unary minus applies to signed integers and floats. On a float it is the IEEE negation, `-0.0 - x`, so `-x` of
`+0.0` is `-0.0`. On a value of an unsigned type (`uint`, `u8`…`u64`) it is `SULTANC-TYPE-MISMATCH` with the reason
`type.unsigned-negation`, in a function and in a module constant alike. An integer literal keeps
its own value check, so `-0` fits an unsigned type and `-1` is out of range. Subtract, or convert to a signed type
first.

A module constant is evaluated at compile time with the run-time semantics. An operation that would trap at run
time (integer overflow, division or remainder by zero, the signed minimum divided by -1, a shift by the width or
more) is `SULTANC-COMPTIME-EVALUATION-FAILED` with the reason `comptime.execution-trapped`. A constant whose declared type is wider than its expression's, such as
`let k: int = (250 as u8) + 3;`, is a widening the type checker accepts, and the constant holds the same number in
the declared type.

## Thread capture rules

A closure handed to another thread may capture only values that are safe to send there and that outlive the
spawning scope. SultanC has no traits, so the frontend checks the closure
literal at the call (`compiler/frontend/types/thread_capture.sn`, run after the unsafe check and before SSF).

| Captured | Verdict | Diagnostic |
|---|---|---|
| a value whose type holds a raw pointer (`*T`, `*mutable T`, anywhere inside records, enums, tuples, options, results, vectors or boxes) | rejected, unless the call that hands the closure over sits inside `unsafe` (the programmer vouches for the pointee's synchronization) | `SULTANC-THREAD-CAPTURE-RAW-POINTER` |
| a value whose type holds a borrowed reference or a slice (`&T`, `&mutable T`, `[] T`) | rejected: the thread may outlive the borrowed storage | `SULTANC-THREAD-CAPTURE-REFERENCE` |
| a writable module variable named in the closure body, outside an unsafe block of that body | rejected: every thread shares it and its reads and writes are not atomic | `SULTANC-THREAD-CAPTURE-NON-ATOMIC-SHARED` |
| plain values (numbers, records of them), owned values (moved into the closure), function values and the std.sync handles built on them (`arc`, whose count is atomic), type parameters | accepted | none |

Each diagnostic names the captured symbol and its type at its first use in the closure, in Arabic and English
(iSpeak).

Which calls send a closure is derived, not listed. The function-typed parameters of a function that calls a
closure-address intrinsic (`بدائي_عنوان_دالة`, `بدائي_مدخل_سي_لدالة`, which is how std.thread hands a closure to a
new thread on every OS) are *sending*, and a parameter a function forwards to a sending parameter is sending too,
to a fixpoint over all units. When no unit names those intrinsics, as in the compiler itself and most programs, the
pass does nothing. See [Concurrency](concurrency.md) for the user-facing side.

## Responsibilities of the type layer

The type layer owns canonical type identity, builtin metadata, type construction and interning, named-type
resolution, equality and compatibility, conversion classification, scalar and signedness queries, integer-literal
fit checks, and the semantic type shape exposed to downstream consumers. Consumers ask these APIs for type facts
instead of reconstructing them from AST syntax or source spelling.

Type identity is distinct from physical representation and machine calling convention. The type system does not
decide source tokenization or grammar, module and name lookup, borrow and lifetime legality, target selection,
physical field offsets or alignment, ABI argument and return locations, register allocation, object format or
instruction encoding. Those facts belong to the frontend, layout, target, ABI and backend layers.

After type resolution, target-aware layers query the type shape and combine it with the selected target's data
model:

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

The meaning of a type stays target-neutral while its physical representation varies by target where required.

## Limitations

- A slice assigned a literal or call result after its declaration (`s := [...]`) owns nothing, so the buffer leaks.
- `[]` with no element and no vector context is a type mismatch.
- A closure passed through a variable instead of a literal is not inspected by the thread-capture check, a captured
  closure is opaque (its own captures are not walked), and a writable module variable reached through a call inside
  the closure is not seen.
- A type whose physical layout is intentionally not fixed (text, vectors, slices, function values) has no size or
  alignment; asking for one at compile time is a diagnostic ([Compile-time evaluation](comptime.md)).

## See also

- [Arabic terminology](arabic-terminology.md)
- [Ownership and destruction](ownership.md)
- [Compile-time evaluation](comptime.md)
- [Concurrency](concurrency.md)
- [Formatting](formatting.md)
- [The SultanC Book](../book/ch00-00-introduction.md)
