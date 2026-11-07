# Compile-Time Evaluation

This page specifies compile-time expressions and blocks (`comptime` / `وقت_الترجمة`), compile-time parameters,
reflection and validation functions, declaration annotations and decorators, and the always-on assertions of
`std.debug.assert`. It is for programmers who move work from run time to compile time and for contributors working
on the evaluator.

## How evaluation works

The frontend owns syntax and typing. MSIR lowering turns every compile-time obligation into ordinary verified MSIR,
and the driver discharges it before any backend or the interpreter's run phase sees the program
(`compiler/driver/compile_time.sn`). The verified MSIR interpreter is the only executor: it is not duplicated for
compile-time execution. It runs with an instruction budget, a call-depth budget and the effect policy of
compile-time execution, which allows no I/O and no host state. A failure is a compile-time diagnostic and never a
run-time fallback; there is no runtime fallback for an explicitly compile-time expression. Target-specific values
respect the selected target's registered model.

Within that policy a primitive may change a value the evaluation owns (append, swap, the vector length store of
pop and truncate) and release the interpreter's own storage (drop glue), so compile-time code can build and shrink
vectors and text. Primitives that observe machine memory or the host stay prohibited
(`SULTANC-COMPTIME-PROHIBITED-EFFECT`).

The lexer's canonical token registry is `tools/generate/registry/token_registry.def`, and the generated C and
SultanC token surfaces must match. The C Stage0 seed need not use new syntax while compiling frontend support for
it; once the compiler's own source uses a construct, the seed must be advanced or the source must remain
Stage0-compatible.

## Expressions and blocks

- `comptime E` / `وقت_الترجمة E` and the braced expression form `comptime { e1; e2; }` (expressions only, the last
  one supplying the value) are evaluated through the dependency slice of the value. Calls to ordinary functions in
  the slice are executed by the interpreter.
- A statement `comptime { statements }` is lowered as an independent void MSIR function, executed once and removed.
- A value block assigns the result of statements to a place. It is written as a `let` initializer or after `:=`.
  The block's final expression, or a final call statement, is the value. The block and its value form an
  independent MSIR function returning the place's type, evaluated once, and the place receives the constant. Like
  a statement block, it cannot use the enclosing function's locals or return from it, and its value must be a
  compile-time representable value: integers, booleans, characters, text, and vectors of these.

```sultan
def main(): int {
    let squares: int = comptime {
        let total: mutable int = 0;
        let i: mutable int = 1;
        while (i <= 4) { total := total + i * i; i := i + 1; }
        total
    };
    return squares;
}
```

```sultan
عرف الرئيسية(): عدد {
    دع المربعات: عدد = وقت_الترجمة {
        دع المجموع: متغير عدد = 0;
        دع ف: متغير عدد = 1;
        طالما (ف <= 4) { المجموع := المجموع + ف * ف; ف := ف + 1; }
        المجموع
    };
    أرجع المربعات;
}
```

Nested explicit compile-time subexpressions, such as `comptime (comptime 20) + (comptime { 22; })`, are evaluated
through the same constant evaluator, with no separate execution context. English and Arabic examples are in
`tests/{en,ar}/language/comptime/nested_constant_expression.sn`.

