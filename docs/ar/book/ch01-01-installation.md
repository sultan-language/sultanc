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
| لينكس على x86_64 | seed الـ WebAssembly | ‏Git و CMake ومترجم C |
| ماك على Apple silicon (‏M1 وما بعده) | seed الـ WebAssembly | ‏Git و CMake ومترجم C |
| ويندوز، ولينكس على ARM، وماك على Intel، و FreeBSD وغيرها | seed الـ C مع LLVM | ‏Git و CMake ومترجم C و LLVM من 19 إلى 23 |

والفرق بينهما هو الـ backend: الجزء من المترجم الذي ينتج شيفرة الآلة. فللسلطان backend خاص به اسمه Native، يعمل على لينكس
x86_64 وماك Apple silicon. وعلى كل نظام آخر ينتج المترجم الشيفرة عبر LLVM، فيجب تثبيت LLVM قبل البناء.

## لينكس (x86_64)

ثبّت الأدوات بمدير الحزم في توزيعتك:

```console
$ sudo apt install git cmake build-essential     # Debian و Ubuntu
$ sudo dnf install git cmake gcc make            # Fedora
$ sudo pacman -S git cmake base-devel            # Arch
```

ثم ابنِ:

```console
$ git clone https://github.com/sultan-language/sultanc.git
$ cd sultanc
$ ./build.sh --bootstrap=wasm
```

## ماك (Apple silicon)

يأتي مترجم C مع أدوات سطر الأوامر من Apple. و CMake متاح من Homebrew أو من [cmake.org](https://cmake.org/download/):

```console
$ xcode-select --install
$ brew install cmake
```

ثم ابنِ بالأوامر نفسها التي في لينكس:

```console
$ git clone https://github.com/sultan-language/sultanc.git
$ cd sultanc
$ ./build.sh --bootstrap=wasm
```

## ويندوز وكل نظام آخر

ثبّت Git و CMake ومترجم C و LLVM من 19 إلى 23، ثم ابنِ بطريق LLVM:

```console
$ git clone https://github.com/sultan-language/sultanc.git
$ cd sultanc
$ ./build.sh --bootstrap=c --enable-llvm --backend=llvm
```

ويجد البناء LLVM في أماكن تثبيته المعتادة. وإن كان في مكان آخر، فسمِّ `llvm-config` الخاص به هكذا:
`LLVM_CONFIG=/path/to/llvm-config ./build.sh …`.

وفي ويندوز، لا يحوي LLVM الذي يثبّته المثبِّت الرسمي كل المعماريات ولا مكتبة LLVM-C التي يرتبط بها السلطان، فيجب بناء LLVM
من مصدره مرة واحدة. ويفعل ذلك السكربت `.github/ci/windows.ps1` في المستودع: مهمته `llvm-sdk` تبني LLVM، ومهمته
`llvm-build` تبني السلطان معه.

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
