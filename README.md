<div dir="rtl">

<div align="center">
<img src="https://raw.githubusercontent.com/sultan-language/sultanc-vscode/main/icons/sultanc-logo.svg" alt="SultanC" width="100">

**العربية** | [English](docs/en/README.md)

# السلطان

**لغة برمجة أنظمة مستضافة ذاتيًا، مكتوبة بالعربية.**

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

السلطان لغة أنظمة تُترجَم إلى شيفرة أصلية (native)، وفيها أمان يراعي الملكية (ownership)، وأنواع البيانات الجبرية، ومطابقة الأنماط، والخيوط. وكلماتها المفتاحية موجودة بالعربية والإنجليزية، واللغتان واجهتان متكافئتان قابلتان للتبادل للصياغة نفسها. والمترجم مكتوب بلغة السلطان ويترجم نفسه: كل بناء يجتاز فحص الاستضافة الذاتية الذي يطابق الناتج بايتًا ببايت (Stage3 == Stage4).

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

شجرة الصياغة المجردة (AST) نفسها، والدلالات نفسها، ورسائل التشخيص نفسها؛ ولا تتغير إلا الكلمات المفتاحية والمعرّفات.

## التثبيت

<div dir="ltr">

```sh
git clone https://github.com/sultan-language/sultanc.git && cd sultanc
./build.sh                          # Stage0 بلغة C -> Stage1 -> Stage2 (يتطلب LLVM)
sudo cp build/sultanc /usr/local/bin/sultanc
```

</div>

| الطريقة | الأمر | المتطلبات |
|---|---|---|
| الإقلاع الذاتي (bootstrap) من لغة C (الافتراضي) | `./build.sh` | مترجم C وLLVM |
| بذرة (seed) WebAssembly | `./build.sh --bootstrap=wasm` | مترجم C فقط |
| إعادة البناء من مترجم موجود | `./build.sh --bootstrap=existing` | `build/sultanc` مبنيّ مسبقًا (نحو دقيقة) |
| مع تضمين خلفية LLVM | `./build.sh --enable-llvm` | LLVM من الإصدار 19 إلى 23 |
| فحص الاستضافة الذاتية | `./build.sh verify` | Stage3 == Stage4، بايتًا ببايت |

> على macOS مع LLVM المثبت عبر Homebrew: نفّذ `export LLVM_CONFIG="$(brew --prefix llvm)/bin/llvm-config"` قبل البناء.
> على Windows: `./build.sh --bootstrap=c --enable-llvm --backend=llvm`.

<div dir="ltr">

```sh
sultanc main.sn -o app && ./app
```

</div>

## اللغة

- **فحص الملكية والاستعارة والنقل**، مع تحقق ساكن (static) قبل توليد الشيفرة
- **أنواع البيانات الجبرية ومطابقة الأنماط**، مع التحقق من أن الحالات كلها مغطاة
- **الأنواع العامة (generics)** و**التقييم في وقت الترجمة** (`وقت_الترجمة`)
- **الأخطاء قيم**: قيم `نتيجة`، وتمرير الخطأ بـ `؟`، و`إنهيار`
- **الخيوط والمزامنة**: الإنشاء والانتظار (spawn/join)، والعمليات الذرية، و`مشترك` (Arc)، و`قفل` (Mutex)، و`شرط_انتظار` (Condvar)، و`مرة` (Once)، والقنوات، مع فحص في وقت الترجمة لما يجوز للخيط أن يلتقطه
- **العربية والإنجليزية** واجهتان متكافئتان للشيفرة المصدرية، بما في ذلك كل رسائل التشخيص
- **تمثيل وسيط متحقق منه (MSIR)**: تمر كل مرحلة على مدقق قبل أن تصل إلى أي خلفية
- **ناتج أصلي**: ELF وMach-O وPE/COFF وWebAssembly، مع برامج ربط (linkers) خاصة بالمترجم؛ ولا حاجة إلى libc على Linux وFreeBSD

<div dir="ltr">

```sh
sultanc check main.sn                       # فحص الأنواع فقط
sultanc run main.sn                         # التشغيل بالمفسر دون ترجمة
sultanc --backend=llvm main.sn -o app       # خلفية LLVM (بمستوى O2)
sultanc --target=aarch64-linux-gnu main.sn  # الترجمة لمنصة أخرى
sultanc --lang=en check main.sn             # رسائل التشخيص بالإنجليزية (العربية افتراضيا)
sultanc --diagnostic-format=json check main.sn
sultanc targets                             # عرض كل الأهداف
```

</div>

