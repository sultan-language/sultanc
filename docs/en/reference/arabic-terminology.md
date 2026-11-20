[العربية](../../ar/reference/arabic-terminology.md) | **English**

# Arabic Terminology

SultanC treats Arabic as a first-class source and compiler language. This page is the canonical
vocabulary: the Arabic spellings of keywords, types, compiler concepts and the public names of std,
with their English counterparts. It is for programmers who read or write Arabic SultanC and for
contributors who name new APIs.

The lexical registries under `tools/generate/registry/` are authoritative for exact keyword and
type spellings; this page does not introduce alternative spellings. Identifier identity folds the
alef forms آ, أ and إ to ا and ignores tatweel
(`tools/generate/registry/identifier_policy_registry.def`).

## Language

### Keywords

| English | Arabic | Meaning |
| --- | --- | --- |
| `def` | `عرف` | Function definition |
| `let` | `دع` | Value declaration |
| `return` | `أرجع` | Return from a function |
| `if` | `إذا` | Conditional branch |
| `else` | `وإلا` | Alternate branch |
| `while` | `طالما` | While loop ("as long as") |
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

The program entry function is `main` in English and `الرئيسية` ("the main function") in Arabic: `عرف الرئيسية(): عدد { … }`.

### Builtin types

| English | Arabic |
| --- | --- |
| `any` | `أي` |
| `bool` | `منطقي` |
| `string` | `نص` |
| `char` | `حرف` |
| `int` | `عدد` |
| `uint` | `طبيعي` (a natural number: an integer ≥ 0, zero included) |
| `void` | `فراغ` |
| `i8`, `i16`, `i32`, `i64` | `ص8`, `ص16`, `ص32`, `ص64` |
| `u8`, `u16`, `u32`, `u64` | `ط8`, `ط16`, `ط32`, `ط64` |
| `f32`, `f64` | `ع32`, `ع64` |

`طبيعي` also reads as "normal" in floating-point text, so the decimal float conversion calls the
smallest normal value `أصغر_عادي_عائم` (`compiler/msir/decimal_float_bits.sn`); `أصغر_طبيعي` is
only the std minimum of two natural numbers.

### Builtin container types

These names are contextual: they name the type only in a type or constructor position.

| English | Arabic | Constructors |
| --- | --- | --- |
| `option` | `اختياري` | `some`/`Some` = `موجود`, `none`/`None` = `لا_شيء` |
| `result` | `نتيجة` | `ok`/`Ok` = `نجح`, `err`/`Err` = `خطأ` |
| `vector` | `متجه` | |

### Propagation and runtime failures

`إنهيار` is the canonical spelling of `panic`, with hamza under the alef; `انهيار` written without
hamza names the same identifier. It is a std function on top of the std process exit, as `اخرج` /
`exit` is, not a compiler primitive or a lexical token; the compiler knows it never returns (see
[Errors](errors.md)). The hidden names of the compiler's expansions (`انتشار__N…`, `نقطة_دخول__…`)
are ordinary identifiers with a reserved prefix that no source declares.

| English | Arabic | Meaning |
| --- | --- | --- |
| `?` | `؟` (U+061F) | Propagation operator: the last postfix of a statement's right-hand side (token `استفهام`, `QUESTION_OPERATOR`) |
| `panic` | `إنهيار` | Stop primitive: print a message and end the program (`std.process.panic`) |
| `SULTANC-RUNTIME-PROGRAM-ERROR` | `انتهى_البرنامج_بخطأ` | Runtime failure identity: `main` returned an error ("the program ended with an error" / «انتهى البرنامج بخطأ») |
| `SULTANC-RUNTIME-PROGRAM-STOPPED` | `توقف_البرنامج` | Runtime failure identity: the program stopped ("the program stopped" / «توقف البرنامج») |
| `runtime.assertion-failed` | `سبب_تشغيل_فشل_تأكيد` | Reason of a stop by a failed assertion ("An assertion failed" / «فشل تأكيد») |
| — | `اطبع_فشل_تشغيل` | The one runtime-failure printing path (`std.process.failure`) |
| — | `البرنامج_بالإنجليزية` | Whether the program speaks English: the primary language of `لغة_من_البيئة()` is not `ar` |
| — | `أبلغ_عن_خطأ_نقطة_الدخول` | Report the error `main` returned |
| — | `إطار_فشل_تشغيل` | iSpeak frame of a runtime failure (`std.process.failure_text`, written by the compiler) |

### Renamed spellings

Some Arabic spellings were renamed because the old word did not mean the concept. Except for the
entry function, the old spelling remains accepted as a deprecated alias. It compiles exactly as
the new spelling, and the self-hosted compiler reports one warning per use,
`SULTANC-SYNTAX-DEPRECATED-SPELLING`, naming the old and the new spelling (Stage0 accepts the old
spelling silently). English spellings never change.

| English | Old Arabic (deprecated) | New Arabic | Why |
| --- | --- | --- | --- |
| `while` | `بينما` | `طالما` | `طالما` means "as long as", which is what the loop tests. `بينما` is a time adverb ("during", "whereas"), a calque of English *while*. |
| entry function `main` | the project nickname "the castle" (removed, no alias) | `الرئيسية` | `الرئيسية` means "the main (function)", the standard term. The old name, "the castle", was a project nickname; it is no longer recognised at all, so a program whose only entry function still has it reports `SULTANC-ENTRYPOINT-NOT-FOUND`. |
| `option` | `اختيار` | `اختياري` | `اختياري` means "optional", the standard word for an optional field or value. `اختيار` is the noun "choice" (and, in the compiler, instruction *selection*). |
| `some` / `Some` | `بعض` (`اختيار.بعض`) | `موجود` (`اختياري.موجود`) | `موجود` says "a value is present". Arabic `بعض` is partitive ("some of", "part of"), while ML/Rust *Some* means "there is a value x"; Haskell and Elm say `Just` for the same reason. |
| `none` / `None` | `لا_شيء` | `لا_شيء` (unchanged) | Only the type prefix changes: `اختياري.لا_شيء`. |
| `uint` | `غير_موقع` | `طبيعي` | `طبيعي` is a natural number. **It includes 0** («طبيعي: عدد صحيح ≥ 0، يشمل الصفر»), although many Arabic sources start the natural numbers at 1. `غير_موقع` was a calque: `موقَّع` means "signed" as in a signed document. |
| `u8`, `u16`, `u32`, `u64` | `غ8`, `غ16`, `غ32`, `غ64` | `ط8`, `ط16`, `ط32`, `ط64` | The fixed-width natural types take the initial of `طبيعي`, as `ص8` takes that of `صحيح`. |

