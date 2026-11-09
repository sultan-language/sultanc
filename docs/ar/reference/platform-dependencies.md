**العربية** | [English](../../en/reference/platform-dependencies.md)

# اعتماديات المكتبة القياسية على المنصات

تسرد هذه الصفحة كل دالة تستوردها المكتبة القياسية من خارج السلطان: المكتبة التي تأتي منها، وسبب كون تلك المكتبة هي
الحد المستقر للمنصة، وتصريح C الذي رُوجعت عليه، وكيف تتعامل سلاسل الأدوات الناضجة مع الحد نفسه. وتسرد
أيضًا ما تنفّذه المكتبة القياسية بلغة السلطان بدل استدعاء المنصة. وهي موجهة إلى المساهمين الذين يضيفون شيفرة خاصة
بالمنصات أو ينقلونها، وإلى كل من يدقق فيما يُربط مع ملف السلطان التنفيذي.

القاعدة هي التي تتبعها Rust وGo: لا تعتمد المكتبة القياسية إلا على واجهة النظام المدعومة في المنصة، ولا تستعمل مكتبة
تشغيل C لميزات اللغة مثل نسخ الذاكرة والتنسيق والرياضيات وتخصيص الذاكرة والتشفير وDNS. وكل استيراد يذكر
مكتبته عند التصريح نفسه، داخل المكتبة القياسية، ولا يحتفظ المترجم بأي جدول لرموز المكتبة القياسية.

| المنصة | الحد الذي تستعمله المكتبة القياسية | المكتبات المستوردة |
|---|---|---|
| Linux، Android | نداءات النواة الخام | لا شيء |
| FreeBSD | نداءات النواة الخام | لا شيء |
| macOS | libSystem | `/usr/lib/libSystem.B.dylib` |
| Windows | واجهة Win32 في مكتبات DLL النظامية، بلا CRT | `kernel32.dll`, `ws2_32.dll`, `api-ms-win-core-synch-l1-2-0.dll`, `shell32.dll`, `bcryptprimitives.dll`, `crypt32.dll` |
| WASI preview 1 | استيرادات WASI | `wasi_snapshot_preview1` |

## كيف يذكر التصريح مكتبته

```
[عام] خارجي [«المكتبة»] عرف الاسم(المعاملات): النتيجة [= «اسم_الربط»];
```

`«المكتبة»` هي المكتبة بالاسم الذي يعرفها به محمّل الهدف: مكتبة DLL بصيغة PE (`«kernel32.dll»`)، أو
اسم تثبيت Mach-O (`«/usr/lib/libSystem.B.dylib»`)، أو وحدة استيراد WebAssembly
(`«wasi_snapshot_preview1»`).

`= «اسم_الربط»` يربط التصريح برمز محدد لدى الرابط (linker) حين يختلف عن الاسم في المصدر، مثل `= «stat$INODE64»`.
ويؤدي دور `link_name` في Rust ودور وسم asm (asm label) في C. والنصان كلاهما غير فارغين ولا يحتويان بايت NUL.
أما البيانات الخارجية (`خارجي دع س: النوع = «الرمز»;`) فلا تأخذ مكتبة.

اسم الربط الموجود على بعض المعماريات فقط يوضع خلف شرط على الهدف
(`لو (الهدف.المعمارية == «x86_64») { … } وإلا { … }`). فأسماء `$INODE64` في Darwin لا توجد إلا على
x86_64. ويجب أن يحترم جدول الدوال الخارجية في المفسّر (`tools/generate/generate_host_externs.py`) الشرط
نفسه: فربط `stat$INODE64` على كل أهداف macOS سيمنع المترجم من التحميل على أجهزة Mac بمعمارية arm64.

تفحص `tools/audit/macos_imports_audit.py --sdk <MacOSX.sdk> --arch arm64|x86_64 <binary>…` كل استيراد
في ملف Mach-O تنفيذي مقابل صادرات `.tbd` في SDK لتلك المعمارية. فـ dyld يرفض الملف الثنائي إذا غاب أي
استيراد، وهذا الفحص يكتشف ذلك دون جهاز Mac.

| سلسلة الأدوات | الصيغة | المقابل في السلطان |
|---|---|---|
| Rust | `#[link(name = "kernel32")] extern "system" { ... }`؛ وتعريفات Windows في مكتبة Rust القياسية تستعمل `windows_targets::link!("kernel32.dll" "system" fn ...)` (raw-dylib)؛ و`#[link(wasm_import_module = "wasi_snapshot_preview1")]`؛ و`#[cfg_attr(all(target_os = "macos", target_arch = "x86_64"), link_name = "stat$INODE64")]` في libc | نص المكتبة مع `= «اسم_الربط»` |
| Go | في Windows: `//sys GetLastError() ... = kernel32.GetLastError` (mkwinsyscall) و`//go:cgo_import_dynamic runtime._CloseHandle CloseHandle%1 "kernel32.dll"`؛ وفي darwin: `//go:cgo_import_dynamic libc_read read "/usr/lib/libSystem.B.dylib"` | هوية المكتبة نفسها لكل رمز |

### كيف تصل المكتبة إلى الملف الثنائي

