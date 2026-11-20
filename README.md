<div dir="rtl">

<div align="center">
<img src="https://raw.githubusercontent.com/sultan-language/sultanc-vscode/main/icons/sultanc-logo.svg" alt="SultanC" width="100">

**العربية** | [English](docs/en/README.md)

# السلطان

**لغة برمجة أنظمة تُكتب بالعربية، ومترجمها مكتوب بها.**

<p>
<a href="https://github.com/sultan-language/sultanc/actions/workflows/ci.yml"><img src="https://github.com/sultan-language/sultanc/actions/workflows/ci.yml/badge.svg" alt="CI"></a>
</p>
<p>
<img src="https://img.shields.io/badge/self--hosted-yes-1a7f37?style=for-the-badge&labelColor=0d1117" alt="Self-hosted">
<img src="https://img.shields.io/badge/Linux-x86__64%20%7C%20arm64%20%7C%20+8-1a7f37?style=for-the-badge&labelColor=0d1117&logo=linux&logoColor=white" alt="Linux">
<img src="https://img.shields.io/badge/macOS-arm64%20%7C%20x86__64-1a7f37?style=for-the-badge&labelColor=0d1117&logo=apple&logoColor=white" alt="macOS">
<img src="https://img.shields.io/badge/Windows-x64%20%7C%20arm64-1a7f37?style=for-the-badge&labelColor=0d1117" alt="Windows">
<img src="https://img.shields.io/badge/FreeBSD%20%7C%20Android%20%7C%20WASI-yes-1a7f37?style=for-the-badge&labelColor=0d1117" alt="FreeBSD, Android, WASI">
<img src="https://img.shields.io/badge/source-Arabic%20%2B%20English-8957e5?style=for-the-badge&labelColor=0d1117" alt="Language">
<img src="https://img.shields.io/badge/license-MIT-8b949e?style=for-the-badge&labelColor=0d1117" alt="License">
</p>

