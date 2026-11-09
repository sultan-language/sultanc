**العربية** | [English](../../en/reference/std-coverage.md)

# تغطية المكتبة القياسية

تسرد هذه الصفحة ما توفره المكتبة القياسية في السلطان (`library/std/**`)، وحدة بعد وحدة، وتقارن كل
مجال بالمكتبتين القياسيتين في Rust وGo. وهي مرجع للمبرمجين الذين يريدون معرفة هل تغطي المكتبة القياسية حاجتهم،
وللمساهمين الذين يبحثون عن الثغرات.

لحالة كل صف إحدى ثلاث قيم. **موجود** يعني أن للاستخدامات الشائعة لأداة Rust وGo مقابلا في المكتبة القياسية.
**جزئي** يعني أن بعضها مغطى، وتُذكر الثغرات بأسمائها. **غير موجود** يعني أن المكتبة القياسية ليس فيها شيء. تُذكر
الأسماء بتهجئتها العربية؛ وكل اسم مسرود مع مقابله الإنجليزي في
[المصطلحات العربية](arabic-terminology.md).

تذكر ملاحظات المنصات ما ينقص كل منصة. والمنصات المدعومة هي Linux وAndroid (استدعاءات نواة مباشرة؛
64 بتا، وi686، وarmv7)، وFreeBSD (المزوّد نفسه المبني على النواة مباشرة)، وmacOS (libSystem)،
وWindows (kernel32، وws2_32)، وWASI preview 1. انظر [الأهداف](targets.md) و
[اعتماديات المنصات](platform-dependencies.md).

## فهرس الوحدات

| مسار الوحدة | ما توفره | الأقرب في Rust / Go |
|---|---|---|
| `std.process.*` | إطلاق العمليات، وانتظارها، وحالة الإنهاء، وباني الأوامر، والعمليات الأبناء بتيارات متصلة، والوسائط، و`اخرج`، و`إنهيار`، وتحليل سطر الأوامر | `std::process`, `env::args`; `os/exec`, `flag` |
| `std.env.*` | متغيرات البيئة، ودليل العمل، والدليل الرئيسي والدليل المؤقت | `std::env`; `os` |
| `std.fs.*` | الملفات، والأدلة، والبيانات الوصفية، والأذونات، والوصلات، والملفات المؤقتة، والأقفال، والمسارات | `std::fs`, `std::path`; `os`, `path/filepath` |
| `std.io.*` | دوال الطباعة والقراءة، والقرّاء، والكتّاب، والتخزين المؤقت، والتسجيل | `std::io`, `log`; `io`, `bufio`, `log` |
| `std.fmt.fmt` | سلاسل التنسيق واللغة المصغرة لمواصفات التنسيق | `std::fmt`; `fmt` |
| `std.net.*` | TCP، وUDP، ومقابس Unix، وعناوين IP وعناوين المقابس، وDNS | `std::net`; `net` |
| `std.time.*` | الساعات، والمدد، والتواريخ التقويمية، والمناطق الزمنية | `std::time`, chrono; `time` |
| `std.text.*` | البحث، والتحويل، والبناة، ونصوص الأعداد والأعداد العائمة، ووسوم اللغات، والاتجاه الثنائي (bidi) | `str`, `String`; `strings`, `strconv` |
| `std.unicode.*` | الفئات، والخصائص، وتحويل حالة الأحرف، والتطبيع، والتقطيع | `char`، وحزم unicode؛ `unicode`, `x/text` |
| `std.collections.*` | الخرائط، والمجموعات، وأشجار B، والطابور ثنائي الطرف، والكومة، ومجموعة البتات، والفرز، وأدوات المتجهات | `std::collections`, `Vec`; `container/*`, `slices`, `sort` |
| `std.math.*` | رياضيات الأعداد الصحيحة والعائمة، والحساب الملتف، وحساب المثلثات | دوال الأعداد في `core`؛ `math`, `math/bits` |
| `std.random.prng`, `std.os.entropy` | مولد ببذرة، وعشوائية نظام التشغيل | rand; `math/rand`, `crypto/rand` |
| `std.encoding.*`, `std.json`, `std.toml`, `std.csv`, `std.uri`, `std.mime`, `std.uuid`, `std.version` | صيغ البيانات والمعرّفات | serde_json, toml, csv, url, uuid, semver; `encoding/*`, `net/url`, `mime` |
| `std.crypto.*`, `std.hash.*`, `std.checksum` | دوال التجزئة، وMACs، وKDFs، وAEADs، والتوقيعات، والأعداد الكبيرة، ودوال التجزئة غير التشفيرية، وCRCs | RustCrypto, ring; `crypto/*`, `hash/*` |
| `std.compression.*`, `std.archive.*` | deflate، وgzip، وzstd، وxz، وLZMA؛ وtar، وzip، وar | flate2, zstd, xz2, tar, zip; `compress/*`, `archive/*` |
| `std.http.*`, `std.tls.*` | عميل وخادم HTTP/1.1 وHTTP/2، وTLS 1.3، وX.509 | hyper, rustls; `net/http`, `crypto/tls` |
| `std.sync.*`, `std.thread`, `std.os.wait_address` | الخيوط، والعمليات الذرية، والأقفال، والقنوات | `std::sync`, `std::thread`; `sync`, goroutines |
| `std.error.*` | قيمة الخطأ المشتركة والمحوّلات | `Error`, anyhow; `errors` |
| `std.bytes.*`, `std.chars.ascii` | مخازن البايتات، والمؤشرات القارئة، والمقارنة؛ وتصنيف ASCII | `bytes`; `bytes` |
| `std.debug.assert`, `std.fuzz.core`, `std.meta.reflect`, `std.memory.system` | التأكيدات، ومدخلات fuzzing، والانعكاس على الأنواع، وتعيينات الذاكرة في نظام التشغيل | `assert!`, arbitrary, `mem::size_of`, memmap; `reflect`, `syscall.Mmap` |
| `std.os.*` | طبقة المنصة تحت الوحدات المذكورة أعلاه | `sys`, libc; `syscall`, `runtime` |