يخزّن المحلل النحوي المكتبة واسم الربط في التصريح
(`compiler/frontend/ast/declarations.sn`: `تصريح_دالة.المكتبة_الخارجية` و`اسم_الربط`). ثم ينقلهما الخفض
إلى MSIR في `دالة_خارجية_مسير.الاسم` (رمز الرابط) و`.المكتبة`. ويستعملهما كل هدف بعد ذلك كما يلي.

| صيغة الهدف | السلوك |
|---|---|
| WebAssembly | كاتب wasm الأصلي ومزوّد LLVM (`wasm-import-module` و`wasm-import-name`) كلاهما يستورد الدالة من الوحدة المصرّح بها، باسم استيراد صريح. والتصريح الذي لا وحدة له يستورد من `env`، وهي القيمة الافتراضية في wasm-ld وclang وrustc. ويستورد رابط wasm الدوال غير المعرّفة من الوحدة التي يذكرها ملف الكائن الخاص بها؛ فملفات الكائنات في wasm تحمل وحداتها بنفسها. |
| PE/COFF | يسجّل منسق المترجم كل زوج (رمز، DLL) مصرّح به في خطة الربط (`compiler/link/model.sn`، `استيراد_مكتبة_مصرح_ربط`). ويحوّل رابط COFF كل رمز غير معرّف له DLL مصرّح بها إلى استيراد من تلك الـ DLL (`استورد_رموز_مصرحة_كوف_في_السياق`، `compiler/link/coff/directives.sn`). ولا يحتاج ذلك إلى مكتبة استيراد، مثل raw-dylib في rustc. |
| Mach-O | يُربط الرمز غير المعرّف برقم ترتيب الـ dylib التي يذكرها تصريحه (`ترتيب_مكتبة_رمز_ماتشو`، `compiler/link/macho/final_image.sn`). ولا تُحمَّل الـ dylib المصرّح بها إلا إذا استُوردت إحدى دوالها. وإذا لم تُذكر مكتبة رُبط الرمز بـ libSystem، ورقم ترتيبها دائمًا 1، وهذا صحيح لأن libSystem هي واجهة النظام في macOS. |
| ELF | لا أثر للمكتبة المصرّح بها. تُحلّ الرموز الخارجية من مدخلات الربط (`-l c` وما شابه). ولا تصرّح المكتبة القياسية بأي دالة خارجية على Linux أو Android أو FreeBSD. |

يذكر المترجم مكتبة واحدة بنفسه: مكتبة LLVM (`LLVM-C.dll`، `مكتبة_إل_إل_في_إم_باسم_للهدف` في
`compiler/backend/target/runtime_symbol_query_context.sn`)، لأن LLVM اعتمادية للمترجم لا للمكتبة القياسية. ولا
تتطلب إضافة دالة خارجية إلى المكتبة القياسية تعديل المترجم أبدًا.

### الاستدعاءات و`غير_آمن`

يجب أن يقع نداء النواة الخام (`بدائي_نداء_نواة_خام`) داخل `غير_آمن`. أما استدعاء دالة خارجية فلا يحتاج
إلى `غير_آمن`، بقرار من مالك المشروع: فتصريح الدالة الخارجية توقيع ذو أنواع، والمكتبة القياسية تحفظ وسائطها ونتائجها
من المؤشرات داخل أغلفة محدودة.

## Linux وAndroid وFreeBSD: نداءات نواة خام بلا libc

تستدعي المكتبة القياسية النواة مباشرة عبر `بدائي_نداء_نواة_خام`. يستعمل Linux وAndroid
`library/std/os/platform/unix/linux/**`، مع جداول نداءات نظام مولَّدة من ترويسات UAPI لكل معمارية.
ويستعمل FreeBSD `platform/unix/freebsd`، الذي يشارك مزوّد النواة المباشر. ولا يستورد أيٌّ منهما شيئًا.
وهذا نموذج Go: فحزمتا `syscall` و`runtime` في Go تُصدران نداءات نظام خامًا على Linux وFreeBSD ولا
تربطان أي libc، لأن كلتا النواتين تضمن واجهة ثنائية (ABI) مستقرة لنداءات النظام.

عمليات النواة التي تتجاوز نداءات الملفات والعمليات الأساسية موجودة في `syscalls/operations.sn`، مع
توقيعات للمفسّر. وأرقامها مولَّدة لكل واجهة ثنائية في Linux ولـ FreeBSD.

