[العربية](../../ar/internals/jisr.md) | **English**

# Jisr: the C frontend

Jisr (جسر, "bridge") is SultanC's own C frontend. `import "x.h"` and `import "x.c"` go through it: it preprocesses,
parses and analyses the C file for the target being compiled and writes its declarations as a SultanC module. A `.c`
import is also compiled through the SultanC pipeline, so no C compiler is needed on any target. This page describes the
pipeline, the code owners, how C declarations map to SultanC, what compiled C code supports, how several C imports
share one program, how C diagnostics reach the user, and the limits. The user-facing rules are in
[Modules and imports](../reference/modules-and-imports.md#c-headers-and-sources).

## Pipeline

```text
import "x.c"  (module loading: compiler/frontend/module_loading/source_locator.sn)
    |
    v
translate.sn   file lookup per target, preprocessor choice, a.c+b.c units, SULTANC_C_DUMP
    |
    v
jisr.sn        target data model, include search, host-header fallback
    |
    +--> lexer.sn -> preprocessor.sn -> parser.sn (+ types.sn, literals.sn, headers.sn)
    |
    +--> body.sn -> lower.sn     function bodies and initializers of a .c import
    |
    v
emit.sn        the SultanC module text; shared.sn and units.sn merge it with other C imports
```

The generated module is ordinary SultanC: the rest of the compiler (names, types, safety, MSIR, the Native and LLVM
backends and the interpreter) sees no C.

## Owners

| Concern | File |
|---|---|
| Spelling interner, tokens, hide sets, the diagnostic record handed to `compiler/diagnostics/foreign_c.sn`, the quoted-include directory rule | `compiler/jisr/core.sn` |
| Translation phases 1-3: UTF-8 source (std.encoding.utf8), C23 identifiers (XID_Start/XID_Continue through std.unicode), universal character names, pp-numbers, digraphs, splices, comments | `compiler/jisr/lexer.sn` |
| Integer, floating, character and string literals (C17 6.4.4, 6.4.5), escapes, u/U/L/u8 encodings | `compiler/jisr/literals.sn` |
| The C data model of a registered target (pointer width, LLP64/LP64/ILP32, 16-bit int, char and wchar_t signedness, long double, va_list kind) and its predefined macros | `compiler/jisr/target.sn` |
| The compiler's own freestanding headers (`stddef.h`, `stdarg.h`, `stdint.h`, `limits.h`, `float.h`, ...) | `compiler/jisr/headers.sn` |
| `#include`/`#include_next`/`#import`, macros, `#if` arithmetic, `__has_*`, `#embed`, `#pragma once/pack/push_macro`, `_Pragma`, `#error`/`#warning`, the `cc -E -dD` reader | `compiler/jisr/preprocessor.sn` |
| C types, record layout (System V and Microsoft bit-fields, packed, aligned, flexible arrays), compatibility | `compiler/jisr/types.sn` |
| Declarations, declarators, extensions, `_Static_assert`, integer constant expressions | `compiler/jisr/parser.sn` |
| Typed expression and statement trees of function bodies and initializers | `compiler/jisr/body.sn` |
| Lowering compiled C to SultanC (memory, control flow, va_list, 128-bit and `_BitInt` arithmetic, thread-locals) | `compiler/jisr/lower.sn` |
| The SultanC module: records, opaque types, aliases, externs, constants, skipped declarations | `compiler/jisr/emit.sn` |
| One declaration per program across C imports | `compiler/jisr/shared.sn` |
| Several translation units in one import (`a.c+b.c`), variadic calls between them | `compiler/jisr/units.sn` |
| Driver: data model, include search, the module for one file | `compiler/jisr/jisr.sn` |
| Entry from module loading, file lookup, external preprocessor or compiler on request | `compiler/jisr/translate.sn` |

Generic text, ASCII, UTF-8, Unicode and path work comes from std (std.text, std.chars.ascii, std.encoding.utf8,
std.unicode, std.fs.path). Only C's own rules stay in Jisr: what an identifier may contain, escape sequences, integer
suffixes, the quoted-include search.

## Preprocessing and include search

Jisr's own preprocessor is the default. `SULTANC_C_PREPROCESSOR=cc` selects the system C compiler's (`cc -E -dD`, or
`$CC`), whose output Jisr reads; any other value is `SULTANC-C-INVALID-PREPROCESSOR`. `SULTANC_C_DUMP=<file>` writes
the generated module.

The search order is the importer's directory, `$SULTANC_C_INCLUDE` (`:`-separated), then the target's system include
directories: `$SULTANC_C_SYSROOT` when set, the host's for the host target, the macOS SDK for Darwin, and the multiarch
directories for a cross Linux target. When the target's own headers are missing and the host has the same data model,
the host's headers are read with the host's predefined macros inside them and the target's elsewhere
(`SULTANC-C-HOST-HEADERS`); otherwise only the freestanding headers are available (`SULTANC-C-NO-SYSTEM-HEADERS`).

The declarations written are those of the imported file's family: the file and the other headers under its root (the
include directory it was found in, or a project file's directory). Other headers are read for their types, enumerators
and macros.

## Declarations

| C | SultanC |
|---|---|
| integer and floating types | the type of the same width on the target (`int` is `i32`, `long` follows LP64/LLP64/ILP32) |
| `char *` / `const char *` | `*mutable u8` / `*u8` |
| other pointers | typed: `*T`, `*mutable T`, `*void`; `const` decides `*T` or `*mutable T` at each level |
| a pointer to a function, or to a type with no SultanC form | an address word of the target's pointer width |
| a struct whose C layout is SultanC's natural layout | a record (an array member is a tuple) |
| a union, a record with bit-fields, a packed record | opaque storage of the C size and alignment, `{ opaque: (u64, ...) }` |
| a struct or union only declared, one a header declares incomplete and only a `.c` unit defines, one with no SultanC layout | `public type NAME = opaque;`, its pointers `*NAME` / `*mutable NAME` |
| `typedef` | a type alias (an alias of an opaque type is the same type) |
| a function | `public extern def` with parameters `p0`, `p1`, ...; an asm label is its link name |
| a variadic function | its fixed parameters and the link name `SYMBOL...N` (a C variadic call shape) |
| an object | external data, `public extern let x: T = "x";` |
| an enumerator, a C23 `constexpr` integer, an object-like macro whose expansion is a constant | `public let`, typed as C types it |

A C name that is a SultanC keyword or builtin gets a trailing `_` and keeps its C symbol as link name. A name C reserves
for the implementation (`__x`, `_X`) is not written from a system header. A C name SultanC identifiers cannot spell is
left out of the module and spelled `_uXXXX` inside compiled code.

A declaration with no SultanC form is left out, never written with a wrong type: the module records it as
`// skipped NAME (file:line): c.skipped.... [SULTANC-C-...]` and the import reports a warning with that typed reason. A
struct used only through pointers is ordinary C and has no diagnostic.

## Compiled `.c` imports

`body.sn` builds typed trees of every function body and initializer, and `lower.sn` turns them into SultanC: C memory
through raw pointers, a frame block per function, a data block per unit, structured control flow with a block
dispatcher for `goto`, irregular `switch` and computed `goto`, and va_list for System V, AAPCS64 and `char *` targets.
Supported: C11 statements and expressions, case ranges, compound literals, designated initializers, static locals,
initialized globals, function pointers, statement expressions, `_Generic`, variadic definitions, calls of C library
functions (variadic too), reads and addresses of C library objects, variable-length arrays (outermost dimension of a
local), `__int128` and `_BitInt` up to 128 bits in computation, thread-local objects (one copy per thread), and `long
double` where it is `double`.

Each compiled function is wrapped by a public SultanC function with the C signature; the wrapper converts typed
pointers to the address words the compiled code uses, inside `unsafe`.

### Callbacks: functions with the C calling convention

As Zig's translate-c gives every translated C function `callconv(.c)` (`pub export fn`, or `pub fn ... callconv(.c)`
for a `static` one), Jisr compiles a function with SultanC's C calling convention, `@@c_abi`. A pointer to such a
function is its machine address, `c_function_address(f)`, so C code calls it directly: a `qsort` comparator, a thread
start routine, a window procedure. There is no trampoline. As in Zig's backends, where one routine
(`resolveCallingConventionValues`) places a function's own parameters and every call to it, and the prologue saves
`getCalleePreservedRegs(cc)`:

- MSIR records the convention per function (`برنامج_مسير.دوال_اتفاقية_سي`), and the one convention query
  (`اتفاقية_دالة_مسير_للواجهة`) gives such a function the target's foreign C plan, for its own entry and for every
  call to it, on every backend.
- Its frame saves the registers the C convention preserves that SultanC code may change: R15 (the closure
  environment's register) on x86-64, and RDI, RSI and XMM6-XMM15 on Windows (`أضف_محفوظة_اتفاقية_سي_إكس`,
  `عائمة_اتفاقية_سي_إكس`; Zig's x86_64 backend leaves the XMM registers as an open item). On AArch64 the internal
  convention already preserves the AAPCS64 callee-saved registers.
- The LLVM provider declares it without the hidden closure-environment parameter, like the process entry.
- `c_function_address(f)` lowers to the function constant typed `u64` (`اخفض_عنوان_دالة_سي_مسير`).

A `@@c_abi` function has a body, no type or compile-time parameters, and only integer, floating-point, boolean or
raw-pointer parameters and result; anything else, and using it as a function value, is `SULTANC-C-ABI-MISUSE`.

The functions that keep a marker are listed below. A pointer to one of them is the address of its marker:

- a variadic definition, whose variadic arguments are 8-byte slots, not C's variadic convention;
- a function that takes or returns a record by value;
- any function on WebAssembly and 32-bit targets, where no machine address can be called.

The same holds for SultanC code: `@@c_abi def compare(a: *i32, b: *i32): i32 { ... }` and
`c_function_address(compare)`. Such a function is no function value: a closure call passes an environment in the
internal convention, so using it as one is rejected (`SULTANC-C-ABI-MISUSE`). A program with one runs as an executable under `sultanc run`,
as imported `.c` code does.

### Calls through function pointers

A pointer to a function compiled in the same file is that function's machine address (above) or its marker, and a
call through a pointer first compares the pointer with those candidates and calls the matching one directly. Any other pointer is a
C function at a machine address: one returned by `dlsym`, or the address of a C library function, such as `labs` or
`objc_msgSend`. Its address is its symbol's, reached through the GOT on Mach-O and ELF.

Jisr is not a foreign-function layer: as with Zig's translate-c, C becomes SultanC source and the compiler's own
backends lower it. Zig types a C function pointer as `*const fn (...) callconv(.c) T` and its LLVM backend lowers a
call through it with the same per-target ABI code as a direct call (`airCall`, `ParamTypeIterator`, `firstParamSRet`),
since the convention belongs to the function type, not to the callee's name. SultanC does the same with a **call
shape at an address**: an extern whose link name is `*` (or `*...N` for a C variadic call with N fixed parameters).
Its first parameter is the address; the others and the result are the C signature
(`نادِ_عنوانا_جسر`, `شكل_عنوان_ل_جسر`):

```sultan
extern def خ_7_عنوان(p0: u64, p1: big): big = "*";
// struct big (*bn)(struct big) = dlsym(...);  r = bn(b);
r := خ_7_عنوان((bn as u64), b);
```

MSIR records it as an external function with `عند_عنوان` set. It names no symbol, so it is neither declared nor
linked, and several shapes may share one link name. Every backend plans the call from the C signature with the same
foreign-call planner as an extern by name (`حل_خطة_واجهة_دالة_خارجية_للهدف`), then calls through the address: the
x86-64 selector with `call *%r11`, AArch64 with `blr x16`, and LLVM with an indirect `call` of the planned signature
carrying the same `sret`/`byval` attributes. So whatever a direct C call can pass, a call through a pointer passes
the same way, records by value included:

| Target ABI | Record arguments | Record results |
|---|---|---|
| System V x86-64 (Linux, Apple x86-64, FreeBSD) | eightbyte classes in integer and SSE registers, all or nothing; a MEMORY record, or one that no longer fits, is copied onto the stack | in `rax`/`rdx`/`xmm0`/`xmm1`, or through a hidden pointer in `rdi` (MEMORY) |
| AAPCS64 (Linux, Apple arm64, Windows on Arm) | an HFA of up to four floats or doubles in `v` registers; up to 16 bytes in one or two `x` registers; anything larger as the address of a copy the caller makes | HFA in `v` registers, ≤16 bytes in `x0`/`x1`, otherwise into memory addressed by `x8` |
| Microsoft x64 | a record of 1, 2, 4 or 8 bytes as an integer in its positional slot; any other as the address of a copy the caller makes | 1, 2, 4 or 8 bytes in `rax`, otherwise through a hidden pointer in `rcx` |

Apple arm64 also has its own rules here, applied the same way: the caller extends narrow integers, stack arguments are
packed at their natural alignment, and variadic arguments always go on the stack. So
`((id (*)(id, SEL))objc_msgSend)(object, selector)` is the call Clang emits on Apple arm64 and x86-64: the method's own
non-variadic signature, with `x0`/`x1` (`rdi`/`rsi`) and `d0` for a `double`. A record-returning method uses
`objc_msgSend` with `x8` on arm64 and `objc_msgSend_stret` on x86-64, as in C: the source names the entry it calls.

A shape calls whatever its first argument holds with a signature nothing checks, so a call of it needs an `unsafe`
scope, like `raw_c_call` ([Safety Flow](safety-flow.md)). Jisr's function bodies are already in one.

Such a call is written for 64-bit x86-64 and AArch64 targets. A parameter or result with no SultanC form (`long double`
wider than `double`, a 128-bit integer) is reported (`SULTANC-C-UNSUPPORTED` with reason
`c.unsupported.indirect-call-signature`) when the pointer can only be such a function. When a candidate compiled in the
file might be the one called, that call still traps.

## Several C imports in one program

- **One declaration per program** (`shared.sn`): a declaration another C import of the same source root and target
  already exports with the same SultanC text is imported from it instead of written again. The frontend makes the names
  a C module takes from another C module part of its own (`compiler/frontend/names/modules.sn`), so every importer sees
  one symbol and one type.
- **Linkage between imports**: a C extern whose symbol names exactly one public function compiled from a `.c` import,
  with the same parameter and result types, is bound to it by MSIR (`compiler/msir/lowering/functions.sn`), so separately
  imported `.c` files call each other.
- **Units** (`units.sn`): `import "a.c+b.c"` compiles each unit on its own, numbers private names per unit, writes
  identical declarations once, and forwards variadic calls between units through an 8-byte-slot entry.

## Diagnostics

A C problem is a typed diagnostic: an identity (`SULTANC-C-*`), a C reason (`compiler/diagnostics/reason.sn`), the C
location as a byte column and length with the source line, typed facts (C name, C type, expected and found token,
counts, value, size, target), related locations (an earlier declaration) and the `#include` chain, innermost first.
iSpeak words it in English and Arabic; Jisr never writes message text. An error in the imported file's family fails the
import; an error in a header the import only reads is the warning `SULTANC-C-READ-HEADER-UNTRANSLATED` with the C error
as a note. Warnings are printed when the import succeeds.

## Limits

- A pointer to a compiled variadic function, or to one that returns a record by value, is still a marker, so it
  cannot be passed to external C code (see "Callbacks"). The address of a function compiled from another `.c`
  import has no C symbol, so taking it fails at link time.
- Windows and macOS callbacks are built (the Windows prologue's saves and argument registers checked in the generated
  code) but have not been run on those hosts. The Windows unwinder does not restore the saved XMM registers on an
  exception; a normal return does.
- Calls through a C function pointer at a machine address on WebAssembly and on 32-bit targets (no callable machine
  addresses there; such a call traps, and taking a library function's address is reported).
- `__int128` and `_BitInt(65..128)` in declarations (externs, records by value, objects), `_BitInt` wider than 128,
  `_Complex`, vector values, `long double` wider than `double`, `asm` statements, inner variable array dimensions,
  thread-local objects of another file, and variadic calls between separately imported `.c` files.
- Bit-fields assume little-endian bit order; 32-bit va_list support covers i386's `char *` form.

## Tests

`tests/{en,ar}/compiler/c_abi_callbacks` and `tests/{en,ar}/diagnostics/c_abi`: C calls `@@c_abi` functions at their
addresses, and their misuse is rejected. `tests/{en,ar}/compiler/c_import*`: headers and system headers, layout, macros, calls and variadics, compiled bodies,
C23 and extensions, linkage and shared declarations, units, thread-locals, opaque handles, Unicode identifiers,
warnings and rejected C (`c_import_errors`).
