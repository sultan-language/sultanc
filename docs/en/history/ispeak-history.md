[العربية](../../ar/history/ispeak-history.md) | **English**

# iSpeak — SultanC message localization

> **Historical record.** This page documents the project as it was when it was written. For the current
> design, see the [documentation index](../index.md).

**Compiler baseline:** `SultanC_Full_20261104.zip` (November 4, 2026). Its
`compiler/` tree is identical to the merged November 4 checkpoint that iSpeak
was audited against; only three bootstrap LLVM files differ.
This directory is the future owner of compiler-authored human-readable text.
It does **not** own error detection, the compiler's `--lang` selection,
source locations, debug symbols, diagnostic records, terminal layout,
JSON/LSP schemas, or transport. Integrating compiler call sites is a
separate compiler-owned migration; those call sites have not yet migrated.

## Layout

Compiler-owned policy is **not** part of iSpeak. Until integration it lives in a
staging tree beside `ispeak/`, at the paths it will take inside `compiler/`:

```
staging/compiler/                      compiler-owned, temporary
  diagnostics/identity.sn              معرف_خطأ, معرف_سبب, معرف_رسالة
  diagnostics/wire.sn                  stable ط16 IDs, nibble decoder, ط8 pair form
  diagnostics/registry.sn              public code, machine kind, severity, class
  diagnostics/grouping.sn              reason -> public identity (خطأ_عام_للسبب)
  runtime/failure_identity.sn          runtime failure kinds
ispeak/                                wording only
  ispeak.sn                            public API; selects the language once
  language.sn                          لغة_رسالة
  format/facts.sn                      typed message facts and context record
  format/quote.sn                      inline quotation, shared builder helpers
  format/compose.sn                    the one context composer (all languages)
  grammar/plural.sn                    CLDR plural categories
  grammar/counts.sn                    counted nouns and count-comparison frames
  keys/{clauses,details,surface,explanations}.sn   non-diagnostic text keys
  locales/<lang>/errors.sn             public diagnostic messages
  locales/<lang>/reasons.sn            contextual reason texts
  locales/<lang>/clauses.sn            context-clause openings, boolean words
  locales/<lang>/counts.sn             count word forms and sentence order
  locales/<lang>/templates.sn          payload templates (locale owns word order)
  locales/<lang>/{details,surface,explanations,runtime}.sn
```

Rules (enforced by the static check):
- Nothing in `ispeak/` defines an identity, wire ID, public code, grouping or
  runtime failure kind; it imports them from `staging.compiler.*`. Integration
  replaces `staging.compiler.` with `compiler.` and deletes `staging/`.
- Every locale has the same file set. A locale file imports no other locale and
  nothing above it (`format/compose.sn`, `ispeak.sn`).
- Mechanics exist once: quoting, sentence separators, fact rendering and context
  order are in `format/`; locales hold words and word order only.
- Every lookup is one typed `طابق`; no catalog scan. The errors and reasons
  tables have no `_` arm, so a missing translation is a compile error.
- Adding a language: add `locales/<lang>/` with the same files, one variant in
  `language.sn`, one arm per dispatcher, and its CLDR rule in `grammar/plural.sn`.

Messages are split by kind (errors, reasons), not by compiler domain: a domain
split would need a `_` fallback per domain file and lose compile-time totality.

## Tests

```
tests/ar/ispeak/            Arabic surface syntax (عرف، دع، أرجع ...)
tests/en/ispeak/            the same tests in English surface syntax (def, let, return ...)
  check_*.sn                22 behavioral tests          (// اختبار: تشغيل)
  bench/bench_*.sn          17 cost benchmarks           (// اختبار: تشغيل)
  fixtures/*.sn             2 rejection fixtures         (// اختبار: رفض)
```

- Each tree is ready to drop into the compiler's `tests/ar/` and `tests/en/`:
  every file carries the compiler runner's own headers (`اختبار`, `أهداف`,
  `خروج`, `تشخيص`) and a `نظير` line naming its counterpart at that final path.
- The English file is the same program as its Arabic counterpart, token for
  token: keywords, builtin types, primitives, `option`/`result` constructors,
  std names that have an English `مرادف`, and test-local names are English.
  Names owned by iSpeak or `staging/` (API functions, identities, fact types,
  record fields) stay as written: they have no English alias.
- Both trees check text in both output languages; the folder is the source
  language of the test, as in the compiler's own `tests/ar` and `tests/en`.
- Tests run from the repository root (imports `ispeak.*`, `staging.compiler.*`).
- Verified 82/82 on `x86_64-linux` and `arm64-darwin` (check, native and LLVM
  build and run for every run test; expected code for every rejection test).

Current source-backed coverage: **135 public diagnostic identities and
456 contextual reasons** (591 active numeric assignments). Public diagnostic
identities and machine-stable contextual reason keys remain distinct.
Existing assigned numbers remain unchanged. Historical IDs **0, 29, 30,
220, 223–226, 228, 236, 238, 240–243, 248, 251–255** are unassigned.
The November 2 checkpoint added four public identities (545–548), retired
one public identity (29), added 18 reasons (549–566) and retired 18 reason keys.
The November 4 update adds one public identity (567), 21 reasons (568–588)
and 23 proposed identities (589–611); see the two November 4 sections at
the end. Highest assigned ID is 611 (`أعلى_رقم_مسجل` in `staging/compiler/diagnostics/wire.sn`).
Do not reuse reserved IDs or treat translated text as identity.