| العملية | Linux | FreeBSD | يستعملها |
|---|---|---|---|
| `وصلة_رمزية_عند` | `symlinkat` | `symlinkat` | `linux/file_attributes.sn` |
| `وصلة_صلبة_عند` | `linkat` | `linkat` | `linux/file_attributes.sn` |
| `قراءة_وصلة_عند` | `readlinkat` | `readlinkat` | `linux/file_attributes.sn` |
| `أذونات_مسار_عند` | `fchmodat` | `fchmodat` | `linux/file_attributes.sn` |
| `اقتطاع_واصف` | `ftruncate` (طول 32 بت على i386 وARM: تُرفض الأطوال الأكبر) | `ftruncate` | `linux/file_attributes.sn` |
| `أوقات_مسار_عند` | `utimensat` (`utimensat_time64` على i386 وARM) | `utimensat` | `linux/file_attributes.sn` |
| `تغيير_دليل_العمل` | `chdir` | `chdir` | `linux/file_attributes.sn` |
| `مضاعفة_واصف` | `dup3` | `dup2` | العملية الفرعية في `شغل_طفلا_لينكس_بإعداد` |
| `اسم_مقبس_محلي` / `اسم_مقبس_نظير` | `getsockname` / `getpeername` | مثلها | `linux/socket_names.sn` |
| `إرسال_إلى_عنوان` / `استقبال_من_عنوان` | `sendto` / `recvfrom` مع عنوان | مثلها | `linux/socket_names.sn` |
| `قفل_ملف` | `flock` | `flock` | `os/file_lock.sn` (`std.fs.lock`) |
| `حجب_إشارات` | `rt_sigprocmask` | `sigprocmask` | `linux/child_stdio.sn`: حجب SIGPIPE حول الكتابة إلى دخل عملية فرعية |
| `انتظر_إشارة` | `rt_sigtimedwait` (`rt_sigtimedwait_time64` على i386 وARM) | `sigtimedwait` | الكتابة نفسها: تأخذ إشارة SIGPIPE التي تركتها كتابة EPIPE معلّقة |
| `مستقبل` | `futex` (WAIT وWAKE، `_PRIVATE`) | `_umtx_op` (`UMTX_OP_WAIT_UINT_PRIVATE` و`UMTX_OP_WAKE_PRIVATE`) | `linux/wait_address.sn`، `freebsd/wait_address.sn` (`std.os.wait_address`)؛ وانتظار انتهاء الخيط في Linux؛ أما انتظار انتهاء الخيط في FreeBSD فينتظر بـ `UMTX_OP_WAIT` على كلمة الحالة الخاصة بـ `thr_exit` |
| `مستقبل_بمهلة` | `futex` بمهلة نسبية (`futex_time64` على i386 وARM) | لا شيء | `linux/wait_address.sn` |
| `مستقبل_بمهلة_بي_إس_دي` | لا شيء | `_umtx_op` مع `struct timespec` نسبي في `uaddr2` | `freebsd/wait_address.sn` |
| `نسخ_عملية` (صيغة الخيط) | `clone(VM\|FS\|FILES\|SIGHAND\|THREAD\|SYSVSEM\|PARENT_SETTID\|CHILD_CLEARTID)`، يصدره `بدائي_استنسخ_خيط_نواة` | لا شيء | `linux/threads.sn` (`std.thread`) |
| `إنشاء_خيط` | لا شيء | `thr_new` | `freebsd/threads.sn` |
| `إنهاء_خيط` | `exit` (آخر ما ينفذه الابن الناتج عن clone) | `thr_exit` | آخر نداء في الخيط |
| `تنازل_جدولة` | `sched_yield` | `sched_yield` | `std.thread.تنازل` |
| `معرف_الخيط` | `gettid` | `thr_self` | `std.thread.معرف_الخيط_الحالي` |
| `ألفة_المعالج` | `sched_getaffinity` | `cpuset_getaffinity`، ثم `sysctl hw.ncpu` | `std.thread.عدد_المعالجات_المتاحة` |

لـ `flock` توقيع في المفسّر من عددين صحيحين، ويُرتَّب وسائط نداءي الإشارات كما في `clone` و`waitid`.
وتوفّر مولّدات UAPI قيم SIGPIPE وSIG_BLOCK وSIG_SETMASK (1 و3 على MIPS وFreeBSD، و1 و4 على SPARC)،
و`LOCK_SH/EX/NB/UN`، و`EAGAIN`، و`EPIPE`، و`AF_UNIX`، و`O_EXCL`، و`O_RDWR`، وحجم `sigset_t` في النواة
(8 بايتات؛ و16 على MIPS وFreeBSD) وحجم `sun_path` في `sockaddr_un` (108؛ و104 على FreeBSD). ويستعمل
الدخل والخرج القياسيان للعملية الفرعية عمليتي `pipe2` و`ppoll` الموجودتين. ولا تحتاج مقابس نطاق Unix (Unix domain) إلى
نداء خاص: فـ `socket(AF_UNIX, ...)` و`bind` و`connect` و`accept4` و`sendto` و`recvfrom` ونداءات الأسماء
تأخذ `sockaddr_un` يبنيه `os/unix_socket.sn`.

### الكتابة إلى دخل عملية فرعية دون SIGPIPE

قد تُطلق الكتابة إلى أنبوب إشارة SIGPIPE، وهي تنهي العملية إذا كانت العملية الفرعية قد أغلقت دخلها.
تتجنبها المقابس بـ `MSG_NOSIGNAL` (Linux، FreeBSD) أو `SO_NOSIGPIPE` (macOS)؛ أما الأنابيب فلا تملك أيًّا
منهما. تتجاهل المكتبة القياسية في Rust إشارة SIGPIPE للعملية كلها وتعيد ضبطها في كل عملية فرعية قبل exec. أما المكتبة القياسية
هنا فتحجب SIGPIPE في الخيط المستدعي طوال عملية الكتابة الواحدة، وبعد EPIPE تأخذ الإشارة المعلّقة بـ
`sigtimedwait` بمهلة صفرية، كما تفعل libpq في PostgreSQL عند الكتابة إلى مقابسها (`pq_block_sigpipe`،
`pq_reset_sigpipe`). ولا تتغير أي حالة إشارات على مستوى العملية، ولا ترث العملية الفرعية شيئًا.