## الملخص

| المجال | الحالة | أهم الثغرات |
|---|---|---|
| العمليات | موجود | مجموعات العمليات، والإشارات غير kill، والخطافات على نمط `pre_exec` |
| البيئة | موجود | |
| الملفات | موجود | الملكية (chown)، ومراقبة الأدلة، وأقفال نطاقات البايتات |
| المسارات | موجود | |
| الدخل والخرج | جزئي | محوّلات التيارات العامة (take، وchain، وtee)، وقفل stdin، والدخل والخرج المتجهي (vectored I/O) |
| الشبكات | جزئي | التعدد غير الحاجب (epoll، kqueue)، والبث المتعدد (multicast)، وتمرير الواصفات عبر مقابس Unix |
| الوقت | موجود | |
| النصوص والسلاسل النصية | موجود | وسائط التنسيق المسماة |
| Unicode | موجود | |
| المجموعات | موجود | القائمة المترابطة |
| الرياضيات | موجود | الأعداد الصحيحة الكبيرة خارج التشفير، والأعداد المركبة |
| الترميز | موجود | XML، وgob، وMessagePack، وCBOR |
| التشفير | موجود | مجموعة موجهة إلى TLS: لا bcrypt ولا scrypt ولا argon2 |
| الضغط | جزئي | bzip2، وlz4؛ والترميز التدفقي (التدريجي) |
| HTTP وTLS | موجود | HTTP/3؛ ودفع الخادم (server push) والأولويات في HTTP/2 |
| التزامن والخيوط | موجود | الخيوط ذات النطاق، والمتغيرات المحلية للخيط، والحواجز؛ والترتيبات الذرية الأضعف؛ انظر [التزامن](concurrency.md) |
| الأخطاء | موجود | |

## العمليات

| الأداة | Rust / Go | السلطان | الحالة |
|---|---|---|---|
| الإطلاق، والانتظار، وحالة الإنهاء | `Command::spawn`, `Child::wait`; `exec.Cmd.Start/Wait` | `std.process.child` (`شغل_عملية`, `انتظر_عملية`, `حالة_إنهاء`) | موجود |
| try_wait، وkill، والانتظار بمهلة | `try_wait`, `kill` | `std.process.child` (`تفقد_عملية`, `اقتل_عملية`, `انتظر_عملية_بمهلة`) على Unix؛ والعمليات الأبناء في `std.process.command` (`تفقد_الطفل`, `اقتل_الطفل`) على كل نظام تشغيل فيه عمليات، ومنها Windows | موجود |
| الباني: الوسائط، وضبط البيئة وإزالتها ومسحها، والدليل الحالي | `Command::arg/env/env_remove/env_clear/current_dir` | `std.process.command` | موجود (WASI: لا عمليات) |
| تيارات الابن: وراثة/فارغ/التقاط، والتقاط الخرج، وبايتات الدخل | `Command::output`, `Stdio`; `Cmd.Output`, `CombinedOutput` | `std.process.command` (`اجمع_خرج_الأمر`, `شغل_الأمر`, `مصدر_تيار`) | موجود |
| أنابيب متدفقة إلى ابن يعمل | `Command::spawn` مع `Stdio::piped`, `Child { stdin, stdout, stderr }`, `wait`, `try_wait`, `kill`, `wait_with_output`; `Cmd.StdinPipe/StdoutPipe/Start` | `std.process.command` (`ابدأ_الأمر`، و`طفل_أمر`، و`دخل_طفل` و`خرج_طفل` بشكلي القارئ والكاتب في `std.io`، و`اكتب_إلى_الطفل`، و`اقرأ_خرج_الطفل`، و`أغلق_دخل_الطفل`، و`أغلق_تيارات_الطفل`، و`انتظر_الطفل`، و`تفقد_الطفل`، و`اقتل_الطفل`، و`انتظر_خرج_الطفل`) | موجود. كل تيار إما وراثة أو فارغ أو التقاط. ولعدم وجود دوال إتلاف (destructors)، يكون إغلاق الدخل والأنابيب المتبقية صريحا، و`انتظر_الطفل` تغلق الدخل كما يفعل Rust. والكتابة إلى ابن أغلق دخله تعطي EPIPE لا SIGPIPE. WASI: غير مدعوم. |
| الوسائط، والخروج، والانهيار | `env::args`, `process::exit`, `panic!` | `std.process.arguments`، و`اخرج`، و`إنهيار` | موجود |
| تحليل سطر الأوامر | (حزمة clap)؛ `flag` | `std.process.cli` | موجود |

## البيئة

