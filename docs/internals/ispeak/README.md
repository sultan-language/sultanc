# iSpeak

iSpeak is the text layer of the compiler (`compiler/ispeak/`). Every string that SultanC prints comes
from it: diagnostics, contextual reasons, labels, notes and help, `sultanc explain`, CLI help, the
version and targets headers, package manifest reasons and lexer token words. It speaks Arabic (the
default for `--lang`) and English. This page explains how a diagnostic becomes text and where each
piece lives. It is written for people who add diagnostics or change compiler wording.

iSpeak owns wording only. Detection, identities, public codes, reasons, grouping, source spans and
output layout belong to the compiler.

## Layout

```text
compiler/diagnostics/identity.sn   معرف_تشخيص (142 public diagnostics)
compiler/diagnostics/reason.sn     سبب_تشخيص (503 typed reasons), machine keys,
                                   تشخيص_عام_لسبب (reason -> exact public identity)
compiler/diagnostics/registry.sn   public code, machine kind, severity, class
compiler/diagnostics/schema.sn     label/help/note keys (مفتاح_صياغة), لغة_تشخيص
compiler/diagnostics/message.sn    record -> iSpeak typed facts (the adapter)
compiler/runtime/failure_identity.sn  runtime failure kinds

compiler/ispeak/ispeak.sn          public API, selects the language once
compiler/ispeak/format/            facts, quoting, the one context composer
compiler/ispeak/grammar/           CLDR plural categories, counted nouns
compiler/ispeak/keys/              clause, surface and explanation keys
compiler/ispeak/locales/{ar,en}/   words and word order only
```

There are no wire IDs. `كل_معرفات_التشخيص()`, `كل_أسباب_التشخيص()`, `كل_مفاتيح_النص_المترجم()`,
`كل_مفاتيح_الصياغة()` and `كل_مفاتيح_رسائل_التشخيص()` enumerate the keys for tests and tools. The first
four are written by hand, and `check_enumerations.sn` reads each type's declaration from the source
and fails when a variant is missing from its list.

Every table is total. A new identity or reason fails to compile until it has its words in both
locales, its public code, its explanation group, its template decision (`templates.sn`) and its
grouping decision (`تشخيص_عام_لسبب`, where `لا_شيء` keeps the producer's identity). Only the clause
tables keep a generic clause per fact for identities without their own phrasing.

## How a diagnostic becomes text

1. A producer emits a `معرف_تشخيص` with arguments (`مفتاح_حجة_تشخيص` mapped to a value). A value is
   text, bilingual text, a number, a boolean, a typed reason (`سبب_تشخيص`) or a typed compiler text
   (`مفتاح_نص_مترجم`).
2. The engine applies reason grouping once, at emit time. A reason that names an exact public
   diagnostic decides the public identity.
3. `message.sn` reads the arguments once into iSpeak's 22-fact context. Identities with a specialized
   template (unknown name, type mismatch, CLI values, LLVM linking, compile-time failure, package
   manifest, keyword suggestion, syntax expectation, interpreter primitives) word some facts as the
   headline. The template clears exactly the facts it used, and every other fact, the reason
   included, is still one sentence after it. Counted comparisons (call arguments, primitive
   arguments, pattern payloads) have no template: the composer words them with the locale's CLDR
   count grammar (Arabic: zero, one, dual, 3-10 plural, 11-99 singular). The pipeline stage is shown
   only for internal and unimplemented failures.
4. Every text fact is quoted as code (`` `i32` ``) in both languages. Numbers are bare, and booleans
   are the language's quoted literal (`` `خطأ` `` or `` `false` ``). The terminal wraps each quoted
   fact of an Arabic message in Unicode first-strong isolates (U+2068 to U+2069) so that left-to-right
   facts keep their order. JSON output carries no isolates.

The facts that each identity may carry, and what iSpeak can and cannot check about them, are described
in [Contextual facts](context-contracts.md).

## Output and language

`--lang=ar|en` selects the language. Without `--lang` the compiler speaks Arabic, and `--lang=auto`
follows the host locale (`LC_ALL`, `LC_MESSAGES`, `LANG`): a value starting with `ar` is Arabic and
anything else is English. An unknown `--lang` value is reported in English. Options between `run` and
its source path are compiler options.

`--diagnostic-format=json` writes schema 3. It has a `language` field and every argument as
`{"key", "type", "value"}` with `value` always a string. The `type` is `text`, `localized` (with `en`
and `ar`), `unsigned`, `signed`, `boolean`, or `reason`, whose `value` is the machine key and whose
`text` is the wording. Notes and help are `{"message", "arguments"}` objects with their arguments
composed into the message.

## Type and name printing

One printer (`frontend/types/tooling_queries.sn`, `تهجئة_نوع_بلغة`) prints types in the requested
language. Builtin spellings come from `tools/generate/registry/type_registry.def` (the Arabic names
and the English `DiagnosticName` column), keywords from the token registry, and user-declared names as
written. Producers attach type facts in both languages (`أضف_نوع_تشخيص`), so `--lang` selects the
spelling when the message is rendered and JSON output carries both. Symbols in ownership diagnostics
are named the same way. A type that cannot be printed keeps its numeric ID.

## Compiling with Stage0

The compiler is also built by the C Stage0, so iSpeak follows its rules: no top-level constants
(wording constants are zero-argument functions), no references stored in records or assigned to
mutable variables (facts own their text), and no constructor calls inside vector literals. See
[Bootstrap](../bootstrap.md).

## Tests

The suites are `tests/ar/ispeak` and `tests/en/ispeak`, the same programs in both surface syntaxes,
run with `tools/tests/ispeak_test.sh`. `tools/tests/ci_run.sh` runs them in CI together with the
compiler's own runner ([Tests](../../contributing/tests.md)).

A reject test may name files with the exact Arabic and English output (`// خرج-تشخيص-عربي: <file>`,
`// خرج-تشخيص-إنجليزي: <file>`). `tests/{ar,en}/diagnostics/e2e` holds such end-to-end tests for the
compiler's runner. `check_arabic_tables.sn` fails on an Arabic table string that is wholly ASCII
English.

## See also

- [Contextual facts](context-contracts.md)
- [iSpeak history](../../history/ispeak-history.md), the development log of the standalone iSpeak tree
  before integration
- [Reason audit](../../history/ispeak-semantic-audit.md)
- [Arabic terminology](../../reference/arabic-terminology.md)
- [Errors](../../reference/errors.md)