## macOS: مكتبة libSystem

لا تدعم Apple نداءات النظام المباشرة: فواجهة نداءات النظام الثنائية خاصة وتتغير بين الإصدارات. والحد
المستقر هو libSystem (`/usr/lib/libSystem.B.dylib`، التي تعيد تصدير libsystem_kernel وlibsystem_c وlibdyld
وغيرها). وتمر Go عبر libSystem على darwin منذ Go 1.12، وتربطها المكتبة القياسية في Rust عبر حزمة libc. وتذكرها المكتبة القياسية
في كل تصريح، كما تفعل Go.

رُوجعت التوقيعات على ترويسات SDK في macOS. في كلتا واجهتي Darwin الثنائيتين، `mode_t` هو u16، و`off_t`
و`ssize_t` هما i64، و`size_t` هو u64، و`socklen_t` هو u32. وكل الوحدات أدناه داخل
`library/std/os/platform/unix/apple/darwin/`.

| الوحدة | الدوال وتصريحات C |
|---|---|
| `file_descriptors.sn` | `int *__error(void)`؛ `int open(const char *, int, ...)` (تُستدعى بوسائط ثابتة فقط، لأن الجزء المتغير على arm64 يُمرَّر عبر المكدس)؛ `int creat(const char *, mode_t)`؛ `ssize_t read(int, void *, size_t)`؛ `ssize_t write(int, const void *, size_t)`؛ `int close(int)`؛ `off_t lseek(int, off_t, int)`؛ `int fsync(int)`؛ `int fchmod(int, mode_t)`؛ `int rename(const char *, const char *)`؛ `int unlink(const char *)`؛ `int stat(const char *, struct stat *)`، مربوطة بـ `stat$INODE64` على x86_64؛ `char *getcwd(char *, size_t)` |
| `directories.sn` | `DIR *opendir(const char *)` (`opendir$INODE64` على x86_64)؛ `struct dirent *readdir(DIR *)` (`readdir$INODE64` على x86_64)؛ `int closedir(DIR *)`؛ `int mkdir(const char *, mode_t)`؛ `int rmdir(const char *)` |
| `process_control.sn` | `int posix_spawnp(pid_t *, const char *, const posix_spawn_file_actions_t *, const posix_spawnattr_t *, char *const[], char *const[])`؛ `posix_spawnattr_init`، `posix_spawnattr_destroy`، `posix_spawnattr_setflags(posix_spawnattr_t *, short)`؛ `posix_spawn_file_actions_init`، `_destroy`، `_addinherit_np(…, int)`؛ `int posix_spawn_file_actions_adddup2(posix_spawn_file_actions_t *, int, int)`؛ `int posix_spawn_file_actions_addchdir_np(posix_spawn_file_actions_t *, const char *)` (macOS 10.15 فما بعد)؛ `pid_t waitpid(pid_t, int *, int)`؛ `int kill(pid_t, int)`؛ `void _exit(int)` |
| `sockets.sn` | `socket`، `connect`، `bind`، `listen`، `accept`، `ssize_t send(int, const void *, size_t, int)`، `recv`، `shutdown`، `setsockopt(int, int, int, const void *, socklen_t)`، `int getaddrinfo(const char *, const char *, const struct addrinfo *, struct addrinfo **)`، `void freeaddrinfo(struct addrinfo *)`. تأخذ bind وconnect قيمة `sockaddr_in` أو `sockaddr_in6` أو `sockaddr_un` (`sun_len`، و`AF_UNIX` قيمته 1، و`sun_path` بطول 104 بايتات). |
| `socket_names.sn` | `int getsockname(int, struct sockaddr *, socklen_t *)`؛ `int getpeername(...)`؛ `ssize_t sendto(int, const void *, size_t, int, const struct sockaddr *, socklen_t)`؛ `ssize_t recvfrom(int, void *, size_t, int, struct sockaddr *, socklen_t *)` |
| `virtual_memory.sn` | `void *mmap(void *, size_t, int, int, int, off_t)`، `int mprotect(void *, size_t, int)`، `int munmap(void *, size_t)`، `int mlock(const void *, size_t)`، `int munlock(const void *, size_t)`، `int madvise(void *, size_t, int)`، `int getpagesize(void)`. هذه هي التصريحات الوحيدة لهذه الدوال: `os/virtual_memory.sn` يستدعيها من هنا. |
| `clocks.sn` | `int clock_gettime(clockid_t, struct timespec *)`؛ `uint64_t mach_absolute_time(void)`؛ `kern_return_t mach_timebase_info(mach_timebase_info_t)`؛ `uint64_t mach_continuous_time(void)`؛ `kern_return_t mach_wait_until(uint64_t)` |
| `entropy.sn` | `int getentropy(void *, size_t)` |
| `dynamic_loader.sn` | `int dladdr(const void *, Dl_info *)` |
| `file_attributes.sn` | `int symlink(const char *, const char *)`؛ `ssize_t readlink(const char *, char *, size_t)`؛ `int link(const char *, const char *)`؛ `int chmod(const char *, mode_t)`؛ `int ftruncate(int, off_t)`؛ `int utimensat(int, const char *, const struct timespec[2], int)` (قيمة `AT_FDCWD` هي -2)؛ `int chdir(const char *)`. يتجنب الإنشاء الحصري وسيط الوضع المتغير في `open`: يُنشأ بـ `creat` ملف مجاور باسم فريد، ثم يُربط بـ `link` بالمسار المطلوب (و EEXIST ذرّي)، ثم يُحذف الملف المجاور بـ `unlink`، ثم `open(O_RDWR\|O_CLOEXEC)`. |
| `child_stdio.sn` | `int pipe(int[2])`؛ `int poll(struct pollfd *, nfds_t, int)` (`nfds_t` هو u32)؛ `int pthread_sigmask(int, const sigset_t *, sigset_t *)`، `int sigpending(sigset_t *)`، `int sigwait(const sigset_t *, int *)` (`sigset_t` هو u32). توفّر هذه الكتابة الآمنة من SIGPIPE إلى دخل عملية فرعية، وهو مسار libpq على Darwin؛ أما `F_SETNOSIGPIPE` فيحتاج إلى الوسيط المتغير في fcntl. |
| `file_locks.sn` | `int flock(int, int)` (`<sys/file.h>`) |
| `wait_address.sn` | `int __ulock_wait2(uint32_t op, void *addr, uint64_t value, uint64_t timeout_ns, uint64_t value2)` (macOS 11 فما بعد) و`int __ulock_wake(uint32_t op, void *addr, uint64_t wake_value)`، من libsystem_kernel عبر libSystem. وهما طبقة futex تحت `std.os.wait_address` و`std.sync`، وهو ما اختارته Rust قبل `os_sync_wait_on_address` (macOS 14.4). |
| `threads.sn` | `int pthread_create(pthread_t *, const pthread_attr_t *, void *(*)(void *), void *)`؛ `int pthread_join(pthread_t, void **)`؛ `pthread_attr_init`، `pthread_attr_setstacksize(pthread_attr_t *, size_t)`، `pthread_attr_destroy` (`pthread_attr_t` حجمه 64 بايتًا)؛ `int pthread_threadid_np(pthread_t, uint64_t *)`؛ `void pthread_yield_np(void)`؛ `int sysctlbyname(const char *, void *, size_t *, void *, size_t)` (`hw.logicalcpu`). دالة البدء هي مدخل C لدالة مغلقة (closure) (`بدائي_مدخل_سي_لدالة`)، كما تبدأ المكتبة القياسية في Rust و`newosproc` في Go الخيوطَ بـ `pthread_create`. |

