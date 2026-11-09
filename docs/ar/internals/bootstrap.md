<div dir="rtl">

**العربية** | [English](../../en/internals/bootstrap.md)

# الإقلاع الذاتي

السلطان مستضاف ذاتيًا: المترجم مكتوب بلغة السلطان، لذا يحتاج بناؤه من المصدر إلى مترجم أول. تصف هذه
الصفحة المسارات الثلاثة التي توفر ذلك المترجم الأول (Stage0 المكتوب بلغة C، وبذرة WebAssembly
المحفوظة في المستودع، ومترجم موجود)، وكيف يشغّل `build.sh` و`cmake/` المراحلَ متتابعة، وكيف تُفحص البذرة
وتُحدَّث، وكيف يثبت التكامل المستمر (CI) صحة كل مسار على كل مضيف. وهي موجهة إلى من يبنون الإقلاع الذاتي
(bootstrap) أو يتحققون منه أو يغيّرونه.

## المراحل

<div dir="ltr">

```text
Stage0   the C seed compiler (bootstrap/)             needs a C compiler and an LLVM SDK
Stage1   the first SultanC compiler, built by one of the routes below
Stage2   Stage1 compiles compiler/main.sn             the compiler that build.sh exports
Stage3   Stage2 compiles compiler/main.sn
Stage4   Stage3 compiles compiler/main.sn             Stage3 and Stage4 must be byte-identical
```

</div>

تقع سياسة المراحل كلها في `cmake/`؛ ولا يفعل `build.sh` سوى استدعاء CMake. وStage1 ملف تنفيذي للمضيف
دائمًا. وStage2 هو مترجم الإنتاج، وهو أصلي فقط ما لم يُعطَ `--enable-llvm`
([بنية المترجم](compiler-architecture.md)، "قدرات بناء المترجم"). ويبني Stage1 المرحلة Stage2 مباشرةً
بخلفية السلطان نفسه؛ فلا يوجد بناء وسيط للمترجم بلغة C.

## المسارات إلى Stage1

| | `--bootstrap=c` (الافتراضي) | `--bootstrap=wasm` | `--bootstrap=existing` |
|---|---|---|---|
| مصدر Stage1 | Stage0 المكتوب بلغة C في `bootstrap/` | البذرة في `bootstrap-wasm/` | مترجم مبني مسبقًا (`--with=PATH`، والافتراضي `build/sultanc`) |
| المتطلبات | مترجم C وحزمة LLVM SDK | مترجم C (وحزمة LLVM SDK مع `--enable-llvm` فقط) | المترجم الموجود (وحزمة LLVM SDK مع `--enable-llvm` فقط) |
| شجرة CMake | `build/bootstrap/cmake` | `build/bootstrap/cmake-wasm` | `build/bootstrap/cmake-existing` |
| Stage1 | مزوَّد بـ LLVM | أصلي فقط | ما يحتويه المترجم الموجود أيًا كان |
| `verify` (أصلي فقط) | التقارب الأصلي والتقارب بين الخلفيتين | التقارب الأصلي (ليس في Stage1 خلفية LLVM) | التقارب الأصلي |
| `stage0` | يبني Stage0 | لا ينطبق (يفشل برسالة) | لا ينطبق (يفشل برسالة) |

مسار C ومسار wasm يبنيان مترجمًا من المصادر وحدها، وهما المساران المرجعيان. استخدمهما على نظام جديد،
وعندما يكون المترجم الموجود أقدم من أن يناسب المصادر (بنية نحوية أو بدائيات جديدة)، وللتحقق من إصدار.
أما مسار المترجم الموجود فهو مسار التطوير السريع، على غرار استخدام Go للمتغير `GOROOT_BOOTSTRAP`: يتخطى
Stage0 وترجمة البذرة، ويخطو خطوة استضافة ذاتية واحدة من المترجم الموجود إلى Stage2 الجديد.

### build.sh

<div dir="ltr">