Measurement sections further down describe the catalog as it was when they
were run (546 assignments); they were not re-run for 591. Dated sections use
the file names of their time (`codes.sn`, `arguments.sn`, `en/errors.sn`, ...);
the Layout section above is current.

Text-only reverse lookup and runtime message-catalog scans are not part of
normal diagnostic emission. A typed identity resolves directly to its
language-specific message; dynamic values remain separate facts. New
languages should add a localized catalog and language dispatch, not a new
identity registry. Every currently established diagnostic argument key is
formatted in both languages. For remaining fixed-message identities, optional
contextual facts become complete language-specific clauses rather than
`field: value` dumps. This supports all 546 active identity mappings and
auxiliary text, explanations and secondary wording without inventing new
semantic facts; future message contracts must be added when the compiler
defines them.

## Dynamic sentence formatting

`رسالة_تشخيص_بحجج` accepts an existing typed identity plus either one of the
21 specialized argument shapes or a fixed record of the 22 canonical compiler
argument fields. The specialized forms retain their exact, source-backed
sentences. Other identities keep their localized diagnostic sentence and
include each supplied contextual fact as a complete localized clause rather
than a machine-style label dump. Expected and actual values share one
sentence, with diagnostic-specific wording for source-backed option, path,
symbol, target, reason, type, pattern and comparison facts. Booleans and typed
contextual reasons use the selected language. Absent optional facts add no
text; an entirely empty context returns the fixed localized message without
constructing a text builder. The same formatter serves secondary wording,
explanatory text and non-diagnostic compiler messages.

Identity-specific context clauses additionally cover module loading, record
fields, name binding, ownership, types, patterns, indexing and interpreter
failures. Other typed contextual facts retain language-specific general
clauses without guessing a diagnostic identity. The compiler producers must
be audited against their exact fact combinations before this can be described
as exhaustive source-backed semantic coverage.

**Payload boundary:** Dynamic facts remain distinct from their typed message
identity and are not modified. Ordinary context sentences append text facts
verbatim. When a specialized sentence surrounds a text fact with backticks,
`arguments.sn` escapes embedded backticks, backslashes, newlines, tabs,
carriage returns, ASCII C0 controls and DEL *inside that quotation* to
preserve the sentence boundary. Unicode remains unchanged. This is a
message-level quoting rule, not terminal/JSON/LSP serialization. Diagnostic
renderers still own transport escaping and bidirectional-text isolation,
including for verbatim ordinary-context values.

`tests/check_dynamic_format.sn` asserts exact English/Arabic sentences for
all 22 fields and auxiliary catalogs. `tests/check_dynamic_coverage.sn`
executes both language paths for every one of the 546 currently assigned
identities. `tests/check_dynamic_families.sn` exercises the 21 specialized
identity-specific formats. `tests/check_dynamic_sentences.sn` asserts exact
English/Arabic sentences for CLI, file/module paths, names, record fields,
warnings, typed reasons, target and type/count comparisons, including
expected-only and actual-only cases. `tests/check_dynamic_matrix.sn` exercises
every active identity with **all 22 distinct textual fields simultaneously**
in both languages (546 × 22 × 2 field-preservation assertions).
`tests/check_dynamic_quoting.sn` checks exact output for quoted names, types,
options, Arabic Unicode, and every ASCII C0/DEL byte, including NUL.
`tests/check_id_golden.sn` independently fixes all 546 historical numeric IDs
against the source checkpoint SHA-256; unlike a decoder round-trip, it fails
if two assignments are mutually swapped. These tests verify iSpeak contracts,
not compiler diagnostic call-site migration.

## O(1) behavior and evidence

`مفتاح_من_رقم` has four fixed 4-bit radix levels and no heap table; its
lookup depth is independent of the number of catalog entries. The exhaustive
`tests/check_byte_space.sn` validates all 65,536 possible two-byte values,
including unassigned and retired IDs.

On the November 2 compiler checkpoint, eligible dense ADT `match` branches
lower through the existing MSIR multiway branch. The Native x86-64 executable
for `tests/bench_numeric_reason_late.sn` contains a bounds check on tag 434,
then a **relative indexed jump table** (`movslq (...,%r10,4)` and `jmp *%r11`).
There is no sequential 435-way comparison in this tested dispatch path.
LLVM similarly lowers these matches to switch operations. Native ARM64 shares
the canonical selection policy but was not executed on this Linux host.
The interpreter caches dense O(1) switch indexes and uses binary search
for sparse cases (`compiler/interpreter/execution/execution_state.sn`);
its runtime behavior was not benchmarked here. Indexed branch selection
establishes constant-time *dispatch* for this case, not a guarantee about
all possible string construction and memory allocation.

**Comparable measurements:** freshly built Stage2, Linux x86-64, same target,
backend settings and six million dynamically indexed operations per fixture.

| Workload (6M operations) | Native early | Native late | LLVM early | LLVM late |
| --- | ---: | ---: | ---: | ---: |
| Numeric reason identity | 0.16 s | 0.15 s | 0.17 s | 0.18 s |
| English reason text + length | 0.25 s | 0.23 s | 0.20 s | 0.17 s |

The late fixtures use the new terminal reason IDs 564–566. The text
benchmarks reported 128 KiB peak RSS under `/usr/bin/time` on this host.
These are bounded-run measurements, not proof of zero allocations or of
O(1) behavior for every target or every possible formatter.