### تفاصيل واجهة Darwin الثنائية

- **واجهة inode الثنائية على x86_64.** تقرأ المكتبة القياسية البنية `struct stat` ذات الـ inode بعرض 64 بت (144 بايتًا)
  والبنية `struct dirent` (`d_namlen` من نوع u16 عند الإزاحة 18). على x86_64 تكون `stat` و`opendir`
  و`readdir` العادية هي الدوال القديمة ذات الـ inode بعرض 32 بت، التي تحوّلها `<sys/stat.h>` و`<dirent.h>`
  إلى `stat$INODE64` و`opendir$INODE64` و`readdir$INODE64` (`__DARWIN_INODE64`، `__DARWIN_ALIAS_I`).
  وتُربط التصريحات باسم الربط تحت شرط على المعمارية، كما تفعل libc في Rust. أما arm64 فيستعمل الأسماء
  العادية.
- **ترتيب حقول `struct addrinfo`.** تستعمل `<netdb.h>` من Apple ترتيب BSD: يأتي `ai_canonname` قبل
  `ai_addr`، وهو عكس glibc. ويتبع السجل في المكتبة القياسية ترتيب Darwin. ولو قُرئ العنوان من `ai_canonname` لفشل كل
  بحث، لأن هذا الحقل يكون NULL دون `AI_CANONNAME`.
- **تصريح واحد لكل دالة.** يُصرَّح بكل دالة من libSystem مرة واحدة: `close` في `file_descriptors.sn`
  (ويستعملها `sockets.sn`)، و`mmap` و`mprotect` و`madvise` في `virtual_memory.sn` فقط.

## Windows: مكتبات DLL النظامية بلا CRT

واجهة Win32 في مكتبات DLL النظامية هي الحد المدعوم على Windows، وأرقام نداءات NT خاصة. لا تربط المكتبة القياسية أي
مكتبة تشغيل C: لا يوجد `mainCRTStartup`، ولا `memcpy` ولا `malloc` ولا `printf`. وتُذكر الـ DLL عند كل
تصريح، كما تفعل المكتبة القياسية في Rust عبر `windows_targets::link!`.

رُوجعت التوقيعات على تصريحات MSDN. `BOOL` و`int` من نوع i32؛ و`DWORD` و`UINT` و`ULONG` (وكذلك
`u_long`، لأن `long` في Windows بعرض 32 بت) من نوع u32؛ و`HANDLE` و`SOCKET` و`SIZE_T` وأنواع المؤشرات
بعرض 64 بت؛ و`LARGE_INTEGER` من نوع i64 يُمرَّر بالقيمة أو بمؤشر. والوحدات داخل
`library/std/os/platform/windows/`.