يشغّل المفسر MSIR نفسه والمكتبة القياسية نفسها التي يستخدمها المترجم، على Linux وmacOS وWindows وFreeBSD.

## المكتبة القياسية

| المجال | أبرز ما فيه |
|---|---|
| النصوص والتنسيق | نصوص UTF-8، وتقطيع Unicode وتوحيده، و`std.fmt` (نصوص تنسيق على طريقة Rust، تُفحص في وقت الترجمة) |
| الحاويات | متجهات ذات سعة، وجداول تقابل (maps)، ومجموعات (sets)، وفرز |
| الملفات والعمليات | الملفات، والمجلدات، وروابط نظام الملفات، وأقفال الملفات، والعمليات الفرعية ذات الدفق المستمر، والبيئة |
| الشبكات | TCP وUDP ومقابس Unix وDNS وTLS 1.3 وHTTP/1.1 وHTTP/2 (عميل وخادم) |
| صيغ البيانات | JSON وTOML وCSV وzip وtar |
| الضغط | gzip/deflate وzstd وxz/LZMA2 |
| التشفير والتجزئة | SHA-2 وX25519 وEd25519 وChaCha20-Poly1305 وAES-GCM وXXH64 وCRC |
| التزامن | `std.thread`، و`std.sync` (العمليات الذرية، و`مشترك`، و`قفل`، و`قفل_قراءة_كتابة`، و`شرط_انتظار`، و`مرة`، والقنوات) |

انظر [تغطية المكتبة القياسية](docs/ar/reference/std-coverage.md) للاطلاع على القائمة الكاملة، مقارنةً بـ Rust وGo.

## الأهداف

| نظام التشغيل | المعماريات |
|---|---|
| Linux | x86_64, aarch64, riscv64, loongarch64, ppc64le, s390x, mips64el, sparcv9, i686, armv7 |
| macOS | arm64, x86_64 |
| Windows | x64, arm64 |
| FreeBSD | x86_64, aarch64, riscv64 |
| Android | x86_64, aarch64, armv7 |
| WebAssembly | WASI (wasm32)؛ وwasm32/wasm64 المستقلان (freestanding) |

توجد خلفيات أصلية لمعماريتي x86_64 وarm64، ويمر كل هدف آخر عبر LLVM. انظر [الأهداف](docs/ar/reference/targets.md).

## التوثيق

| | |
|---|---|
| [البنية وMSIR](docs/ar/internals/msir-and-codegen.md) | كيف تتحول الشيفرة المصدرية إلى شيفرة أصلية |
| [بنية النواة](docs/ar/internals/compiler-architecture.md) | البنية الداخلية للمترجم |
| [الإقلاع الذاتي](docs/ar/internals/bootstrap.md) | Stage0 المكتوب بلغة C، وبذرة WebAssembly |
| [مسار الأمان](docs/ar/internals/safety-flow.md) | فحص الملكية والاستعارة |
| [نظام الأنواع](docs/ar/reference/type-system.md) | الأنواع، والاستنتاج، والأنواع العامة، والتحويلات |
| [الملكية والإتلاف](docs/ar/reference/ownership.md) | النقل، والإسقاط، والقيم المؤقتة، والسعة |
| [الأخطاء](docs/ar/reference/errors.md) | النتائج، و`؟`، والانهيار، والرسائل المترجمة إلى لغة المستخدم |
| [التزامن](docs/ar/reference/concurrency.md) | الخيوط، والعمليات الذرية، والمزامنة |
| [التنسيق](docs/ar/reference/formatting.md) | `std.fmt` |
| [المفسر](docs/ar/internals/interpreter.md) | `sultanc run` |
| [اعتماديات المنصة](docs/ar/reference/platform-dependencies.md) | ما تحتاجه المكتبة القياسية من كل نظام تشغيل |
| [المصطلحات العربية](docs/ar/reference/arabic-terminology.md) | المفردات العربية المعتمدة للمترجم |
| [الأمثلة](docs/ar/contributing/examples.md) · [الاختبارات](docs/ar/contributing/tests.md) | المجلد `examples/` ومشغّل الاختبارات |

## الحالة

الاستضافة الذاتية متحققة على `x86_64-linux` و`arm64-darwin`، ويُثبتها تقارب متطابق بايتًا ببايت بين أجيال المترجم، انطلاقًا من Stage0 المكتوب بلغة C ومن بذرة WebAssembly. ويبني التكامل المستمر (CI) كل طرق البناء على Linux وmacOS وWindows (x64 وarm64).

## الترخيص

[ترخيص MIT](LICENSE)

</div>