## Verification

After placing `ispeak/` at repository root, use the built Stage2 compiler:

```sh
./build/sultanc check --lang=en ispeak/tests/check_roundtrip.sn
./build/sultanc ispeak/tests/check_roundtrip.sn --backend=native --target=x86_64-linux --emit=exe -o build/ispeak-roundtrip
./build/ispeak-roundtrip
./build/sultanc ispeak/tests/check_new_reasons.sn --backend=llvm --target=x86_64-linux --emit=exe -o build/ispeak-new-reasons
./build/ispeak-new-reasons
```

The uploaded starting checkpoint's November 2 run is historical evidence,
not verification of this revision. The revised sources were instead compiled
and executed on the November 1 Stage2 baseline, as documented below; a
repeat on the authoritative November 2 compiler remains unverified.
`SEMANTIC_AUDIT.md` maps all 435 active machine reason keys to their current
compiler source locations and current localized messages. Presence of a key
in source proves its association, not semantic correctness of every possible
dynamically formatted message.

## Verified dynamic formatting and cost (this scoped update)

On 2026-11-03, the `ispeak/` sources in this archive were placed into a
**November 1 `SultanC_RSS_Fixed_Checkpoint_20261101.zip`** compiler tree,
which is *older than* the November 2 baseline named above. The documented
CMake Stage2 build using LLVM 19.1.7 succeeded. All **13 executable iSpeak
behavioral checks passed in both Native and LLVM** (26 compile/run PASS),
including the complete 16-bit round-trip, 546 golden stable IDs, 546 × 22
context facts in both languages, specialized formats and quoted controls.
The November 2 compiler integration baseline was not available for an exact
checkpoint-to-checkpoint qualification; this result does not verify its
compiler-owned diagnostic producers or output renderers.

`tests/bench_dynamic_context.sn`, `tests/bench_dynamic_empty.sn`,
`tests/bench_dynamic_quoted.sn` and `tests/bench_dynamic_escaped.sn` each
validate the result of **200,000** localized formatting calls. In matched
x86-64 Linux processes using the same Stage2 compiler, target and default
build settings, four interleaved runs of before/after executables produced:

| Fixture | Native pre / post (median) | LLVM pre / post (median) | Peak RSS pre / post |
| --- | --- | --- | --- |
| One nonempty symbolic fact | 1.255 / 1.100 s | 1.040 / 0.895 s | Native 84,928 / 84,928 KiB; LLVM 84,736 / 84,736 KiB |
| Empty context | 0.190 / 0.180 s | 0.105 / 0.100 s | Native 576 / 576 KiB; LLVM 384 / 384 KiB |

The nonempty formatter pre-reserves room for a typical clause and evaluates
identity-specific labels only for fields actually present. Empty contexts
retain the direct fixed-text return. Their peak RSS did not change.

The specialized backtick-quoted formatter now writes escaped bytes directly
through the existing canonical byte builder, instead of allocating an
intermediate vector and text. Four interleaved runs of the **same escaped
output fixture** before/after this change measured:

| Quoted fact with one embedded backtick (200k calls) | Intermediate copy | Direct append | Peak RSS before / after |
| --- | ---: | ---: | --- |
| Native | 1.280 s median | 1.010 s median | 122,432 / 84,928 KiB |
| LLVM | 1.270 s median | 0.990 s median | 122,240 / 84,736 KiB |

All four benchmarks check returned text, not just discarded computation.
Timing variation means these measurements **do not prove a universal speedup**.
The ordinary nonempty formatter's peak RSS **did not improve**. Each returned text
still entails storage on this compiler/runtime, which lacks automatic
reclamation for the relevant temporary values. Runtime allocation/reclamation
is owned outside `ispeak/**`; do not infer that this benchmark proves an
iSpeak-specific memory leak. The benchmark and its memory usage are explicitly
not a compiler-process RSS comparison.


## Context streaming and language-quality qualification (2026-11-03)

The canonical English and Arabic **context** formatting code now has a
caller-provided, reusable-buffer entry point:

```sultanc
import std.text.builder;
import ispeak.ispeak;

// Create once, outside a diagnostic loop.
let builder: mutable text_builder = new_text_builder_with_capacity(256 as uint);
// Inside the loop: no owned Text is required if the consumer can read the
// builder's bytes directly before the next clear.
text_builder_clear(&mutable builder);
let appended: bool = ألحق_تشخيص_بسياق(&mutable builder, language, key, context);
```

`ألحق_تشخيص_بسياق` accepts the same `معرف_رسالة` and
`حقائق_سياق_رسالة` as the established generic `سياق_تشخيص` path and
**shares its one implementation**; the owned `رسالة_تشخيص_بحجج` remains
unchanged for source compatibility, including the 21 specialized payload
variants. The builder is **appended to**, not cleared by iSpeak. A false
append may leave partial output; the caller must clear it before retrying.
Clearing retains capacity; `نص_باني_النص` still creates an owned copy.
The caller must consume the builder synchronously before its next reset.
The API is not a replacement for the compiler's structured diagnostic record,
source spans, notes, edits, JSON/LSP/terminal formatting or ownership.

