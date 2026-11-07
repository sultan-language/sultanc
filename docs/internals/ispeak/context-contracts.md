# iSpeak Contextual Facts

The compiler producer decides which facts are true of a diagnostic. iSpeak owns the type and the
wording of those facts after the producer has chosen an identity. This page records the contract
between the two: which specialized payloads iSpeak accepts for which identities, how the generic
context behaves, and what iSpeak deliberately does not validate. It is written for people who add a
diagnostic or a producer.

The two contracts are different and must not be equated. The specialized payloads are checked by
iSpeak; the facts that a given compiler producer actually supplies are the producer's responsibility.

## Specialized payloads

`compiler/ispeak/format/facts.sn` provides ten typed specialized payload alternatives of
`حقائق_رسالة`. `compiler/ispeak/locales/en/templates.sn` and `compiler/ispeak/locales/ar/templates.sn`
are the canonical identity-to-payload validators: a specialized payload is accepted only for its listed
identities, and the acceptance is the same in both languages. Any other specialized combination returns
`اختياري.لا_شيء()`. There is no parallel contract registry and no duplicate locale dispatcher.

`tests/<lang>/ispeak/check_specialized_contracts.sn` tests the accepted combinations exhaustively:
21 identities against 10 specialized alternatives in each language, plus reason rejections.

| iSpeak message family and exact diagnostic identities | Permitted specialized facts | What the producer provides |
| --- | --- | --- |
| `اسم_غير_معروف` | `اسم_غير_معروف`: referenced name (text) | `compiler/frontend/semantic_pipeline.sn` emits this diagnostic with a name-stage reason. A source name string is not always available, so the producer supplies the actual name or selects the generic context. |
| `نوعان_غير_متوافقين` | `نوعان_غير_متوافقين`: expected and actual type text | The compiler's spelling is `نوعين_غير_متوافقين`, and some producers supply type IDs, not display text. No canonical adapter from a type ID to localized text exists yet. |
| `عدد_حجج_غير_مطابق` | `عدد_حجج_غير_مطابق`: unsigned expected and actual counts | `compiler/frontend/semantic_pipeline.sn` emits both counts. `compiler/msir/lowering/diagnostics.sn` emits this identity with a reason but without counts, and counts are never fabricated. |
| `خيار_مترجم_غير_معروف`, `قيمة_خيار_مترجم_مفقودة`, `لغة_تشخيص_مترجم_غير_صالحة`, `صيغة_تشخيص_مترجم_غير_صالحة`, `لون_تشخيص_مترجم_غير_صالح`, `معامل_موضعي_غير_متوقع`, `مصدر_رئيسي_متعدد`, `هدف_مترجم_غير_معروف` | `نص_محدد`: one text value | `compiler/diagnostics/message.sn` reads `القيمة` for most of them and `الهدف` for an unknown target. The adapter preserves that distinction; the existence of a text value is not evidence of its meaning. |
| `مكتبة_إل_إل_في_إم_للهدف_مفقودة`, `رمز_إل_إل_في_إم_غير_محلول` | `ربط_إل_إل_في_إم`: target and symbol text | Adapter in `compiler/diagnostics/message.sn`; producer in `compiler/link/diagnostic_translation.sn`. The specialized shape requires both values. |
| `فشل_وقت_الترجمة_مطلوب` | `فشل_وقت_الترجمة`: text reason | `compiler/driver/compiler_driver.sn` stores the failure's text as `القيمة`; it is not an iSpeak identity. |
| `بيان_حزمة_غير_صالح` | `بيان_حزمة`: path text, optional byte index and optional translated reason | `compiler/driver/compiler_driver.sn` and `compiler/diagnostics/message.sn`. The index and reason can be absent, but the typed shape requires a path. |
| `تهجئة_كلمة_محجوزة_غير_معروفة` | `اقتراح_كلمة`: replacement text | `compiler/diagnostics/message.sn` includes the optional `الاستبدال`. When none was produced, no suggestion is invented and fixed or generic wording is used. |
| `خطأ_نحوي` | `توقع_نحوي`: expected text, optional actual text | `compiler/diagnostics/message.sn`. The compiler may know only the expected form, in which case the actual form is omitted and not fabricated. |
| `بدائي_مضيف_تفسير_غير_مدعوم`, `جسر_نص_مضيف_تفسير_غير_متاح`, `حجة_بدائي_تفسير_غير_صالحة`, `عدد_حجج_بدائي_تفسير_غير_مطابق` | `بدائي_تفسير`: primitive name, optional index and optional expected and actual unsigned values | `compiler/diagnostics/message.sn`. The producer decides which optional values are pertinent. |