```sh
./build.sh                                  # Stage2 -> build/sultanc (مسار C، أصلي فقط)
./build.sh --enable-llvm                    # Stage2 يحتوي خلفية LLVM أيضًا
./build.sh verify                           # Stage2، ثم Stage3 == Stage4
./build.sh verify --enable-llvm             # يضيف تقارب LLVM والتقارب بين الخلفيتين
./build.sh bootstrap                        # Stage0 + Stage1 فقط
./build.sh stage0                           # Stage0 فقط

./build.sh --bootstrap=wasm                 # Stage2 من البذرة (لا يحتاج إلا إلى مترجم C)
./build.sh verify --bootstrap=wasm          # + Stage3 == Stage4
./build.sh bootstrap --bootstrap=wasm       # Stage1 فقط -> build/bootstrap/sultanc-stage1

./build.sh --bootstrap=existing             # خطوة استضافة ذاتية واحدة من build/sultanc
./build.sh --bootstrap=existing --with=/path/to/sultanc
```

</div>

يجوز أن يسمّي الوسيط الأول مرحلةً (`stage0` أو `stage1` أو `stage2` أو `bootstrap` أو `verify`)؛ والافتراضي
`stage2`. والخيارات الأخرى هي:

| الخيار | المعنى |
|---|---|
| `--enable-llvm` | إدراج خلفية LLVM في المترجم المبني (`SULTANC_COMPILER_LLVM`)؛ ومع `--bootstrap=c` يستخدم Stage0 وStage1 دائمًا LLVM |
| `--backend=native\|llvm` | الخلفية التي يستخدمها Stage1 لتوليد Stage2 (`SULTANC_STAGE2_BACKEND`، والافتراضي `native`) |
| `--bootstrap=c\|wasm\|existing` | المسار (`SULTANC_BOOTSTRAP`) |
| `--with=PATH` | المترجم الموجود في `--bootstrap=existing` (`SULTANC_EXISTING_COMPILER`) |
| `--target=TRIPLE` | هدف Stage2؛ أما Stage1 فيعمل دائمًا على مضيف البناء |
| `OUTPUT` | مسار لناتج المرحلة المطلوبة |

تبقى النواتج في شجرة البناء الخاصة بكل منها (`bin/`)، وتُنسخ المرحلة المطلوبة إلى `build/sultanc`
(`build/sultanc.exe` على Windows) إن كانت Stage2، أو إلى `build/bootstrap/` إن كانت Stage0 أو Stage1.
ويختار `LLVM_CONFIG=/path/to/llvm-config`، أو `-DLLVM_DIR` عبر CMake، حزمةَ LLVM SDK. ويمكن تشغيل CMake
مباشرةً:

<div dir="ltr">

```sh
cmake -S cmake -B build/bootstrap/cmake -DSULTANC_REQUESTED_STAGE=stage2
cmake --build build/bootstrap/cmake --target sultanc-stage2
```

</div>

يجوز لـ Stage1 أن يستخدم أي خلفية يحتويها لبناء Stage2، لكن الخلفية الأصلية ليس لها هدف Windows. لذا
يحتاج Stage2 لنظام Windows إلى مسار C مع LLVM:
`./build.sh --bootstrap=c --enable-llvm --backend=llvm`. ويعمل Stage1 الناتج عن مسار wasm على Windows،
لكن هذا المسار لا ينتج هناك إلا Stage1. ولا يبني مسار المترجم الموجود Stage2 مزوَّدًا بـ LLVM لنظام Windows.

ولبناء Stage2 مزوَّد بـ LLVM، يمرر مسار wasm ومسار المترجم الموجود مكتبةَ libLLVM المختارة (بمسار ملفها
المحلول) إلى Stage1 صراحةً. أما Stage1 في مسار C فيسجل المسار نفسه بنفسه، فتنتج كل المسارات Stage2
متطابقًا بايتًا ببايت.