Exact Native/LLVM executable tests compare both paths for all **546** live
identities with **all 22** context facts and both locales
(`check_reuse_context.sn`). `check_natural_quality.sn` pins English and
Arabic count comparisons (0, 1, 2, 3, 11 and 102 values), call-argument
and return-type pairs, and selected Arabic reason translations. Targeted
Arabic corrections address agreement of *عدد* with *المقدم*, type-comparison
wording, and several unsupported-instruction reason translations. These are
curated samples, **not independent linguistic certification** for every
possible identity/fact pairing. Arbitrary combinations require the compiler
producer to specify which facts are semantically valid. Contextual facts
which cannot be inferred from the producer must not be fabricated by iSpeak.
Plural-sensitive or gender-sensitive future templates require explicit typed
semantic properties and native-speaker review; the current invariant `عدد`
phrases do not assert a plural-selection engine.

### Matched streaming benchmark

All values are **three-run medians**, 200,000 outputs each, on the same
November 1 Stage2 compiler, Linux x86-64 target, same backend and default
settings. Both paths validate output (the reusable path checks the length
on every iteration, exact text on the first and final iterations, and full
parity across identities in the test above). Peak RSS is `time -f %M` KiB.

| Backend | Owned text time / RSS | Reused-buffer time / RSS |
| --- | --- | --- |
| Native | 1.35 s / 84,928 KiB | 0.46 s / 384 KiB |
| LLVM | 1.09 s / 84,736 KiB | 0.26 s / 256 KiB |

**Do not interpret this as a universal memory fix for returned texts.** The
owned interface still allocates per message, and these two paths have
different output ownership contracts. Improvements are attainable when a
consumer adopts synchronous streaming; compiler integration and any runtime
reclamation are outside `ispeak/**`.

`bench_catalog_cycle.sn` runs **10,000 scans × 546 active identities × two
locales** (10.92 million localized lookups) and checks a known checksum. On
this host Native median is 0.52 s at 320 KiB RSS, LLVM median is 0.64 s at
256 KiB RSS. `check_byte_space.sn` independently covers all 65,536 possible
16-bit wire values. These tests verify existing-identity repeated dispatch,
**not** thousands of distinct future active identities; do not inflate the
canonical ID list with synthetic/placeholder identities to simulate growth.

### Integration ownership and remaining proof

The current compiler's `compiler/diagnostics/record.sn` stores its canonical
`تشخيص` record, a **vector** of `حجة_تشخيص` values (including bilingual-text
values) and source spans/notes/fixes. `compiler/diagnostics/schema.sn`
defines the 22 field keys and compiler identities. These are not the same
type or storage representation as iSpeak's fixed optional-field context.
`compiler/diagnostics/localization.sn` and its consumers still emit compiler-
owned messages. Compiler-owned code must choose iSpeak identities, map the
existing argument record to typed facts without losing bilingual or
structural information, and connect the existing renderer without duplicating
records/identities or changing source-span/transport ownership. **No such
compiler integration is claimed or provided by this scoped archive.**
The authoritative November 2 compiler baseline and complete professional
Arabic-English linguistic review have not been verified here. Temporary
large-identity stress is documented below; it is not production coverage.

## Linguistic composition and identity-scale qualification (2026-11-03)

**Canonical owners remain unchanged.** `arguments.sn` owns fact quotation;
`en/errors.sn` and `ar/errors.sn` own localized clauses; `codes.sn` is the
single identity and stable-wire-ID owner. No compiler diagnostic records,
renderers, transports, or source producers were changed.

For dynamic contexts, an arbitrary combination of the 22 optional facts is
rendered as independent localized clauses. Comparisons of expected/actual
values remain a single clause and the Arabic count examples use an invariant
numeric noun phrase, so they do not require choosing different noun inflections
for 0, 1, 2, 3–10, 11–99 or larger numbers. Context strings with punctuation,
backticks, ASCII C0/DEL controls, non-ASCII characters, or zero length are
quoted in backticks using the existing byte-wise escape path. Simple identifiers
use the previous unquoted path. The underlying argument bytes are untouched.
A clause that introduces a reason now uses a neutral *“described as follows”*
construction / «على النحو الآتي» instead of assuming that arbitrary text is a
subordinate clause after English *because* or Arabic «لأن». Known Arabic
word-order and primitive-wording errors were also corrected.

These rules ensure that **fact boundaries and clause construction** remain
stable for the tested combinations, even for empty, punctuated or multiline
values. They do **not** establish that any possible value is *semantically
relevant* to any possible diagnostic identity, that arbitrary free-form text
is a valid natural-language sentence, or that the compiler has supplied the
correct facts. A compiler producer must define each diagnostic's allowed fact
contract; the compiler's terminal/JSON/LSP renderers still own transport
escaping and bidirectional-text isolation. No human editorial acceptance of
all 546 catalog messages is implied.

### Executed tests

The authorized November 1 Linux x86-64 Stage2 compiler (LLVM 19.1.7) compiled
and executed **17/17 iSpeak tests on Native and 17/17 on LLVM**. Added:

- `tests/check_context_opaque_values.sn`: exact English and Arabic sentences
  for a path containing periods, empty string, Arabic punctuation, newline,
  tab, embedded NUL/backtick and an arbitrary reason string. The owned-text API
  is checked against precise approved bytes, not just fact preservation.
- `tests/check_context_subsets.sn`: 262,144 distinct mixed field subsets
  across all 22 positions, with both languages and a single reusable text
  builder. Native execution: **3.79 s / 384 KiB peak RSS**. LLVM execution:
  **2.52 s / 256 KiB peak RSS**. This is a deterministic coverage sweep, **not
  a complete enumeration of all 2²² subsets** or a human language audit.

