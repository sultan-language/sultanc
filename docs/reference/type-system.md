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

- tuples, written `(int, int)` (`(عدد, عدد)`);
- boxes;
- option and result values;
- function types, written `fn(int): int` (`دالة(عدد): عدد`);
- pointers, and shared and mutable references (`&T`, `&mutable T`);
- slices and vectors;
- mutable wrappers (`mutable T`);
- named record, enum and alias types, which may take type parameters.

The identity of a named record or enum is its resolved declaration, not its spelling. Generic arguments nest, and
`مشترك<قفل<ص64>>` closes both lists with one `>>` token, as Rust, C++11 and Java do; the parser takes it as two
closing angle brackets. A generic call infers a type parameter from the type of the value passed, so a `متغير`
local passed for a parameter `ق` binds `ق` to the value's type rather than to `متغير T`
(`compiler/frontend/types/call_signature.sn`). A generic instance whose result type is `فراغ` may bind and return
that result, as in `دع ن: R = f(); أرجع ن;` with `R` = void: the binding holds no value.

```sultan
type Pair<A, B> = { first: A, second: B };

def main(): int {
    let pair: Pair<int, bool> = { first: 1, second: true };
    let present: option int = option.some(3);
    let nested: vector vector int = [[1], [2, 3]];
    return pair.first + length(nested);
}
```

```sultan
نوع زوج<أ, ب> = { الأول: أ, الثاني: ب };

عرف الرئيسية(): عدد {
    دع الزوج: زوج<عدد, منطقي> = { الأول: 1, الثاني: صواب };
    دع موجود: اختياري عدد = اختياري.موجود(3);
    دع متداخل: متجه متجه عدد = [[1], [2, 3]];
    أرجع الزوج.الأول + الطول(متداخل);
}
```

### Option and result

`option T` (`اختياري T`) holds a value or nothing. Its constructors are `some(x)`, `Some(x)` and
`اختياري.موجود(س)`, and `none`, `None` and `اختياري.لا_شيء`. `result T, E` (`نتيجة T, E`) has `ok` / `Ok` / `نجح`
and `err` / `Err` / `خطأ`. These names are contextual: lexically they are ordinary identifiers, and they name the
builtin type only in a type or constructor position, so a local may still be called `اختيار` or `نتيجة`
(`tools/generate/registry/type_registry.def`; the constructor spellings are matched in
`compiler/frontend/types/type_semantic_shape_queries.sn`).

The old Arabic spellings `اختيار` for the type and `بعض` for the constructor are deprecated aliases. They name the
same type and constructor, and each use is warned once with `SULTANC-SYNTAX-DEPRECATED-SPELLING`.

### Slices

A slice `[] T` is a non-owning view with the vector layout. A `vector T` value or place is accepted wherever `[] T`
is expected: as a call argument, or in a declaration or assignment of a slice. It is not accepted as a returned
value. The slice views the vector's buffer without a copy, and the vector keeps owning it
(`هل_عرض_متجه_كشريحة`, `compiler/frontend/types/compatibility.sn`). SSF treats the view as a shared borrow of the
vector's root local. A vector argument viewed as a slice is borrowed for the call, not moved, and a slice local
viewing a vector place holds a shared borrow, so the vector cannot be changed while the view is alive. MSIR retypes
the vector header with `نقل_تسلسل_مملوك`.

A slice declaration initialized from a slice literal (`let s: [] T = [...]`) or from a call result builds or
receives a vector, which an anonymous owner in the enclosing block destroys at the block's end, as Rust extends a
temporary's lifetime. The same holds for a call result passed directly as a slice argument. A slice assigned a
literal or call result later (`s := [...]`) keeps no owner for it, so the buffer leaks instead of letting the view
outlive it.

### Vector literal expressions

A vector literal `[a, b, c]` is an expression. It can be a call argument, a returned value, a record field, an
element of another literal (`[[1], [2, 3]]`), an indexed temporary (`[a, b][i]`) or an operand of a reader primitive
(`length([..])`). `let v: T = [..];` keeps the initializer statement (`تهيئة_متجه`), which also builds slices.