### kernel32.dll

| المجال (الوحدة داخل `win32/`) | الدوال |
|---|---|
| الملفات (`file_handles.sn`، `file_paths.sn`، `directory_streams.sn`) | `GetStdHandle`، `CreateFileW`، `ReadFile`، `WriteFile`، `CloseHandle`، `FlushFileBuffers`، `GetFileTime`، `GetFileSizeEx`، `SetFilePointerEx`، `CreateDirectoryW`، `DeleteFileW`، `GetFileAttributesW`، `GetFileAttributesExW`، `RemoveDirectoryW`، `MoveFileExW`، `FindFirstFileW`، `FindNextFileW`، `FindClose`. تُفتح الملفات بـ `FILE_SHARE_READ \| WRITE \| DELETE`، وهو وضع المشاركة الافتراضي في Rust، حتى تستطيع عملية أخرى فتح ملف مفتوح في هذه العملية (وقفله). |
| الوصلات والسمات (`file_attributes.sn`) | `BOOLEAN CreateSymbolicLinkW(LPCWSTR, LPCWSTR, DWORD)`، `CreateHardLinkW`، `SetFileAttributesW`، `DeviceIoControl` (`FSCTL_GET_REPARSE_POINT`)، `SetFileInformationByHandle` (`FileEndOfFileInfo`)، `SetFileTime`، `GetFinalPathNameByHandleW` |
| أقفال الملفات (`file_locks.sn`) | `LockFileEx`، `UnlockFileEx` على الملف كله |
| العمليات (`process_control.sn`) | `CreateProcessW`، `WaitForSingleObject`، `GetExitCodeProcess`، `TerminateProcess`، `ExitProcess`، `GetCommandLineW`، `LocalFree`، `GetProcessId` (معرّف العملية الفرعية). try_wait هي `WaitForSingleObject(h, 0)` وkill هي `TerminateProcess(h, 1)`. |
| الدخل والخرج القياسيان للعملية الفرعية (`child_stdio.sn`) | `CreatePipe`، `PeekNamedPipe`، `DuplicateHandle` (نسخ قابلة للوراثة من أجل `STARTF_USESTDHANDLES`)، و`WriteFile` إلى دخل العملية الفرعية، مع كتلة البيئة في `CreateProcessW` (`CREATE_UNICODE_ENVIRONMENT`) و`lpCurrentDirectory`. لا ترث العملية الفرعية إلا هذه النسخ الثلاث، المذكورة في `PROC_THREAD_ATTRIBUTE_HANDLE_LIST` (`STARTUPINFOEXW`، `EXTENDED_STARTUPINFO_PRESENT`)، كما تفعل `syscall.StartProcess` في Go: `InitializeProcThreadAttributeList`، `UpdateProcThreadAttribute`، `DeleteProcThreadAttributeList`، مع وضع القائمة ومصفوفة المقابض التي تشير إليها في كتلة `HeapAlloc` واحدة (`GetProcessHeap`، `HeapFree`). وتُنسخ المصفوفة إليها بـ `RtlMoveMemory` لأن القائمة تحتفظ بعنوانها حتى تُحذف. |
| الذاكرة (`virtual_memory.sn`) | `VirtualAlloc`، `VirtualFree`، `VirtualProtect`، `VirtualLock`، `VirtualUnlock`، `GetSystemInfo`، `GetCurrentProcess`، `PrefetchVirtualMemory` (Windows 8 فما بعد) |
| الوقت (`clocks.sn`) | `GetSystemTimePreciseAsFileTime`، `QueryPerformanceCounter`، `QueryPerformanceFrequency`، `GetTickCount64`، `Sleep`، `CreateWaitableTimerW`، `SetWaitableTimer` |
| البيئة (`environment.sn`، `environment_block.sn`) | `GetEnvironmentVariableW`، `SetEnvironmentVariableW`، `GetCurrentDirectoryW`، `GetTempPathW`، `SetCurrentDirectoryW`، `GetEnvironmentStringsW`، `FreeEnvironmentStringsW` |
| الأخطاء (`error.sn`) | `GetLastError`، `SetLastError` |
| الخيوط (`threads.sn`، كما في المكتبة القياسية في Rust) | `CreateThread` (2 MiB مع `STACK_SIZE_PARAM_IS_A_RESERVATION`؛ ودالة البدء هي مدخل C لدالة مغلقة)، `WaitForSingleObject`، `GetExitCodeThread`، `CloseHandle`، `SwitchToThread`، `GetCurrentThreadId`، `GetSystemInfo` (`dwNumberOfProcessors`) |

### مكتبات DLL أخرى

