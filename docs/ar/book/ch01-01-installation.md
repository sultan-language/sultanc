<div dir="rtl">

**العربية** | [English](../../en/book/ch01-01-installation.md)

# التثبيت

تُبنى لغة السلطان من الشيفرة المصدرية. والمترجم نفسه مكتوب بالسلطان، فيبدأ البناء من seed صغير موجود في المستودع،
ويبني به المترجم الحقيقي. فلا تحتاج إلى مترجم سلطان مثبت من قبل.

> **الأوامر في هذا الكتاب.** السطر الذي يبدأ بـ `$` أمر تكتبه في الطرفية، دون `$`. والأسطر التي بلا `$` هي ما يطبعه
> الأمر.

وللبناء طريقان، ويحدد نظامك أيهما تستخدم:

| نظامك | الطريق | ما تحتاجه |
| --- | --- | --- |
| لينكس على x86_64 | seed الـ WebAssembly | ‏Git ومترجم C |
| ماك على Apple silicon (‏M1 وما بعده) | seed الـ WebAssembly | ‏Git ومترجم C |
| ويندوز، ولينكس على ARM، وماك على Intel، و FreeBSD وغيرها | سلسلة Stage0 المكتوب بلغة C مع LLVM | ‏Git (بالتاريخ الكامل) ومترجم C و LLVM من 19 إلى 23 |

والفرق بينهما هو الـ backend: الجزء من المترجم الذي ينتج شيفرة الآلة. فللسلطان backend خاص به اسمه Native، يعمل على لينكس
x86_64 وماك Apple silicon. وعلى كل نظام آخر ينتج المترجم الشيفرة عبر LLVM، فيجب تثبيت LLVM قبل البناء.

## لينكس (x86_64)

ثبّت الأدوات بمدير الحزم في توزيعتك:

```console
$ sudo apt install git build-essential     # Debian و Ubuntu
$ sudo dnf install git gcc make            # Fedora
$ sudo pacman -S git base-devel            # Arch
```

ثم ابنِ:

```console
$ git clone https://github.com/sultan-language/sultanc.git
$ cd sultanc
$ ./build.sh
build: translating the seed to C (bootstrap-wasm/seed/sultanc.wasm.zst)
build: compiling C (11 files)
build: seed compiler: build/bootstrap/seed/sultanc-stage1
build: building the build driver (tools/build)
build: building the compiler from compiler/ with the seed compiler
build: compiler: build/sultanc
```

يسمي السطر الأخير الناتج: المترجم هو `build/sultanc`. يبدأ البناء الأول من الـ seed، ثم يخطو كل `./build.sh` بعده
خطوة واحدة من المترجم الذي عندك، ولا يفعل شيئًا إن لم تتغير مصادر المترجم. ويعرض `./build.sh --verbose` كذلك كل أمر
يشغّله.

وبلا shell يبني ملف C واحد البناءَ نفسه، بمترجم C وحده، وينتهي بـ `build/sultanc` نفسه:

```console
$ cc -o build-bootstrap bootstrap-wasm/bootstrap.c && ./build-bootstrap
```

ولفحص المترجم، يبنيه `./build.sh verify` ثم يبنيه مرة أخرى بنفسه، ويشترط أن يتطابق الاثنان (و`./build-bootstrap --verify`
بلا shell). وأوامر المشرفين (`./build.sh seed-update` و`seed-check` و`chain`) في [الإقلاع الذاتي](../internals/bootstrap.md).

## ماك (Apple silicon)

يأتي مترجم C مع أدوات سطر الأوامر من Apple:

```console
$ xcode-select --install
```

ثم ابنِ بالأوامر نفسها التي في لينكس:

```console
$ git clone https://github.com/sultan-language/sultanc.git
$ cd sultanc
$ ./build.sh
```

## ويندوز وكل نظام آخر