All previously supplied golden numeric identity, complete-field matrix,
Arabic/English comparison, specialized formatting, and quoted-C0 tests also
pass. Compiler-owned real diagnostic emissions and the authoritative
November 2 baseline remain outside this qualification.

### Thousands of identities — temporary expanded-catalog stress

A **temporary, non-delivered** source overlay added synthetic typed reason
variants and localized English/Arabic messages to the **existing canonical
`codes.sn` and locale match functions**, keeping all 546 production mappings
and historical reserved IDs intact. The existing four-nibble reverse decoder
was expanded to the tested set without introducing a registry or alternate
production architecture. Each compiled test exhaustively checked its active
IDs for numeric round trip and nonempty bilingual text, plus exact text for
its first and last synthetic identities. Observed on the same Stage2,
x86-64 Linux, default compilation settings:

| Active typed identities | Native compile | Native peak compiler RSS | LLVM compile | LLVM peak compiler RSS | Executed |
| ---: | ---: | ---: | ---: | ---: | --- |
| 1,536 | 7.36 s | 248,720 KiB | not run | — | Native PASS |
| 2,048 | 11.00 s | 294,672 KiB | not run | — | Native PASS |
| 3,072 | 21.86 s | 404,368 KiB | 20.87 s | 352,008 KiB | Native + LLVM PASS |
| 4,096 | 37.92 s | 497,296 KiB | 36.47 s | 435,544 KiB | **Native + LLVM PASS** |

### Follow-up: reproducing the former 4,096 timeout

The earlier 4,096-identity timeout is **not reproducible as a compiler error**
when the compilation is allowed to complete. On the available November 1
Stage2 x86-64 Linux compiler, both native and LLVM built and executed the
4,096-entry test successfully. The Native object-only build also passed
(38.00 s / 526,480 KiB peak compiler RSS / 5,211,848 bytes). The shared `check` path passed at
33.77 s / 290,320 KiB. In the actual driver
(`compiler/driver/compiler_driver.sn:622-721`), `check` executes the
Frontend, MSIR lowering and the canonical MSIR verification, then returns
**before** backend generation. The object and executable builds add backend
and output stages. The previously reported timeout is therefore consistent
with an external command time limit, not evidence of a failed compiler pass.

The generated source's dominant size-amplifying constructs are an expanded
ADT with 3,550 *temporary* variants; thousands of typed `match` arms in
the forward ID map, four-nibble reverse ID decoder, and each language's
message catalog. Focused import checks on exactly the **same** 4,096-entry
synthetic overlay, with the same compiler's `check` command, completed:

| Imported synthetic content | Compile time | Peak compiler RSS |
| --- | ---: | ---: |
| Expanded typed variants only (forward/reverse map kept at 546; compile-cost ablation, not runtime-valid for synthetic IDs) | 0.98 s | 54,416 KiB |
| Expanded variants plus 4,096-arm forward map, original 546-ID reverse map (compile-cost ablation) | 7.65 s | 78,992 KiB |
| Numeric codec and ADTs only | 13.47 s | 135,440 KiB |
| Codec plus English catalog | 24.36 s | 204,560 KiB |
| Codec plus Arabic catalog | 23.61 s | 204,560 KiB |
| Full bilingual 4,096-entry catalog | 33.77 s | 290,320 KiB |

These ablations hold the same 4,096 typed constructors and direct synthetic-ID
fixture constant; only the compiled match bodies change. They show the
forward ID match and four-level reverse-ID match contribute approximately
6.67 s and 5.82 s respectively in this run; the two 4,096-arm locale
matches further increase shared compilation cost. The compile-only ablated
maps intentionally lack complete synthetic reverse/forward behavior and
are **not** executable correctness tests.

This ablation identifies large generated typed-match functions as substantial
compile-cost contributors. **It does not isolate which part of the common
Frontend → MSIR → verify sequence takes those seconds.** That finer-grained
attribution requires approved compiler-owned pass instrumentation, not edits
in iSpeak. Source compilation, byte-ID roundtrip, bilingual message lookup,
linking and executable execution pass for 4,096 on both backends. The
**production** catalog remains exactly 546 identities; synthetic catalogs
were never inserted into shipped `codes.sn` or locale files.

**Reproduction:** Copy this archive's `ispeak/` to a throwaway compiler
workspace; retain the 546 existing assignments. Append named synthetic
`معرف_سبب.سبب_اختبار_توسع_N()` constructors for numeric IDs 567 through
4116 inclusive, and matching branches in the existing reason forward map,
existing four-nibble reverse map and existing EN/AR reason-message matches.
Leave the 111 public IDs unchanged. Use a throwaway test file that loops
through 1–4116, verifies 4,096 existing/synthetic successful ID roundtrips
and nonempty text in both languages, and checks exact first/last synthetic
messages. The temporary overlay used here had SHA-256
`447d23a277aa1b0f3ec1c853b2b3868aa56116b7ef7171074852a2ab35d1a47b`
for `codes.sn`. With the documented Stage2 CLI, run `check`, then
`--backend=native`/`--backend=llvm --target=x86_64-linux --emit=exe`,
and measure using `/usr/bin/time -f '%e %M'`. This is a temporary
measurement fixture, **not** a production catalog or replacement generator.
The exact November 2 compiler baseline was unavailable in this verification
environment; these numbers must not be reported as November 2 measurements.