Acceptance of an identity by a specialized shape does not imply that every field is available at every
producer. Some compiler diagnostics are emitted with only phase and reason facts, without the detailed
payload above. The typed iSpeak API rejects unrelated specialized shapes, so `بلا_حجج` is not
guaranteed to succeed for every specialized identity. The fixed lookup `رسالة_تشخيص` is always
available.

## Generic context

`حقائق_سياق_رسالة` has exactly the 22 named optional argument fields of `مفتاح_حجة_تشخيص`
(`compiler/diagnostics/schema.sn`). The canonical compiler record accepts text, bilingual text,
unsigned, signed and boolean values (`compiler/diagnostics/record.sn`). iSpeak's `قيمة_حجة_رسالة`
accepts referenced text, unsigned, signed, boolean and a typed reason identity. The two sets are not a
byte-for-byte match: bilingual text in the compiler needs a compiler-owned language-selection adapter,
and the compiler's textual contextual reason is not silently converted to a typed reason.

Generic facts may be used with all 645 identities and reasons (142 public identities and 503 reasons)
and with auxiliary text. The formatter therefore reports supplied facts faithfully, but this does not
prove that they are semantically permissible for a particular identity. `check_dynamic_matrix.sn` and
`check_reuse_context.sn` test preservation and output parity across the whole matrix of identities and
fields, not the semantic validity of each identity and field pair.

The producers narrow the generic contract in practice:

- `compiler/msir/diagnostic_translation.sn`: `المرحلة` and `السبب` are text; `الدالة`, `الكتلة`, `الفهرس`
  and `القيمة` can be unsigned IDs; comparisons use two unsigned type or value identifiers.
- `compiler/msir/lowering/diagnostics.sn`: a contextual `السبب` and `المرحلة` plus optional numeric
  symbol, function, value and block facts. Not every call-site error has counts.
- `compiler/frontend/semantic_pipeline.sn`: the count error of the `type` phase has unsigned `المتوقع`
  and `الموجود`, and the type error has an additional argument index, so the two contracts differ.
- `compiler/driver/compiler_driver.sn`: a compile-time error may carry text `القيمة`, not a numeric
  reason identity.

## What iSpeak does not validate

iSpeak does not reject arbitrary generic contexts, reinterpret a numeric field as a type name, or claim
that every possible set of 22 facts forms a meaningful sentence. A per-producer map from
`معرف_تشخيص` to the permitted `مفتاح_حجة_تشخيص` set, with types and optionality, would belong in
`compiler/diagnostics/**` with participation from the `compiler/frontend/**`, `compiler/msir/**`,
`compiler/link/**` and `compiler/driver/**` producers. It does not exist yet. Source spans, notes,
help, fixes, LSP and JSON presentation, and `--lang` stay with their current compiler owners, and a
canonical conversion for unsigned type IDs and bilingual-text facts is part of that future work.

## Verification

`check_specialized_contracts.sn` executes the complete known matrix of specialized identities and
payloads in English and Arabic. Its negative cases ensure that adding a case to one locale without
updating the other fails.

`check_reuse_context.sn` passes one mutable text builder through all 645 identities and reasons in both
languages, clearing it between messages. It compares every result byte for byte to the owned-text API,
including all 22 present fields and empty contexts.

These are checks of the iSpeak interface. They do not cover producer migration or a universal proof of
Arabic grammar.

## See also

- [iSpeak](README.md)
- [Reason audit](../../history/ispeak-semantic-audit.md)
- [Compiler Architecture](../compiler-architecture.md)