The deprecated keyword spellings are rows of `tools/generate/registry/token_registry.def` (`SULTANC__KEYWORD_DEPRECATED__`), the deprecated contextual type spellings rows of `tools/generate/registry/type_registry.def` (`SULTANC__TYPE_CONTEXTUAL_DEPRECATED__`); the Option constructor spellings are matched in `compiler/frontend/types/type_semantic_shape_queries.sn` and Stage0's `bootstrap/src/kernel/type/type_tagged.c`. A keyword warns at the token; the contextual type name and constructor warn where the name pass meets them in a type or constructor position, so a local named `اختيار` does not warn. Rewrite sources with the lexer-based tool, which changes only the renamed tokens (never strings, comments or longer identifiers) and is idempotent:

```sh
python3 tools/rename/rename_tokens.py --wave 1 path/to/sources   # بينما -> طالما
python3 tools/rename/rename_tokens.py --wave 7 path/to/sources   # the entry function -> الرئيسية
python3 tools/rename/rename_tokens.py --wave 2 path/to/sources   # اختيار -> اختياري, اختيار.بعض -> اختياري.موجود
python3 tools/rename/rename_tokens.py --wave 3 path/to/sources   # غير_موقع -> طبيعي, غN -> طN
python3 tools/rename/rename_tokens.py --wave 3 --segments path/to/sources   # أصغر_غير_موقع -> أصغر_طبيعي
python3 tools/rename/rename_tokens.py --wave 1 --check path/to/sources   # counts what is left
```

Names built on the unsigned types follow them (`--segments`): the segment `غير_موقع` becomes `طبيعي` (`أصغر_طبيعي`, `طبيعي_إلى_نص`, `عدد_صحيح_طبيعي`), the feminine adjective `غير_موقعة` becomes `طبيعية` (`قيمة_حجة_رسالة.طبيعية`, `قيمة_طبيعية_عشرية_ويب`), `لغير_موقع` becomes `لطبيعي` (`سالب_لطبيعي`), and a width segment `غ8`…`غ64` becomes `ط8`…`ط64` (`ألحق_ط32_كوف`, `اقرأ_ط16_صغير`). Every other use of the word `غير` is unchanged. A public std name keeps its old name as an alias (`عام مرادف أصغر_غير_موقع = أصغر_طبيعي;`, `عام مرادف خطأ_تحليل_عدد.سالب_لغير_موقع = خطأ_تحليل_عدد.سالب_لطبيعي;`); the pass leaves that alias table alone.

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

Compiler terminology names the exact concept rather than translating an ambiguous English word
mechanically. Terms such as *target*, *object*, *load* and *store* carry enough context to
distinguish compilation targets, object files, module loading and memory operations.

## Verbs used in std names