| الأداة | Rust / Go | السلطان | الحالة |
|---|---|---|---|
| قراءة متغير واحد | `env::var`; `os.Getenv/LookupEnv` | `std.env.environment` (`متغير_بيئة`) | موجود |
| كل المتغيرات | `env::vars`; `os.Environ` | `std.env.variables` (`متغيرات_البيئة`) | موجود |
| ضبط متغيرات العملية نفسها وإزالتها | `env::set_var/remove_var`; `os.Setenv/Unsetenv` | `std.env.variables` (`اضبط_متغير_بيئة`, `أزل_متغير_بيئة`) فوق `std.env.table`، ويرثها `std.process.command` | موجود. قفل واحد على مستوى العملية متى وُجد خيط، مثل `ENV_LOCK` في Rust. |
| قراءة الدليل الحالي وضبطه | `env::current_dir/set_current_dir`; `os.Getwd/Chdir` | `دليل_العمل`, `غير_دليل_العمل` | موجود (WASI: لا chdir) |
| الدليل الرئيسي والدليل المؤقت | `env::home_dir/temp_dir`; `os.UserHomeDir/TempDir` | `الدليل_الرئيسي`, `الدليل_المؤقت` | موجود |

## الملفات والمسارات

| الأداة | Rust / Go | السلطان | الحالة |
|---|---|---|---|
| قراءة الملفات كاملة وكتابتها، والنسخ | `fs::read/write/copy`; `os.ReadFile/WriteFile` | `std.fs.read_file`, `write_file`, `write_bytes` | موجود |
| المقابض: الفتح، والقراءة، والكتابة، والانتقال، والمزامنة، والإغلاق | `File`; `os.File` | `std.fs.handle` | موجود |
| الاقتطاع، وضبط الطول | `File::set_len`; `os.Truncate` | `std.fs.attributes` (`اقتطع_الملف`, `اقتطع_مقبض_ملف`) | موجود |
| البيانات الوصفية: النوع، والحجم، ووقت التعديل، والوجود | `fs::metadata`; `os.Stat` | `std.fs.metadata`, `directory` | موجود |
| ضبط أوقات الملف | `File::set_times`; `os.Chtimes` | `اضبط_أوقات_الملف` | موجود |
| قراءة الأذونات وchmod | `Permissions`, `set_permissions`; `os.Chmod` | `std.fs.permissions` | موجود (Windows: بت القراءة فقط؛ WASI: لا شيء) |
| إنشاء الوصلات الرمزية وقراءتها، والوصلة الصلبة | `os::unix::fs::symlink`, `read_link`, `hard_link`; `os.Symlink/Readlink/Link` | `std.fs.links` | موجود |
| المسار القانوني، والمسار المطلق | `fs::canonicalize`, `path::absolute`; `filepath.EvalSymlinks/Abs` | `std.fs.canonical` | موجود |
| create_dir_all، وremove_dir_all، ونسخ شجرة، والتجوال | `create_dir_all`, `remove_dir_all`; `os.MkdirAll/RemoveAll`, `filepath.WalkDir` | `std.fs.walk` | موجود |
| الملفات والأدلة المؤقتة | (حزمة tempfile)؛ `os.CreateTemp/MkdirTemp` | `std.fs.temporary` | موجود |
| إعادة التسمية، والحذف، وإنشاء دليل، وسرد دليل | `fs::rename/remove_file/read_dir` | `std.fs.mutation`, `directory` | موجود |
| أقفال الملف كاملا | `File::lock/lock_shared/try_lock/try_lock_shared/unlock` (Rust 1.89); `syscall.Flock` | `std.fs.lock` (`اقفل_الملف`، و`اقفل_الملف_مشتركا`، و`حاول_قفل_الملف`، و`حاول_قفل_الملف_مشتركا`، و`افتح_قفل_الملف`؛ و`خطأ_محاولة_قفل.ستحظر` هو `TryLockError::WouldBlock` في Rust) | موجود (flock على Linux وAndroid وFreeBSD وmacOS؛ و`LockFileEx` على Windows؛ WASI: غير مدعوم) |
| chown، والمراقبة | `os::unix::fs::chown`; `os.Chown`, (fsnotify) | لا شيء | غير موجود |
| الضم، والأب، واسم الملف، والامتداد، والمكونات، والتطبيع | `Path`; `path/filepath` | `std.fs.path` (صيغتا POSIX وWindows) | موجود |

## الدخل والخرج

| الأداة | Rust / Go | السلطان | الحالة |
|---|---|---|---|
| print، وeprint، وأسطر stdin | `println!`, `stdin().lines()`; `fmt.Println`, `bufio.Scanner` | `std.io.print`, `println`, `read_line` | موجود |
| القارئ والكاتب ذوا التخزين المؤقت | `BufReader/BufWriter`; `bufio` | `std.io.read_line` (`قارئ_مخزن`), `buffered_writer` | موجود |
| القارئ والكاتب، والنسخ | `Read/Write`, `io::copy`; `io.Reader/Writer`, `io.Copy` | `std.io.reader`, `writer` | موجود |
| المحوّلات (take، وchain، والكاتب المتعدد)، والدخل والخرج المتجهي | `Read::take/chain`; `io.MultiWriter`, `TeeReader` | لا شيء | غير موجود |
| التسجيل | (حزمة log)؛ `log`, `slog` | `std.io.log` | موجود |

## التنسيق