لا backend أصليًا لهذه الأنظمة. ويظل `./build.sh` (أو `build-bootstrap`، الذي لا يحتاج إلا مترجم C، وعلى ويندوز أيضًا)
ينتهي بـ `build/sultanc`، لكنه هناك مترجم الـ seed: يعمل على نظامك ولا يولّد الشيفرة إلا لـ x86_64-linux وarm64-darwin
وwasm32-wasi، ويقول البناء ذلك في سطر واحد. أما مترجم لهذه الأنظمة نفسها فيولّده LLVM. ويبنيه Stage0 المجمَّد المكتوب بلغة C في المستودع: يترجم مترجم
التزام مثبَّت، ويبني هذا المترجمَ الحالي. ثبّت Git ومترجم C و LLVM من 19 إلى 23، واستنسخ التاريخ كله، ثم شغّل السلسلة:

```console
$ git clone https://github.com/sultan-language/sultanc.git
$ cd sultanc
$ sh bootstrap/chain.sh
$ cp build/chain/sultanc-a build/sultanc
```

وتجد السلسلة LLVM عبر `llvm-config`. وإن كان في مكان آخر، فسمِّه هكذا:
`LLVM_CONFIG=/path/to/llvm-config sh bootstrap/chain.sh`.

وفي ويندوز، شغّل `bootstrap/chain.ps1` من PowerShell المطورين في Visual Studio، مع `LLVM_SDK` يسمي بناءً لـ LLVM فيه
`include\` و`lib\LLVM-C.lib` و`bin\LLVM-C.dll`. ولا يحوي LLVM الذي يثبّته المثبِّت الرسمي كل المعماريات ولا مكتبة
LLVM-C، فيجب بناء LLVM من مصدره مرة واحدة: وتفعل ذلك مهمة `llvm-sdk` في `.github/ci/windows.ps1`. والمترجم هو
`build\chain\sultanc-a.exe`؛ وأبقِ `LLVM-C.dll` بجواره.

وبديلًا عن ذلك، على لينكس x86_64 أو Apple silicon، يبني `./build.sh --enable-llvm` ثم
`./build.sh --target=x86_64-windows-msvc --backend=llvm --with=build/sultanc` مترجمًا لنظام آخر (هنا ويندوز، في
`build/cross/`). وهو لا يحوي إلا الـ backend الأصلي، فيفحص البرامج ويشغّلها هناك، لكنه لا يولّد الشيفرة إلا للأهداف
الأصلية.

## النتيجة

يستغرق البناء بضع دقائق. وحين ينتهي يكون المترجم في `build/sultanc`. تأكد أنه يعمل:

```console
$ build/sultanc version
السلطان إصدار تطويري
الخلفيات: native
```

ويسرد السطر الأخير الـ backends التي في مترجمك: `native` أو `llvm` أو كليهما.

## تثبيت `sultanc` وسوباكس

سوباكس هو مدير الحزم في لغة السلطان. ينشئ المشاريع، ويجلب الحزم التي تعتمد عليها، ويبنيها ويشغّلها ويختبرها. ويضع مثبِّته
أيضا `sultanc` والمكتبة القياسية في الـ `PATH`. انسخ سوباكس بجوار المترجم، وشغّل المثبِّت مع مسار مجلد المترجم:

```console
$ cd ..
$ git clone https://github.com/sultan-language/supax.git
$ cd supax
$ tools/install.sh ../sultanc
```

وفي ويندوز شغّل `tools\install.ps1` بدلا منه. ويضع كلاهما `sultanc` و `supax` والمكتبة القياسية في المجلد `.supax` داخل
مجلد المستخدم، ويضيف `.supax/bin` إلى الـ `PATH`. افتح طرفية جديدة وتأكد:

```console
$ sultanc version
$ supax --help
```

## تجربة السلطان دون تثبيت

يترجم [الملعب](https://sultan-language.github.io/playground/) برامج السلطان ويشغّلها في المتصفح، ويكفي لبرامج الفصول
الأولى من هذا الكتاب.

</div>