[التثبيت](#التثبيت) &nbsp;·&nbsp; [اللغة](#اللغة) &nbsp;·&nbsp; [المكتبة القياسية](#المكتبة-القياسية) &nbsp;·&nbsp; [الأهداف](#الأهداف) &nbsp;·&nbsp; [التوثيق](#التوثيق)

</div>

<br>

السلطان لغة برمجة أنظمة تُترجَم إلى شيفرة آلة أصلية، وتحمي الذاكرة بقواعد الملكية، وفيها أنواع البيانات الجبرية ومطابقة الأنماط والخيوط. كلماتها المفتاحية متاحة بالعربية والإنجليزية، وكلتاهما صياغة كاملة للغة نفسها، فيمكن الكتابة بأيٍّ منهما. ومترجم السلطان مكتوب بالسلطان، وهو يترجم نفسه: ويجتاز كل بناء فحص الاستضافة الذاتية، الذي يتحقق من أن جيلين متتاليين من المترجم متطابقان بايتًا ببايت (Stage3 == Stage4).

<br>

<table>
<tr><th width="50%">العربية</th><th width="50%">English</th></tr>
<tr>
<td valign="top" dir="rtl">

```
عرف الرئيسية(): عدد {
    دع س: عدد = 40;
    دع ص: عدد = 2;
    أرجع س + ص;
}
```

</td>
<td valign="top">

<div dir="ltr">

```
def main(): int {
    let x: int = 40;
    let y: int = 2;
    return x + y;
}
```

</div>

</td>
</tr>
</table>

البرنامجان متطابقان في شجرة النحو المجردة (AST) وفي المعنى وفي رسائل التشخيص، ولا يختلفان إلا في الكلمات المفتاحية وأسماء المعرّفات.

لماذا وُجد السلطان؟ اقرأ [قصته](docs/ar/story.md).

## التثبيت

<div dir="ltr">

```sh
git clone https://github.com/sultan-language/sultanc.git && cd sultanc
./build.sh                          # Stage0 بلغة C -> Stage1 -> Stage2 (يتطلب LLVM)
sudo cp build/sultanc /usr/local/bin/sultanc
```

</div>

| طريقة البناء | الأمر | ما تحتاجه |
|---|---|---|
| الإقلاع الذاتي (bootstrap) من C، وهي الطريقة الافتراضية | `./build.sh` | مترجم C وLLVM |
| بذرة WebAssembly (seed) | `./build.sh --bootstrap=wasm` | مترجم C فقط |
| إعادة البناء بمترجم مبني مسبقًا (نحو دقيقة) | `./build.sh --bootstrap=existing` | نسخة مبنية من `build/sultanc` |
| بناء المترجم مع خلفية LLVM | `./build.sh --enable-llvm` | LLVM من الإصدار 19 إلى 23 |
| فحص الاستضافة الذاتية | `./build.sh verify` | يتحقق من تطابق Stage3 وStage4 بايتًا ببايت |

> على macOS، إذا ثبّتَّ LLVM عبر Homebrew، فنفّذ قبل البناء: `export LLVM_CONFIG="$(brew --prefix llvm)/bin/llvm-config"`
> على Windows، ابنِ بالأمر: `./build.sh --bootstrap=c --enable-llvm --backend=llvm`

<div dir="ltr">

```sh
sultanc main.sn -o app && ./app
```

</div>

## اللغة

- **فحص الملكية والاستعارة والنقل** في وقت الترجمة، قبل توليد الشيفرة
- **أنواع البيانات الجبرية ومطابقة الأنماط**، مع التحقق من تغطية جميع الحالات
- **الأنواع العامة (generics)** و**التقييم في وقت الترجمة** (`وقت_الترجمة`)
- **الأخطاء قيمٌ عادية**: النوع `نتيجة`، وتمرير الخطأ إلى المستدعي بالعامل `؟`، وإيقاف البرنامج بـ `إنهيار`
- **الخيوط والمزامنة**: إنشاء الخيوط وانتظارها، والعمليات الذرية، و`مشترك` (Arc)، و`قفل` (Mutex)، و`شرط_انتظار` (Condvar)، و`مرة` (Once)، والقنوات، مع فحص في وقت الترجمة لما يجوز للخيط أن يلتقطه من القيم
- **العربية والإنجليزية** صياغتان متكافئتان للشيفرة المصدرية، وكل رسالة تشخيص متاحة باللغتين
- **تمثيل وسيط مُتحقَّق منه (MSIR)**: يمر ناتج كل مرحلة على مدقق قبل أن يصل إلى أي خلفية
- **ملفات تنفيذية أصلية** بصيغ ELF وMach-O وPE/COFF وWebAssembly، يربطها المترجم ببرامج ربط (linkers) خاصة به، ولا تحتاج إلى مكتبة libc على Linux وFreeBSD

<div dir="ltr">

```sh
sultanc check main.sn                       # الفحص فقط، دون بناء
sultanc run main.sn                         # تشغيل البرنامج بالمفسر دون بناء ملف تنفيذي
sultanc --backend=llvm main.sn -o app       # البناء بخلفية LLVM (بمستوى التحسين O2)
sultanc --target=aarch64-linux-gnu main.sn  # الترجمة المتقاطعة لمنصة أخرى
sultanc --lang=en check main.sn             # رسائل التشخيص بالإنجليزية (العربية هي الافتراضية)
sultanc --diagnostic-format=json check main.sn
sultanc targets                             # عرض جميع الأهداف المدعومة
```

</div>

يشغّل المفسرُ تمثيلَ MSIR نفسه الذي يبني منه المترجمُ الملفاتِ التنفيذية، ويستخدم المكتبة القياسية نفسها، ويعمل على Linux وmacOS وWindows وFreeBSD.

## المكتبة القياسية

| المجال | أبرز ما يقدمه |
|---|---|
| النصوص والتنسيق | نصوص UTF-8، وتقطيع نصوص Unicode وتوحيدها (normalization)، و`std.fmt` لتنسيق النصوص بقوالب على طريقة Rust تُفحص في وقت الترجمة |
| الحاويات | المتجهات مع التحكم في سعتها، والقواميس (maps)، والمجموعات (sets)، والفرز |
| الملفات والعمليات | الملفات، والمجلدات، والروابط الرمزية، وأقفال الملفات، وتشغيل العمليات الفرعية مع قراءة مخرجاتها أولًا بأول، ومتغيرات البيئة |
| الشبكات | TCP وUDP ومقابس Unix وDNS وTLS 1.3، وHTTP/1.1 وHTTP/2 من جهة العميل والخادم |
| صيغ البيانات والأرشفة | JSON وTOML وCSV، وأرشيفات zip وtar |
| الضغط | gzip/deflate وzstd وxz/LZMA2 |
| التشفير ودوال التجزئة | SHA-2 وX25519 وEd25519 وChaCha20-Poly1305 وAES-GCM وXXH64 وCRC |
| التزامن | `std.thread`، و`std.sync` (العمليات الذرية، و`مشترك`، و`قفل`، و`قفل_قراءة_كتابة`، و`شرط_انتظار`، و`مرة`، والقنوات) |

وتجد القائمة الكاملة، مقارنةً بمكتبتي Rust وGo، في صفحة [تغطية المكتبة القياسية](docs/ar/reference/std-coverage.md).

## الأهداف

| نظام التشغيل | المعماريات |
|---|---|
| Linux | x86_64، aarch64، riscv64، loongarch64، ppc64le، s390x، mips64el، sparcv9، i686، armv7 |
| macOS | arm64، x86_64 |
| Windows | x64، arm64 |
| FreeBSD | x86_64، aarch64، riscv64 |
| Android | x86_64، aarch64، armv7 |
| WebAssembly | WASI (wasm32)، وwasm32 وwasm64 دون نظام تشغيل (freestanding) |

للمعماريتين x86_64 وarm64 خلفيتان أصليتان، وتُبنى بقية الأهداف عبر LLVM. التفاصيل في صفحة [الأهداف](docs/ar/reference/targets.md).

## التوثيق

| | |
|---|---|
| [MSIR وتوليد الشيفرة](docs/ar/internals/msir-and-codegen.md) | كيف تتحول الشيفرة المصدرية إلى شيفرة آلة أصلية |
| [بنية المترجم](docs/ar/internals/compiler-architecture.md) | أجزاء المترجم وطريقة عملها |
| [الإقلاع الذاتي](docs/ar/internals/bootstrap.md) | Stage0 المكتوب بلغة C، وبذرة WebAssembly |
| [تدفق الأمان](docs/ar/internals/safety-flow.md) | كيف يفحص المترجم الملكية والاستعارة |
| [نظام الأنواع](docs/ar/reference/type-system.md) | الأنواع، واستنتاج الأنواع، والأنواع العامة، والتحويل بين الأنواع |
| [الملكية والإتلاف](docs/ar/reference/ownership.md) | النقل، والإتلاف، والقيم المؤقتة، وسعة المتجهات |
| [الأخطاء](docs/ar/reference/errors.md) | النوع `نتيجة`، والعامل `؟`، و`إنهيار`، ورسائل الأخطاء بلغة المستخدم |
| [التزامن](docs/ar/reference/concurrency.md) | الخيوط، والعمليات الذرية، والمزامنة |
| [التنسيق](docs/ar/reference/formatting.md) | تنسيق النصوص بالوحدة `std.fmt` |
| [المفسر](docs/ar/internals/interpreter.md) | تشغيل البرامج بالأمر `sultanc run` دون بنائها |
| [اعتماديات المنصة](docs/ar/reference/platform-dependencies.md) | ما تحتاجه المكتبة القياسية من كل نظام تشغيل |
| [المصطلحات العربية](docs/ar/reference/arabic-terminology.md) | المصطلحات العربية المعتمدة في اللغة والمترجم |
| [الأمثلة](docs/ar/contributing/examples.md) · [الاختبارات](docs/ar/contributing/tests.md) | مجلد الأمثلة `examples/` وأداة تشغيل الاختبارات |

## الحالة

المترجم مستضاف ذاتيًا على `x86_64-linux` و`arm64-darwin`، والدليل على ذلك أن الأجيال المتتالية من المترجم تنتج ملفات متطابقة بايتًا ببايت، سواء بدأ البناء من Stage0 المكتوب بلغة C أو من بذرة WebAssembly. ويبني التكامل المستمر (CI) المترجمَ بجميع طرق البناء على Linux وmacOS وWindows (x64 وarm64).

## الترخيص

[ترخيص MIT](LICENSE)

</div>