يُنسخ المترجم الموجود إلى شجرة البناء عند تهيئة CMake، فلا يكتب Stage2 الذي يصدّره هذا المسار إلى
`build/sultanc` أبدًا فوق المترجم الذي يتولى إنتاجه. وفي التهيئة التالية يُعتمد `build/sultanc` الجديد
مترجمًا موجودًا.

## التقارب

يفحص `verify` أن المترجم يعيد إنتاج نفسه. يبني Stage2 المرحلة Stage3، ويبني Stage3 المرحلة Stage4، ويجب
أن تتطابق الاثنتان بايتًا ببايت. وكل مترجم يُنفَّذ يستهدف مضيف البناء، وتجري الخطوات واحدة تلو الأخرى لأن
كل بناء ذاتي للمترجم يحتاج إلى عدة غيغابايتات من الذاكرة.

- على مضيف له خلفية أصلية، يُفحص التقارب الأصلي دائمًا.
- مع Stage2 مزوَّد بـ LLVM، يُفحص كذلك تقارب LLVM والتساوي بين الخلفيتين (مترجم مولَّد بـ LLVM يبني
  مترجمًا أصليًا).
- مع Stage2 أصلي فقط، يستخدم فحص التساوي بين الخلفيتين Stage3 أصليًا فقط ولّدته خلفية LLVM في Stage1.
  ويُتخطى هذا الفحص في مسار wasm، لأن Stage1 فيه ليس له خلفية LLVM، وفي مسار المترجم الموجود، الذي لا
  يفترض أن للمترجم الموجود هذه الخلفية.
- ليس لنظام Windows خلفية أصلية، لذا يحتاج `verify` على Windows إلى مترجم مزوَّد بـ LLVM.

## Stage0 المكتوب بلغة C

يحتوي `bootstrap/` على مترجم للغة السلطان مكتوب بلغة C11. يحلل مصادر السلطان معجميًا ونحويًا ودلاليًا،
ويُصدر ملفات الكائنات عبر واجهة C البرمجية لـ LLVM (LLVM C API، `bootstrap/src/llvm`)، لذا يحتاج بناؤه
إلى حزمة LLVM SDK. ويترجم Stage0 الملفات `compiler/**` ووحدات المكتبة القياسية التي يستوردها المترجم إلى
ملف الكائن الخاص بـ Stage1، الذي يربطه CMake بمشغّل الربط الخاص بمترجم C على المضيف.

Stage0 هو البذرة، وليس تنفيذًا إنتاجيًا ثانيًا. يُترجم مع معاملة التحذيرات أخطاءً
(`SULTANC_STAGE0_WARNINGS_AS_ERRORS`)، ويفحصه CodeQL في التكامل المستمر، وله توليده الخاص لعمليات
الإتلاف (drop elaboration) كي يحرر Stage1 الذي يبنيه بياناتِ عمله ([تدفق الأمان](safety-flow.md)).
ولأنه يترجم المترجم، تقيّد حدود اللغة في Stage0 مصادر المترجم والمكتبة القياسية؛ انظر
[المساهمة](../contributing/README.md) و[الملكية والإتلاف](../reference/ownership.md).

## بذرة WebAssembly

يحفظ الإقلاع الذاتي بالبذرة في المستودع بناءً للمترجم بصيغة WebAssembly ويترجمه إلى C، فلا يحتاج بناء
السلطان إلا إلى مترجم C.

<div dir="ltr">

```text
seed/sultanc.wasm.zst --wasm2c.c (+ zstd_decoder.h)--> C --cc + wasi.c--> sultanc-stage1
sultanc-stage1 --compiler/main.sn--> sultanc-stage2 (native) --> Stage3 == Stage4
```

</div>

لا يحتوي `bootstrap-wasm/` إلا على البذرة ومصادرها بلغة C؛ أما قواعد CMake ففي `cmake/`.

