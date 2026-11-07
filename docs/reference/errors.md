# Errors

SultanC treats failure as a value. This page specifies how errors are carried (`نتيجة` / `result` and `اختياري` /
`option`), passed up with `؟` / `?`, described with the `std.error` contract (`فشل` / `Error`), and reported when a
program stops. It also covers the optional language versions of an error message and the locale values that choose
among them. It is for programmers who write fallible code and for contributors working on std and the runtime
failure path.

## The layers

| Layer | What | Where |
|---|---|---|
| Carry | `نتيجة T, E` / `result T, E` and `اختياري T` / `option T` | compiler (`tools/generate/registry/type_registry.def`) |
| Pass up | the propagation operator `؟` / `?` | compiler, frontend expansion (`compiler/frontend/desugar/propagation.sn`) |
| Stop | the stop primitive `إنهيار` / `panic`: print a message and end the program | `library/std/process/panic.sn`; the compiler knows it never returns |
| Contract | the `فشل` / `Error` value, its constructors and accessors | `library/std/error/` |
| Helpers | notes, wrapping, chain search, developer rendering, converters from typed std errors, context helpers for `؟` | `library/std/error/` |
| Language versions | optional per-language texts of one error, locale values, BCP 47 tags, direction isolation | `library/std/error/failure.sn`, `library/std/text/` (`language.sn`, `system_language.sn`, `bidi.sn`) |