- **Typing.** Typing is bidirectional, as in Rust and Swift. In a `vector E` context (a parameter, a declared or
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

## Numeric casts

`x مثل T` / `x as T` converts between scalar types. The type checker accepts a cast between two scalar types only
when a conversion joins them (`تحويل_سلمي_مرفوض`, `compiler/frontend/types/compatibility.sn`). Any other pair, such
as `منطقي مثل عدد`, is `SULTANC-TYPE-MISMATCH` at the cast, with the reason `type.invalid-cast`. Casts that involve
references, pointers or byte vectors are checked where they are lowered.

| From | To | Result |
|---|---|---|
| integer | integer | two's-complement truncation or extension; wraps and never traps |
| `ط8` | `حرف` | the code point |
| `حرف` | an unsigned integer of 32 bits or more | the code point |
| float | integer | truncated toward zero, saturated at the type's bounds; NaN gives 0 |
| integer | float | the nearest float, ties to even |
| `ع32` | `ع64` | exact |
| `ع64` | `ع32` | the nearest `ع32`, ties to even; out of range gives an infinity; a NaN stays a NaN |

The float conversions follow Rust's `as` (saturating float-to-integer, defined by rust-lang/rust#71269), which
is also the behavior of Java's `d2l` and WebAssembly's non-trapping `iNN.trunc_sat_fMM`. Go and C leave an
out-of-range float-to-integer result implementation-defined or undefined; SultanC does not. Integer-to-float
rounding is IEEE 754 roundTiesToEven, as in Rust, Go, Swift and C.

```sultan
def main(): int {
    let small: u8 = 200;
    let wide: u32 = (small as u32) + 100;
    let wrapped: u8 = (wide as u8);   // 300 wraps to 44
    let ratio: f64 = 7.9;
    return (ratio as int) + (wrapped as int);
}
```

```sultan
عرف الرئيسية(): عدد {
    دع صغير: ط8 = 200;
    دع واسع: ط32 = (صغير مثل ط32) + 100;
    دع ملتف: ط8 = (واسع مثل ط8);   // يلتف 300 إلى 44
    دع النسبة: ع64 = 7.9;
    أرجع (النسبة مثل عدد) + (ملتف مثل عدد);
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
  (`compiler/interpreter/operations/float_software.sn`). A module constant such as `دع ك: عدد = 2.5 مثل عدد;` is
  folded with the same code, and a float result is kept as its exact decimal text
  (`نص_عشري_دقيق_من_بتات`, `compiler/msir/decimal_float_bits.sn`).

The bit patterns of floats are available as `بتات_ع64` / `f64_to_bits`, `ع64_من_بتات` / `f64_from_bits` and the
`ع32` forms in `std.math.float`, over the compiler's bit-reinterpret intrinsics.

## Wrapping arithmetic

`+`, `-`, `*` and `<<` trap on overflow. Wrapping arithmetic, the result modulo 2^N, is provided by four MSIR
operations (`جمع_ملتف`, `طرح_ملتف`, `ضرب_ملتف`, `إزاحة_يسرى_ملتفة`) that lower to the plain machine instruction on
every backend. They are reached through the compiler intrinsics `بدائي_جمع_ملتف`, `بدائي_طرح_ملتف`,
`بدائي_ضرب_ملتف`, `بدائي_إزاحة_يسرى_ملتفة` and `بدائي_إزاحة_يمنى_ملتفة`, whose two operands share one integer
type. A shift takes its amount modulo the width, as Rust's `wrapping_shl` does.

`std.math.wrapping` gives every integer type a wrapping form `جمع_T_ملتف` / `wrapping_add_T` (and `sub`, `mul`,
`shl`, `shr`), a checked form `جمع_T_متحقق` / `checked_add_T` that returns an `اختياري`, and a saturating form
`جمع_T_مشبع` / `saturating_add_T`, after Rust's `wrapping_*`, `checked_*` and `saturating_*`.

## Float operations

Beyond `+ - * /` and the comparisons, the IEEE 754 operations that Rust exposes as `f64` methods are compiler
primitives. Each is one `هوية_بدائي_مترجم.عملية_عائمة` carrying a `عملية_عائمة_بدائية`, and its operands and result
have the first operand's float type, `ع32` or `ع64`.

| Operation | Intrinsic | std.math.float | Result |
|---|---|---|---|
| square root | `بدائي_جذر_تربيعي_عائم` | `جذر_تربيعي64` / `sqrt_f64` | correctly rounded (IEEE squareRoot); -0 gives -0, a negative operand NaN |
| absolute value | `بدائي_قيمة_مطلقة_عائمة` | `مطلق64` / `abs_f64` | clears the sign bit, also of a NaN |
| floor, ceil, trunc | `بدائي_أرضية_عائمة`, `بدائي_سقف_عائم`, `بدائي_بتر_عائم` | `أرضية64`, `سقف64`, `بتر64` | exact; a zero result keeps the operand's sign |
| round to nearest, ties to even | `بدائي_تقريب_زوجي_عائم` | `تقريب_زوجي64` / `round_ties_even_f64` | exact (roundToIntegralTiesToEven) |
| minimum, maximum | `بدائي_أدنى_عائم`, `بدائي_أقصى_عائم` | `أصغر64` / `min_f64`, `أكبر64` / `max_f64` | IEEE 754-2019 minimum and maximum: a NaN operand gives a NaN, and -0 < +0 |
| copysign | `بدائي_نسخ_إشارة_عائمة` | `نسخ_إشارة64` / `copysign_f64` | the first operand's magnitude with the second's sign |
| fused multiply-add | `بدائي_ضرب_جمع_مدمج_عائم` | `ضرب_جمع_مدمج64` / `mul_add_f64` | a * b + c rounded once |

The `ع32` forms exist as well. Every backend produces the same bits: the native backends and LLVM lower each
operation to the target's instruction (see [MSIR and code generation](../internals/msir-and-codegen.md)), and the
interpreter computes it exactly on the bits. Where a target's baseline instruction set has no integral rounding
(baseline x86-64 has no ROUNDSD) or no fused multiply-add, std.math selects an exact bit-level form by architecture
instead of the primitive, as Rust's libm does, so no libm call is ever needed. The payload and sign of a NaN result
are not specified, as in Rust; abs and copysign only move the sign bit.

## Negation and constant arithmetic

Unary minus applies to signed integers and floats. On a float it is the IEEE negation, `-0.0 - x`, so `-x` of
`+0.0` is `-0.0`. On a value of an unsigned type (`طبيعي`, `ط8`…`ط64`) it is `SULTANC-TYPE-MISMATCH` with the reason
`type.unsigned-negation`, as Rust's E0600, in a function and in a module constant alike. An integer literal keeps
its own value check, so `-0` fits an unsigned type and `-1` is out of range. Subtract, or convert to a signed type
first.

A module constant is evaluated at compile time with the run-time semantics. An operation that would trap at run
time (integer overflow, division or remainder by zero, the signed minimum divided by -1, a shift by the width or
more) is `SULTANC-COMPTIME-EVALUATION-FAILED` with the reason `comptime.execution-trapped`, as Rust's E0080 is for a
`const`. A constant whose declared type is wider than its expression's, such as
`دع ك: عدد = (250 مثل ط8) + 3;`, is a widening the type checker accepts, and the constant holds the same number in
the declared type.

## Thread capture rules

A closure handed to another thread may capture only values that are safe to send there, the role Rust's
`F: Send + 'static` bound plays on `std::thread::spawn`. SultanC has no traits, so the frontend checks the closure
literal at the call (`compiler/frontend/types/thread_capture.sn`, run after the unsafe check and before SSF).

| Captured | Verdict | Diagnostic |
|---|---|---|
| a value whose type holds a raw pointer (`*T`, `*متغير T`, anywhere inside records, enums, tuples, options, results, vectors or boxes) | rejected, unless the call that hands the closure over sits inside `غير_آمن` (the programmer vouches for the pointee's synchronization, as with `unsafe impl Send`) | `SULTANC-THREAD-CAPTURE-RAW-POINTER` |
| a value whose type holds a borrowed reference or a slice (`&T`, `&متغير T`, `[] T`) | rejected: the thread may outlive the borrowed storage (`'static`) | `SULTANC-THREAD-CAPTURE-REFERENCE` |
| a writable module variable named in the closure body, outside an unsafe block of that body | rejected: every thread shares it and its reads and writes are not atomic (Rust's `static mut`) | `SULTANC-THREAD-CAPTURE-NON-ATOMIC-SHARED` |
| plain values (numbers, records of them), owned values (moved into the closure), function values and the std.sync handles built on them (`مشترك` / Arc, whose count is atomic), type parameters | accepted | none |

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
  the closure is not seen. Rust tracks all three through auto traits and `static` rules.
- A type whose physical layout is intentionally not fixed (text, vectors, slices, function values) has no size or
  alignment; asking for one at compile time is a diagnostic ([Compile-time evaluation](comptime.md)).

## See also

- [Arabic terminology](arabic-terminology.md)
- [Ownership and destruction](ownership.md)
- [Compile-time evaluation](comptime.md)
- [Concurrency](concurrency.md)
- [Formatting](formatting.md)
- [Language tour](../guide/language-tour.md)