| الملف | الدور |
|---|---|
| `bootstrap-wasm/seed/sultanc.wasm.zst` | البذرة: المترجم مبنيًا للهدف `wasm32-wasi`، أصليًا فقط (مزود LLVM مُعيَّن إلى `unavailable.sn`)، ومضغوطًا بـ `zstd -19` |
| `bootstrap-wasm/wasm2c.c`, `zstd_decoder.h` | أداة ترجمة البذرة إلى C99، مع فاك ضغط zstd موجود في المستودع (MIT) |
| `bootstrap-wasm/wasi.c` | مضيف WASI الذي تُربط به البذرة المترجَمة (POSIX وWindows) |
| `bootstrap-wasm/bootstrap.c` | المشغّل المستقل: من البذرة إلى Stage1 إلى Stage2، إضافة إلى `--verify` و`--check-seed` و`--update-seed` |
| `cmake/modules/WasmSeed.cmake` | أداة الترجمة `sultanc-wasm2c` وStage1 المترجَم |
| `cmake/modules/Stage1Wasm.cmake` | مسار wasm في البناء الرئيسي والهدف `sultanc-check-seed` |
| `cmake/scripts/CheckSeed.cmake` | المقارنة التي يقوم عليها `sultanc-check-seed` |

يبلغ حجم البذرة المضغوطة نحو 3.2 MB، وحجم الوحدة نحو 12 MB. ويوفر البناء الرئيسي مع
`-DSULTANC_BOOTSTRAP=wasm` الأهداف `sultanc-stage1` و`sultanc-stage2` و`sultanc-verify`
و`sultanc-check-seed`. ويستخدم فحص البذرة المرحلة Stage2 حين تعمل على المضيف، وإلا فيستخدم Stage1 نفسه
(Windows، والمضيفات التي ليس لها خلفية أصلية). وللبناء المتقاطع، مرّر
`-DSULTANC_WASM2C=<sultanc-wasm2c مبنيًا لآلة البناء>`.

### المشغّل المستقل

من جذر المستودع، لا يحتاج `bootstrap.c` إلا إلى مترجم C يقبل خيارات على طريقة gcc:

<div dir="ltr">

```sh
cc -o build-bootstrap bootstrap-wasm/bootstrap.c
./build-bootstrap --verify            # -> build/bootstrap-wasm/sultanc-stage2
```

</div>

على Windows مع MSVC، استخدم البناء الرئيسي:

<div dir="ltr">

```sh
cmake -S cmake -B build/bootstrap/cmake-wasm -DSULTANC_BOOTSTRAP=wasm -DSULTANC_REQUESTED_STAGE=stage1
cmake --build build/bootstrap/cmake-wasm --target sultanc-stage1
cmake --build build/bootstrap/cmake-wasm --target sultanc-check-seed
```

</div>

| الخيار | المعنى |
|---|---|
| `--cc=CC`, `--cflags=FLAGS` | مترجم C وخياراته (الافتراضي `$CC` أو `cc`، و`$CFLAGS` أو `-std=c99 -O2`) |
| `--jobs=N` | عدد عمليات ترجمة C المتوازية (الافتراضي 4) |
| `--out=DIR` | مجلد المخرجات (الافتراضي `build/bootstrap-wasm`) |
| `--target=TRIPLE` | هدف Stage2 (الافتراضي: هذا المضيف) |
| `--stage1-only` | التوقف بعد Stage1 |
| `--verify` | بناء Stage3 وStage4 واشتراط Stage3 == Stage4 |
| `--check-seed` | إعادة بناء البذرة باستخدام Stage2 ومقارنتها بالبذرة المحفوظة |
| `--update-seed` | إعادة البناء والضغط (`zstd -19`، ويحتاج إلى الأمر `zstd`) واستبدال البذرة المحفوظة |

### سياسة فحص البذرة وتحديثها