| DLL | الدوال (الوحدة داخل `win32/` أو `winsock/`) |
|---|---|
| `ws2_32.dll` | `winsock/sockets.sn`: `socket` (تعيد `SOCKET` من نوع u64؛ و`AF_UNIX` قيمته 1 من أجل `std.net.unix`، مع مقابس التدفق على Windows 10 1803 فما بعد)، `closesocket`، `connect`، `bind`، `listen`، `send` و`recv` (بأطوال من نوع `int`)، `accept`، `shutdown`، `ioctlsocket(SOCKET, long, u_long *)`، `setsockopt`، `getsockopt`، `getsockname`، `getpeername`، `sendto`، `recvfrom`، `WSAStartup`، `WSACleanup`، `WSAGetLastError`. و`winsock/name_resolution.sn`: `GetAddrInfoW`، `FreeAddrInfoW`. هذه دوال Winsock أصلية، لا `getaddrinfo` الخاصة بـ CRT. وفي `ADDRINFOW` يأتي `ai_canonname` قبل `ai_addr`. |
| `api-ms-win-core-synch-l1-2-0.dll` | `wait_address.sn`: `BOOL WaitOnAddress(volatile VOID *, PVOID, SIZE_T, DWORD)`، `VOID WakeByAddressSingle(PVOID)`، `VOID WakeByAddressAll(PVOID)` (Windows 8 فما بعد). هذه مجموعة واجهات التزامن التي تربطها المكتبة القياسية في Rust لطبقة futex لديها، ويستعملها `std.os.wait_address` وكل ما في `std.sync`. |
| `shell32.dll` | `CommandLineToArgvW(LPCWSTR, int *)` توفّر التقسيم المعتاد للوسائط في Windows، وتحرر المكتبة القياسية تخصيصها الوحيد بـ `LocalFree`. أما المكتبة القياسية في Rust وGo فتحلّلان `GetCommandLineW` بشيفرتهما الخاصة، ولذلك يمكن استبدال هذا الاستيراد بشيفرة السلطان. |
| `bcryptprimitives.dll` | `BOOL ProcessPrng(PBYTE, SIZE_T)`، مولّد الأعداد العشوائية الآمن تشفيريًا (CSPRNG) في النظام، الذي تستعمله المكتبة القياسية في Rust (1.78 فما بعد) ووقت التشغيل في Go |
| `crypt32.dll` | `certificates.sn`: `CertOpenSystemStoreW(HCRYPTPROV_LEGACY, LPCWSTR)`، و`PCCERT_CONTEXT CertEnumCertificatesInStore(HCERTSTORE, PCCERT_CONTEXT)` و`CertCloseStore(HCERTSTORE, DWORD)`. تقرأ هذه مخزن الثقة ROOT الذي يستعمله التحقق في TLS، كما تفعل crypto/x509 في Go وrustls-native-certs. ويُصرَّح بـ `CERT_CONTEXT` سجلًّا، وتُقرأ بايتات DER الخاصة به عبر مؤشره داخل `غير_آمن`. |

## WASI preview 1

WASI preview 1 هي واجهة النظام لوحدة `wasm32-wasi`. تُستورد كل دالة من الوحدة `wasi_snapshot_preview1`،
كما تفعل حزمة `wasi` في Rust بـ `#[link(wasm_import_module = "wasi_snapshot_preview1")]`. وتتبع
التوقيعات تعريفات witx: المقابض والأحجام والمؤشرات من نوع u32 على wasm32، والرايات من نوع u16 أو u8،
والإزاحات والصلاحيات من نوع u64، وكل دالة تعيد `errno` من نوع i32. والوحدات داخل
`library/std/os/platform/wasi/preview1/`.

| الوحدة | الدوال |
|---|---|
| `file_descriptors.sn` | `fd_read`، `fd_write`، `fd_close`، `fd_seek(fd, filedelta i64, whence u8, *filesize)`، `fd_sync`، `fd_prestat_get`، `fd_prestat_dir_name`، `path_open(fd, lookupflags u32, path, path_len, oflags u16, rights u64, rights u64, fdflags u16, *fd)`، `path_filestat_get`، `path_unlink_file`، `path_rename`، `path_create_directory`، `path_remove_directory`، `fd_readdir(fd, buf, len, cookie u64, *bufused)` |
| `sockets.sn` | `sock_accept(fd, fdflags u16, *fd)`، `sock_recv`، `sock_send`، `sock_shutdown(fd, sdflags u8)` |
| `clocks.sn` | `clock_time_get(clockid u32, precision u64, *timestamp)`، `poll_oneoff` |
| `process_startup.sn` | `args_sizes_get`، `args_get`، `environ_sizes_get`، `environ_get` |
| `file_attributes.sn` | `path_symlink(old_path, old_len, fd, new_path, new_len)`، `path_readlink(fd, path, path_len, buf, buf_len, *bufused)`، `path_link(old_fd, old_flags u32, old_path, old_len, new_fd, new_path, new_len)`، `fd_filestat_set_size(fd, size u64)`، `path_filestat_set_times(fd, flags u32, path, path_len, atim u64, mtim u64, fst_flags u16)`. والإنشاء الحصري هو `path_open` مع oflags بقيمة `creat\|excl`. |
| `process_control.sn`، `random.sn` | `proc_exit(u32)`، `random_get(buf, len)` |

لا يوجد في WASI بتات أذونات ولا مجلد عمل ولا عمليات ولا أنابيب ولا مقابس غير متصلة ولا أقفال ملفات ولا
مقابس نطاق Unix. ودوال المكتبة القياسية المقابلة تُبلغ عن ENOTSUP (58).

## ما تنفّذه المكتبة القياسية بلغة السلطان

