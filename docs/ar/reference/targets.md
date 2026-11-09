**العربية** | [English](../../en/reference/targets.md)

# الأهداف

يسرد `sultanc targets` أهداف الترجمة المسجلة وعددها 34. وتسجل هذه الصفحة لكل هدف منها الخلفية التي تولد شيفرته،
ودعم المكتبة القياسية المتوفر له، وطريقة ربط ملفه التنفيذي، وطريقة التحقق منه. وتصف أيضا مواضع الشيفرة الخاصة
بكل هدف في المستودع، للمساهمين الذين يضيفون هدفا أو ينقلون اللغة إلى هدف جديد.

## الخلفيات

للسلطان خلفيتان لتوليد الشيفرة، تُختاران بـ`--backend=native` أو `--backend=llvm`.

الخلفية الأصلية هي مولد الشيفرة الخاص بالسلطان، ولا تحتاج مع الروابط (linker) المدمجة إلى أي سلسلة أدوات
خارجية. وهي متاحة لأربعة أهداف: `x86_64-linux` و`arm64-darwin` و`wasm32-wasi` و`wasm32-browser`. وطلبها لأي
هدف آخر يُبلغ عنه بـ`SULTANC-BACKEND-UNAVAILABLE`.

خلفية LLVM متاحة لكل الأهداف، وتتطلب مترجما بُني مع LLVM (`./build.sh --enable-llvm`). وهي تنتج ملفات كائنات لكل
الأهداف. والأهداف التي لا صيغة لملفات الكائنات فيها (NVPTX، XCore) تُترجم بـ`--emit=asm`، الذي يعمل مع كل الأهداف.

يحمل عمود `الدعم` في مخرجات `sultanc targets` إحدى ثلاث قيم: `مضيف ذاتي` (`arm64-darwin` و`x86_64-linux`)،
و`ترجمة متقاطعة` (كل الأهداف الأخرى)، و`تجريبي` (لا هدف اليوم).

## مصفوفة الأهداف

### أهداف أنظمة التشغيل

لهذه الأهداف المكتبة القياسية كاملة، بما فيها الملفات والعمليات والمقابس والخيوط (إلا حيث يُذكر غير ذلك). وكلها
تستعمل استدعاءات النواة الخام أو واجهة النظام في المنصة، ولا تربط أي libc؛ وتسرد صفحة
[اعتماديات المنصات](platform-dependencies.md) كل ما يُستورد.

| الهدف | المعمارية | الخلفية | الملف التنفيذي | التشغيل المتحقق منه |
|---|---|---|---|---|
| `x86_64-linux` | x86_64 | الأصلية، LLVM | رابط ELF مدمج | أصلي |
| `aarch64-linux-gnu` | aarch64 | LLVM | رابط ELF مدمج | qemu |
| `riscv64-linux-gnu` | riscv64 | LLVM | `ld.lld` | qemu: 12 مثالا مطابقة لـx86_64 |
| `loongarch64-linux-gnu` | loongarch64 | LLVM | `ld.lld` | qemu: الأمثلة الـ12 نفسها |
| `powerpc64le-linux-gnu` | powerpc64le | LLVM | `ld.lld` | qemu: الأمثلة الـ12 نفسها |
| `s390x-linux-gnu` | s390x | LLVM | `ld.lld` | qemu: الأمثلة الـ12 نفسها |
| `mips64el-linux-gnuabi64` | mips64el (n64) | LLVM | `ld.lld` | qemu: الأمثلة الـ12 نفسها |
| `sparcv9-linux-gnu` | sparcv9 | LLVM | `ld.lld` | qemu: الأمثلة الـ12 نفسها |
| `i686-linux-gnu` | x86 | LLVM | `ld.lld` | qemu-i386: كل مثال مطابق لـx86_64 |
| `armv7-linux-gnueabihf` | arm | LLVM | `ld.lld` | qemu-arm: كل مثال مطابق لـx86_64 |
| `x86_64-android` | x86_64 | LLVM | `ld.lld` | qemu-user |
| `aarch64-android` | aarch64 | LLVM | `ld.lld` | qemu-user |
| `armv7-android` | arm | LLVM | `ld.lld` | qemu-arm |
| `arm64-darwin` | aarch64 | الأصلية، LLVM | رابط Mach-O مدمج | أصليا على arm64 (الخلفية الأصلية) |
| `x86_64-darwin` | x86_64 | LLVM | رابط Mach-O مدمج | بُني |
| `x86_64-windows-msvc` | x86_64 | LLVM | رابط COFF مدمج | Wine |
| `aarch64-windows-msvc` | aarch64 | LLVM | رابط COFF مدمج | بُني |
| `x86_64-freebsd` | x86_64 | LLVM | `ld.lld` | آلة افتراضية FreeBSD 14.2 amd64 |
| `aarch64-freebsd` | aarch64 | LLVM | `ld.lld` | آلة افتراضية FreeBSD 14.5 تحت qemu |
| `riscv64-freebsd` | riscv64 | LLVM | `ld.lld` | آلة افتراضية FreeBSD 14.5 تحت qemu |