تُستخدم البذرة كما حُفظت في المستودع. لا شيء يثبّتها بمجموع اختباري (checksum)، ولا توجد ملفات تجزئة (hash)
بجانبها. يترجم `wasm2c` أي بذرة صالحة تُعطى له، ويرفض إطار zstd المشوّه أو الوحدة المشوّهة برسالة تشخيص،
ولا ينهار أبدًا. ويحافظ تاريخ Git والمراجعة على سلامة البذرة، ويبيّن فحص البذرة ما إذا كانت لا تزال
مطابقةً للمصادر.

يعيد `--check-seed` (وهدف CMake المسمى `sultanc-check-seed`) بناء الوحدة من المصادر الحالية باستخدام
Stage2، ويفك ضغط البذرة المحفوظة بفاك الضغط الموجود في المستودع، ويقارن بين الاثنين بايتًا ببايت. ويبلّغ عما
إذا كانا متساويين. وعند اختلافهما يخرج `bootstrap.c` بحالة الخروج 3 ويفشل هدف CMake.

يعيد `--update-seed` بناء الوحدة، ويضغطها بـ `zstd -19`، ثم يفك ضغط النتيجة مرة أخرى بفاك الضغط الموجود
في المستودع للتأكد من أنها تعيد الوحدة نفسها، وعندئذ فقط يستبدل `seed/sultanc.wasm.zst`. ولا يكتب شيئًا
غيره.

تنبع هذه السياسة من التكامل المستمر، الذي يشترط أن تكون البذرة المحفوظة هي بالضبط ما تعيد المصادر الحالية
بناءه. لذا فإن أي تغيير في مصادر المترجم أو المكتبة القياسية يغيّر وحدة البذرة تصحبه بذرة محدَّثة في التغيير
نفسه الخاضع للمراجعة:

<div dir="ltr">

```sh
cc -o build-bootstrap bootstrap-wasm/bootstrap.c
./build-bootstrap --update-seed
./build-bootstrap --out=build/bootstrap-check --verify --check-seed    # يجب أن ينجح دون أخطاء
```

</div>

### المكونات

**`wasm2c.c`** يترجم WebAssembly إلى C99.

- المدخل المقبول هو ما تستطيع خلفية wasm32 في السلطان إصداره (`compiler/backend/architectures/wasm32/`):
  عمليات الأعداد الصحيحة في الإصدار الأساسي (MVP) وعوامل تمديد الإشارة؛ وثوابت f32 وf64 وتحميلها وتخزينها
  و`+ - * /` وabs وneg وcopysign والمقارنات وإعادة التفسير؛ و`memory.copy` و`memory.fill`؛ وجدول funcref
  واحد وذاكرة واحدة بعرض 32 بت؛ والمتغيرات العامة والمقاطع النشطة ودالة بدء. وأي شيء آخر يُرفض مع ذكر
  الدالة والإزاحة.
- تُحَل الاستيرادات بالاسم، ويُفحص كل توقيع مقابل ما ينفذه `wasi.c`.
- تصبح خانات المكدس متغيراتِ C محلية ذات أنواع عبر ترجمة ساكنة للمكدس. وتُفحص أنواع كل تعليمة، فإذا
  كانت الوحدة مشوّهة أو كان في أداة الترجمة خلل، وقع الفشل وقت الترجمة.
- عمليات الوصول إلى الذاكرة في شيفرة C المولَّدة مفحوصة الحدود. ولا تستخدم العمليات الحسابية إلا ما هو
  معرّف السلوك في C: لا فيضان للأعداد ذات الإشارة، ولا إزاحات أو تحويلات يحدد سلوكها التنفيذ. ويُتعامل مع
  المضيفات التي ترتّب البايتات بدءًا بالأكبر (big-endian) بتبديل البايتات.