لا شيء مما يلي يستدعي المنصة.

| القدرة | الموضع | المقابل في Rust أو Go |
|---|---|---|
| تخصيص ذاكرة الكومة | `os/allocation.sn`. يحصل المخصِّص على الصفحات من `mmap` أو `VirtualAlloc` أو `memory.grow` في wasm، ولا يستدعي `malloc` أبدًا. | مخصِّص وقت التشغيل في Go. والافتراضي في Rust هو malloc الخاص بالنظام. |
| نسخ الذاكرة وملؤها | الشيفرة المترجمة. يُصدر مزوّد LLVM التعليمة `llvm.memcpy.inline` حتى لا يظهر أي استدعاء لـ `memcpy` من libc ([MSIR وتوليد الشيفرة](../internals/msir-and-codegen.md)). | `memmove` في وقت تشغيل Go. وتستعمل Rust وحدة `mem` من compiler-builtins على الأهداف التي لا libc فيها. |
| تنسيق النصوص وتحليلها (الأعداد، الأساس العددي، UTF-8) | `std/text/**`، `std/unicode` | Go `strconv`، Rust `core::fmt` |
| رياضيات الأعداد العائمة والصحيحة (`sqrt`، `hypot`، …) | `std/math/**` | حزمة `math` في Go مكتوبة بـ Go خالصة. وRust تستدعي libm. |
| التشفير وTLS 1.3 | `std/crypto/**`، `std/tls/**` ([TLS](../internals/tls.md)) | Go `crypto/tls` |
| DNS على Linux | ينفّذ `os/platform/unix/linux/name_resolution.sn` بروتوكول DNS عبر UDP وTCP إلى خادم الأسماء المضبوط. ولا يستعمل محلّل الأسماء في libc. | محلّل الأسماء المكتوب بـ Go خالصة |
| بدء العملية على Unix | تأتي `_start` ومتجها الوسائط والبيئة من الخلفية (رموز البيانات `sultanc.bootstrap.*`)، بلا crt1.o | `rt0` في Go |
| الضغط، JSON، TOML، CSV، HTTP، URI، UUID | `std/**` | |

يؤكد بحث بـ grep أنه لا تُستورد أي دالة لغة من libc أو CRT. فمن بين الدوال الخارجية المصرّح بها داخل
`library/` لا توجد `memcpy` ولا `memmove` ولا `memset` ولا `memcmp` ولا `strlen` ولا `strcmp` ولا `printf`
ولا `snprintf` ولا `puts` ولا `malloc` ولا `calloc` ولا `realloc` ولا `free` ولا `sin` ولا `cos` ولا `sqrt`
ولا `pow` ولا `exp` ولا `log` ولا `floor` ولا `fmod`، ولا تأتي أي منها من مكتبة DLL لتشغيل C (`ucrtbase`،
`msvcrt`، `vcruntime`).

```sh
grep -rnE '(خارجي|extern)' library | grep -wE 'memcpy|memmove|memset|strlen|printf|malloc|free|sin|cos|sqrt|pow'
```

لا يطبع الأمر شيئًا. وجداول الاستيراد في الأمثلة المبنية تتفق مع ذلك: الملف التنفيذي على Windows لا يستورد
إلا دوال من `kernel32` و`ws2_32` و`crypt32` و`shell32` و`bcryptprimitives`.

## اعتماديات خارج المكتبة القياسية

تبقى اعتماديتان خارج القاعدة السابقة، وليست أي منهما جزءًا من المكتبة القياسية.

- مترجم الإقلاع الذاتي (bootstrap) Stage0 (`bootstrap/`) برنامج مكتوب بـ C. وStage1 الذي ينتجه يستعمل مكتبة
  C في المضيف (`malloc`، `memcpy`)؛ أما Stage2 والمراحل التالية فلا.
- مزوّد LLVM في المترجم يستورد واجهة LLVM البرمجية بلغة C (`compiler/backend/provider/llvm/ffi`)، ومنها
  `LLVMBuildFPExt` و`LLVMBuildFPTrunc` وعائلة `LLVMBuildFPToSI`، ويستعمل الدالتين الداخليتين
  `llvm.fptosi.sat` و`llvm.fptoui.sat`.

## القيود

- يمكن استبدال `CommandLineToArgvW` من `shell32.dll` بشيفرة السلطان، وهذا يزيل ذلك الاستيراد.
- لا توجد طبقة نظام تشغيل في المكتبة القياسية للهدف `ve-linux-gnu`؛ انظر [الأهداف](targets.md).
- ما زال بإمكان التطبيقات طلب libc عبر `-l c` على أهداف ELF، لكن المكتبة القياسية نفسها لا تفعل ذلك أبدًا.

## انظر أيضًا

- [الأهداف](targets.md): مصفوفة الأهداف وكيف يُربط كل هدف ويُتحقق منه.
- [التزامن](concurrency.md): طبقتا الانتظار والإيقاظ والخيوط المبنيتان على هذه النداءات.
- [MSIR وتوليد الشيفرة](../internals/msir-and-codegen.md): كيف تصل التصريحات إلى الخلفيات.
- [تغطية المكتبة القياسية](std-coverage.md): ما توفّره المكتبة القياسية فوق هذه الواجهات.
