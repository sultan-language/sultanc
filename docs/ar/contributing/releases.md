<div dir="rtl">

**العربية** | [English](../../en/contributing/releases.md)

# الإصدارات

إصدار السلطان مجموعة أرشيفات مبنية مسبقًا، واحد لكل مضيف، يبنيها التكامل المستمر من التزام موسوم واحد على
`master`. تقول هذه الصفحة ما في الإصدار، وكيف يُحدَّد رقمه، وكيف يُصنع. وخطوات المالك وإعدادات المستودع في
[التكامل المستمر](../../../.github/CI.ar.md#الإصدارات).

## ما في الإصدار

| الأرشيف | المضيف |
| --- | --- |
| `sultanc-VERSION-x86_64-linux.tar.gz` | Linux ‏x86_64 |
| `sultanc-VERSION-aarch64-linux-gnu.tar.gz` | Linux ‏arm64 |
| `sultanc-VERSION-arm64-darwin.tar.gz` | macOS على Apple silicon |
| `sultanc-VERSION-x86_64-darwin.tar.gz` | macOS على Intel |
| `sultanc-VERSION-x86_64-windows-msvc.zip` | Windows ‏x64 |

ويُفك كل أرشيف إلى مجلد واحد هو `sultanc-VERSION-TARGET/`:

<div dir="ltr">

```text
bin/sultanc            المترجم
bin/supax              مدير الحزم
bin/sultanc-analyzer   خادم اللغة (LSP)
library/               المكتبة القياسية، ويجدها المترجم بجوار bin/
LICENSE  README.md  INSTALL.txt
```

</div>

وعلى Windows تنتهي البرامج بـ`.exe`. وعلى المضيفات التي لا خلفية أصلية لها (Linux ‏arm64، وmacOS على Intel،
وWindows) تولّد LLVM المترجمَ وهو يحتويها، فيحمل الأرشيف مكتبة LLVM أيضًا: في `lib/` على Linux وmacOS، و
`bin/LLVM-C.dll` على Windows. وبجوار الأرشيفات في الإصدار ملف `.sha256` لكل أرشيف، و`SHA256SUMS.txt` الذي يجمعها،
وشهادة لمصدر بناء (build provenance) كل أرشيف.

ولتثبيت أحدها: تحقق منه بالملف `SHA256SUMS.txt`، وفكّه، وأضف مجلده `bin/` إلى `PATH`
([README](../../../README.md#التثبيت)). ويتحقق `gh attestation verify ARCHIVE --repo sultan-language/sultanc` من أن
التكامل المستمر بناه من الالتزام الموسوم.

## الأرقام

رقم واحد يغطي سلسلة الأدوات: `sultanc` وسوباكس وsultanc-analyzer في الإصدار لها الرقم نفسه. ويُكتب رقم المترجم
مرة واحدة، في `version` من `Supax.toml` (الحزمة `sultanc`) ومن `compiler/Supax.toml` (حزمة المكتبة `compiler`)،
ويجب أن يساوي كلاهما الوسم: فالوسم `v0.1.2` والمرشح `v0.1.2-rc.1` يحتاجان كلاهما `0.1.2`. ويختم بناء الإصدار
الرقمَ بخيار مشغّل البناء `--release-version` ([الإقلاع الذاتي](../internals/bootstrap.md#buildsh))، فيطبع
`sultanc --version` رقم الوسم، وأول 12 خانة ست عشرية من الالتزام، والخلفيات:

<div dir="ltr">

```text
$ sultanc --version
السلطان 0.1.2+<أول 12 خانة من الالتزام>
الخلفيات: native
```

</div>

ويقول بناء التطوير «إصدار تطويري» بدل الرقم. ويُبنى سوباكس وsultanc-analyzer من الالتزامين اللذين يثبّتهما
`release/components.toml`، واحد لكل مستودع، ويملؤهما المالك لكل إصدار.

## كيف يُصنع الإصدار

1. تُكتب تغييرات الإصدار تحت **غير مُصدَر** في [CHANGELOG.ar.md](../../../CHANGELOG.ar.md) (و
   [CHANGELOG.md](../../../CHANGELOG.md)) حين تدخل، ثم ينقلها الإصدار تحت رقمه، في الأقسام: تغييرات تكسر التوافق،
   وأُضيف، وتغيّر، وأُصلح، والأداء.
2. يضبط التزام الإصدار الرقمَ وقسم سجل التغييرات والمكونات المثبَّتة، ويُرقّى إلى `master` كأي التزام، فيكون
   مؤهلًا.
3. يَسِمه المالك `vX.Y.Z-rc.N` ثم `vX.Y.Z`. ويبدأ الوسم `.github/workflows/release.yml`: يفحص الوسم، ويبني كل
   أرشيف من الالتزام الموسوم، وينتظر موافقة المالك، ثم ينشئ إصدارًا **مسودة** ملاحظاته قسم سجل التغييرات (ومسودة
   إصدار تجريبي لوسوم `-rc.`).
4. يراجع المالك المسودة وينشرها. ولا ينشر التكامل المستمر إصدارًا أبدًا، ولا يدفع وسمًا ولا ينقله.

## انظر أيضًا

- [التكامل المستمر](../../../.github/CI.ar.md#الإصدارات): سير العمل، وخطوات المالك، والإعدادات
- [الإقلاع الذاتي](../internals/bootstrap.md): كيف يُبنى مترجم كل مضيف
- [دليل المساهمين](README.md)

</div>