تستعمل أهداف Linux وAndroid الواجهة الثنائية لنواة Linux مباشرة. وتستعمل الأهداف ذات 32 بت (`i686-linux-gnu`،
`armv7-linux-gnueabihf`، `armv7-android`) استدعاءات النظام الخاصة بـtime64 وبالملفات الكبيرة. وعلى هذه الأهداف،
وعلى كل هدف آخر يفتقر إلى تعليمة قسمة أو إلى قسمة عريضة، توسّع الخلفية القسمة في التمثيل الوسيط (IR) فلا يُربط compiler-rt (انظر
[ما الذي يقع أين](#ما-الذي-يقع-أين)).

تستعمل أهداف FreeBSD استدعاءات النظام الخام بلا libc. وتتطلب FreeBSD 13 أو أحدث (من أجل `close_range`)، وتُبنى
على ترويسات الإصدار 14.2. والملف التنفيذي ساكن (static)، مع `ELFOSABI_FREEBSD` وملاحظة الواجهة الثنائية
`.note.tag`.

### WebAssembly

| الهدف | الخلفية | الملف التنفيذي | ملاحظات |
|---|---|---|---|
| `wasm32-wasi` | الأصلية، LLVM | رابط wasm مدمج | المكتبة القياسية كاملة فوق WASI preview 1؛ والمقابس مقصورة على المفتوحة مسبقا؛ ولا عمليات ولا خيوط ولا أقفال ملفات. يعمل تحت مشغل متوافق مع wasmtime. |
| `wasm32-browser` | الأصلية، LLVM | رابط wasm مدمج | مستقل عن نظام التشغيل (freestanding): نواة اللغة فقط |
| `wasm64-unknown-unknown` | LLVM | ملفات كائنات | مستقل عن نظام التشغيل: نواة اللغة فقط |

### الأهداف المستقلة وأهداف المسرعات

ليس لهذه الأهداف طبقة نظام تشغيل؛ فالبرامج تستعمل نواة اللغة ووحدات `std` التي لا تعتمد على نظام تشغيل.

| الهدف | المعمارية | المخرجات |
|---|---|---|
| `riscv32-unknown-elf` | riscv32 | ملفات كائنات، شيفرة تجميع |
| `avr-unknown-unknown` | avr | ملفات كائنات، شيفرة تجميع |
| `msp430-unknown-elf` | msp430 | ملفات كائنات، شيفرة تجميع |
| `hexagon-unknown-elf` | hexagon | ملفات كائنات، شيفرة تجميع |
| `lanai-unknown-unknown` | lanai | ملفات كائنات، شيفرة تجميع |
| `bpfel-unknown-none` | bpf | ملفات كائنات، شيفرة تجميع |
| `amdgcn-amd-amdhsa` | amdgpu | ملفات كائنات، شيفرة تجميع |
| `spirv64-unknown-unknown` | spirv64 | ملفات كائنات SPIR-V، شيفرة تجميع |
| `nvptx64-nvidia-cuda` | nvptx64 | شيفرة تجميع فقط |
| `xcore-unknown-unknown` | xcore | شيفرة تجميع فقط |

لا يستطيع السلطان ربط هذه الأهداف في ملف تنفيذي: فطلب `exe` يُبلغ عنه بـ`SULTANC-LINK-OUTPUT-UNSUPPORTED`.
استعمل `-c` أو `--emit=obj` لملفات الكائنات، و`--emit=asm` لشيفرة التجميع.

### ما لم يُدعم بعد: ve-linux-gnu

يُترجم `ve-linux-gnu` (NEC Vector Engine) إلى ملفات كائنات وشيفرة تجميع، لكن ليس له طبقة نظام تشغيل في المكتبة القياسية. فلا
يوجد في VE مصيدة نواة: استدعاء النظام طلب يُرسل إلى خدمة VEOS. يُخزن الرقم والوسائط في منطقة ذاكرة مشتركة خاصة بكل
خيط، يضع بدء التشغيل في VEOS وglibc عنوانها في كتلة التحكم بالخيط (`tcbhead_t.shm_offset`، glibc-ve
`sysdeps/unix/sysv/linux/ve/sysdep.h`)، ثم تُنفذ `monc`. ويحتاج النقل إلى بروتوكول بدء التشغيل ذاك، وإلى عتاد VE أو
محاكٍ للتحقق منه.

## ما الذي يقع أين

| الطبقة | المالك | ملاحظات |
|---|---|---|
| الواجهة الثنائية للنواة (أرقام استدعاءات النظام، الرايات، تخطيطات السجلات، ترتيب البايتات) | المكتبة القياسية: `library/std/os/platform/**` | تُولد جداول Linux من ترويسات UAPI لنواة كل معمارية (`tools/generate/generate_linux_syscalls.py`، `generate_linux_uapi.py`). وتأخذ الواجهات الثنائية ذات 32 بت صفوف `ilp32:` من `tools/generate/registry/linux_syscalls.def` (clock_gettime64، ppoll_time64، mmap2، _llseek، fcntl64). وتُولد جداول FreeBSD من ترويساته (releng/14.2) في `platform/unix/freebsd`: جدول واحد لاستدعاءات النظام لكل المعماريات (`generate_freebsd_syscalls.py`، `registry/freebsd_syscalls.def`، ولكل صف نموذجه من `syscalls.master`)، وقيم الرايات وإزاحات `struct stat` وتصنيف errno (`generate_freebsd_uapi.py`، تُقرأ بـ`clang -E -dM` و`offsetof`). |
| مزودات نظام التشغيل (الملفات، العمليات، المقابس، الذاكرة، الساعات) | المكتبة القياسية | فرع واحد محروس لكل نظام تشغيل في `library/std/os/*.sn`. ويخدم مزود النواة المباشر في `platform/unix/linux` نظام FreeBSD أيضا: فهو يأخذ أرقامه من الجدول المختار (`syscalls/selection.sn`) وثوابته من قيم النواة المختارة (`linux/uapi.sn` أو `freebsd/uapi.sn`، بأسماء الدوال نفسها). وحيث يختلف استدعاء FreeBSD، تتولاه كتلة خاصة بـFreeBSD: `fstatat` بدل `statx`، وسجلات `getdirentries`، و`__getcwd`، و`wait4` بدل `waitid`، و`fork` بدل `clone`، و`getrlimit`/`setrlimit` بدل `prlimit64`، و`hw.pagesize` عبر `__sysctl` بدل auxv في procfs. |
| تعليمة الدخول إلى النواة، و`_start`، وتوحيد راية الخطأ | الخلفية (`compiler/backend/provider/llvm/{lowering,entry}.sn`) | الشيفرة الوحيدة الخاصة بكل معمارية خارج المكتبة القياسية. في i386 يُمرر الرقم والوسيط السادس عبر كتلة في المكدس حتى يمكن حفظ EBP. ويشير FreeBSD إلى الأخطاء براية الحمل (amd64، arm64) أو بـt0 (riscv) مع errno موجب، وتحوله التعليمات إلى -errno. وتأخذ `_start` فيه كتلة الوسائط من rdi أو x0 أو a0، ويحمل الملف التنفيذي ملاحظة الواجهة الثنائية `.note.tag` (osrel 1402000) إلى جانب `ELFOSABI_FREEBSD`. |
| العمليات الحسابية التي يفتقر إليها الهدف | الخلفية (`compiler/backend/provider/llvm/wide_division.sn`؛ والضرب المتحقق منه في `lowering.sn`) | توسّع في التمثيل الوسيط فلا يُربط compiler-rt. تُوسّع القسمة على 64 بت في الأهداف ذات 32 و16 بت، وكذلك القسمة بكل عرض في الأهداف التي لا تعليمة قسمة فيها (ARMv7-A، الذي لا يضمن SDIV/UDIV؛ وAVR؛ وMSP430؛ وHexagon؛ وLanai)، بدالة واحدة لكل عرض تعتمد الإزاحة والطرح مع الاستعادة (`sultanc.udivmodN`). ويقسم SPARC V9 المعاملات ذات 8 و16 و32 بت على 64 بت (`SDIVX`/`UDIVX`) بدل `SDIV`/`UDIV` المهملتين اللتين تعتمدان على السجل Y، لأن `sdiv` في QEMU 8.2 يقسم أيضا على المقسوم عليه الممدد بالأصفار ويعطي حواصل قسمة ضيقة ذات إشارة خاطئة. |
| الربط | الروابط المدمجة (ELF لـx86_64 وaarch64 على Linux، وMach-O، وCOFF، وwasm)؛ و`ld.lld` لأهداف Linux ELF الأخرى وFreeBSD (`compiler/driver/external_linker.sn`، `$SULTANC_LINKER`) | ملفات تنفيذية ساكنة بلا libc. |

لا تحتاج الخلفية الأصلية لمعمارية جديدة إلا إلى الدخول إلى النواة و`_start`؛ أما طبقة المكتبة القياسية فمشتركة مع خلفية LLVM.

## التحقق

يُتحقق من كل هدف بأقوى مستوى تسمح به بيئته.

| المستوى | الأهداف | المعنى |
|---|---|---|
| التشغيل الأصلي | `x86_64-linux`؛ و`arm64-darwin` على جهاز Mac بمعالج arm64 | يعمل الملف التنفيذي المبني على المضيف |
| qemu-user | aarch64 وriscv64 وloongarch64 وpowerpc64le وs390x وmips64el وsparcv9 وi686 وarmv7 على Linux؛ وأهداف Android | تعمل الأمثلة تحت `qemu-<arch>` وتُقارن مخرجاتها بمخرجات x86_64 |
| Wine | `x86_64-windows-msvc` | يعمل الملف التنفيذي PE تحت Wine |
| آلة FreeBSD الافتراضية | `x86_64-freebsd`؛ و`aarch64-freebsd` و`riscv64-freebsd` تحت محاكاة النظام في qemu | تعمل الأمثلة في نظام FreeBSD ضيف |
| مشغل WASI | `wasm32-wasi` | مشغل متوافق مع wasmtime |
| البناء | `x86_64-darwin`، `aarch64-windows-msvc` | يُربط الملف التنفيذي؛ ويتحقق `tools/audit/macos_imports_audit.py` من استيرادات Mach-O مقابل SDK |
| ملفات الكائنات وشيفرة التجميع | الأهداف المستقلة وأهداف المسرعات | ينتج المترجم ملفات كائنات أو شيفرة تجميع |

في مقارنة الأمثلة الـ12 تحت qemu لكل من riscv64 وloongarch64 وpowerpc64le وs390x وmips64el وsparcv9، تكون
المخرجات مطابقة لـx86_64. وينجح i686 وarmv7 في كل مثال. وعلى FreeBSD x86_64، كل مثال قابل للبناء مطابق لنظيره على
x86_64 Linux، وتُفحص استدعاءات النظام فيه بـ`truss`. وعلى الآلتين الافتراضيتين aarch64 وriscv64، يعطي كل مثال قابل
للبناء (87 مثالا، مبنية بخلفية LLVM) المخرجات والحالة نفسيهما اللتين يعطيهما على x86_64 Linux. ويخلط `io/logging` بين الخرج القياسي (stdout)
والخطأ القياسي (stderr) بترتيب مختلف عبر ssh، مع تطابق كل تدفق على حدة. ويشغل `tools/tests/freebsd_examples.sh` أمثلة x86_64 على
مضيف Linux عبر أداة تحويل لاستدعاءات النظام تعتمد على ptrace (`tools/tests/freebsd_abi_emulator.c`).

### التحقق المتقاطع من هدف Linux

```sh
./build/sultanc --backend=llvm --target=riscv64-linux-gnu examples/ar/fs/files.sn -o files
qemu-riscv64 ./files
```

### إعادة توليد جداول FreeBSD

لا تقرأ المولدات إلا ترويسات `sys/` و`include/` من شجرة مصدر FreeBSD.

```sh
git clone --depth 1 -b releng/14.2 https://github.com/freebsd/freebsd-src
python3 tools/generate/generate_freebsd_syscalls.py --src freebsd-src
python3 tools/generate/generate_freebsd_uapi.py --src freebsd-src
```

### تشغيل FreeBSD على aarch64 وriscv64 تحت qemu

تعمل صور الآلات الافتراضية الرسمية تحت محاكاة النظام في qemu (TCG؛ ولا حاجة إلى KVM). وعلى Ubuntu 24.04 الحزم
هي `qemu-system-arm qemu-system-misc qemu-efi-aarch64 u-boot-qemu opensbi`. والصور موجودة في
`https://download.freebsd.org/releases/VM-IMAGES/14.5-RELEASE/{aarch64,riscv64}/Latest/`:
`FreeBSD-14.5-RELEASE-arm64-aarch64-ufs.qcow2.xz` و
`FreeBSD-14.5-RELEASE-riscv-riscv64-ufs.qcow2.xz`. تحقق منها مقابل `CHECKSUM.SHA256`. ولم يعد الإصدار 14.2
موجودا على المرآة؛ والإصدار 14.5 يشغل الملفات التنفيذية المبنية على ترويسات 14.2. وتحويل الصورة بـ
`qemu-img convert -c -O qcow2` يخفض حجمها إلى النصف.

```sh
# aarch64: برنامج UEFI الثابت، وأجهزة virtio PCI، وSSH على المنفذ 2223 في المضيف
qemu-system-aarch64 -M virt -cpu cortex-a72 -smp 4 -m 2048 -display none \
  -bios /usr/share/qemu-efi-aarch64/QEMU_EFI.fd \
  -drive file=arm64-aarch64.qcow2,if=none,id=hd0,format=qcow2 -device virtio-blk-pci,drive=hd0 \
  -netdev user,id=n0,hostfwd=tcp:127.0.0.1:2223-:22 -device virtio-net-pci,netdev=n0 \
  -serial unix:ser-aarch64.sock,server,nowait -monitor none

# riscv64: يبدأ OpenSBI، ثم U-Boot (في النمط S)، الذي يُقلع محمّل EFI في الصورة؛ وSSH على المنفذ 2224
qemu-system-riscv64 -M virt -smp 4 -m 2048 -display none \
  -bios /usr/lib/riscv64-linux-gnu/opensbi/generic/fw_jump.bin \
  -kernel /usr/lib/u-boot/qemu-riscv64_smode/u-boot.bin \
  -drive file=riscv-riscv64.qcow2,if=none,id=hd0,format=qcow2 -device virtio-blk-device,drive=hd0 \
  -netdev user,id=n0,hostfwd=tcp:127.0.0.1:2224-:22 -device virtio-net-device,netdev=n0 \
  -serial unix:ser-riscv64.sock,server,nowait -monitor none
```

تُدخل الصور المستخدم `root` في وحدة التحكم التسلسلية بلا كلمة مرور. وعبر مقبس وحدة التحكم، ثبّت مفتاحا في
`/root/.ssh/authorized_keys`، وأضف `PermitRootLogin prohibit-password` إلى `/etc/ssh/sshd_config`، ثم شغّل
`sysrc sshd_enable=YES; service sshd keygen; service sshd start` (فـsshd معطل في الصور، وتوليد مفاتيح المضيف
يستغرق عدة دقائق على riscv64 تحت TCG). انسخ البرنامج بـ`scp -P 2223` (أو 2224) وشغّله عبر `ssh`. وتحت TCG على
مضيف رباعي النوى، تُقلع الآلة الافتراضية aarch64 في نحو 3 دقائق والآلة riscv64 في نحو 10، وتعمل برامج riscv64
أبطأ بنحو عشر مرات من برامج aarch64.

## القيود

- تغطي الخلفية الأصلية أربعة أهداف؛ وكل هدف آخر يحتاج إلى خلفية LLVM.
- ليس لـ`ve-linux-gnu` طبقة نظام تشغيل في المكتبة القياسية.
- لا تنتج الأهداف المستقلة وأهداف المسرعات إلا ملفات كائنات أو شيفرة تجميع.
- يُبنى `x86_64-darwin` و`aarch64-windows-msvc` لكنهما لا يُشغلان.
- ليس في `wasm32-wasi` عمليات ولا خيوط ولا أقفال ملفات، ومقابسه مقصورة على المفتوحة مسبقا.

## انظر أيضا

- [اعتماديات المنصات](platform-dependencies.md): كل واجهة نظام تستوردها المكتبة القياسية، لكل منصة.
- [التزامن](concurrency.md#خيوط-نظام-التشغيل): دعم الخيوط في كل نظام تشغيل.
- [MSIR وتوليد الشيفرة](../internals/msir-and-codegen.md): كيف تتكامل الخلفيات والروابط.
- [الإقلاع الذاتي](../internals/bootstrap.md): بناء المترجم مع LLVM أو بدونه.