| الأداة | Rust / Go | السلطان | الحالة |
|---|---|---|---|
| سلاسل التنسيق، و`Display` و`Debug`، واللغة المصغرة لمواصفات التنسيق | `format!`, `write!`, `print!`; `fmt.Sprintf`, `Fprintf`, `Printf` | `std.fmt.fmt` (`نسق`، و`نسق_في`، و`اكتب_منسقا`، و`اطبع_سطرا_منسقا` وصيغ الطباعة الأخرى، و`قيمة_تنسيق` مع حالة دالة مغلقة (closure) مخصصة، و`منسق`)؛ و`سجل_منسقا` في `std.io.log` | جزئي. تُفحص سلسلة التنسيق الحرفية وقت الترجمة مقابل قائمة وسائط على شكل متجه حرفي (العدد، والمواضع، والأنواع، والأعداد)؛ وتُفحص سلسلة التنسيق المبنية وقت التشغيل وقت التشغيل. لا وسائط مسماة. ولا تلتقط الدوال المغلقة المخصصة إلا قيما بسيطة. انظر [التنسيق](formatting.md). |

## الشبكات

| الأداة | Rust / Go | السلطان | الحالة |
|---|---|---|---|
| اتصال TCP، والاستماع، والقبول، والقراءة، والكتابة، والإيقاف، والخيارات | `TcpStream/TcpListener`; `net.Dial/Listen` | `std.net.tcp`, `options` | موجود (WASI: المقابس المفتوحة مسبقا فقط) |
| ربط UDP، والاتصال، والإرسال والاستقبال | `UdpSocket` | `std.net.udp` | موجود |
| send_to وrecv_from في UDP دون اتصال | `UdpSocket::send_to/recv_from`; `WriteTo/ReadFrom` | `std.net.socket_names` | موجود |
| العناوين المحلية وعناوين النظير | `local_addr/peer_addr`; `LocalAddr/RemoteAddr` | `std.net.socket_names` | موجود |
| عناوين IP وعناوين المقابس، وDNS | `IpAddr`, `ToSocketAddrs`; `net.ParseIP`, `LookupHost` | `std.net.ip`, `socket_address`, `dns` | موجود |
| المهل | `set_read_timeout` | `خيار_مقبس.مهلة_استقبال` و`مهلة_إرسال` | موجود |
| مقابس Unix: مستمع تدفقي وتدفقات، ورسائل datagram، وعناوين بمسار وعناوين مجردة | `std::os::unix::net::{UnixListener, UnixStream, UnixDatagram, SocketAddr}`; `net.UnixListener/UnixConn` | `std.net.unix` (`مستمع_يونكس`, `تدفق_يونكس`, `مقبس_رسائل_يونكس`, `عنوان_يونكس`) فوق `std.os.unix_socket` | موجود على Linux وAndroid وFreeBSD وmacOS؛ والأسماء المجردة على Linux وAndroid؛ وWindows 10 1803+: التدفقات فقط؛ WASI: غير مدعوم. لا socketpair، ولا تمرير للواصفات، ولا بيانات اعتماد النظير. |
| استطلاع مقابس كثيرة، والبث المتعدد | (mio); `JoinGroup` | لا شيء | غير موجود |

## الوقت

| الأداة | Rust / Go | السلطان | الحالة |
|---|---|---|---|
| الساعة الرتيبة وساعة النظام، والنوم | `Instant`, `SystemTime`, `sleep`; `time.Now`, `Sleep` | `std.time.time` | موجود |
| المدد: الحساب، والمقارنة | `Duration`; `time.Duration` | `std.time.duration`، مع القيمة المطلقة والقسمة المتحققتين | موجود |
| نص المدة | `Duration` (Debug)؛ `Duration.String`, `ParseDuration` | `std.time.duration_text` | موجود |
| التقويم، وRFC 3339 وISO 8601، وتواريخ HTTP | (chrono); `time.Parse/Format` | `std.time.calendar`, `date` | موجود |
| المناطق الزمنية (TZif) | (chrono-tz); `time.LoadLocation` | `std.time.timezone` | موجود |
| الثواني العائمة | `Duration::as_secs_f64`, `try_from_secs_f64`; `Seconds()` | `std.time.duration_float` (`ثواني_المدة_ع64`، و`مدة_من_ثواني_ع64`، وع32) | موجود |

## النصوص وUnicode

| الأداة | Rust / Go | السلطان | الحالة |
|---|---|---|---|
| find، وrfind، وcontains، والبدء والانتهاء بـ | `str::find`; `strings.Index/Contains/HasPrefix` | `std.text.search` | موجود |
| trim، وsplit، وjoin، وreplace، وlines، وrepeat | `trim/split/join/replace`; `strings.TrimSpace/Split/Join` | `std.text.transform` | موجود |
| split_whitespace، وsplitn، وreplacen، وstrip_prefix وsuffix، وحالة أحرف ASCII | `split_whitespace/splitn/replacen/strip_prefix`; `strings.Fields/SplitN/CutPrefix` | `std.text.transform` | موجود |
| العدّ، وعدد المحارف، وeq_ignore_ascii_case | `matches().count`, `chars().count`; `strings.Count`, `EqualFold` | `std.text.search` | موجود |
| الباني، والأعداد، والأساس العددي، والمقارنة | `String`, `parse`; `strings.Builder`, `strconv` | `std.text.builder`, `number`, `radix`, `compare` | موجود |
| تنسيق الأعداد العائمة وتحليلها | `f64::to_string`, `parse`; `strconv.FormatFloat/ParseFloat` | `std.text.float`: `ع64_إلى_نص` (أقصر نص يعيد القيمة نفسها، مع مسار سريع Grisu3)، و`حلل_ع64` (دقيق)، و`أرقام_ع64_الأقصر`، و`أرقام_ع64_الدقيقة`، و`أرقام_ع64_بخانات_كسرية`، و`أرقام_ع64_بخانات_معنوية` (أرقام للمنسقات)؛ وع32 كذلك | موجود |
| اللغات المحلية (locales)، وعزل الاتجاه | (icu) | `std.text.language`, `bidi` | موجود |
| الفئات، والخصائص، وتحويل حالة الأحرف وطيّها، والتطبيع، وتقطيع الوحدات الكتابية (grapheme) والكلمات والجمل | دوال `char` وحزم unicode؛ `unicode`, `x/text` | `std.unicode.**` (جداول Unicode مولَّدة) | موجود |
| UTF-8 وUTF-16 | `str`, `encode_utf16`; `unicode/utf8`, `utf16` | `std.encoding.utf8`, `utf16` | موجود |