Names follow a verb dictionary: `أنشئ` creates, `اقرأ` reads, `اضبط` sets, `أزل` removes an item
(a prefix, a variable), `امسح` clears, `احذف` deletes files, `هل_…` asks a question, `حلل` parses,
`حول_…_إلى_…` converts, `ابدأ` starts, `خذ` takes a value out of its owner (Rust's `Option::take`),
`تفقد` polls without waiting, `اقفل` and `افتح_قفل` lock and unlock, `حاول_…` is the non-waiting
attempt, `نفذ` runs something, `انتظر` / `أيقظ` / `نبه` wait, wake and notify, `أرسل` / `استقبل`
send and receive, `أسقط` drops an end, `استنسخ` clones and `نسق` formats.

## Errors and locales (std.error)

`std.error` ([Errors](errors.md)) names are Arabic, with English aliases (`مرادف`). The boolean
`خطأ` / `false` and the result variants `نتيجة.نجح` / `نتيجة.خطأ` keep their names.

### Error values

| English | Arabic | Meaning |
| --- | --- | --- |
| `Error` | `فشل` | The common error value (`std.error.failure`) |
| `error` (package) | `أخطاء` | `std.error` / `المكتبة.أخطاء` |
| domain | `النطاق` | Owner part of an error's identity |
| kind | `النوع` | Machine part of an error's identity |
| cause | `السبب` | The error an error wraps |
| note | `ملاحظة` | Extra line added to an error |
| `error_new` | `فشل_من` | Ad-hoc error |
| `error_with_kind` | `فشل_بهوية` | Error with an identity |
| `error_caused_by` | `فشل_بسبب` | Ad-hoc error wrapping a cause |
| `error_with_kind_caused_by` | `فشل_بهوية_وسبب` | Error with an identity wrapping a cause |
| `error_copy` | `انسخ_فشل` | Deep copy |
| `error_message` | `رسالة_فشل` | The message |
| `error_domain` | `نطاق_فشل` | The domain |
| `error_kind` | `نوع_فشل` | The kind |
| `error_has_cause` | `له_سبب` | Whether there is a cause |
| `error_cause` | `سبب_فشل` | The direct cause |
| `error_notes` | `ملاحظات_فشل` | The notes |
| `error_is` | `هل_من_نوع` | Identity search through the cause chain |
| `error_add_note` | `أضف_ملاحظة` | Append a note |
| `error_describe` | `صف_فشل` | Developer rendering |
| `error_print` | `اطبع_فشل` | Developer rendering on standard error |
| `error_from_os_error_class` | `فشل_من_فئة_خطأ_نظام` | From std.fs's error class |
| `error_from_fs_error` | `فشل_من_خطأ_ملف` | From a std.fs status |
| `error_from_io_error` | `فشل_من_خطأ_دخل_خرج` | From a std.io status |
| `error_from_process_error` | `فشل_من_خطأ_عملية` | From a std.process status |
| `error_from_integer_parse_error` | `فشل_من_خطأ_تحليل_عدد` | From a std.text.number parse error |
| `error_from_json_error` | `فشل_من_خطأ_جسون` | From a std.json error |
| `error_from_ip_parse_error` | `فشل_من_خطأ_تحليل_آيبي` | From a std.net IP parse error |
| `error_from_socket_address_parse_error` | `فشل_من_خطأ_تحليل_عنوان_مقبس` | From a std.net socket-address parse error |
| `with_context` | `مع_سياق` | On an error, wrap it as the cause of a new one (`std.error.context`) |
| `to_error` | `إلى_فشل` | Convert a typed error with a converter function value |
| `with_note` | `مع_ملاحظة` | On an error, add a note |

`ملاحظة: ` and `بسبب: ` are the fixed connector words of `صف_فشل`, which renders in Arabic only.

### Languages and locales

| English | Arabic | Meaning |
| --- | --- | --- |
| language version | `نسخة لغة` | The message of an error in another language, under a BCP 47 tag |
| language tag | `وسم لغة` | A BCP 47 tag such as `ar-SA` |
| locale | `لغة` | A language priority list passed as a value (`std.text.language`) |
| `Locale` | `لغة` | The locale type |
| `error_add_language_version` | `أضف_نسخة_لغة` | Add the message in a language |
| `error_text_in` | `نص_فشل_بلغة` | An error's message for a locale (RFC 4647 lookup) |
| `error_describe_in` | `صف_فشل_بلغة` | Developer rendering for a locale |
| `error_print_in` | `اطبع_فشل_بلغة` | Developer rendering for a locale on standard error |
| `note: ` / `caused by: ` | `ملاحظة: ` / `بسبب: ` | Connector words of `صف_فشل_بلغة`: Arabic for an Arabic locale or none, English otherwise |
| `locale_from_tag` | `لغة_من_وسم` | A locale of one tag |
| `locale_from_accept_language` | `لغة_من_قبول_اللغة` | A locale from an HTTP Accept-Language header |
| `locale_from_environment` | `لغة_من_البيئة` | The program's locale (`std.text.system_language`) |
| `locale_tags` | `وسوم_لغة` | A locale's tags |
| `locale_tag` | `وسم_لغة` | A locale's most preferred tag |
| `locale_language` | `اللغة_الأساسية` | The primary language of that tag |
| `locale_copy` | `انسخ_لغة` | Copy a locale |
| `is_language_tag` | `وسم_لغة_صالح` | Whether a text is a well-formed BCP 47 tag |
| `normalize_language_tag` | `طبع_وسم_لغة` | A tag in canonical case |
| `language_tag_base` | `اللغة_الأساسية_لوسم` | A tag's primary language subtag |
| `truncate_language_tag` | `اختصر_وسم_لغة` | One RFC 4647 lookup fallback step |

### Text direction (std.text.bidi)

| English | Arabic | Meaning |
| --- | --- | --- |
| `text_direction` | `اتجاه_نص` | A text direction (`std.text.bidi`) |
| `text_direction.neutral` | `اتجاه_نص.محايد` | Neutral |
| `text_direction.left_to_right` | `اتجاه_نص.من_اليسار_إلى_اليمين` | Left to right |
| `text_direction.right_to_left` | `اتجاه_نص.من_اليمين_إلى_اليسار` | Right to left |
| `text_direction_of` | `اتجاه_النص` | Direction of a text's first strong character |
| `bidi_isolate` | `اعزل_اتجاه` | Wrap a text in U+2068 … U+2069 |
| `bidi_isolate_in` | `اعزل_اتجاه_في` | Isolate a text when its direction differs from the context's |

## std vocabulary by area

The tables below list the public names of std with their English aliases. A blank module cell
continues the module above it.

### Files and paths (std.fs)

| Module | English | Arabic | Meaning |
| --- | --- | --- | --- |
| std.fs.links | `symlink` | `أنشئ_وصلة_رمزية` | create a symbolic link |
| | `read_link` | `اقرأ_وصلة_رمزية` | read a link's target |
| | `hard_link` | `أنشئ_وصلة_صلبة` | create a hard link |
| std.fs.permissions | `permissions` | `اقرأ_أذونات` | permission bits of a path |
| | `set_permissions` | `اضبط_أذونات` | chmod |
| | `is_readonly` | `هل_للقراءة_فقط` | whether a mode denies writing |
| std.fs.attributes | `truncate_file` | `اقتطع_الملف` | set a file's length |
| | `file_handle_set_len` | `اقتطع_مقبض_ملف` | set the length through a handle |
| | `set_file_times` | `اضبط_أوقات_الملف` | set access and modification times |
| | `os_error_class_of` | `فئة_خطأ_حد_نظام` | classify a std.os boundary error |
| std.fs.canonical | `absolute` | `حول_إلى_مسار_مطلق` | make a path absolute (lexically) |
| | `canonicalize` | `مسار_قانوني` | resolve links, `.` and `..` |
| | `host_path_flavor` | `صيغة_مسار_المضيف` | the host's path flavor |
| std.fs.temporary | `temp_file` | `ملف_مؤقت` | a created temporary file (path and handle) |
| | `create_temp_dir / create_temp_dir_in` | `أنشئ_دليلا_مؤقتا / أنشئ_دليلا_مؤقتا_في` | new temporary directory |
| | `create_temp_file / create_temp_file_in` | `أنشئ_ملفا_مؤقتا / أنشئ_ملفا_مؤقتا_في` | new temporary file |
| | `close_temp_file` | `أغلق_ملفا_مؤقتا` | close it, keep the path |

| std.fs.lock | `file_lock / file_lock_shared` | `اقفل_الملف / اقفل_الملف_مشتركا` | Rust `File::lock` / `lock_shared` |
| | `file_try_lock / file_try_lock_shared` | `حاول_قفل_الملف / حاول_قفل_الملف_مشتركا` | `try_lock` / `try_lock_shared` |
| | `file_unlock` | `افتح_قفل_الملف` | `unlock` |
| | `try_lock_error (would_block, os)` | `خطأ_محاولة_قفل (ستحظر، نظام)` | Rust `TryLockError` |

### Environment (std.env)

| Module | English | Arabic | Meaning |
| --- | --- | --- | --- |
| std.env.variables | `environment_entry` | `مدخل_بيئة` | one NAME=VALUE pair |
| | `environment_variables` | `متغيرات_البيئة` | all variables |
| | `home_directory` | `الدليل_الرئيسي` | the user's home directory |
| | `set_current_directory` | `غير_دليل_العمل` | change the working directory |
| std.env.variables | `set_environment_variable` | `اضبط_متغير_بيئة` | Rust `env::set_var`, Go `os.Setenv` |
| | `remove_environment_variable` | `أزل_متغير_بيئة` | Rust `env::remove_var`, Go `os.Unsetenv` |
| std.env.system | — | `متغير_بيئة_النظام`, `دليل_العمل_للنظام`, `الدليل_المؤقت_للنظام` | the environment as the system holds it (what the compiler reads) |
| std.env.table | — | `جدول البيئة` | the environment table std owns |

### Processes (std.process)

| Module | English | Arabic | Meaning |
| --- | --- | --- | --- |
| std.process.command | `command` | `أمر` | a command to run |
| | `stdio (inherit, null, piped)` | `مصدر_تيار (وراثة، فارغ، التقاط)` | what a child stream is connected to |
| | `command_output` | `ناتج_أمر` | status and captured output |
| | `command_error (spawn, io, wait)` | `خطأ_أمر (تشغيل، دخل_خرج، انتظار)` | where running a command failed |
| | `new_command` | `أنشئ_أمرا` | new command |
| | `command_arg / command_args` | `أضف_وسيطة_للأمر / أضف_وسائط_للأمر` | add arguments |
| | `command_env / command_env_remove / command_env_clear` | `اضبط_بيئة_للأمر / أزل_بيئة_من_الأمر / امسح_بيئة_الأمر` | the child's environment |
| | `command_current_dir` | `اضبط_دليل_الأمر` | the child's working directory |
| | `command_stdin / command_stdout / command_stderr` | `اضبط_دخل_الأمر / اضبط_خرج_الأمر / اضبط_خطأ_الأمر` | stream choices |
| | `command_input` | `اضبط_بيانات_دخل_الأمر` | input bytes |
| | `command_status / command_output_of` | `شغل_الأمر / اجمع_خرج_الأمر` | run (status) / run and collect output |
| | `exit_success` | `هل_نجح_الأمر` | exit status 0 |

#### Streaming children

A child's standard error is `أخطاء` ("errors"), so it never reads as the boolean `خطأ`.

| Module | English | Arabic | Meaning |
| --- | --- | --- | --- |
| std.process.command | `command_spawn` | `ابدأ_الأمر` | Rust `Command::spawn` |
| | `command_child` | `طفل_أمر` | a running child (Rust `Child`) |
| | `child_stdin / child_stdout / child_stderr` | `دخل_طفل / خرج_طفل / خرج_طفل` | a stream taken from the child |
| | `child_take_stdin / child_take_stdout / child_take_stderr` | `خذ_دخل_الطفل / خذ_خرج_الطفل / خذ_أخطاء_الطفل` | Rust `child.stdin.take()` |
| | `child_stdin_write / child_stdout_read` | `اكتب_دخل_طفل / اقرأ_خرج_طفل` | the std.io writer and reader shapes |
| | `child_stdin_close / child_stdout_close` | `أغلق_دخل_طفل / أغلق_خرج_طفل` | close a taken stream |
| | `child_write_stdin / child_close_stdin` | `اكتب_إلى_الطفل / أغلق_دخل_الطفل` | write all to, or close, the child's input |
| | `child_close_streams` | `أغلق_تيارات_الطفل` | close every pipe the child still holds (no destructors) |
| | `child_read_stdout / child_read_stderr` | `اقرأ_خرج_الطفل / اقرأ_أخطاء_الطفل` | read what the child wrote |
| | `child_wait / child_try_wait / child_kill` | `انتظر_الطفل / تفقد_الطفل / اقتل_الطفل` | Rust `wait`, `try_wait`, `kill` |
| | `child_wait_with_output` | `انتظر_خرج_الطفل` | Rust `wait_with_output` |
| | `child_id` | `معرف_عملية_الطفل` | the process ID |

### Networking (std.net)

| Module | English | Arabic | Meaning |
| --- | --- | --- | --- |
| std.net.socket_names | `udp_datagram` | `حزمة_يو_دي_بي` | a received datagram and its sender |
| | `udp_send_to / udp_recv_from` | `أرسل_إلى_يو_دي_بي / استقبل_من_يو_دي_بي` | datagrams to/from an address |
| | `udp_local_addr / udp_peer_addr` | `العنوان_المحلي_يو_دي_بي / عنوان_نظير_يو_دي_بي` | socket names |
| | `tcp_local_addr / tcp_peer_addr / tcp_listener_local_addr` | `العنوان_المحلي_تي_سي_بي / عنوان_نظير_تي_سي_بي / العنوان_المحلي_لمستمع_تي_سي_بي` | socket names |

`يونكس` names the socket family, as `تي_سي_بي` names TCP.

| Module | English | Arabic | Meaning |
| --- | --- | --- | --- |
| std.net.unix | `unix_socket_addr (unnamed, pathname, abstract_name)` | `عنوان_يونكس (بلا_اسم، مسار، مجرد)` | Rust `std::os::unix::net::SocketAddr` |
| | `unix_listener / unix_stream / unix_datagram` | `مستمع_يونكس / تدفق_يونكس / مقبس_رسائل_يونكس` | `UnixListener` / `UnixStream` / `UnixDatagram` |
| | `unix_listener_bind / unix_accept / unix_connect` | `اربط_مستمع_يونكس / اقبل_يونكس / اتصل_يونكس` | (also `…_بعنوان` / `…_addr`) |
| | `unix_read / unix_write / unix_shutdown` | `اقرأ_يونكس / اكتب_يونكس / أوقف_يونكس` | stream I/O |
| | `unix_local_addr / unix_peer_addr / unix_listener_local_addr` | `العنوان_المحلي_يونكس / عنوان_نظير_يونكس / العنوان_المحلي_لمستمع_يونكس` | socket names |
| | `unix_datagram_bind / unix_datagram_unbound / unix_datagram_connect` | `اربط_رسائل_يونكس / رسائل_يونكس_بلا_ربط / اتصل_رسائل_يونكس` | datagram sockets |
| | `unix_send_to / unix_recv_from / unix_send / unix_recv` | `أرسل_إلى_يونكس / استقبل_من_يونكس / أرسل_رسائل_يونكس / استقبل_رسائل_يونكس` | datagrams |
| | `unix_datagram_packet` | `حزمة_يونكس` | a received datagram and its sender |
| | `unix_abstract_supported` | `هل_العنوان_المجرد_مدعوم` | whether abstract names exist here |

### Time (std.time)

| Module | English | Arabic | Meaning |
| --- | --- | --- | --- |
| std.time.duration_text | `format_duration / parse_duration` | `نسق_المدة / حلل_المدة` | Go duration notation |
| | `duration_error (invalid_format, unknown_unit, overflow)` | `خطأ_مدة (صيغة_غير_صالحة، وحدة_مجهولة، تجاوز)` | parse errors |
| | `duration_abs / duration_divide` | `مطلق_المدة / اقسم_المدة` | checked abs and division |
| std.time.duration_float | `duration_seconds_f64 / duration_from_seconds_f64` (and f32) | `ثواني_المدة_ع64 / مدة_من_ثواني_ع64` (and ع32) | float seconds |

### Numbers and math

`ملتف` ("wrapped around") names modular arithmetic, `متحقق` ("checked") an operation that answers `اختياري.لا_شيء` on overflow and `مشبع` ("saturated") one that clamps at the type's bounds, as `std.math.integer` names its checked and saturating functions (`جمع_طبيعي_متحقق`, `جمع_طبيعي_مشبع`). The type comes between the operation and the kind: `جمع_ط32_ملتف`. A shift is feminine (`إزاحة`), so its adjective is `ملتفة`.

| Module | English | Arabic | Meaning |
| --- | --- | --- | --- |
| std.math.trigonometry | `sin_f64 / cos_f64 / tan_f64` | `جيب64 / جيب_تمام64 / ظل64` | sine, cosine, tangent (also …32) |
| language | `x as f64`, `y as i32` | `س مثل ع64`, `ص مثل ص32` | float/integer conversion (see [Type system](type-system.md), "Numeric casts") |
| std.math.float | `f64_to_bits / f64_from_bits` | `بتات_ع64 / ع64_من_بتات` | IEEE 754 bit patterns (also `…ع32`) |
| std.math.wrapping | `wrapping_add_T / wrapping_sub_T / wrapping_mul_T` | `جمع_T_ملتف / طرح_T_ملتف / ضرب_T_ملتف` | modulo 2^N |
| | `wrapping_shl_T / wrapping_shr_T` | `إزاحة_يسرى_T_ملتفة / إزاحة_يمنى_T_ملتفة` | shift by the amount modulo the width |
| | `checked_add_T / checked_sub_T / checked_mul_T` | `جمع_T_متحقق / طرح_T_متحقق / ضرب_T_متحقق` | none on overflow |
| | `saturating_add_T / saturating_sub_T / saturating_mul_T` | `جمع_T_مشبع / طرح_T_مشبع / ضرب_T_مشبع` | clamped |
| compiler intrinsics | — | `بدائي_جمع_ملتف`, `بدائي_طرح_ملتف`, `بدائي_ضرب_ملتف`, `بدائي_إزاحة_يسرى_ملتفة`, `بدائي_إزاحة_يمنى_ملتفة` | reached through private aliases, as `بدائي_بتات_ع64` |
| std.math.float | `copysign_f64 / round_ties_even_f64 / mul_add_f64` (and f32) | `نسخ_إشارة64 / تقريب_زوجي64 / ضرب_جمع_مدمج64` (and …32) | IEEE copysign, roundToIntegralTiesToEven, fused multiply-add (see [Type system](type-system.md), "Float operations") |
| compiler intrinsics | — | `بدائي_جذر_تربيعي_عائم`, `بدائي_قيمة_مطلقة_عائمة`, `بدائي_أرضية_عائمة`, `بدائي_سقف_عائم`, `بدائي_بتر_عائم`, `بدائي_تقريب_زوجي_عائم`, `بدائي_أدنى_عائم`, `بدائي_أقصى_عائم`, `بدائي_نسخ_إشارة_عائمة`, `بدائي_ضرب_جمع_مدمج_عائم` | the float primitives behind std.math.float; `أدنى`/`أقصى` are IEEE minimum/maximum |

`T` is the type's own spelling (`ص8`…`ص64`, `ط8`…`ط64`, `عدد`, `طبيعي`) in Arabic and `i8`…`u64`, `int`, `uint` in English.

### Collections (std.collections and vector primitives)

| Module | English | Arabic | Meaning |
| --- | --- | --- | --- |
| std.collections.vector | `reverse_vector / rotate_left` | `اعكس_المتجه / دور_المتجه_يسارا` | reorder in place |
| | `index_of_by / contains_by` | `ابحث_في_المتجه / هل_يحتوي_المتجه` | search with an equality |
| | `position_min_by / position_max_by` | `موضع_الأصغر_في_المتجه / موضع_الأكبر_في_المتجه` | extremes with an ordering |
| language | `pop(v)` | `أزل_الأخير(م)` | Rust `Vec::pop`: the last element as `اختياري` (see [Ownership](ownership.md), "Pop and truncate") |
| | `truncate(v, n)` | `اقتطع(م, ن)` | Rust `Vec::truncate`: drop the elements past `n` (`ن: طبيعي`); the verb of `اقتطع_الملف` |
| std.collections.vector | `retain / dedup_by` | `احتفظ_في_المتجه / أزل_المتكرر_المتتالي` | Rust `Vec::retain` / `Vec::dedup_by`, Go `slices.DeleteFunc` / `CompactFunc` |
| | `remove_at / swap_remove / insert_at` | `أزل_عند / أزل_بالتبديل / أدرج_عند` | Rust `Vec::remove` / `swap_remove` / `insert` |
| | `clear_vector` | `امسح_المتجه` | Rust `Vec::clear` (keeps the buffer) |
| language | `capacity(v)` | `سعة(م)` | Rust `Vec::capacity`, Go `cap`: elements the buffer holds before an append reallocates |
| | `reserve_exact(v, n)` | `احجز_سعة(م, ن)` | Rust `Vec::reserve_exact`: room for `ن` more elements, exactly (`ن: طبيعي`) |
| std.collections.vector | `with_capacity / reserve / shrink_to_fit` | `متجه_بسعة / احجز_في_المتجه / قلص_المتجه` | Rust `Vec::with_capacity` / `reserve` (amortized) / `shrink_to_fit` |

### Text and floats (std.text)

| Module | English | Arabic | Meaning |
| --- | --- | --- | --- |
| std.text.transform | `split_whitespace / splitn / replacen` | `قسم_بالفراغات / قسم_النص_بحد / استبدل_في_النص_بحد` | splitting and replacing |
| | `strip_prefix / strip_suffix` | `أزل_البادئة / أزل_اللاحقة` | remove a prefix/suffix |
| | `to_ascii_lowercase / to_ascii_uppercase` | `حول_النص_إلى_صغير_أسكي / حول_النص_إلى_كبير_أسكي` | ASCII case |
| std.text.search | `count_matches / char_count / eq_ignore_ascii_case` | `عد_تكرارات_النص / عدد_محارف_النص / نصان_متساويان_بلا_حالة_أسكي` | counting and comparing |
| std.text.float | `f64_to_string / f32_to_string` | `ع64_إلى_نص / ع32_إلى_نص` | shortest round-trip text |
| | `parse_f64 / parse_f32`, `float_parse_error (empty, invalid_format)` | `حلل_ع64 / حلل_ع32`, `خطأ_تحليل_عائم (فارغ، صيغة_غير_صالحة)` | exact parsing |

### Formatting (std.fmt)

The verb is `نسق` ("arrange, format"): `نسق` formats, `نسق_في` formats into a formatter, `اكتب_منسقا` writes formatted text to a writer and `اطبع_…_منسقا` prints it; the formatter is `منسق` (the one who formats) and its spec `مواصفة`. `احش` (pad) fills to a width, as the fill character is `الحشو`. The package is `المكتبة.تنسيق` (`std.fmt`). See [Formatting](formatting.md).

| Module | English | Arabic | Meaning |
| --- | --- | --- | --- |
| std.fmt.fmt | `format` / `format_into` | `نسق` / `نسق_في` | Rust's `format!` / `write!` to a String or Formatter |
| | `write_fmt` / `write_fmt_with` | `اكتب_منسقا` / `اكتب_منسقا_بمنسق` | `write!` to a std.io writer (with a reused formatter) |
| | `print_fmt` / `println_fmt` / `eprint_fmt` / `eprintln_fmt` | `اطبع_منسقا` / `اطبع_سطرا_منسقا` / `اطبع_خطأ_منسقا` / `اطبع_سطر_خطأ_منسقا` | `print!` / `println!` / `eprint!` / `eprintln!` |
| | `fmt_arg` (`int`, `uint`, `i8`…`u64`, `f32`, `f64`, `bool`, `char`, `text`, `bytes`, `ints`, `texts`, `list`, `custom`) | `قيمة_تنسيق` (`عدد`، `طبيعي`، `ص8`…`ط64`، `ع32`، `ع64`، `منطقي`، `حرف`، `نص`، `بايتات`، `أعداد`، `نصوص`، `قائمة`، `مخصص`) | one argument; `مخصص` holds a closure (Display/Debug of a user type) |
| | `fmt_error` (`unclosed_brace`, `unmatched_close_brace`, `invalid_spec`, `unknown_type`, `named_argument`, `missing_argument`, `unused_argument`, `mismatched_type`, `invalid_count`, `write_failed`, `custom_failed`) | `خطأ_تنسيق` (`قوس_غير_مغلق`، `قوس_إغلاق_منفرد`، `مواصفة_غير_صالحة`، `نوع_غير_معروف`، `وسيط_مسمى`، `وسيط_مفقود`، `وسيط_غير_مستخدم`، `نوع_غير_مناسب`، `عدد_غير_صالح`، `فشل_الكتابة`، `فشل_مخصص`) | a format or formatting error |
| | `formatter` / `fmt_spec` | `منسق` / `مواصفة_تنسيق` | Rust's `Formatter` and its spec |
| | `fmt_alignment` (`unknown`, `left`, `center`, `right`) | `محاذاة_تنسيق` (`بلا`، `يسار`، `وسط`، `يمين`) | `fmt::Alignment` |
| | `fmt_kind` (`display`, `debug`, `lower_hex`, `upper_hex`, `octal`, `binary`, `lower_exp`, `upper_exp`, `debug_lower_hex`, `debug_upper_hex`) | `صيغة_تنسيق` (`عرض`، `تصحيح`، `ست_عشري`، `ست_عشري_كبير`، `ثماني`، `ثنائي`، `أسي`، `أسي_كبير`، `تصحيح_ست_عشري`، `تصحيح_ست_عشري_كبير`) | the placeholder's type: `{}` `{:?}` `{:x}` `{:X}` `{:o}` `{:b}` `{:e}` `{:E}` `{:x?}` `{:X?}` |
| | `new_formatter` / `formatter_with_capacity` | `أنشئ_منسقا` / `أنشئ_منسقا_بسعة` | a formatter |
| | `formatter_text` / `take_formatter_text` / `formatter_length` / `formatter_clear` | `نص_المنسق` / `خذ_نص_المنسق` / `طول_المنسق` / `امسح_المنسق` | copy, move out, length, empty keeping the buffer |
| | `write_str` / `write_char` / `pad` | `اكتب_نصا_في_المنسق` / `اكتب_حرفا_في_المنسق` / `احش_نصا` | `Formatter::write_str`, `write_char`, `pad` |
| | `formatter_width` / `formatter_precision` / `formatter_fill` / `formatter_align` | `عرض_المنسق` / `دقة_المنسق` / `حشو_المنسق` / `محاذاة_المنسق` | the placeholder's width, precision, fill, alignment |
| | `formatter_alternate` / `formatter_sign_plus` / `formatter_sign_aware_zero_pad` / `formatter_kind` / `formatter_debug` | `المنسق_بديل` / `المنسق_بإشارة` / `المنسق_بأصفار` / `صيغة_المنسق` / `المنسق_للتصحيح` | `#`, `+`, `0`, the type, whether it is Debug |
| std.io.log | `log_fmt` | `سجل_منسقا` | a formatted log entry |
| std.error.from_fmt | `error_from_fmt_error` | `فشل_من_خطأ_تنسيق` | `خطأ_تنسيق` as `فشل` |
| std.text.float | `float_digits` / `f64_shortest_digits` / `f64_exact_digits` (and f32) | `أرقام_عائم` / `أرقام_ع64_الأقصر` / `أرقام_ع64_الدقيقة` (and ع32) | a float's decimal digits for formatters |
| | `f64_fixed_digits` / `f64_significant_digits` (and f32) | `أرقام_ع64_بخانات_كسرية` / `أرقام_ع64_بخانات_معنوية` (and ع32) | digits rounded half to even to N places after the point / N significant digits |

### Compression, checksums and hashes

Format names are transliterated as std already writes `جي_زيب` (gzip) and `تي_إل_إس` (TLS): `زد_ستد` (zstd), `إكس_زد` (xz), `إل_زد_إم_إيه` (LZMA), `إتش_باك` (HPACK); HTTP/2 is `ويب2` beside HTTP/1's `ويب1`. Compression uses the verbs of `std.compression.gzip`: `اضغط` (compress) and `فك_ضغط` (decompress).

| Module | English | Arabic | Meaning |
| --- | --- | --- | --- |
| std.compression.zstd | `zstd_compress` / `zstd_compress_with_options` | `اضغط_زد_ستد` / `اضغط_زد_ستد_بخيارات` | one zstd frame (RFC 8878), with or without the checksum |
| | `zstd_decompress` / `zstd_decompress_limited` / `zstd_decompress_with_dictionary` / `zstd_decompress_with_options` | `فك_ضغط_زد_ستد` / `…_بحد` / `…_بقاموس` / `…_بخيارات` | decode every frame; output limit, dictionary, window limit |
| std.compression.xz | `xz_compress`, `xz_decompress`, `xz_decompress_limited` | `اضغط_إكس_زد`, `فك_ضغط_إكس_زد`, `فك_ضغط_إكس_زد_بحد` | the .xz container |
| std.compression.lzma | `lzma_compress`, `lzma_decompress`, `lzma2_compress`, `lzma2_decompress` | `اضغط_إل_زد_إم_إيه`, `فك_ضغط_إل_زد_إم_إيه`, `اضغط_إل_زد_إم_إيه2`, `فك_ضغط_إل_زد_إم_إيه2` | .lzma files and raw LZMA2 |
| std.hash.xxh64 | `xxh64`, `xxh64_with_seed`, `xxh64_range` | `هاش_إكس_إكس_إتش64`, `…_ببذرة`, `…_لمدى` | XXH64 |
| std.checksum.checksum | `crc32_update`, `crc64`, `crc64_update` | `حدث_سي_ار_سي32`, `احسب_سي_ار_سي64`, `حدث_سي_ار_سي64` | incremental CRC-32; CRC-64/XZ |

### TLS and HTTP

| Module | English | Arabic | Meaning |
| --- | --- | --- | --- |
| std.tls.config | `tls_client_config_set_protocols`, `tls_server_config_set_protocols` | `اضبط_بروتوكولات_عميل_تي_إل_إس`, `اضبط_بروتوكولات_خادم_تي_إل_إس` | ALPN lists (RFC 7301) |
| std.tls.stream | `tls_negotiated_protocol` | `بروتوكول_تدفق_تي_إل_إس` | the agreed ALPN protocol |
| std.http.transport | `http_transport` (`tcp`, `tls`) | `ناقل_ويب` (`تي_سي_بي`, `تي_إل_إس`) | the byte transport under HTTP/1 and HTTP/2 |
| std.http.server | `run_https_server`, `serve_http_connection`, `serve_https_connection` | `شغل_خادم_ويب_آمن`, `خدم_اتصال_ويب`, `خدم_اتصال_ويب_آمن` | HTTPS with ALPN; one connection |
| std.http.http2.hpack | `hpack_encode_headers`, `hpack_decode_block` | `رمز_رؤوس_إتش_باك`, `فك_كتلة_إتش_باك` | header blocks |
| std.http.http2.connection | `http2_connection`, `http2_read_and_process_frame`, `http2_flush_data`, `http2_send_headers` | `اتصال_ويب2`, `اقرأ_وعالج_إطار_ويب2`, `ادفع_بيانات_ويب2`, `أرسل_رؤوس_ويب2` | the shared connection layer |
| std.http.http2.client | `http2_connect`, `http2_send_requests`, `http2_close_client` | `اتصل_عميل_ويب2`, `أرسل_طلبات_ويب2`, `أغلق_عميل_ويب2` | the client; concurrent streams |
| std.http.http2.server | `serve_http2_connection` | `خدم_اتصال_ويب2` | the server side of one connection |

### Threads and synchronization (std.sync, std.os.wait_address)

The type names are nouns: `مشترك` ("shared": Rust's Arc), `قفل` ("lock": Mutex), `قفل_قراءة_كتابة` (RwLock), `شرط_انتظار` ("wait condition": Condvar), `مرة` ("once"), `خلية_مرة` ("once cell": OnceLock), `قناة` ("channel"), `مرسل` / `مستلم` ("sender" / "receiver"; `مستقبل` already names the futex in the system-call tables). The verbs are the dictionary's: `أنشئ`, `استنسخ` (clone), `اقرأ` (read through), `نفذ` (run something), `حاول` (try), `انتظر` / `أيقظ` / `نبه` (wait / wake / notify), `أرسل` / `استقبل`, `أسقط` (drop an end), `اضبط` (set).

| Module | English | Arabic | Meaning |
| --- | --- | --- | --- |
| std.sync.arc | `arc`, `arc_new`, `arc_clone`, `arc_get` | `مشترك`, `أنشئ_مشترك`, `استنسخ_مشترك`, `اقرأ_مشترك` | Rust `Arc`, `Arc::new`, `Arc::clone`, `Deref` |
| | `arc_strong_count`, `arc_ptr_eq`, `arc_as_ptr` | `عدد_مراجع_مشترك`, `هل_المشترك_نفسه`, `مؤشر_مشترك` | `Arc::strong_count`, `Arc::ptr_eq`, `Arc::as_ptr` |
| std.sync.mutex | `mutex`, `mutex_new`, `with_lock`, `try_lock`, `mutex_is_locked` | `قفل`, `أنشئ_قفل`, `نفذ_مع_القفل`, `حاول_القفل`, `هل_القفل_مأخوذ` | Rust `Mutex`; the lock held for one closure (the guard's scope) |
| std.sync.rwlock | `rwlock`, `rwlock_new`, `with_read`, `with_write` | `قفل_قراءة_كتابة`, `أنشئ_قفل_قراءة_كتابة`, `نفذ_مع_قراءة`, `نفذ_مع_كتابة` | Rust `RwLock::read` / `write` |
| std.sync.condvar | `condvar`, `condvar_new`, `condvar_wait`, `condvar_wait_timeout`, `condvar_wait_while` | `شرط_انتظار`, `أنشئ_شرط_انتظار`, `انتظر_الشرط`, `انتظر_الشرط_بمهلة`, `انتظر_طالما` | Rust `Condvar::wait`, `wait_timeout`, `wait_while` |
| | `condvar_notify_one`, `condvar_notify_all` | `نبه_واحدا`, `نبه_الكل` | `notify_one`, `notify_all` |
| std.sync.once | `once`, `once_new`, `call_once`, `once_is_completed` | `مرة`, `أنشئ_مرة`, `نفذ_مرة`, `هل_اكتملت_المرة` | Rust `Once` |
| | `once_lock`, `once_lock_new`, `once_lock_get`, `once_lock_get_or_init`, `once_lock_set` | `خلية_مرة`, `أنشئ_خلية_مرة`, `اقرأ_خلية_مرة`, `اقرأ_أو_هيئ_خلية_مرة`, `اضبط_خلية_مرة` | Rust `OnceLock` |
| std.sync.channel | `channel`, `sync_channel`, `channel_ends` | `قناة`, `قناة_متزامنة`, `طرفا_قناة` (`المرسل`, `المستلم`) | Rust `mpsc::channel`, `sync_channel` and the pair they return |
| | `sender`, `receiver`, `sender_clone`, `sender_for`, `channel_receiver`, `sync_channel_receiver` | `مرسل`, `مستلم`, `استنسخ_مرسل`, `مرسل_لمستلم`, `مستلم_قناة`, `مستلم_قناة_متزامنة` | the ends; a receiver alone and a sender made for it |
| | `send`, `try_send`, `recv`, `try_recv`, `recv_timeout` | `أرسل`, `حاول_الإرسال`, `استقبل`, `حاول_الاستقبال`, `استقبل_بمهلة` | Rust's methods of the same names |
| | `drop_sender`, `drop_receiver` | `أسقط_مرسل`, `أسقط_مستلم` | Rust `drop(tx)` / `drop(rx)` |
| | `try_recv_error (empty, disconnected)`, `recv_timeout_error (timeout, disconnected)`, `recv_error (disconnected)`, `try_send_error (full, disconnected)` | `خطأ_محاولة_استقبال (فارغة، منقطعة)`, `خطأ_استقبال_بمهلة (انتهت_المهلة، منقطعة)`, `خطأ_استقبال (منقطعة)`, `خطأ_محاولة_إرسال (ممتلئة، منقطعة)` | Rust `TryRecvError`, `RecvTimeoutError`, `RecvError`, `TrySendError` |
| std.sync.atomic | `memory_ordering.relaxed / release / acquire / acq_rel / seq_cst` | `ترتيب_ذاكرة.مسترخ / إطلاق / اكتساب / اكتساب_وإطلاق / تسلسلي` | Rust `Ordering` |
| std.os.wait_address | `wait_on_address`, `wake_by_address_single`, `wake_by_address_all`, `wait_on_address_supported` | `انتظر_عند_العنوان`, `أيقظ_واحدا_عند_العنوان`, `أيقظ_الكل_عند_العنوان`, `هل_الانتظار_عند_العنوان_مدعوم` | futex wait / wake (Windows `WaitOnAddress` names) |
| | `wait_on_cell`, `wake_one_on_cell` | `انتظر_عند_الخلية`, `أيقظ_واحدا_عند_الخلية` | the same on a bare 32-bit cell |
| diagnostics | thread capture | `التقاط_خيط_مؤشر_خام`, `التقاط_خيط_مرجع`, `التقاط_خيط_مشترك_غير_ذري` | `SULTANC-THREAD-CAPTURE-RAW-POINTER`, `…-REFERENCE`, `…-NON-ATOMIC-SHARED` |

### Error converters

| Module | English | Arabic | Meaning |
| --- | --- | --- | --- |
| std.error | `error_from_command_error / error_from_duration_error` | `فشل_من_خطأ_أمر / فشل_من_خطأ_مدة` | converters |

## See also

- [Errors](errors.md): `std.error` in detail.
- [Formatting](formatting.md): `std.fmt` in detail.
- [Concurrency](concurrency.md): the sync and thread vocabulary in use.
- [Type system](type-system.md): numeric casts and float operations.