### Fact contracts and output parity

See [`CONTEXT_CONTRACTS.md`](../internals/ispeak/context-contracts.md) for the ten supported
specialized payload kinds, the 21 matching public identities, compiler-source
evidence, exactly which related facts remain unknown, and the required
compiler-owner adapter work. The exhaustive specialized identity/payload
negative/positive test is `tests/check_specialized_contracts.sn`. The
`tests/check_reuse_context.sn` regression now exercises one *actually
reused* mutable builder across all 546 identities and both locales and
compares every rendered message against the owned-text API for all 22 fields
and empty facts. These tests validate typed-interface behavior; they cannot
establish semantic correctness of arbitrary combinations of generic facts.

### Focused re-verification on the available November 1 Stage2 compiler

All **18 iSpeak `check*.sn` fixtures compiled and executed successfully on
Native**, and all **18 compiled and executed successfully on LLVM**: **36/36**
compile + run PASS, with `x86_64-linux`, LLVM 19.1.7 and unchanged compiler
source/build settings. The two additional checks are the new specialized
contract matrix and the updated reusable-buffer/owned-text parity check.
The historical numeric-ID golden fixture also passed on both backends;
`codes.sn` and all production localized message sources were unchanged.

For the same production `check_reuse_context.sn` parity workload:

| Metric | Native | LLVM |
| --- | ---: | ---: |
| Compile duration (single run) | 2.48 s | 2.34 s |
| Compiler peak RSS | 173,072 KiB | 155,988 KiB |
| Executable size | 1,083,664 B | 579,720 B |
| Executable peak RSS | 12,200 KiB | 11,776 KiB |
| Executable result | PASS | PASS |

The *temporary* 4,096-ID executable comparison (same source, target, default
settings, single run per backend) was:

| Metric | Native | LLVM |
| --- | ---: | ---: |
| Compile duration | 37.92 s | 36.47 s |
| Compiler peak RSS | 497,296 KiB | 435,544 KiB |
| Executable size | 3,179,880 B | 2,040,888 B |
| Executable peak RSS | 2,624 KiB | 1,664 KiB |
| Executable result | PASS | PASS |

These are **November 1 compiler** measurements, not November 2. The actual
`SultanC_RSS_Optimized_20261102.zip` bytes were unavailable here.
November 2 compile times, executable sizes, RSS, and integration behaviors
are **BLOCKED by that missing compiler checkpoint**; no results have been
extrapolated from November 1. The new tests and docs are scoped to iSpeak;
no compiler-owned source was changed.

## Current-tree coverage update (2026-11-04)

Compared against `SultanC_Merged_Full_Checkpoint_20261104.zip`. Only
`ispeak/**` changed; no compiler source was touched.

| ID | Kind | Typed identity | Compiler source |
| ---: | --- | --- | --- |
| 567 | public | `نقل_من_مرجع` (`SULTANC-OWNERSHIP-MOVE-OUT-OF-REFERENCE`) | `compiler/frontend/semantic_pipeline.sn:1519` |
| 568–586 | reason | 19 `comptime.*` keys | `compiler/driver/compile_time.sn` |
| 587 | reason | `annotation.processing-not-implemented` | `compiler/frontend/semantic_pipeline.sn:850` |
| 588 | reason | `ssf.move-out-of-reference` | `compiler/frontend/semantic_pipeline.sn:1534` |

The new public identity has its own explanation group (`مفتاح_شرح.نقل_من_مرجع`)
and an identity-specific symbol clause in both languages. Every new reason
row, with exact English and Arabic text, is in `SEMANTIC_AUDIT.md` and is
asserted by `tests/check_new_reasons.sn`. `tests/check_id_golden.sn` freezes
the 22 new assignments; existing golden lines are unchanged.

**Same-fact key pairs (compiler-side cleanup, not changed here).** The driver
and the interpreter name three compile-time facts with different machine keys,
so iSpeak keeps one typed reason per key:

| Interpreter key (existing) | Driver key (new) |
| --- | --- |
| `comptime.argument-not-constant` (561) | `comptime.argument-not-closed` (569) |
| `comptime.call-depth-exceeded` (563) | `comptime.call-depth-exhausted` (570) |
| `comptime.result-escapes-evaluator` (562) | `comptime.result-not-closed` (580) |

## Remaining-gap update (2026-11-04)

Only `ispeak/**` changed. Compiler sources were read for meaning, never edited.

**1. Reason wording.** 253 reasons whose text was built from key words
("Subsystem: key words", e.g. «المفسر: وقت التشغيل، قراءة فقط موضع») now read
as sentences in both languages. Meaning comes from the compiler producer and
the machine key. `SEMANTIC_AUDIT.md` shows the new text for every reason.

**5–7. Public codes, the iSpeak registry and exact reason grouping.**
`registry.sn` gives every one of the 135 identities its public code, machine
kind, severity and class. The 112 identities that also exist in the compiler
are copied unchanged from `compiler/diagnostics/registry.sn` (the static check
compares them). The 23 identities below are proposals that exist only in iSpeak
until everything is proven; then they move to the compiler registry.
`معرف_خطأ_من_كود_عام` resolves a code back to its identity and
`شرح_كود_عام` serves `--explain <code>`. `tests/<lang>/ispeak/check_registry.sn` checks
that every code round-trips to its own identity (so codes are unique) and
freezes the proposed codes and classes.