- كل نداء بين دوال الوحدة يُحتسب ضمن حد ثابت قدره 100000 نداء متداخل، ويفحص حد المكدس الأصلي الذي ضبطه
  المضيف (`w2c_limit_native_stack`). والعودية العميقة تتوقف بخطأ (trap) هو
  `call stack exhausted (more than 100000 nested calls)`، أو `(native stack)` على مضيف مكدسه أصغر مما يحتاجه
  الحد، بدل الانهيار. ومكدس الظل الخاص بالضيف (8 MiB في الذاكرة الخطية) يتوقف بخطأ وصول خارج الحدود حين
  يفيض أولًا، كما في wasmtime.
- المخرج حتمي ومقسّم إلى أجزاء للترجمة المتوازية. ويخدم وضع الأداة
  `--decompress FILE.zst OUT` مشغّلات البناء وفحص البذرة.

**`zstd_decoder.h`** فاك ضغط لصيغة Zstandard (RFC 8878) مكتوب من المواصفة لهذا المشروع، دون أي شيفرة من
التنفيذ المرجعي. يدعم كل ترميزات الكتل والحرفيات والتسلسلات، والإطارات المتعددة والقابلة للتخطي، ويتحقق
من المجموع الاختباري XXH64 للمحتوى. ولا يدعم القواميس.

**`wasi.c`** ينفذ دوال WASI Preview 1 الخمس والعشرين التي يمكن أن تستوردها المكتبة القياسية، على طبقة محمولة
واحدة لها خلفيتان خاصتان بنظام التشغيل.

- تغطي خلفية POSIX أنظمة Linux وmacOS وأنظمة BSD وillumos وHaiku وCygwin.
- تحوّل خلفية Win32 بين UTF-8 وUTF-16 للمسارات والوسائط والبيئة، وتتعامل مع المقابض (handles) وساعات
  Windows والأرقام العشوائية ومخرج الطرفية بترميز UTF-8 ومسارات حروف الأقراص ومسارات UNC. ولا تحتاج وقت
  الربط إلا إلى kernel32.

المشترك بين الخلفيتين: المجلدات المفتوحة مسبقًا (preopens) هي `.` بالواصف 3، و`/` بالواصف 4 على مضيفات
POSIX. وكل مؤشر من الضيف مفحوص الحدود (`EFAULT`). ويملأ `fd_readdir` مخزنه المؤقت كما تشترط WASI: تقرأ
قيمة `cookie` المساوية 0 المجلد كله مرة واحدة، وترتبه بالاسم (بايتًا ببايت) وتخزنه في ذاكرة مؤقتة (cache)
على الواصف، وتقرأ قيم `cookie` اللاحقة من الذاكرة المؤقتة، فتكون القوائم واحدة على كل مضيف ونظام ملفات،
ولا تكلّف القائمةُ المقروءة على نداءات كثيرة إلا مسحًا واحدًا. ويعمل الضيف على مكدس أصلي معروف: يستخدم
Windows خيطًا بحجز مكدس قدره 256 MiB (و64 MiB على 32 بت)، ويرفع Linux الحد المرن لمكدس الخيط الرئيسي إلى
64 MiB حيث يسمح الحد الصارم (كما يفعل المترجم الأصلي)، وتستخدم مضيفات POSIX الأخرى حد الخيط الرئيسي.
ويُمرَّر الحد إلى فحص النداء في الشيفرة المترجَمة.

**`bootstrap.c`** يشغّل البناء. يستخدم على POSIX الدالة `fork`، وعلى Windows الدالة `CreateProcess`
و`cmd`. ولا تستطيع WASI Preview 1 ضبط بت التنفيذ، لذا يشغّل المشغّل `chmod 0755` على مخرجاته.

## المضيفات

| المضيف | Stage1 (مترجم للسلطان يعمل) | Stage2 والتحقق وفحص البذرة |
|---|---|---|
| Linux x86_64 | نعم | نعم |
| macOS arm64 | نعم | نعم |
| Linux aarch64 وi686 وs390x وriscv64 وغيرها؛ FreeBSD وNetBSD وOpenBSD وillumos | نعم | لا: ليس للسلطان خلفية أصلية لهذه الأنظمة، ويحتاج Stage2 الأصلي لهذا المضيف إلى مترجم مزوَّد بـ LLVM |
| Windows x64 وarm64 (MinGW وclang وMSVC عبر CMake) | نعم | ليس من البذرة، للسبب نفسه؛ ويتوقف البناء الرئيسي مع ذكر المسار المدعوم. ويأتي Stage2 لنظام Windows من مسار C مع LLVM: `./build.sh --bootstrap=c --enable-llvm --backend=llvm` |

