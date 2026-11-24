<div dir="rtl">

**العربية** | [English](../../en/book/ch19-00-targets.md)

# المنصات والترجمة لمنصة أخرى

يعمل البرنامج المترجم على نوع واحد من الأجهزة: معمارية معالج واحدة ونظام تشغيل واحد. وهذا الزوج هو *هدف* البرنامج (target).
ويترجم `sultanc` افتراضيا للجهاز الذي يعمل عليه، *المضيف* (host). ومع `--target` يترجم لأي هدف آخر يعرفه، من الحاسوب نفسه،
دون تثبيت أي شيء آخر. وهذه *الترجمة المتقاطعة* (cross-compiling).

## الأهداف

يسرد `sultanc targets` كل هدف يعرفه المترجم. وهذه أسطره الأولى:

```console
$ sultanc targets
⁨الهدف⁩                    ⁨المعمارية⁩    ⁨النظام⁩        ⁨الكائن⁩       ⁨الدعم⁩
⁨arm64-darwin⁩             ⁨aarch64⁩      ⁨داروين⁩        ⁨Mach-O⁩       ⁨مضيف ذاتي⁩
⁨x86_64-linux⁩             ⁨x86_64⁩       ⁨لينكس⁩         ⁨ELF⁩          ⁨مضيف ذاتي⁩
⁨aarch64-linux-gnu⁩        ⁨aarch64⁩      ⁨لينكس⁩         ⁨ELF⁩          ⁨ترجمة متقاطعة⁩
⁨aarch64-windows-msvc⁩     ⁨aarch64⁩      ⁨ويندوز⁩        ⁨COFF⁩         ⁨ترجمة متقاطعة⁩
⁨aarch64-android⁩          ⁨aarch64⁩      ⁨أندرويد⁩       ⁨ELF⁩          ⁨ترجمة متقاطعة⁩
⁨aarch64-freebsd⁩          ⁨aarch64⁩      ⁨فري بي إس دي⁩  ⁨ELF⁩          ⁨ترجمة متقاطعة⁩
⁨x86_64-darwin⁩            ⁨x86_64⁩       ⁨داروين⁩        ⁨Mach-O⁩       ⁨ترجمة متقاطعة⁩
⁨x86_64-windows-msvc⁩      ⁨x86_64⁩       ⁨ويندوز⁩        ⁨COFF⁩         ⁨ترجمة متقاطعة⁩
⁨x86_64-android⁩           ⁨x86_64⁩       ⁨أندرويد⁩       ⁨ELF⁩          ⁨ترجمة متقاطعة⁩
⁨x86_64-freebsd⁩           ⁨x86_64⁩       ⁨فري بي إس دي⁩  ⁨ELF⁩          ⁨ترجمة متقاطعة⁩
...
⁨wasm32-wasi⁩              ⁨wasm32⁩       ⁨واسي⁩          ⁨WebAssembly⁩  ⁨ترجمة متقاطعة⁩
...
```

يقول اسم الهدف معماريته ثم نظام تشغيله: فـ `aarch64-linux-gnu` هو ARM ذو 64 بت يعمل بلينكس. و*الكائن* صيغة ملف البرنامج الذي
يحمّله النظام: ELF على لينكس و FreeBSD، و Mach-O على ماك، و PE/COFF على ويندوز. وأهداف *المضيف الذاتي* هي التي يُبنى المترجم
نفسه ويُختبر عليها، وأهداف *الترجمة المتقاطعة* هي التي يترجم لها.

## الترجمة لهدف آخر

```console
$ sultanc --target=aarch64-linux-gnu main.sn -o hello-arm
$ sultanc --target=x86_64-windows-msvc main.sn -o hello.exe
$ sultanc --target=arm64-darwin main.sn -o hello-mac
$ sultanc --target=wasm32-wasi main.sn -o hello.wasm
```

ويُظهر الأمر `file` ما هو كل منها:

```console
$ file hello-arm hello.exe hello-mac hello.wasm
hello-arm:  ELF 64-bit LSB executable, ARM aarch64, version 1 (SYSV), statically linked, with debug_info, not stripped
hello.exe:  PE32+ executable (console) x86-64, for MS Windows, 7 sections
hello-mac:  Mach-O 64-bit arm64 executable, flags:<|DYLDLINK|PIE>
hello.wasm: WebAssembly (wasm) binary module version 0x1 (MVP)
```

وللمترجم رابطاته (linkers) الخاصة لهذه الصيغ، فلا تشارك أدوات أخرى. وبرامج لينكس و FreeBSD *مربوطة ساكنا* (statically linked)
ولا تستخدم مكتبة C: انسخ الملف إلى جهاز من ذلك الهدف، فيعمل.

و `wasm32-wasi` هو WebAssembly مع WASI، وهي المجموعة القياسية من دوال النظام لبرامج WebAssembly خارج المتصفح. فيعمل برنامج
`.wasm` في أي بيئة تشغيل WASI، على أي جهاز.

## الواجهات الخلفية

الجزء من المترجم الذي ينتج شيفرة الآلة هو *الـ backend*، وله اثنان. فالـ backend الأصلي Native، الخاص بالسلطان، ينتج شيفرة
x86_64 و arm64 و wasm32. والـ backend الخاص بـ LLVM يغطي كل هدف في القائمة. ويسرد `sultanc version` الـ backends التي في
مترجمك، والهدف الذي يحتاج إلى backend غير موجود يقول ذلك:

```console
$ sultanc --target=riscv64-linux-gnu main.sn -o hello-rv
خطأ[SULTANC-BACKEND-UNAVAILABLE]: خلفية توليد الشفرة المختارة غير متاحة في هذا المترجم — الخلفية غير متاحة للهدف ⁨`riscv64-linux-gnu`⁩. خلفية LLVM غير مضمنة في هذا المترجم.
```

والمترجم المبني بـ `--enable-llvm` (الفصل الأول) فيه كلاهما. ويختار `--backend=llvm` الـ LLVM حيث يصلح كلاهما، مثلا لاستخدام
تحسيناته.

## الترجمة المتقاطعة لحزمة

يأخذ سوباكس الخيار نفسه، وتذهب البرامج إلى مجلد باسم الهدف:

```console
$ supax build --target=aarch64-linux-gnu
        يبني تطبيق v0.1.0 (/projects/تطبيق)
       انتهى في 0.14 ث → build/bin/aarch64-linux-gnu/تطبيق
```

## شيفرة تعتمد على الهدف

تخفي المكتبة القياسية أغلب الفروق بين الأنظمة: فـ `اقرأ_الملف` نفسها تعمل في كل مكان. وحين يجب أن يعرف البرنامج نفسه أين يعمل،
يفحص `sultanc check --target=الهدف` أنواعه لذلك الهدف دون بناء، فيكتشف استدعاء غير موجود هناك. ويسرد مرجع
[المنصات](../reference/targets.md) ما يدعمه كل هدف اليوم.

</div>