Classes: `مستخدم` (user), `بيئة` (environment), `داخلي` (internal) as in the
compiler, plus the proposed `غير_منفذ` (unimplemented): a compiler limitation
that is neither the user's mistake nor an internal failure, the same split GCC
makes with its "sorry, unimplemented" diagnostics. It shows the full message
and its own explanation.

Naming follows the compiler's `SULTANC-<DOMAIN>-<MEANING>` form; the domain is
the subject that failed (`TARGET`, `BACKEND`, `OBJECT`, `LINK`, `HOST`,
`COMPTIME`, `INTERPRETER`).

| ID | Typed identity | Proposed public code | Class | Reasons |
| ---: | --- | --- | --- | ---: |
| 589 | `هدف_لا_يدعم_الميزة` | `SULTANC-TARGET-FEATURE-UNSUPPORTED` | environment | 4 |
| 590 | `بناء_غير_مدعوم_في_الخلفية` | `SULTANC-BACKEND-UNSUPPORTED-CONSTRUCT` | unimplemented | 26 |
| 591 | `مدى_ترميز_متجاوز` | `SULTANC-BACKEND-RANGE-EXCEEDED` | user | 9 |
| 592 | `رمز_ربط_غير_معرف` | `SULTANC-LINK-UNDEFINED-SYMBOL` | user | 3 |
| 593 | `رمز_ربط_مكرر` | `SULTANC-LINK-DUPLICATE-SYMBOL` | user | 3 |
| 594 | `موفر_وقت_تشغيل_مفقود` | `SULTANC-TARGET-RUNTIME-PROVIDER-MISSING` | environment | 4 |
| 595 | `نفاد_ذاكرة_المترجم` | `SULTANC-HOST-OUT-OF-MEMORY` | environment | 1 |
| 596 | `رمز_ربط_بنوع_غير_متطابق` | `SULTANC-LINK-SYMBOL-TYPE-MISMATCH` | user | 1 |
| 597 | `رمز_خارجي_غير_مدعوم` | `SULTANC-LINK-EXTERNAL-SYMBOL-UNSUPPORTED` | unimplemented | 1 |
| 598 | `إعادة_توطين_غير_مدعومة` | `SULTANC-OBJECT-RELOCATION-UNSUPPORTED` | unimplemented | 10 |
| 599 | `صيغة_كائن_غير_مدعومة_للهدف` | `SULTANC-OBJECT-FORMAT-UNSUPPORTED` | environment | 4 |
| 600 | `مخرج_ربط_غير_مدعوم` | `SULTANC-LINK-OUTPUT-UNSUPPORTED` | environment | 4 |
| 601 | `واجهة_استدعاء_غير_متاحة_للهدف` | `SULTANC-TARGET-ABI-UNAVAILABLE` | environment | 4 |
| 602 | `حد_صيغة_كائن_متجاوز` | `SULTANC-OBJECT-FORMAT-LIMIT-EXCEEDED` | user | 4 |
| 603 | `حد_وقت_الترجمة_متجاوز` | `SULTANC-COMPTIME-LIMIT-EXCEEDED` | user | 4 |
| 604 | `أثر_ممنوع_وقت_الترجمة` | `SULTANC-COMPTIME-PROHIBITED-EFFECT` | user | 1 |
| 605 | `فشل_تنفيذ_وقت_الترجمة` | `SULTANC-COMPTIME-EVALUATION-FAILED` | user | 2 |
| 606 | `عملية_غير_مدعومة_وقت_الترجمة` | `SULTANC-COMPTIME-UNSUPPORTED-OPERATION` | user | 2 |
| 607 | `ناتج_وقت_الترجمة_غير_ثابت` | `SULTANC-COMPTIME-RESULT-NOT-CONSTANT` | user | 5 |
| 608 | `تخطيط_غير_متاح_وقت_الترجمة` | `SULTANC-COMPTIME-LAYOUT-UNAVAILABLE` | user | 2 |
| 609 | `عدد_حجج_وقت_الترجمة_غير_مطابق` | `SULTANC-COMPTIME-ARGUMENT-COUNT-MISMATCH` | user | 1 |
| 610 | `نداء_خارجي_تفسير_غير_مدعوم` | `SULTANC-INTERPRETER-UNSUPPORTED-EXTERNAL-CALL` | user | 1 |
| 611 | `بدائي_آلة_تفسير_غير_مدعوم` | `SULTANC-INTERPRETER-UNSUPPORTED-MACHINE-PRIMITIVE` | user | 1 |

`خطأ_عام_للسبب` (end of `codes.sn`) maps 141 of the 456 reasons to the
identity whose meaning they match exactly. Nothing is grouped loosely: where a
reason fit no identity it received its own (for example
`link.wasm.symbol-type-mismatch` → `SULTANC-LINK-SYMBOL-TYPE-MISMATCH`,
relocations → `SULTANC-OBJECT-RELOCATION-UNSUPPORTED`, and each kind of
compile-time failure has its own `SULTANC-COMPTIME-*` code). Reasons that map
to identities the compiler already has:

