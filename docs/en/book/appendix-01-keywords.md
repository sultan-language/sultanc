[العربية](../../ar/book/appendix-01-keywords.md) | **English**

# A. Keywords in Arabic and English

The keywords are reserved: they cannot name a variable, a function or a type. Each has an English and an
Arabic spelling, and both mean exactly the same; a program may use either, and may mix them. This list
is the compiler's own (`tools/generate/registry/token_registry.def`).

| English | Arabic | Meaning |
| --- | --- | --- |
| `comptime` | `وقت_الترجمة` | evaluates an expression or block while compiling |
| `fn` | `دالة` | a function type, or an anonymous function |
| `of` | `بنوع` | the value an enum case carries |
| `from` | `من` | selective import: `from folder import names` |
| `extern` | `خارجي` | a function or value defined outside SultanC |
| `import` | `استورد` | imports a module |
| `module` | `وحدة` | declares a module's spellings in a root file |
| `use` | `استخدم` | reserved |
| `as` | `مثل` | converts a number type; names an import |
| `undefined` | `غير_معرف` | reserved |
| `def` | `عرف` | defines a function |
| `alias` | `مرادف` | declares another name for a declaration |
| `let` | `ليكن` | declares a variable or a constant |
| `return` | `أرجع` | ends a function, giving back a value |
| `else` | `وإلا` | the branch of `if` taken otherwise |
| `if` | `إذا` | runs a block when a condition is true |
| `any` | `أي` | built-in type: any type |
| `bool` | `منطقي` | built-in type: true or false |
| `string` | `نص` | built-in type: UTF-8 text |
| `char` | `حرف` | built-in type: one Unicode character |
| `int` | `عدد` | built-in type: 64-bit signed integer |
| `uint` | `طبيعي` | built-in type: 64-bit unsigned integer |
| `float` | `عدد_عائم` | built-in type: floating-point number |
| `void` | `فراغ` | built-in type: no value |
| `asm` | `تجميع` | inline assembly |
| `unsafe` | `غير_آمن` | a block where raw memory operations are allowed |
| `while` | `طالما` | repeats a block while a condition is true |
| `for` | `لكل` | a counting loop |
| `break` | `اخرج_من_حلقة` | leaves a loop |
| `continue` | `تابع_حلقة` | starts the next round of a loop |
| `type` | `نوع` | declares a type: record, enum, alias or opaque |
| `match` | `طابق` | chooses a branch by pattern |
| `with` | `مع` | follows the value in `match value with` |
| `mutable` | `متغير` | allows a variable, parameter or reference to change |
| `public` | `عام` | makes a declaration visible outside its module |
| `slice` | `شريحة` | a borrowed view of part of a sequence |
| `box` | `صندوق` | a value stored on the heap |
| `true` | `صواب` | the boolean true |
| `false` | `خطأ` | the boolean false |

## Number types

| English | Arabic | Meaning |
| --- | --- | --- |
| `i8` | `ص8` | signed integer, 8 bits |
| `i16` | `ص16` | signed integer, 16 bits |
| `i32` | `ص32` | signed integer, 32 bits |
| `i64` | `ص64` | signed integer, 64 bits |
| `u8` | `ط8` | unsigned integer, 8 bits |
| `u16` | `ط16` | unsigned integer, 16 bits |
| `u32` | `ط32` | unsigned integer, 32 bits |
| `u64` | `ط64` | unsigned integer, 64 bits |
| `f32` | `ع32` | floating-point number, 32 bits |
| `f64` | `ع64` | floating-point number, 64 bits |

## Words with a meaning in context

These are not reserved, but have a meaning where they appear:

| English | Arabic | Meaning |
| --- | --- | --- |
| `option` | `اختياري` | a value or nothing |
| `result` | `نتيجة` | a value or an error |
| `vector` | `متجه` | a growable sequence |
| `range … to …` | `مدى … إلى …` | a range pattern in `match` |
| `main` | `الرئيسية` | the function where a program starts |
| `@@test` | `@@اختبار` | marks a test function |
| `opaque` | `معتم` | a type known only by name, used through pointers |

The standard library's names have both spellings too: `println` and `اطبع_سطر` are one function, and
`std.io` and `المكتبة.دخ` one module. [Arabic terminology](../reference/arabic-terminology.md) in the
reference lists the canonical vocabulary.