## المجموعات

| الأداة | Rust / Go | السلطان | الحالة |
|---|---|---|---|
| خريطة ومجموعة التجزئة، وخريطة ومجموعة B-tree | `HashMap/HashSet/BTreeMap/BTreeSet`; `map` | `std.collections.map`, `set`, `btree_map`, `btree_set`, `text_index_map` | موجود |
| الطابور ثنائي الطرف، والكومة الثنائية، ومجموعة البتات | `VecDeque/BinaryHeap`; `container/heap` | `deque`, `heap`, `bitset` | موجود |
| الفرز، والبحث الثنائي | `sort_by`, `binary_search_by`; `slices.Sort` | `std.collections.sort` | موجود |
| عكس المتجه، والبحث فيه، وموضع الأصغر والأكبر، والتدوير | `reverse/position/iter().max_by/rotate_left`; `slices.Reverse/IndexFunc` | `std.collections.vector` | موجود |
| الإزالة من متجه: pop، وtruncate، وretain، وdedup، وremove، وswap_remove، وinsert، وclear | `Vec::pop/truncate/retain/dedup_by/remove/swap_remove/insert/clear`; `slices.DeleteFunc/CompactFunc/Delete/Insert` | الأداتان الأساسيتان `أزل_الأخير` و`اقتطع`؛ و`std.collections.vector` (`احتفظ_في_المتجه`، و`أزل_المتكرر_المتتالي`، و`أزل_عند`، و`أزل_بالتبديل`، و`أدرج_عند`، و`امسح_المتجه`) | موجود |
| سعة المتجه: capacity، وreserve، وreserve_exact، وwith_capacity، وshrink_to_fit | `Vec::capacity/reserve/reserve_exact/with_capacity/shrink_to_fit`; `cap`, `make([]T, 0, n)`, `slices.Grow/Clip` | الأداتان الأساسيتان `سعة` و`احجز_سعة`؛ و`std.collections.vector` (`متجه_بسعة`، و`احجز_في_المتجه`، و`قلص_المتجه`) | موجود. يحتفظ الاقتطاع، والإزالة حتى الفراغ، والمسح بالمخزن، كما يفعل `Vec` في Rust. |
| القائمة المترابطة | `LinkedList`; `container/list` | لا شيء | غير موجود |

## الرياضيات والعشوائية

| الأداة | Rust / Go | السلطان | الحالة |
|---|---|---|---|
| الأعداد الصحيحة: العمليات المتحققة والمشبعة، وgcd، والقوى، والبتات | `checked_*`, `leading_zeros`; `math/bits` | `std.math.integer` | موجود |
| العمليات الملتفة والمتحققة والمشبعة لكل عرض | `wrapping_*`, `checked_*`, `saturating_*` لكل نوع عدد صحيح | `std.math.wrapping` | موجود |
| أنماط بتات الأعداد العائمة | `f64::to_bits/from_bits`; `math.Float64bits` | `std.math.float` (`بتات_ع64`, `ع64_من_بتات`) | موجود |
| الأعداد العائمة: sqrt، وexp، وln، وpow، والتقريب، والدوال الزائدية، والدوال المثلثية العكسية | دوال `f64`؛ `math` | `std.math.float`. تكون sqrt، وabs، وmin، وmax، وcopysign، وكذلك floor، وceil، وtrunc، وround_ties_even، وmul_add حيث يملكها الهدف، تعليمة واحدة (أدوات المترجم الأساسية للأعداد العائمة). | موجود |
| الأعداد العائمة: copysign، وround_ties_even، وmul_add | `f64::copysign`, `round_ties_even`, `mul_add`; `math.Copysign`, `math.RoundToEven`, `math.FMA` | `std.math.float` (`نسخ_إشارة64`، و`تقريب_زوجي64`، و`ضرب_جمع_مدمج64`، وع32) | موجود |
| sin، وcos، وtan | `f64::sin/cos/tan`; `math.Sin/Cos/Tan` | `std.math.trigonometry` (binary64 وbinary32) | موجود |
| العشوائية | (rand); `math/rand`, `crypto/rand` | `std.random.prng`, `std.os.entropy` | موجود |
| الأعداد الصحيحة الكبيرة، والأعداد المركبة | (num); `math/big`, `math/cmplx` | `std.crypto.bignum` (للتشفير فقط) | جزئي |

## الترميز وصيغ البيانات