Module-level `let` declarations are constants evaluated at compile time with the run-time semantics. An operation
that would trap at run time makes the constant a `SULTANC-COMPTIME-EVALUATION-FAILED` error
(see [Type system](type-system.md#negation-and-constant-arithmetic)).

## Compile-time parameters

A function with a `comptime` parameter is a template.

- If every parameter is `comptime`, each call is evaluated at compile time.
- Otherwise each call is specialized. Its compile-time arguments must be compile-time known (constants,
  compile-time results, or a function name used as a value), and the call is redirected to a copy of the function in
  which those parameters are that value and only the runtime parameters remain. Copies are shared per (function,
  compile-time arguments), so each distinct set of compile-time arguments is instantiated once. Inside the copy,
  `comptime` expressions may use the compile-time parameters. At most 4096 copies are created, so a recursion on a
  compile-time parameter without a compile-time base case is a diagnostic
  (`comptime.specialization-limit-exceeded`).

```sultan
def scale(comptime factor: int, x: int): int { return x * comptime (factor + 0); }

def main(): int {
    let n: int = 7;
    return scale(6, n);   // specialized for 6
}
```

```sultan
عرف قياس(وقت_الترجمة معامل: عدد, س: عدد): عدد { أرجع س * وقت_الترجمة (معامل + 0); }

عرف الرئيسية(): عدد {
    دع ع: عدد = 7;
    أرجع قياس(6, ع);   // مخصصة للقيمة 6
}
```

A compile-time parameter is a constant of the copy everywhere a `comptime` expression can use it: alone
(`comptime factor`), in arithmetic, as a call argument (`comptime double(factor)`), and in a block after run-time
control flow. The copy defines the parameter in its entry block, so the evaluation of a `comptime` expression seeds
the constants of the whole function, not only of its own block: every constant instruction, and every block
parameter that all incoming edges of reachable blocks pass the same constant
(`ابذر_ثوابت_خارج_شريحة_وقت_الترجمة`, `compiler/driver/compile_time.sn`).

### Branches the compile-time arguments decide

In a specialized copy, a run-time `if` (or `&&`, `||`, or a `while` condition) whose condition the compile-time
arguments decide is folded before the copy's compile-time work, so only the taken branch is analyzed
(`اطو_فروع_تخصيص_وقت_الترجمة`). A branch condition, and every value a reachable block passes along its edges, is
evaluated where only constants, integer arithmetic, comparisons and casts lead to it, and a local that lowering
keeps in a place (a parameter read after an `if`) is forwarded from its one store. Nothing that needs a call or
run-time data is folded. As a result:

- `comptime` expressions and calls of compile-time templates on a folded-away path are never evaluated;
- a `compile_error` (`فشل_وقت_الترجمة`) left on a reachable path of the copy fails the compilation with its message
  (`SULTANC-COMPTIME-FAILED`), whether the compile-time arguments take that path or run-time data decides it. An
  analyzed `compile_error` is always rejected, and in specialized code it is never a run-time trap.

```sultan
import std.meta.reflect;

def checked(comptime n: int, x: int): int {
    if (n > 10) { compile_error("n must be at most 10"); };
    return n + x;
}

def main(): int {
    let x: int = 7;
    return checked(4, x);    // compiles: the branch is folded away
    // checked(42, x) would fail: SULTANC-COMPTIME-FAILED: n must be at most 10
}
```

```sultan
استورد std.meta.reflect;

عرف فحص(وقت_الترجمة ن: عدد, س: عدد): عدد {
    لو (ن > 10) { فشل_وقت_الترجمة("ن يجب ألا تزيد على 10"); };
    أرجع ن + س;
}

عرف الرئيسية(): عدد {
    دع س: عدد = 7;
    أرجع فحص(4, س);    // تترجم: الفرع مطوي
}
```

Tests: `tests/{ar,en}/language/comptime/parameter_in_comptime_expression.sn`, `folded_compile_error.sn`,
`reject_folded_compile_error.sn` and `reject_runtime_branch_compile_error.sn`.

## Reflection and validation

`std.meta.reflect` (`المكتبة.انعكاس.reflect`) provides facts of the type a reference refers to. The driver folds
them into constants for the selected target before any evaluation or code generation.

| Arabic | English | Result |
|---|---|---|
| `حجم_النوع(&v)` | `size_of` | target byte size (layout owner `backend/layout`) |
| `محاذاة_النوع(&v)` | `align_of` | target alignment |
| `عدد_حقول_النوع(&v)` | `field_count` | record fields or tuple elements, else 0 |
| `عدد_منشئات_النوع(&v)` | `constructor_count` | enum constructors, else 0 |
| `فشل_وقت_الترجمة(msg)` | `compile_error` | fails compile-time evaluation |

```sultan
import std.meta.reflect;

type Pair = { left: i32, right: i32 };
type Shape = | Circle | Square | Triangle;

def main(): int {
    let p: Pair = { left: 1, right: 2 };
    let s: Shape = Shape.Square();
    let size: u64 = size_of(&p);
    let fields: u64 = field_count(&p);
    let ctors: u64 = constructor_count(&s);
    return (size + fields + ctors) as int;
}
```

A type whose physical layout is intentionally not fixed (text, vectors, slices, function values) has no size or
alignment, and asking for one is a compile-time diagnostic. A `compile_error` reached during compile-time
evaluation, or left on a reachable path of a specialized copy, reports `SULTANC-COMPTIME-FAILED` with its message.
In an ordinary function, a run-time path that reaches it traps. The compiler primitives behind these functions
(`بدائي_حجم_النوع` and the others) are registered like every other compiler primitive.

### Checks the compiler inserts

The format check of std.fmt is a compile-time obligation that lowering adds before a call of a std format function
whose format string is a literal. It is a call of std.fmt's own checker with the literal and the argument kinds,
evaluated by the interpreter like any compile-time call and then removed ([Formatting](formatting.md), "Compile-time
check"). All such checks of one compilation share one interpreter context over the verified program
(`أنشئ_سياق_وقت_الترجمة_مسير`).

## Declaration annotations and decorators

`@public` / `@عام` is the public declaration modifier. Any other annotation on a function is a decorator: an
ordinary function applied to the decorated function, resolved like any call target (`@d`, `@d(args)`,
`@module.d(args)`).

```text
@d(a1, a2) def f(x: T): R { body }
// f is bound to:  def f(y: T): R { return d(a1, a2, <original f>, y); }
```

The decorator receives its arguments, the original function as a value of type `fn(T...): R`, and the call's
arguments, and returns `R`. Everything is checked as an ordinary call, including the ownership of the forwarded
arguments. Decorators compose from the declaration outwards, so `@a @b def f` is `a` applied to (`b` applied to
`f`). Declaring the decorator's function parameter `comptime` specializes each application at compile time with a
direct function value, where the decorator can validate its compile-time arguments. Decorator arguments are
evaluated at each call.

```sultan
def add_one(comptime f: fn(int): int, x: int): int { return f(x) + 1; }

@add_one
def triple(x: int): int { return x * 3; }

def main(): int {
    let n: int = 7;
    return triple(n);
}
```

```sultan
عرف زد_واحدا(وقت_الترجمة د: دالة(عدد): عدد, س: عدد): عدد { أرجع د(س) + 1; }

@زد_واحدا
عرف ثلاثة(س: عدد): عدد { أرجع س * 3; }

عرف الرئيسية(): عدد {
    دع ع: عدد = 7;
    أرجع ثلاثة(ع);
}
```

The frontend performs the expansion on source text appended after the unit
(`compiler/frontend/annotations/decorators.sn`), so diagnostics in a decorator application point into that generated
text. Annotations on types and module values, and on functions that are external, generic or have compile-time
parameters, are rejected with `SULTANC-ANNOTATION-UNSUPPORTED`. Behavior such as HTTP route registration, caching
or test discovery belongs to libraries, not to the compiler.

## std.debug.assert

The canonical declaration is `تأكد(condition: bool)`, with the English alias `assert`. `تأكد_برسالة` /
`assert_with_message` takes a boolean and a borrowed message string. Both are always active, including in
optimized and release output. They evaluate the condition exactly once and, on failure, write the diagnostic to
standard error and terminate the process with exit code 1. They use the existing `std.io` and `std.process`
facilities, not a compiler primitive. See [Errors](errors.md#assertions) for the failure frame.

```sultan
import std.debug.assert;

def main(): int {
    assert(2 + 2 == 4);
    let message: string = "invariant failed";
    assert_with_message(true, &message);
    return 0;
}
```

```sultan
استورد المكتبة.تصحيح.assert;

عرف الرئيسية(): عدد {
    تأكد(2 + 2 == 4);
    أرجع 0;
}
```

The optional message expression is evaluated by the caller as an ordinary function argument, so unlike Rust's
`assert!` macro it is not lazy. A failing assertion needs the interpreter's `std.io` and `std.process` effects to
complete, and it is not a compile-time diagnostic.

## Limitations

- Compile-time results must be compile-time representable values: integers, booleans, characters, text and vectors
  of these.
- A value block and a statement block cannot use the enclosing function's locals or return from it.
- At most 4096 specializations are created for one program.
- Annotations on types, module values, external functions, generic functions and functions with compile-time
  parameters are not supported.
- Compile-time execution has an instruction budget and a call-depth budget, no I/O and no host state.

## See also

- [Type system](type-system.md)
- [Errors](errors.md)
- [Formatting](formatting.md)
- [Interpreter](../internals/interpreter.md)
- [Language tour](../guide/language-tour.md)
