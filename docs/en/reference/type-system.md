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

A slice declaration initialized from a slice literal (`let s: [] T = [...]`) or from a call result builds or
receives a vector, which an anonymous owner in the enclosing block destroys at the block's end, extending the
temporary's lifetime to the block. The same holds for a call result passed directly as a slice argument. A slice assigned a
literal or call result later (`s := [...]`) keeps no owner for it, so the buffer leaks instead of letting the view
outlive it.

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

## Comparison operators

`==` and `!=` are defined for integers, floats, booleans, characters, text, raw pointers (by address) and enums
without payloads (by constructor). Text compares by content, byte by byte, as std's `texts_equal`
(`نصان_متساويان`) does, so `name == "Sultan"` is true for a `string` holding those bytes. Neither operand is moved:
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