| الأداة | Rust / Go | السلطان | الحالة |
|---|---|---|---|
| base64، وbase32، وhex، والصيغ الثنائية، وvarint، وPEM، وDER | (حزمة base64)؛ `encoding/*` | `std.encoding.*` | موجود |
| JSON، وTOML (قراءة وكتابة)، وCSV | (serde_json, toml); `encoding/json`, `encoding/csv` | `std.json`, `std.toml`, `std.csv` | موجود |
| URI، وMIME، وUUID، وsemver | (url, uuid); `net/url`, `mime` | `std.uri`، و`std.mime`، و`std.uuid` (v4 وv7)، و`std.version` | موجود |
| XML، وCBOR، وMessagePack | (quick-xml); `encoding/xml` | لا شيء | غير موجود |

## التشفير والتجزئة

| الأداة | Rust / Go | السلطان | الحالة |
|---|---|---|---|
| SHA-1، وSHA-256، وSHA-384، وSHA-512، وHMAC، وHKDF، وPBKDF2 | (sha2, hmac); `crypto/sha256`, `crypto/hmac` | `std.crypto.*` | موجود |
| AES-GCM، وChaCha20-Poly1305، وX25519، وEd25519، وECDSA P-256 وP-384، وRSA PKCS#1 وPSS | (ring); `crypto/*` | `std.crypto.*` | موجود |
| تجزئة كلمات المرور (bcrypt، وscrypt، وargon2) | حزم؛ `x/crypto` | لا شيء | غير موجود |
| دوال التجزئة غير التشفيرية | `Hasher`; `hash/fnv` | `std.hash.fnv`, `std.hash.xxh64`, `std.hash.combine` | موجود |

## الضغط والأرشيفات

| الأداة | Rust / Go | السلطان | الحالة |
|---|---|---|---|
| DEFLATE، وgzip، ومجاميع التحقق | (flate2); `compress/flate`, `gzip` | `std.compression.deflate`، و`gzip`؛ و`std.checksum` (CRC-32 مبني على جدول مع صيغة تحديث `حدث_سي_ار_سي32`، وAdler-32، وCRC-64/XZ `احسب_سي_ار_سي64`) | موجود |
| zstd (RFC 8878) | (zstd); `klauspost/compress/zstd` | `std.compression.zstd`: مفكك للإطارات، والإطارات القابلة للتخطي، والكتل الخام وRLE والمضغوطة، والحروف الحرفية بترميز Huffman مع الشجرة ودونها (تيار واحد أو 4 تيارات)، وتسلسلات FSE بأنماطها الأربعة كلها، والإزاحات المكررة، وحد النافذة، ومجموع التحقق XXH64، والقواميس الخام وبصيغة zstd، مع حدود للخرج والنافذة؛ وضاغط على نمط المستوى 1 (LZ77 بجدول تجزئة، وحروف حرفية بترميز Huffman مع أوزان مضغوطة بـ FSE، وجداول FSE معرّفة مسبقا أو محسوبة)؛ وXXH64 في `std.hash.xxh64` | موجود |
| xz، وLZMA، وLZMA2 | (xz2, lzma-rs); `ulikunitz/xz` | `std.compression.xz` (التيارات، والكتل، والفهرس، والحشو، والتسلسل؛ وفحوص CRC32 وCRC64 وSHA-256؛ وLZMA2 مع مرشحي x86 BCJ وDelta؛ ومرمّز يكتب كتلة LZMA2 واحدة مع CRC64) و`std.compression.lzma` (مفككا LZMA2 الخام و`.lzma`، ومرمّز LZMA بسلسلة تجزئة جشع وكسول) | موجود |
| الأرشيفات tar، وzip، وar | (tar, zip); `archive/tar`, `archive/zip` | `std.archive.*` | موجود |
| bzip2، وlz4 | حزم؛ `compress/bzip2` (قراءة) | لا شيء | غير موجود |
| الضواغط والمفككات التدفقية (التدريجية) | محوّلات `Read` و`Write`؛ `io.Reader` | لا شيء: تعمل أدوات الترميز على مخازن كاملة | غير موجود |

## HTTP وTLS

| الأداة | Rust / Go | السلطان | الحالة |
|---|---|---|---|
| عميل وخادم HTTP/1.1، والموجّه، وملفات تعريف الارتباط، وmultipart، وchunked | (hyper); `net/http` | `std.http.**` | موجود |
| عميل وخادم TLS 1.3، والتحقق من X.509، وPKCS#12، وجذور الثقة في النظام | (rustls); `crypto/tls`, `crypto/x509` | `std.tls.**` | موجود |
| ALPN (RFC 7301) | `ClientConfig::alpn_protocols`; `tls.Config.NextProtos` | `std.tls.config` (`اضبط_بروتوكولات_عميل_تي_إل_إس`، و`اضبط_بروتوكولات_خادم_تي_إل_إس`)، و`std.tls.stream` (`بروتوكول_تدفق_تي_إل_إس`) | موجود |
| HTTP/2 (RFC 9113، وHPACK RFC 7541) | (h2); `net/http` (HTTP/2), `x/net/http2` | `std.http.http2.*`: الإطارات؛ وHPACK مع Huffman والجداول الديناميكية؛ واتصال بحالات التيارات، والتحكم في التدفق على مستوى الاتصال والتيار، وSETTINGS، وPING، وGOAWAY، وRST_STREAM، وحدود (حجم الإطار، وقائمة الرؤوس، وكتلة الرؤوس، والتيارات المتزامنة، والجسم)؛ وعميل بتيارات متزامنة فوق h2c (بمعرفة مسبقة) أو TLS مع ALPN؛ وخادم يقدم `خدمة_ويب` نفسها التي يقدمها HTTP/1، ويُوزَّع بحسب مقدمة العميل (`std.http.server`) أو بحسب ALPN (`شغل_خادم_ويب_آمن`). وتستخدم الطلبات والاستجابات `std.http.model` المشتركة. | موجود |
| دفع الخادم في HTTP/2، وجدولة الأولويات، وترقية h2c من HTTP/1.1 | `x/net/http2` | لا شيء: الدفع معطل، والأولويات متجاهلة، والمعرفة المسبقة وحدها مدعومة | غير موجود |
| HTTP/3 | (h3, quinn) | لا شيء | غير موجود |