على المضيفات التي ليس لها خلفية أصلية، يعيد فحص البذرة بناء البذرة باستخدام Stage1. ويستطيع Stage1
على أي مضيف ترجمة برامج السلطان للأهداف x86_64-linux وarm64-darwin وwasm32-wasi، بما في ذلك بناء المترجم
نفسه لهذه الأهداف.

## التكامل المستمر

يثبت `.github/workflows/ci.yml` صحة كل مسار على كل مضيف يمكن أن يعمل عليه. ويخدم سكربت واحد كل عائلة من
المضيفات: `.github/ci/linux.sh` و`.github/ci/macos.sh` (POSIX sh) و`.github/ci/windows.ps1` (PowerShell).
وهناك ثلاث مهام، تعمل كل منها على المضيفات الستة linux-x86_64 وlinux-arm64 وmacos-arm64 وmacos-x86_64
وwindows-x86_64 وwindows-arm64، باستثناء `seed-update`.

**`seed`** تبني عبر مسار wasm بمترجم C وحده دون LLVM.

| المضيف | ما يُشغَّل |
|---|---|
| linux-x86_64, macos-arm64 (خلفية أصلية) | `./build.sh verify --bootstrap=wasm` (من البذرة إلى Stage1 إلى Stage2، وStage3 == Stage4)، ثم `sultanc-check-seed` (يعيد Stage2 بناء البذرة المحفوظة مطابقةً بايتًا ببايت)، ثم مجموعة الاختبارات |
| linux-arm64, macos-x86_64 (دون خلفية أصلية) | `./build.sh bootstrap --bootstrap=wasm` و`sultanc-stage1 version`، ثم `sultanc-check-seed` (يعيد Stage1 بناء البذرة) |
| windows (x64 وarm64) | Stage1 فقط، عبر كل سلسلة أدوات C: MSVC (`cl`) عبر CMake، وclang-cl عبر CMake، وعلى x64 أيضًا MinGW gcc مع `bootstrap.c`. ويعيد Stage1 المبني بـ MSVC بناء البذرة المحفوظة، ويجب أن تطابقها النتيجة بايتًا ببايت |

**`seed-update`** (linux-x86_64) تشغّل `bootstrap.c --update-seed` ثم تشغّل `--verify
--check-seed` تشغيلًا نظيفًا انطلاقًا من البذرة المحدَّثة، فيُعرف مسبقًا أن التحديث التالي للبذرة سينجح.

**`llvm`** تستخدم مسار C مع LLVM 22.1.8، المثبّت بالإصدار واسم الملف والمخزَّن في الذاكرة المؤقتة بواسطة سير
العمل. تبني Stage2 مزوَّدًا بـ LLVM، وتفحص التقارب، وتشغّل مجموعة الاختبارات.

| المضيف | ما يُشغَّل |
|---|---|
| linux-x86_64, macos-arm64 | `./build.sh verify --enable-llvm` (التقارب الأصلي، وتقارب LLVM، وتقارب الأصلية == LLVM)، ثم مجموعة الاختبارات |
| linux-arm64, macos-x86_64 | Stage2 مولَّد بـ LLVM (`./build.sh --enable-llvm --backend=llvm`)، ثم يجب أن يتطابق بايتًا ببايت Stage3 وStage4 المولَّدان بـ LLVM والمبنيّان مع libLLVM مدخلًا للربط، ثم مجموعة الاختبارات |
| windows | `sultanc-verify` بمسار C، مع `SULTANC_COMPILER_LLVM=ON` و`SULTANC_STAGE2_BACKEND=llvm` (Stage2 مولَّد بـ LLVM، وStage3 == Stage4 لـ LLVM)، ثم مجموعة الاختبارات تحت bash الخاص بـ Git for Windows |