Only the first layers live in the compiler: the carrying types, the `؟` expansion, and the knowledge that the stop
primitive never returns. Compiler diagnostics are a separate system. iSpeak and `compiler/diagnostics/**` own every compiler, runtime-trap and
interpreter message, and the frames a program prints when it ends on an error or stops (see
[Runtime failures of a program](#runtime-failures-of-a-program)). `std.error` imports nothing from the compiler and
defines no `SULTANC-` code, diagnostic record, severity or message registry. The runner's architecture suite
(`tests/runner/suites/architecture.sn`) fails if a file under `library/std/error/` mentions `compiler.`,
`compiler/` or `SULTANC-`.

## Design rules

- **Any type can be an error.** `نتيجة T, E` takes any `E`. Typed error enums such as `خطأ_تحليل_عدد` and
  `خطأ_جسون` remain the primary API of std. `فشل` is the common shape for code that collects errors of different
  origins, and it is never required.
- **Code never inspects message text.** It decides on a typed enum or on a `فشل`'s identity, never on its message.
- **One author's message per `فشل`.** The message is the author's text in the author's language, and every function
  except the locale-aware ones works on it alone. Only the frame around it follows the program's language. Language
  versions are optional additions that only the functions taking a locale read.
- **Patterns use the canonical Arabic variant names** of typed std errors, also in English source
  (`examples/en/errors/results.sn`).

## Results and options

`result T, E` holds `result.ok(value)` (`نتيجة.نجح`) or `result.err(error)` (`نتيجة.خطأ`), and `option T` holds
`option.some(value)` (`اختياري.موجود`) or `option.none` (`اختياري.لا_شيء`). `match` takes either apart, and `؟`
passes the error or the absence up.

```sultan
import std.io.println;
import std.text.number;

def divide(a: int, b: int): result int, string {
    if (b == 0) { return result.err("division by zero"); };
    return result.ok(a / b);
}

def first_item(values: &vector int): option int {
    if ((length(*values) as uint) == 0) { return option.none(); };
    return option.some((*values)[0]);
}

def main(): int {
    match divide(84, 2) with
    | result.ok(q) -> { println(int_to_text(q)); }
    | result.err(message) -> { println(message); };
    let empty: vector int = [];
    match first_item(&empty) with
    | option.some(v) -> { println(int_to_text(v)); }
    | option.none -> { println("no items"); };
    return 0;
}
```

```sultan
استورد std.io.println;
استورد std.text.number;

عرف اقسم(أ: عدد, ب: عدد): نتيجة عدد, نص {
    لو (ب == 0) { أرجع نتيجة.خطأ("القسمة على صفر"); };
    أرجع نتيجة.نجح(أ / ب);
}

عرف أول_عنصر(القيم: &متجه عدد): اختياري عدد {
    لو ((الطول(*القيم) مثل طبيعي) == 0) { أرجع اختياري.لا_شيء(); };
    أرجع اختياري.موجود((*القيم)[0]);
}

عرف الرئيسية(): عدد {
    طابق اقسم(84, 2) مع
    | نتيجة.نجح(ق) -> { اطبع_سطر(عدد_إلى_نص(ق)); }
    | نتيجة.خطأ(رسالة) -> { اطبع_سطر(رسالة); };
    دع فارغ: متجه عدد = [];
    طابق أول_عنصر(&فارغ) مع
    | اختياري.موجود(ع) -> { اطبع_سطر(عدد_إلى_نص(ع)); }
    | اختياري.لا_شيء -> { اطبع_سطر("لا عناصر"); };
    أرجع 0;
}
```

## Propagation: `؟` / `?`

`EXPR؟` passes an error, or an absence, up to the caller. It is accepted only as the last postfix of a statement's
whole right-hand side, in four forms:

| Form | Meaning on success |
|---|---|
| `دع x: T = EXPR؟;` | `x` takes the value |
| `x := EXPR؟;` | the place takes the value |
| `أرجع EXPR؟;` | the value is returned |
| `EXPR؟;` | the value is dropped |

Anywhere else, inside a call argument or as an operand for instance, `؟` is an unexpected token
(`SULTANC-PARSE-SYNTAX`). `؟` (U+061F) and `?` are two spellings of one token (`QUESTION_OPERATOR`,
`tools/generate/registry/token_registry.def`).

```sultan
def divide(a: int, b: int): result int, string {
    if (b == 0) { return result.err("division by zero"); };
    return result.ok(a / b);
}

def half_quotient(a: int, b: int): result int, string {
    let q: int = divide(a, b)?;
    return result.ok(q / 2);
}
```

```sultan
عرف اقسم(أ: عدد, ب: عدد): نتيجة عدد, نص {
    لو (ب == 0) { أرجع نتيجة.خطأ("القسمة على صفر"); };
    أرجع نتيجة.نجح(أ / ب);
}

عرف نصف_الناتج(أ: عدد, ب: عدد): نتيجة عدد, نص {
    دع ق: عدد = اقسم(أ, ب)؟;
    أرجع نتيجة.نجح(ق / 2);
}
```

The typing rules are:

- `EXPR: نتيجة T, E` in a function returning `نتيجة U, E` gives `T`. On an error the function returns
  `نتيجة.خطأ(e)`. The error type must be the same `E`: nothing converts it, because there are no traits, so a
  different error type is reported at the `؟`, naming both types (`SULTANC-CALL-ARGUMENT-TYPE-MISMATCH`). Convert
  explicitly with the [context helpers](#context-helpers).
- `EXPR: اختياري T` in a function returning `اختياري U` gives `T`. On `اختياري.لا_شيء` the function returns
  `اختياري.لا_شيء()`.
- In a function that returns neither, `؟` is a return type mismatch at the `؟` (`SULTANC-TYPE-RETURN-MISMATCH`,
  naming the function's type and the expression's). An option inside a function returning a result is
  `SULTANC-PATTERN-INVALID` at the `؟`.
- `أرجع EXPR؟;` returns `T` itself, so `T` must be the function's result type, as in Rust.

The arms follow the return type as written. `اختياري …` gives option arms, and `نتيجة …` or a named type (an alias
of a result) gives result arms. An alias of an option is not recognized, so write `اختياري …` in the signature of a
function that uses `؟` on options.

### How it is built

`compiler/frontend/desugar/propagation.sn` expands every marked statement right after parsing, before name
resolution, so names, types, patterns, the ownership flow (SSF), MSIR, every backend and the interpreter see
ordinary statements. For `دع x: T = EXPR؟;` in a function returning a result, the expansion is:

```text
let x: T;
match EXPR with
| hidden -> {
    match hidden with
    | result.ok(value) -> { x := value; }
    | result.err(error) -> { return result.err(error); };
};
```

- `EXPR` is bound first to a hidden owned local, by an irrefutable binding pattern whose type is the expression's
  own, and the dispatching match is on that local, never on the call result.
- Each arm binds one payload, and the early return is the ordinary return, so the drop frame releases every owned
  local exactly once. `tests/{ar,en}/language/propagation/drops.sn` returns early 10^5 times from a function holding
  text, a vector and a box, and checks that the allocator's live bytes are equal before and after.
- The expansion is SultanC text appended after the unit's own text and parsed by the ordinary parser, as decorator
  expansion is, so every generated name has a source position. `EXPR` and an assignment's target keep their own
  positions; they are spliced in from the original statement. The hidden names start with `انتشار__`.
- The source map records each generated region and the `؟` it stands for (`compiler/source/source_map.sn`,
  `إحالة_نص_مولد`), so a diagnostic inside the expansion is reported at the `؟`.

The C Stage0 implements the same expansion in its parser (`bootstrap/src/frontend/parser/parser_statement_propagation.c`):
a hidden temporary of inferred type, then the match. The compiler and the std modules it imports use `؟`, for
example `compiler/driver/package_manifest.sn` and `std.toml.parser`.

## The std.error module

| Module | Arabic import | Contents |
|---|---|---|
| `std.error.failure` | `المكتبة.أخطاء.failure` | the `فشل` value and its whole API |
| `std.error.from_os` | `المكتبة.أخطاء.from_os` | converters for std.fs, std.io and std.process |
| `std.error.from_number` | `المكتبة.أخطاء.from_number` | converter for std.text.number |
| `std.error.from_json` | `المكتبة.أخطاء.from_json` | converter for std.json |
| `std.error.from_net` | `المكتبة.أخطاء.from_net` | converters for std.net |
| `std.error.from_command` | `المكتبة.أخطاء.from_command` | converter for std.process.command's `خطأ_أمر` (domain `std.process`, kind `تشغيل`, `دخل_خرج` or `انتظار`, cause = the classified system error) |
| `std.error.from_duration` | `المكتبة.أخطاء.from_duration` | converter for std.time.duration_text's `خطأ_مدة` (domain `std.time`) |
| `std.error.from_fmt` | `المكتبة.أخطاء.from_fmt` | converter for std.fmt's `خطأ_تنسيق` (domain `std.fmt`, kind = the variant name, the byte offset, argument index or writer status as a note) |
| `std.error.context` | `المكتبة.أخطاء.context` | the context helpers for `؟` |
| `std.text.language` | `المكتبة.نصوص.language` | the locale value `لغة` / `Locale`, BCP 47 tags, Accept-Language |
| `std.text.system_language` | `المكتبة.نصوص.system_language` | the locale of the program's environment |
| `std.text.bidi` | `المكتبة.نصوص.bidi` | direction isolation |

`أخطاء` is the Arabic spelling of the `error` package (`library/Supax.toml`; see
[Modules and imports](modules-and-imports.md#roots)). The C Stage0 resolves the Arabic package names from the same
manifest (`bootstrap/src/support/path/source_import_path.c`).

### The value

A `فشل` (`library/std/error/failure.sn`) holds:

- an **identity** `(النطاق, النوع)`, domain and kind. The domain names the owner, a module path such as `std.fs` or
  a package such as `payments`. The kind is a stable machine name inside that domain, such as `غير_موجود`. An empty
  kind means an ad-hoc error with no identity. This is the idea of gRPC's `ErrorInfo (domain, reason)` and the RFC
  9457 `type`;
- a **message** for developers: one sentence in the author's language, without a final period;
- an optional **cause**, another `فشل`, so errors form a chain;
- **notes**, in the order they were added (Python's `add_note`);
- optional **language versions** of the message (see [Language versions](#language-versions)).

The value is opaque. Use the functions below, not its fields, so that fields can be added later. It owns text, a
vector and a box, so it is destroyed automatically like any owning value, and nothing needs releasing by hand.
Copies are explicit: the `استنسخ` / `clone` primitive or `انسخ_فشل`. Drop glue releases the text, the notes vector
and the boxed cause chain, and clone glue copies them.

### API

| Arabic | English | Signature | Meaning |
|---|---|---|---|
| `فشل` | `Error` | type | the error value |
| `فشل_من` | `error_new` | `(الرسالة: نص): فشل` | ad-hoc error, empty identity (Go `errors.New`) |
| `فشل_بهوية` | `error_with_kind` | `(النطاق: نص, النوع: نص, الرسالة: نص): فشل` | error with an identity |
| `فشل_بسبب` | `error_caused_by` | `(الرسالة: نص, السبب: فشل): فشل` | ad-hoc error wrapping a cause, which is moved in (Go `%w`) |
| `فشل_بهوية_وسبب` | `error_with_kind_caused_by` | `(النطاق: نص, النوع: نص, الرسالة: نص, السبب: فشل): فشل` | error with an identity wrapping a cause |
| `انسخ_فشل` | `error_copy` | `(&فشل): فشل` | independent deep copy, causes included; the same result as `استنسخ`, in a form Stage0 compiles |
| `رسالة_فشل` | `error_message` | `(&فشل): نص` | a copy of the message |
| `نطاق_فشل` | `error_domain` | `(&فشل): نص` | a copy of the domain |
| `نوع_فشل` | `error_kind` | `(&فشل): نص` | a copy of the kind |
| `له_سبب` | `error_has_cause` | `(&فشل): منطقي` | whether there is a cause |
| `سبب_فشل` | `error_cause` | `(&فشل): اختياري فشل` | a copy of the direct cause |
| `ملاحظات_فشل` | `error_notes` | `(&فشل): متجه نص` | a copy of the notes |
| `هل_من_نوع` | `error_is` | `(&فشل, النطاق: &نص, النوع: &نص): منطقي` | whether this error **or any cause in its chain** has the identity (Go `errors.Is`) |
| `أضف_ملاحظة` | `error_add_note` | `(&متغير فشل, الملاحظة: نص): فراغ` | appends a note; the identity and message do not change (PEP 678) |
| `صف_فشل` | `error_describe` | `(&فشل): نص` | developer rendering, below |
| `اطبع_فشل` | `error_print` | `(&فشل): فراغ` | writes `صف_فشل` and a newline to standard error |

Accessors return owned copies, because Stage0 cannot return references. `هل_من_نوع` never matches an empty kind: an
ad-hoc error has no identity to match.

```sultan
import std.error.failure;
import std.io.println;

def main(): int {
    let root: Error = error_with_kind("std.fs", "not_found", "file not found");
    let wrapped: mutable Error = error_with_kind_caused_by("app", "config", "could not load the configuration", root);
    error_add_note(&mutable wrapped, "path: /etc/app.toml");
    println(error_describe(&wrapped));
    let domain: string = "std.fs";
    let kind: string = "not_found";
    if (error_is(&wrapped, &domain, &kind)) { println("found in chain"); };
    return 0;
}
```

```sultan
استورد المكتبة.أخطاء.failure;
استورد std.io.println;

عرف الرئيسية(): عدد {
    دع الجذر: فشل = فشل_بهوية("std.fs", "غير_موجود", "الملف غير موجود");
    دع الملفوف: متغير فشل = فشل_بهوية_وسبب("app", "إعدادات", "تعذر تحميل الإعدادات", الجذر);
    أضف_ملاحظة(&متغير الملفوف, "المسار: /etc/app.toml");
    اطبع_سطر(صف_فشل(&الملفوف));
    دع النطاق: نص = "std.fs";
    دع النوع: نص = "غير_موجود";
    لو (هل_من_نوع(&الملفوف, &النطاق, &النوع)) { اطبع_سطر("موجود في السلسلة"); };
    أرجع 0;
}
```

### Developer rendering

`صف_فشل` gives the message, then each note on a line starting `ملاحظة: `, then for each cause in the chain a line
starting `بسبب: ` with the cause's message, followed by that cause's notes. Lines are separated by `\n`, with no
newline at the end. The two connector words are fixed text owned by std, in Arabic, the project's default language.
They are not iSpeak text, and `صف_فشل` never changes them, so an English author's messages appear between Arabic
connectors. `صف_فشل_بلغة` chooses the connectors by locale.

```text
أثناء بدء الخادم
ملاحظة: مسار الإعدادات من SULTANC_CONFIG
بسبب: تعذر قراءة الإعدادات
بسبب: ملف الإعدادات فارغ
ملاحظة: الحجم 0 بايت
```

### Converters from typed std errors

Each converter takes its typed error by reference, the shape of a converter function value (`دالة(&E): فشل`). The
domain is the module, the kind is the canonical Arabic name of the variant, and the message is a fixed Arabic
sentence. Details that vary are notes: the numeric status, the byte offset, the byte position.

| Converter | English | From | Domain | Kinds | Note |
|---|---|---|---|---|---|
| `فشل_من_فئة_خطأ_نظام` | `error_from_os_error_class` | `&فئة_خطأ_نظام` | `std.fs` | the 11 variants of `فئة_خطأ_نظام` | none |
| `فشل_من_خطأ_ملف` | `error_from_fs_error` | `&عدد`, a std.fs status | `std.fs` | as above | `رمز الحالة: N` |
| `فشل_من_خطأ_دخل_خرج` | `error_from_io_error` | `&عدد`, a std.io status | `std.io` | as above | `رمز الحالة: N` |
| `فشل_من_خطأ_عملية` | `error_from_process_error` | `&عدد`, a std.process status | `std.process` | as above | `رمز الحالة: N` |
| `فشل_من_خطأ_تحليل_عدد` | `error_from_integer_parse_error` | `&خطأ_تحليل_عدد` | `std.text.number` | `نص_فارغ`, `إشارة_بلا_أرقام`, `رقم_غير_صالح`, `تجاوز`, `سالب_لطبيعي` | `موضع البايت: N` for `رقم_غير_صالح` |
| `فشل_من_خطأ_جسون` | `error_from_json_error` | `&خطأ_جسون` | `std.json` | the 9 variants of `صنف_خطأ_جسون` | `إزاحة البايت: N` |
| `فشل_من_خطأ_تحليل_آيبي` | `error_from_ip_parse_error` | `&خطأ_تحليل_آيبي` | `std.net` | `صيغة_غير_صالحة`, `جزء_خارج_المدى` | none |
| `فشل_من_خطأ_تحليل_عنوان_مقبس` | `error_from_socket_address_parse_error` | `&خطأ_تحليل_عنوان_مقبس` | `std.net` | `صيغة_غير_صالحة`, `عنوان_غير_صالح`, `منفذ_غير_صالح` | none |
| `فشل_من_خطأ_تنسيق` | `error_from_fmt_error` | `&خطأ_تنسيق` | `std.fmt` | the 11 variants of `خطأ_تنسيق` | `موضع البايت: N`, `رقم الوسيط: N` or `رمز الحالة: N` |

std.fs, std.io and std.process report the inherited numeric runtime status (`عدد`, negative on failure), not an
enum. Their converters classify it with std.fs's `صنف_خطأ_نظام`, the target's file-error classifier, so the kind is
a `فئة_خطأ_نظام` variant (`-2` gives `غير_موجود` on Linux and macOS). Like that classifier, they are not meant for
socket statuses. The two std.net enums share the kind `صيغة_غير_صالحة`, which means invalid syntax in either.

std.fs's links, permissions, attributes, canonical paths and temporary files, and std.env.variables, return
`نتيجة T, فئة_خطأ_نظام`, converted with `فشل_من_فئة_خطأ_نظام`. std.process.command returns `خطأ_أمر`
(`فشل_من_خطأ_أمر`), and std.time.duration_text returns `خطأ_مدة` (`فشل_من_خطأ_مدة`). std.net.socket_names keeps
std.net's numeric statuses.

`examples/{ar,en}/errors/error_values.sn` shows a whole program.

### Context helpers

`std.error.context` provides plain functions for a result in passing, made to be used with `؟`. Each takes the
result by value and moves the success value through untouched, so an owning `T` (text, a vector) needs no plain
bound.

| Arabic | English | Signature | Meaning |
|---|---|---|---|
| `مع_سياق` | `with_context` | `<T>(نتيجة T, فشل, الرسالة: نص): نتيجة T, فشل` | on an error, a new ad-hoc `فشل` with this message whose cause is the error |
| `إلى_فشل` | `to_error` | `<T, E>(نتيجة T, E, حول: دالة(&E): فشل): نتيجة T, فشل` | a typed error converted by a converter function value, such as `فشل_من_خطأ_تحليل_عدد` |
| `مع_ملاحظة` | `with_note` | `<T>(نتيجة T, فشل, الملاحظة: نص): نتيجة T, فشل` | on an error, the same error with one more note |

```sultan
import std.error.failure;
import std.error.context;
import std.error.from_number;
import std.text.number;

def read_port(text: string): result int, Error {
    let port: int = to_error(parse_int(&text), error_from_integer_parse_error)?;
    return result.ok(port);
}

def main(): result int, Error {
    let port: int = with_context(read_port("80x"), "while starting the server")?;
    return result.ok(port);
}
```

```sultan
استورد المكتبة.أخطاء.failure;
استورد المكتبة.أخطاء.context;
استورد المكتبة.أخطاء.from_number;
استورد std.text.number;

عرف اقرأ_المنفذ(النص: نص): نتيجة عدد, فشل {
    دع المنفذ: عدد = إلى_فشل(حلل_عدد(&النص), فشل_من_خطأ_تحليل_عدد)؟;
    أرجع نتيجة.نجح(المنفذ);
}

عرف الرئيسية(): نتيجة عدد, فشل {
    دع المنفذ: عدد = مع_سياق(اقرأ_المنفذ("80x"), "أثناء بدء الخادم")؟;
    أرجع نتيجة.نجح(المنفذ);
}
```

### Constraints from Stage0

`std.error.failure`, `std.error.from_os`, `std.error.from_number` and `std.error.context` stay within what the C
Stage0 compiles: no copy primitive (hence `انسخ_فشل`), no reference returns, no references in records or mutable
locals, no calls in vector literals and no top-level constants. `tests/{ar,en}/stdlib/error/stage0_contract.sn`
checks this when the runner is given a Stage0 binary in `SULTANC_STAGE0_BIN`. `std.error.from_json` and
`std.error.from_net` cannot meet it, because std.json and `std.net.socket_address` already use forms Stage0
rejects. The compiler does not import `std.error`.

## Language versions

Language versions are optional. An author may add the message of a `فشل` in other languages, each under a BCP 47
tag, and code that knows the reader's language passes a locale value and gets the matching text. Nothing else
changes: the identity, the author's message, `صف_فشل`, `هل_من_نوع`, the context helpers and the frame `main` prints
all work as before. There is no global locale. A locale is a value passed to the function that needs it, so a server
answers each request in its own language.

```sultan
import std.error.failure;
import std.text.language;
import std.io.println;

def main(): int {
    let failure: mutable Error = error_with_kind("settings", "empty", "الملف فارغ");
    error_add_language_version(&mutable failure, "en", "the settings file is empty");

    let header: string = "fr-CH, fr;q=0.9, en;q=0.8";
    let request: Locale = locale_from_accept_language(&header);
    println(error_text_in(&failure, &request));
    return 0;
}
```

```sultan
استورد المكتبة.أخطاء.failure;
استورد المكتبة.نصوص.language;
استورد std.io.println;

عرف الرئيسية(): عدد {
    دع الفشل: متغير فشل = فشل_بهوية("settings", "فارغ", "ملف الإعدادات فارغ");
    أضف_نسخة_لغة(&متغير الفشل, "en", "the settings file is empty");

    دع الترويسة: نص = "fr-CH, fr;q=0.9, en;q=0.8";
    دع الطلب: لغة = لغة_من_قبول_اللغة(&الترويسة);
    اطبع_سطر(نص_فشل_بلغة(&الفشل, &الطلب));
    أرجع 0;
}
```

`examples/{ar,en}/errors/languages.sn` shows a whole program.

### Versions on a `فشل`

The versions are a private field of `فشل`: a list of (tag, text) pairs in the order added.

| Arabic | English | Signature | Meaning |
|---|---|---|---|
| `أضف_نسخة_لغة` | `error_add_language_version` | `(&متغير فشل, الوسم: نص, النص: نص): فراغ` | adds the message in a language. The tag is stored in canonical case (`en_gb` becomes `en-GB`), and a second version under the same tag replaces the first. The tag is not validated: an ill-formed tag is never found by a lookup, but can still be the first version |
| `نص_فشل_بلغة` | `error_text_in` | `(&فشل, &لغة): نص` | the message of this error for a locale (lookup below); causes are not included |
| `صف_فشل_بلغة` | `error_describe_in` | `(&فشل, &لغة): نص` | `صف_فشل` for a locale (below) |
| `اطبع_فشل_بلغة` | `error_print_in` | `(&فشل, &لغة): فراغ` | writes `صف_فشل_بلغة` and a newline to standard error |

`نص_فشل_بلغة` follows the RFC 4647 section 3.4 lookup:

1. For each tag of the locale, most preferred first, it takes the version under exactly that tag, then under each
   shorter form of it down to its primary language (`ar-SA` to `ar`; `zh-Hant-TW` to `zh-Hant` to `zh`). A singleton
   left at the end is dropped with its subtag (`en-GB-x-test` to `en-GB`).
2. Otherwise it returns the author's message.
3. When that is empty, it returns the first version added.

A tag matches only a version as specific or less specific, so a locale `en` does not find a version `en-GB`.
Comparison is case-insensitive, since both sides are in canonical case. For a locale of one tag of one or two
subtags, the order is exact tag, base language, author's message, first version.

`صف_فشل_بلغة` is `صف_فشل` with three differences:

- The message of every error in the chain is chosen by `نص_فشل_بلغة` with the same locale, so a chain reads in one
  language as far as its versions go.
- The connector words follow the locale: `ملاحظة: ` and `بسبب: ` when its primary language is `ar` or it has no tag,
  and `note: ` and `caused by: ` for any other language. They are fixed std text, not iSpeak and not a translation
  table.
- A message or note after a connector whose direction differs from the connectors' (a Latin message after `بسبب: `,
  an Arabic one after `caused by: `) is wrapped in U+2068 … U+2069 (`std.text.bidi`). The first line, which has no
  connector, is left as it is.

Notes have no versions: they are details (a path, a status) in the author's text. With an Arabic locale and no
Arabic version, `صف_فشل_بلغة` gives exactly `صف_فشل` for Arabic text.

### Locales: `std.text.language`

| Arabic | English | Signature | Meaning |
|---|---|---|---|
| `لغة` | `Locale` | type | a language priority list (RFC 4647 section 2.3): one tag, or the ranges of an Accept-Language header, most preferred first; possibly no tag. Opaque |
| `لغة_من_وسم` | `locale_from_tag` | `(&نص): لغة` | a locale of one tag, in canonical case, spaces around it dropped, `_` accepted; a text that is not a well-formed tag gives a locale without a tag |
| `لغة_من_قبول_اللغة` | `locale_from_accept_language` | `(&نص): لغة` | the ranges of an HTTP `Accept-Language` header (RFC 9110 section 12.5.4) ordered by `q`, highest first, equal weights in the order written. A range of weight 0, the wildcard `*`, a range that is not a well-formed tag and an element with a malformed weight (`q=1.5`, `q=.5`, more than three decimals) are left out; other parameters are ignored |
| `وسوم_لغة` | `locale_tags` | `(&لغة): متجه نص` | a copy of the tags, most preferred first |
| `وسم_لغة` | `locale_tag` | `(&لغة): نص` | the most preferred tag, or empty |
| `اللغة_الأساسية` | `locale_language` | `(&لغة): نص` | the primary language of the most preferred tag (`ar-SA` gives `ar`), or empty |
| `انسخ_لغة` | `locale_copy` | `(&لغة): لغة` | a copy; the same as `استنسخ`, in a form Stage0 compiles |

`fr-CH, fr;q=0.9, en;q=0.8, *;q=0.5` gives `fr-CH`, `fr`, `en`.

BCP 47 tags (RFC 5646) are implemented once in std for any user:

| Arabic | English | Signature | Meaning |
|---|---|---|---|
| `وسم_لغة_صالح` | `is_language_tag` | `(&نص): منطقي` | whether the text is a well-formed tag: language (2 to 8 letters, with up to three extended subtags after 2 to 3 letters), script (4 letters), region (2 letters or 3 digits), variants, extensions, private use; or a private-use tag `x-…`. `_` is accepted as a separator. Registration is not checked, and the irregular grandfathered tags (`i-klingon`) are not accepted |
| `طبع_وسم_لغة` | `normalize_language_tag` | `(&نص): نص` | canonical case with `-` (RFC 5646 section 2.1.1): lower case, a region in upper case, a script in title case, all lower case after a singleton (`sr_latn_rs` gives `sr-Latn-RS`) |
| `اللغة_الأساسية_لوسم` | `language_tag_base` | `(&نص): نص` | the primary language subtag in lower case |
| `اختصر_وسم_لغة` | `truncate_language_tag` | `(&نص): نص` | one RFC 4647 lookup fallback step (`zh-Hant-CN` gives `zh-Hant`; `ar` gives empty) |

### The environment's locale: `std.text.system_language`

| Arabic | English | Signature | Meaning |
|---|---|---|---|
| `لغة_من_البيئة` | `locale_from_environment` | `(): لغة` | the program's locale: `SULTANC_LANG`, then `LC_ALL`, `LC_MESSAGES`, `LANG`, then the language the program was compiled with (`--lang`, written by the compiler into `std.process.failure_text`), then Arabic. Always one tag |

A value is read as described under [Runtime failures of a program](#runtime-failures-of-a-program): a tag or a
POSIX locale, with `C`, `POSIX`, empty and ill-formed values skipped. The runtime-failure path uses this same
function for the language of its frames, so std has one reading of these variables. It reads the environment on
every call; the result is a value, not a cache.

### Direction isolation: `std.text.bidi`

| Arabic | English | Signature | Meaning |
|---|---|---|---|
| `اتجاه_نص` | `text_direction` | type: `محايد` / `neutral`, `من_اليسار_إلى_اليمين` / `left_to_right`, `من_اليمين_إلى_اليسار` / `right_to_left` | a direction |
| `اتجاه_النص` | `text_direction_of` | `(&نص): اتجاه_نص` | the direction of the first strong character, skipping what lies inside isolates (UAX #9 P2 to P3): the direction a first-strong isolate gives the text |
| `اعزل_اتجاه` | `bidi_isolate` | `(&نص): نص` | U+2068 (first-strong isolate), the text, U+2069 (pop directional isolate) |
| `اعزل_اتجاه_في` | `bidi_isolate_in` | `(&نص, السياق: اتجاه_نص): نص` | the text isolated when its direction is strong and differs from the context's; otherwise an unchanged copy |

A character's direction comes from script ranges, not from the Unicode Bidi_Class table. Hebrew, Arabic, Syriac,
Thaana, NKo and the other right-to-left blocks are right-to-left, apart from their digits and combining marks.
Latin, Greek, Cyrillic, Armenian, the Indic and the East Asian scripts are left-to-right. Everything else, digits and
punctuation included, is neutral. Patterns use the Arabic variant names.

The compiler's iSpeak keeps its own isolation of quoted facts in Arabic terminal diagnostics
(`compiler/diagnostics/output/terminal.sn`): it isolates backtick-quoted spans while copying. std does not depend on
iSpeak and holds none of its tables.

### English versions of std's own errors

Every converter in `std.error.from_os`, `from_number`, `from_json` and `from_net` adds an English version (`en`) of
its Arabic message. The message, identity and notes are unchanged, and notes stay as written (`رمز الحالة: -2`).

`std.text.language`, `std.text.bidi`, `std.text.system_language` and the language functions of
`std.error.failure` are within the C Stage0's language, and `tests/{ar,en}/stdlib/error/stage0_contract.sn`
calls each of them.

## Stopping a program

### `إنهيار` / `panic`

```sultan
import std.process.panic;

def divide(a: int, b: int): int {
    if (b != 0) { return a / b; };
    panic("division by zero");
}
```

```sultan
استورد std.process.panic;

عرف اقسم(أ: عدد, ب: عدد): عدد {
    لو (ب != 0) { أرجع أ / ب; };
    إنهيار("القسمة على صفر");
}
```

`إنهيار(الرسالة: نص)` (`std.process.panic`) prints the frame "the program stopped" and the message, then ends the
process with status 1 through the std process exit, as `اخرج` / `exit` does. It is a std function, not a compiler
primitive. `انهيار` without a hamza names the same identifier.

- **Never returning.** The frontend identifies `إنهيار` of the trusted std module `process/panic.sn` by its RootID
  and module key, the way it authenticates the Linux kernel contract, and never by a spelling a user module could
  copy. It hands the symbol to MSIR lowering. A call statement to it ends its block in a trap, as a compile-time
  failure does, so a function returning a value needs no return after the call. The C Stage0 does the same
  (`bootstrap/src/semantic/never_return.c`): it identifies `إنهيار` by the canonical path of
  `library/std/process/panic.sn` under the project root, ends the path in its flow check and emits `unreachable`
  after the call.
- **Abort semantics.** The process ends at once. Owned values that are still alive are not dropped, as with Rust's
  `panic=abort`, and the operating system reclaims the memory.

### Assertions

`تأكد` / `assert` and `تأكد_برسالة` / `assert_with_message` (`std.debug.assert`) stop the program through the same
path: the frame "the program stopped" with the reason "an assertion failed", the optional message under it, and
status 1. [Compile-time evaluation](comptime.md#stddebugassert) specifies them.

## Runtime failures of a program

A program prints why it ends through one path, `std.process.failure` (`library/std/process/failure.sn`), for native
executables (x86_64, arm64), wasm32, the LLVM backend and the interpreter alike. The output is the frame of a
runtime-failure identity, then the payload on the lines under it, on standard error.

| Identity | Frame (Arabic / English) | When |
|---|---|---|
| `SULTANC-RUNTIME-PROGRAM-ERROR` | `خطأ[…]: انتهى البرنامج بخطأ` / `error[…]: the program ended with an error` | `main` returned an error; payload: its `صف_فشل` text |
| `SULTANC-RUNTIME-PROGRAM-STOPPED` | `خطأ[…]: توقف البرنامج` / `error[…]: the program stopped` | `إنهيار` / `panic`; payload: its message |
| `SULTANC-RUNTIME-PROGRAM-STOPPED` with the reason `runtime.assertion-failed` | `… — فشل تأكيد.` / `… — An assertion failed.` | a failed `تأكد` / `assert`; payload: the optional message |

The frames are iSpeak's. The two identities and the reason are registered like every diagnostic
(`compiler/diagnostics/identity.sn`, `reason.sn`, `registry.sn`, both locales), and a frame is the headline a
diagnostic of that identity has on a terminal. A program cannot run iSpeak, so the compiler renders the frames in
both languages (`compiler/driver/program_failure_text.sn`) and writes them, at every compilation, as the text of the
std module `std.process.failure_text` (a source overlay on `library/std/process/failure_text.sn`), together with the
language the program is compiled with. The file's own text is the Arabic rendering, which the C Stage0 and tools
read; `tests/{ar,en}/ispeak/check_program_failure_text.sn` checks it against the compiler's rendering. std holds no
table of its own.

The program's language is that of `لغة_من_البيئة()` (`std.text.system_language`), the one place std reads these
variables. It is the first of:

1. `SULTANC_LANG`;
2. `LC_ALL`, `LC_MESSAGES`, `LANG`;
3. the `--lang` the program was compiled with (with no `--lang`, Arabic; with `--lang=auto`, the compiler's host
   locale);
4. Arabic.

A value is a BCP 47 tag (`en-US`) or a POSIX locale (`ar_SA.UTF-8`, `de_DE@euro`, read as `ar-SA`, `de-DE`). A
locale whose primary language is `ar` gives Arabic frames, and any other language gives English. An empty value, `C`,
`C.…`, `POSIX` and a value that is not a language tag carry no language, and the next source is read, so a
`LANG=C.UTF-8` CI machine falls through to the compiled language.

### `main` returning an error

```sultan
import std.error.failure;
import std.error.context;
import std.error.from_number;
import std.text.number;

def read_port(text: string): result int, Error {
    let port: int = to_error(parse_int(&text), error_from_integer_parse_error)?;
    return result.ok(port);
}

def main(): result int, Error {
    let port: int = with_context(read_port("80x"), "while starting the server")?;
    return result.ok(port);
}
```

On `نتيجة.نجح(n)` the program exits with status `n`. On `نتيجة.خطأ(f)` it prints the frame "the program ended with an
error" and `صف_فشل(&f)`, then exits with status 1. With `SULTANC_LANG=en` the output of the program above is:

```text
error[SULTANC-RUNTIME-PROGRAM-ERROR]: the program ended with an error
while starting the server
بسبب: في النص محرف ليس رقما عشريا
ملاحظة: موضع البايت: 2
```

`compiler/frontend/desugar/entry_result.sn` does this in the frontend. In the root module, an entry function
(`الرئيسية` / `main`) without parameters whose result is written `نتيجة T, E` keeps its body under a hidden name
(`نقطة_دخول__…`), and the entry becomes an ordinary function returning `T`. It matches the hidden function's result,
returns the value, or calls `أبلغ_عن_خطأ_نقطة_الدخول` and returns 1. The generated text imports
`std.process.failure`. Every backend's entry adapter (`backend/target/executable_entry_query_context.sn`, the LLVM
`entry.sn`) and the interpreter thus see an entry with an exit status, and the entry check
(`SULTANC-ENTRYPOINT-INVALID-SIGNATURE`) applies to `T`: a `T` that is not an exit status, or an `E` that is not
`فشل`, is reported at the written result type. The error is moved into the report and dropped on the normal return
path, so the program ends cleanly.

### The interpreter

`sultanc run` executes the same std code, so it prints the same frames. The interpreter gives a program the
language variables of its environment (`SULTANC_LANG`, `LC_ALL`, `LC_MESSAGES`, `LANG`) as its startup environment;
other variables are absent. A raw Linux `exit_group` ends the interpretation with the status, so `exit`, `panic` and
assertions end an interpreted program with the native status.

## Tests

The tests come in Arabic and English pairs, and the English one spells `?`.

- `tests/{ar,en}/language/propagation/` covers the four positions on results and options (native, interpreter and
  Stage0 agree), drop correctness over 10^5 early returns, and the rejections in a disallowed position, in a function
  returning no result, and on an option in a result function.
- `tests/{ar,en}/diagnostics/e2e/propagation_error_type.*` checks a different error type, reported at the `؟` with
  both types, in exact Arabic and English output.
- `tests/{ar,en}/stdlib/error/` covers constructors and accessors, a three-level chain searched at each level, notes
  order, the exact `صف_فشل` text, `استنسخ` and `انسخ_فشل` round trips, every variant of every converted enum,
  the context helpers, the language lookup, Accept-Language parsing, the chain in several languages, bidirectional
  isolation, the Stage0 contract, and flat memory (`rss_flat.sn` and `language_rss_flat.sn` run 10^5 chains and check
  that the allocator's live bytes are equal before and after; the interpreter does not use the std allocator, so
  these run native only).
- `tests/{ar,en}/runtime/failure/` covers `main` returning an error (`SULTANC_LANG=ar` and `en`) and success, and
  `panic` and failed assertions in both program languages, with the exact standard error, exit status, and native
  and interpreter agreement. The runner's `// بيئة:` header sets the program's environment and
  `// خطأ-قياسي-ملف:` names the expected standard error.
- `tests/{ar,en}/ispeak/check_program_failure_text.sn` checks the words module against the compiler's rendering.

## Not yet supported

- `compile_error` with a `فشل` is not available; `compile_error` takes text.
- Catalog files, translation tools and `.ar()`-style sugar are out of scope.
- The payload of the frame `main` prints is `صف_فشل`, the author's messages. Printing it with `صف_فشل_بلغة` and the
  program's locale is not done.
- An alias of an option type is not recognized by `؟`.

## See also

- [Type system](type-system.md)
- [Ownership and destruction](ownership.md)
- [Compile-time evaluation](comptime.md)
- [Formatting](formatting.md)
- [Arabic terminology](arabic-terminology.md)
- [Interpreter](../internals/interpreter.md)
- [Language tour](../guide/language-tour.md)