## التزامن والخيوط

| الأداة | Rust / Go | السلطان | الحالة |
|---|---|---|---|
| الأعداد الصحيحة الذرية (القراءة، والكتابة، والتبديل، وCAS، وfetch_add/and/or/xor، والترتيبات، والسياج) | `std::sync::atomic`; `sync/atomic` | `std.sync.atomic`، مخفوضة إلى تعليمات ذرية على x86_64 وarm64 وwasm32 وLLVM، ويشغّلها المفسر ذو الخيط الواحد تشغيلا عاديا؛ ومنشئات ترتيب الذاكرة (`ترتيب_ذاكرة.تسلسلي`، …) | موجود (متعدد الخيوط على Linux: `sync_atomic_register_pressure`) |
| خيوط نظام التشغيل: الإطلاق، والانضمام، والانضمام مع قيمة، والنوم، والتنازل، والمعرّف الحالي، والتوازي المتاح | `std::thread` (`spawn`, `JoinHandle<T>::join`, `sleep`, `yield_now`, `current().id()`, `available_parallelism`); goroutines | `std.thread` (`أطلق`، و`انضم`، و`أطلق_بقيمة`، و`انضم_بقيمة`، و`نم`، و`تنازل`، و`معرف_الخيط_الحالي`، و`معرف_المقبض`، و`عدد_المعالجات_المتاحة`) فوق `os.threads`. يتخلص الخيط المنتهي من دالته المغلقة ومما التقطته؛ ويحرر الانضمام الباقي. `clone` وfutex المباشران على Linux وAndroid (كل المعماريات)؛ و`thr_new` و`_umtx_op` على FreeBSD؛ و`pthread_create` و`pthread_join` على macOS؛ و`CreateThread` و`WaitForSingleObject` على Windows. | موجود. WASI والمفسر: غير مدعوم، مع خطأ واضح. |
| الانتظار على عنوان | `sys::futex` (داخلي)؛ `runtime.futex`, `sema` | `std.os.wait_address`: futex (Linux، وAndroid)، و`_umtx_op` (FreeBSD)، و`__ulock_wait2` و`__ulock_wake` (macOS)، و`WaitOnAddress` (Windows)، وبديل بخيط واحد (WASI) | موجود |
| الملكية المشتركة | `Arc` (`strong_count`, `ptr_eq`) | `std.sync.arc` (`مشترك`): العدّاد الذري والإتلاف الوحيد لبيئة دالة مغلقة تملك القيمة المغلفة | موجود |
| الاستبعاد المتبادل | `Mutex`, `RwLock`; `sync.Mutex`, `sync.RWMutex` | `std.sync.mutex` (`قفل`: قفل futex بثلاث حالات، و`نفذ_مع_القفل`، و`حاول_القفل`، ولا poisoning لأن الانهيار ينهي العملية) و`std.sync.rwlock` (`قفل_قراءة_كتابة`: `نفذ_مع_قراءة`، و`نفذ_مع_كتابة`، ولا أولوية للكاتب) | موجود |
| متغيرات الشرط | `Condvar`; `sync.Cond` | `std.sync.condvar` (`شرط_انتظار`: `انتظر_الشرط`، و`انتظر_الشرط_بمهلة`، و`انتظر_طالما`، و`نبه_واحدا`، و`نبه_الكل`) | موجود |
| التهيئة لمرة واحدة | `Once`, `OnceLock`; `sync.Once` | `std.sync.once` (`مرة`، و`خلية_مرة` مع `اقرأ_خلية_مرة`، و`اقرأ_أو_هيئ_خلية_مرة`، و`اضبط_خلية_مرة`) | موجود |
| القنوات | `mpsc::channel`, `sync_channel`; Go channels | `std.sync.channel`: غير محدودة ومحدودة، و`أرسل`، و`حاول_الإرسال`، و`استقبل`، و`حاول_الاستقبال`، و`استقبل_بمهلة`، وأخطاء الانقطاع في الاتجاهين؛ والحد 0 يحمل قيمة واحدة (لا rendezvous) | موجود |
| الخلو من سباق البيانات | `Send`, `Sync` | فحص الالتقاط عند الإطلاق ([نظام الأنواع](type-system.md#قواعد-الالتقاط-في-الخيوط)): لا مؤشر خام خارج `غير_آمن`، ولا مرجع مستعار، ولا متغير وحدة قابل للكتابة في دالة مغلقة مرسلة إلى خيط آخر | جزئي: لا تُفحص الدوال المغلقة الملتقطة ولا الاستدعاءات |
| الخيوط ذات النطاق، والمتغيرات المحلية للخيط، والحواجز | `thread::scope`, `thread_local!`, `Barrier`; `sync.WaitGroup` | لا شيء | غير موجود |

## الأخطاء

| الأداة | Rust / Go | السلطان | الحالة |
|---|---|---|---|
| قيمة الخطأ المشتركة، وسلاسل الأسباب، والسياق، والانتشار | `Error`, `?`, (anyhow); `errors`, `%w` | `std.error` (`فشل`, `؟`, `مع_سياق`, `إنهيار`)، مع محوّلات لأخطاء المكتبة القياسية ذات الأنواع، ومنها `خطأ_أمر` و`خطأ_مدة` و`خطأ_تنسيق` | موجود |

## الوحدات المساندة

| الوحدة | ما توفره | Rust / Go |
|---|---|---|
| `std.bytes.builder`, `cursor`, `range`, `compare` | مخزن بايتات قابل للنمو، ومؤشر قراءة يقرأ أعدادا صحيحة بعرض ثابت، ونطاقات ونسخ متحققة، والمساواة، والترتيب، واختبارات البادئة واللاحقة، والبحث | `bytes::BytesMut`, `Cursor`; `bytes.Buffer`, `bytes.Compare` |
| `std.chars.ascii` | تصنيف ASCII للأرقام، والأرقام الست عشرية، والحروف، والفواصل | `u8::is_ascii_*` |
| `std.debug.assert` | `تأكد` و`تأكد_برسالة`: تأكيدات تبقى فعالة في كل أنماط البناء وتوقف البرنامج مثل `إنهيار` | `assert!` |
| `std.fuzz.core` | مزوّد حتمي يحوّل بايتات الدخل إلى اختيارات ذات أنواع قابلة لإعادة الإنتاج لأهداف fuzzing (أما التغطية وجدولة المدوّنة فمن شأن الأدوات) | arbitrary |
| `std.meta.reflect` | حجم النوع، ومحاذاته، وعدد حقوله، وعدد منشئاته، وإخفاق وقت الترجمة | `mem::size_of`, `align_of`; `reflect` |
| `std.memory.system` | حجم الصفحة وتعيينات الذاكرة في نظام التشغيل مع الحماية والإرشادات | memmap2; `syscall.Mmap` |
| `std.os.*` | طبقة المنصة: حجز الذاكرة، والملفات، والمقابس، والساعات، والعشوائية، والخيوط، والانتظار على العنوان، والذاكرة الافتراضية | `sys`, libc; `syscall`, `runtime` |

## خصائص اللغة التي تعتمد عليها المكتبة القياسية

تُبنى عدة وحدات في المكتبة القياسية على خصائص في اللغة لها قواعدها الخاصة.

- التحويل العددي: تحوّل `مثل` بين الأعداد العائمة والأعداد الصحيحة بدلالات Rust
  ([نظام الأنواع](type-system.md)، "التحويلات العددية"). وأنماط البتات هي `بتات_ع64` و`ع64_من_بتات` في
  `std.math.float`، وعليها يُبنى نص الأعداد العائمة والثواني العائمة.
- الحساب الملتف: دوال مدمجة لا تُطلق اعتراضا على كل خلفية، و`std.math.wrapping`. وتستخدمها دوال
  الكلمات ذات 64 بتا المساعدة في SHA-512 (`std.crypto.words64`).
- الحالة على مستوى العملية: المتغير العام القابل للكتابة الذي ليس عدديا قابل للاستخدام في نطاق غير
  آمن، مثل `static mut` في Rust. وتستخدمه `std.env.table`.
- الإزالة من المتجه وسعته: الأدوات الأساسية `أزل_الأخير`، و`اقتطع`، و`سعة`
  و`احجز_سعة` ([الملكية](ownership.md)، "الإزالة من الآخر والاقتطاع" و
  "السعة")، وعليها تبني `std.collections.vector` دوال الاحتفاظ، وإزالة
  المتكرر، والإزالة عند موضع، والإزالة بالتبديل، والإدراج، والمسح، والإنشاء بسعة، والحجز، والتقليص. والمتجه الذي أُفرغ يحتفظ بمخزنه.

## الحدود

- لا تلتقط الدوال المغلقة إلا قيما بسيطة، فلا يستطيع منسق `std.fmt` مخصص أن يستعير قيمة تملك نصوصا أو
  متجهات. وتُنسَّق مثل هذه القيمة أولا في منسق ثم تُمرَّر نصا.
- الوحدات التي يستوردها المترجم نفسه (`std.math.integer`، و`std.time.duration`، و`std.env.system`،
  و`std.crypto.words`، …) يبنيها مترجم الإقلاع الذاتي (bootstrap) المكتوب بلغة C، أي Stage0. ولهذا السبب جُعلت
  الوحدات المرافقة لها (`std.time.duration_float`، و`std.crypto.words64`، و`std.env.table`،
  و`std.collections.vector`) وحدات منفصلة.
- الثغرات المذكورة في جداول الحالة أعلاه (محوّلات التيارات العامة، والمقابس المتعددة، وXML، وbzip2،
  وlz4، والترميز التدفقي، وHTTP/3، والخيوط ذات النطاق، وما بقي) غير منفذة.

## انظر أيضا

- [التزامن](concurrency.md): تصميم `std.sync` و`std.thread`.
- [التنسيق](formatting.md): `std.fmt`.
- [الأخطاء](errors.md): `std.error`.
- [الأهداف](targets.md) و[اعتماديات المنصات](platform-dependencies.md): الدعم لكل منصة.
