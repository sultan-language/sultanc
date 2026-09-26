# Arabic Terminology

SultanC treats Arabic as a first-class source and compiler language. This reference lists the public Arabic vocabulary used across source code and compiler documentation.

The lexical registry remains authoritative for exact token spellings.

## Language keywords

| English | Arabic | Meaning |
| --- | --- | --- |
| `def` | `عرف` | Function definition |
| `let` | `دع` | Value declaration |
| `return` | `أرجع` | Return from a function |
| `if` | `لو` | Conditional branch |
| `else` | `وإلا` | Alternate branch |
| `while` | `بينما` | While loop |
| `for` | `لكل` | For loop |
| `break` | `اخرج_من_حلقة` | Exit the current loop |
| `continue` | `تابع_حلقة` | Continue the current loop |
| `type` | `نوع` | Type declaration |
| `match` | `طابق` | Pattern match |
| `with` | `مع` | Match-arm introducer |
| `mutable` | `متغير` | Mutable wrapper |
| `public` | `عام` | Public declaration |
| `extern` | `خارجي` | External function declaration |
| `import` | `استورد` | Module import |
| `as` | `مثل` | Conversion/cast syntax |
| `unsafe` | `غير_آمن` | Unsafe context |

## Builtin types

| English | Arabic |
| --- | --- |
| `any` | `أي` |
| `bool` | `منطقي` |
| `string` | `نص` |
| `char` | `حرف` |
| `int` | `عدد` |
| `uint` | `غير_موقع` |
| `void` | `فراغ` |
| `i8`, `i16`, `i32`, `i64` | `ص8`, `ص16`, `ص32`, `ص64` |
| `u8`, `u16`, `u32`, `u64` | `غ8`, `غ16`, `غ32`, `غ64` |
| `f32`, `f64` | `ع32`, `ع64` |

## Compiler terms

| English | Arabic |
| --- | --- |
| declaration | `تصريح` |
| value | `قيمة` |
| semantic symbol | `رمز_دلالي` |
| module ID | `معرف_وحدة` |
| logical module path | `مسار_وحدة_منطقي` |
| filesystem path | `مسار_نظام_الملفات` |
| reference | `مرجع` |
| ownership | `ملكية` |
| borrow | `استعارة` |
| move | `نقل` |
| lowering | `خفض` |
| basic block | `كتلة_أساسية` |
| runtime service | `خدمة_تشغيل` |
| compiler backend | `خلفية_المترجم` |
| compilation target | `هدف_الترجمة` |
| ABI | `واجهة_ثنائية` |
| register allocation | `تخصيص_السجلات` |
| virtual register | `سجل_افتراضي` |
| physical register | `سجل_فعلي` |
| compiler driver | `منسق_المترجم` |
| module loading | `تحميل_وحدة` |

## Naming rule

Compiler terminology should name the exact concept rather than translate an ambiguous English word mechanically. In particular, terms such as *target*, *object*, *load*, and *store* should include enough context to distinguish compilation targets, object files, module loading, and memory operations.

Exact keyword/type spellings are generated from the registries under `tools/generate/registry/`; documentation should not introduce alternative spellings independently.