يوفّر كل سكربت حزمة LLVM SDK بنفسه. فعلى Linux تُربط مكتبة `libLLVM` موحّدة من أرشيفات مكونات الإصدار
الرسمي. ويفعل macOS arm64 الشيء نفسه، بينما يبني macOS x86_64 المصدر المثبّت، وهو بناء يستغرق ساعات في
المرة الأولى ثم يُخزَّن في الذاكرة المؤقتة. ويبني Windows المكتبة `LLVM-C.dll` من المصدر المثبّت باستخدام
clang-cl من الأرشيف المثبّت. ومجموعة الاختبارات هي `tools/tests/ci_run.sh` ([الاختبارات](../contributing/tests.md)).

يشغّل سير عمل منفصل، `.github/workflows/security.yml`، أداة CodeQL على Stage0 المكتوب بلغة C في
`bootstrap/`. ولا يحلل مصادر `.sn`.

## تأهيل سلسلة أدوات البذرة

تُفحص أداة الترجمة من WebAssembly إلى C ومضيف WASI وفاك ضغط zstd مقابل تنفيذات مرجعية، بمعزل عن اختبارات
المترجم نفسه.

- المطابقة: تعطي وحدةٌ تختبر كل عامل مقبول على القيم الحدية، وكل عروض التحميل والتخزين، وبنى تدفق التحكم،
  نتائجَ مطابقة لنتائج wasmtime. والقسمة على صفر والفيضان والوصول خارج الحدود و`unreachable` والنداءات غير
  المباشرة الخاطئة تتوقف بخطأ بالطريقة نفسها.
- البناء المتقاطع: يبني Stage1 المترجَم إلى C المترجمَ كاملًا مطابقًا بايتًا ببايت للمترجم المبني أصليًا،
  وذلك على Linux بعرض 32 بت وعلى aarch64 وعلى مضيف يرتّب البايتات بدءًا بالأكبر (s390x) تحت qemu-user،
  وعلى Windows x64 تحت wine.
- يطابق فاك ضغط zstd الأداة `zstd` عبر تركيبات المستويات والخيارات، والمدخل الممرَّر عبر أنبوب دون حجم
  محتوى، والإطارات المتعددة والقابلة للتخطي، والمدخل الفارغ، ويتحمل المدخلات التالفة دون انهيار.
- تُترجم المصادر دون تحذيرات مع `-Wall -Wextra -pedantic` على gcc وclang وMinGW، وتحمل الشيفرة المولَّدة
  بديلًا لـ `inline` لإصدارات MSVC الأقدم، التي يضبط لها CMake الخيارين `/w /bigobj` على المصادر المولَّدة.

## القيود

- يجب أن تساوي البذرة المحفوظة ما تعيد المصادر الحالية بناءه، لذا فإن تغيير المصدر الذي يبدّل الوحدة
  يحتاج إلى تحديث البذرة في التغيير نفسه.
- لا يوجد Stage2 من البذرة إلا على المضيفات التي لها خلفية أصلية (انظر "المضيفات").
- لا يقبل `wasm2c` إلا المجموعة الجزئية من WebAssembly التي تصدرها خلفية wasm32 الخاصة بالسلطان، ولا
  يدعم فاك ضغط zstd القواميس.
- ليس لـ Stage1 الناتج عن مسار wasm خلفية LLVM، لذا تحتاج المراحل المولَّدة بـ LLVM إلى مسار C.

## انظر أيضًا

- [بنية المترجم](compiler-architecture.md)
- [دليل المساهمين](../contributing/README.md)
- [الاختبارات](../contributing/tests.md)
- [الملكية والإتلاف](../reference/ownership.md)

</div>