| Existing identity | Public code | Reasons |
| --- | --- | ---: |
| `تعبير_وقت_الترجمة_غير_ثابت` | `SULTANC-COMPTIME-NON-CONSTANT` | 4 |
| `حالة_تفسير_غير_صالحة` | `SULTANC-INTERPRETER-INVALID-RUNTIME-STATE` | 5 |
| `حجة_بدائي_تفسير_غير_صالحة` | `SULTANC-ICE-INTERPRETER-PRIMITIVE-ARGUMENT` | 1 |
| `خلفية_مترجم_غير_متاحة` | `SULTANC-BACKEND-UNAVAILABLE` | 6 |
| `ذاكرة_تفسير_تالفة` | `SULTANC-ICE-INTERPRETER-CORRUPT-MEMORY` | 4 |
| `عدد_حجج_بدائي_تفسير_غير_مطابق` | `SULTANC-ICE-INTERPRETER-PRIMITIVE-ARGUMENT-COUNT` | 1 |
| `فخ_تفسير` | `SULTANC-INTERPRETER-TRAP` | 1 |
| `فشل_وقت_الترجمة_مطلوب` | `SULTANC-COMPTIME-FAILED` | 1 |
| `فهرس_تفسير_خارج_الحدود` | `SULTANC-INTERPRETER-INDEX-OUT-OF-BOUNDS` | 4 |
| `نص_تفسير_غير_صالح` | `SULTANC-INTERPRETER-INVALID-UTF8` | 1 |
| `نقطة_دخول_مفقودة` | `SULTANC-ENTRYPOINT-NOT-FOUND` | 6 |
| `نوع_تكراري_لانهائي` | `SULTANC-TYPE-INFINITE-RECURSION` | 1 |
| `نوع_قيمة_تفسير_غير_متوقع` | `SULTANC-ICE-INTERPRETER-UNEXPECTED-VALUE-KIND` | 7 |
| `هدف_مترجم_غير_معروف` | `SULTANC-TARGET-UNKNOWN` | 2 |

The other 315 reasons return `لا_شيء`: the producer keeps its identity. For
backend, MSIR, lowering and interpreter-internal reasons that is a genuine
internal failure (ICE). `tests/check_reason_grouping.sn` freezes all 456
decisions, and `SEMANTIC_AUDIT.md` shows the decision in its last column.

**8–9. Explanations.** The 30 shared groups (with four wrong assignments:
generic warning → interpreter, internal failure → CLI, unknown input type →
name resolution, argument/return mismatch → "MSIR facts") are replaced by one
explanation per user or environment identity (113). The 22 internal identities
share one explanation, because the user action is the same. Each explanation
stays within 220 UTF-8 bytes.

**10. Runtime failures of compiled programs.** `runtime.sn` mirrors the four
kinds in `compiler/runtime/failure_identity.sn` (bounds, argument range,
invalid operation, I/O). `عنوان_فشل_تشغيل` and `نص_فشل_تشغيل` return the
title and failure text in both languages. They have no wire IDs yet; add them
only if the runtime will store a numeric failure code in the binary.

**11. Count agreement.** Count comparisons are verbal sentences in both
languages; the count is the object of the verb, so Arabic uses the accusative
dual, and zero becomes a negated verb:

| Count | English | Arabic |
| ---: | --- | --- |
| 0 | expects no arguments | لا تتوقع أي حجة |
| 1 | expects 1 argument | تتوقع حجة واحدة |
| 2 | expects 2 arguments | تتوقع حجتين |
| 3–10 (last two digits) | expects N arguments | تتوقع N حجج |
| otherwise | expects N arguments | تتوقع N حجة |

| Identity | Arabic sentence (2 vs 3) |
| --- | --- |
| `عدد_حجج_غير_مطابق` | الدالة تتوقع حجتين، لكن النداء يمرر 3 حجج. |
| `عدد_حجج_بدائي_تفسير_غير_مطابق` | العملية البدائية تتوقع حجتين، لكن النداء يمرر 3 حجج. |
| `عدد_حمولات_نمط_غير_مطابق` | المنشئ يحمل حمولتين، لكن النمط يربط 3 حمولات. |

English: "The function expects 2 arguments, but the call passes 3 arguments."
The verb agrees with its subject (الدالة تتوقع / النداء يمرر). The specialized
English payload no longer prints "1 arguments". Single-sided counts keep the
invariant «عدد … هو N» wording, which needs no agreement.
`tests/check_count_agreement.sn` pins 0, 1, 2, 3, 10, 11, 99, 100, 102, 103,
110 and 111 in both languages. Arabic gender is a property of each counted
noun (`مؤنث_معدود_عربي`); both current nouns are feminine.

**Verification state (2026-11-04).** Built from `SultanC_Full_20261104.zip`
with `./build.sh stage2 --enable-llvm` (LLVM 20.1.2, Linux x86-64, Stage2 in
2 m 22 s). All 22 `tests/check*.sn` fixtures pass every step: `check`, Native
build, Native run, LLVM build and LLVM run (110/110). Two fixes came out of
this run: `كود_الخطأ_العام` now copies the code text instead of moving a text
field out of the record (`SULTANC-OWNERSHIP-PARTIAL-MOVE`), and the
count-agreement test builds its context from parameters like the other tests
(`SULTANC-BORROW-REFERENCE-ESCAPE`). As a guard against tests that cannot
fail, a deliberately wrong Arabic reason text and a wrong grouping were
planted in a scratch copy: `check_new_reasons` failed with 21 and
`check_reason_grouping` with 1, as expected. The `bench_*.sn` fixtures were
not run. Static checks (coverage, unique IDs and codes, registry mirror of
the compiler) also pass.
