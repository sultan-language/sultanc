<div dir="rtl">

**العربية** | [English](../../en/reference/std-coverage.md)

# تغطية المكتبة القياسية

تسرد هذه الصفحة ما توفّره المكتبة القياسية في السلطان (`library/std/**`) وحدةً وحدة، وتقارن كل مجال
بالمكتبتين القياسيتين في Rust وGo. وهي مرجع للمبرمجين الذين يريدون معرفة ما إذا كانت المكتبة القياسية تلبي
حاجتهم، وللمساهمين الذين يبحثون عن الثغرات.

لحالة كل صف إحدى ثلاث قيم. **موجود** يعني أن للاستخدامات الشائعة للأداة المقابلة في Rust وGo مقابلًا في
المكتبة القياسية. **جزئي** يعني أن بعضها مغطًّى، وتُذكر الثغرات بأسمائها. **غير موجود** يعني أن المكتبة القياسية
لا تقدّم شيئًا. تُذكر الأسماء بتهجئتها العربية، ويُسرد مقابل كل منها بالإنجليزية في
[المصطلحات العربية](arabic-terminology.md).

تذكر ملاحظات المنصات ما ينقص كل منصة. والمنصات المدعومة هي Linux وAndroid (نداءات نواة خام؛ معماريات
64 بت، وi686، وarmv7)، وFreeBSD (مزوّد النواة المباشر نفسه)، وmacOS (libSystem)،
وWindows (kernel32، وws2_32)، وWASI preview 1. انظر [الأهداف](targets.md) و
[اعتماديات المنصات](platform-dependencies.md).

## فهرس الوحدات

| مسار الوحدة | ما توفّره | الأقرب في Rust / Go |
|---|---|---|
| `std.process.*` | إطلاق العمليات، وانتظارها، وحالة الإنهاء، وباني الأوامر، والعمليات الأبناء ذات الأنابيب المتدفقة، والوسائط، و`اخرج`، و`إنهيار`، وتحليل سطر الأوامر | `std::process`, `env::args`; `os/exec`, `flag` |
| `std.env.*` | متغيرات البيئة، ومجلد العمل، والمجلد الرئيسي والمجلد المؤقت | `std::env`; `os` |
| `std.fs.*` | الملفات، والمجلدات، والبيانات الوصفية، والأذونات، والوصلات، والملفات المؤقتة، والأقفال، والمسارات | `std::fs`, `std::path`; `os`, `path/filepath` |
| `std.io.*` | دوال الطباعة والقراءة، وأدوات القراءة والكتابة (readers وwriters)، والتخزين المؤقت، والتسجيل | `std::io`, `log`; `io`, `bufio`, `log` |
| `std.fmt.fmt` | نصوص التنسيق واللغة المصغّرة لمواصفات التنسيق | `std::fmt`; `fmt` |
| `std.net.*` | TCP، وUDP، ومقابس Unix، وعناوين IP وعناوين المقابس، وDNS | `std::net`; `net` |
| `std.time.*` | الساعات، والمدد، والتواريخ التقويمية، والمناطق الزمنية | `std::time`, chrono; `time` |
| `std.text.*` | البحث، والتحويل، والبناة، وتمثيل الأعداد الصحيحة والعائمة نصًّا، ووسوم اللغات، والاتجاه الثنائي (bidi) | `str`, `String`; `strings`, `strconv` |
| `std.unicode.*` | الفئات، والخصائص، وتحويل حالة الأحرف، والتطبيع، والتقطيع | `char`، وحزم unicode؛ `unicode`, `x/text` |
| `std.collections.*` | الخرائط، والمجموعات، وأشجار B، والطابور ثنائي الطرف، والكومة، ومجموعة البتات، والفرز، وأدوات المتجهات | `std::collections`, `Vec`; `container/*`, `slices`, `sort` |
| `std.math.*` | رياضيات الأعداد الصحيحة والعائمة، والحساب الملتف، وحساب المثلثات | دوال الأعداد في `core`؛ `math`, `math/bits` |
| `std.random.prng`, `std.os.entropy` | مولّد ذو بذرة، وعشوائية نظام التشغيل | rand; `math/rand`, `crypto/rand` |
| `std.encoding.*`, `std.json`, `std.toml`, `std.csv`, `std.uri`, `std.mime`, `std.uuid`, `std.version` | صيغ البيانات والمعرّفات | serde_json, toml, csv, url, uuid, semver; `encoding/*`, `net/url`, `mime` |
| `std.crypto.*`, `std.hash.*`, `std.checksum` | دوال التجزئة، وMACs، وKDFs، وAEADs، والتوقيعات، والأعداد الكبيرة، ودوال التجزئة غير التشفيرية، وCRCs | RustCrypto, ring; `crypto/*`, `hash/*` |
| `std.compression.*`, `std.archive.*` | deflate، وgzip، وzstd، وxz، وLZMA؛ وtar، وzip، وar | flate2, zstd, xz2, tar, zip; `compress/*`, `archive/*` |
| `std.http.*`, `std.tls.*` | عميل وخادم HTTP/1.1 وHTTP/2، وTLS 1.3، وX.509 | hyper, rustls; `net/http`, `crypto/tls` |
| `std.sync.*`, `std.thread`, `std.os.wait_address` | الخيوط، والعمليات الذرية، والأقفال، والقنوات | `std::sync`, `std::thread`; `sync`, goroutines |
| `std.error.*` | قيمة الخطأ المشتركة والمحوّلات | `Error`, anyhow; `errors` |
| `std.bytes.*`, `std.chars.ascii` | مخازن البايتات، ومؤشرات القراءة (cursors)، والمقارنة؛ وتصنيف محارف ASCII | `bytes`; `bytes` |
| `std.debug.assert`, `std.fuzz.core`, `std.meta.reflect`, `std.memory.system` | التأكيدات، ومدخلات اختبار fuzzing، والانعكاس (reflection) على الأنواع، وتعيينات الذاكرة في نظام التشغيل | `assert!`, arbitrary, `mem::size_of`, memmap; `reflect`, `syscall.Mmap` |
| `std.os.*` | طبقة المنصة التي تقوم عليها الوحدات المذكورة أعلاه | `sys`, libc; `syscall`, `runtime` |

## الملخص

| المجال | الحالة | أهم الثغرات |
|---|---|---|
| العمليات | موجود | مجموعات العمليات، والإشارات عدا kill، والخطافات على نمط `pre_exec` |
| البيئة | موجود | |
| الملفات | موجود | تغيير المالك (chown)، ومراقبة المجلدات، وأقفال نطاقات البايتات |
| المسارات | موجود | |
| الدخل والخرج | جزئي | محوّلات التدفقات العامة (take، وchain، وtee)، وقفل stdin، والدخل والخرج المتجهي (vectored I/O) |
| الشبكات | جزئي | تعدد الإرسال غير الحاجب (epoll، وkqueue)، والبث المتعدد (multicast)، وتمرير الواصفات عبر مقابس Unix |
| الوقت | موجود | |
| النصوص والسلاسل النصية | موجود | وسائط التنسيق المسمّاة |
| Unicode | موجود | |
| المجموعات | موجود | القائمة المترابطة |
| الرياضيات | موجود | الأعداد الصحيحة الكبيرة خارج التشفير، والأعداد المركّبة |
| الترميز | موجود | XML، وgob، وMessagePack، وCBOR |
| التشفير | موجود | مجموعة موجّهة إلى TLS: لا bcrypt ولا scrypt ولا argon2 |
| الضغط | جزئي | bzip2، وlz4؛ وأدوات الترميز التدفقية (التدريجية) |
| HTTP وTLS | موجود | HTTP/3؛ ودفع الخادم (server push) والأولويات في HTTP/2 |
| التزامن والخيوط | موجود | الخيوط ذات النطاق، والمتغيرات المحلية للخيط، والحواجز؛ وترتيبات الذاكرة الذرية الأضعف؛ انظر [التزامن](concurrency.md) |
| الأخطاء | موجود | |

## العمليات

| الأداة | Rust / Go | السلطان | الحالة |
|---|---|---|---|
| الإطلاق، والانتظار، وحالة الإنهاء | `Command::spawn`, `Child::wait`; `exec.Cmd.Start/Wait` | `std.process.child` (`شغل_عملية`, `انتظر_عملية`, `حالة_إنهاء`) | موجود |
| try_wait، وkill، والانتظار بمهلة | `try_wait`, `kill` | `std.process.child` (`تفقد_عملية`, `اقتل_عملية`, `انتظر_عملية_بمهلة`) على Unix؛ والعمليات الأبناء في `std.process.command` (`تفقد_الطفل`, `اقتل_الطفل`) على كل نظام تشغيل يدعم العمليات، ومنها Windows | موجود |
| الباني: الوسائط، وضبط متغيرات البيئة وإزالتها ومسحها، والمجلد الحالي | `Command::arg/env/env_remove/env_clear/current_dir` | `std.process.command` | موجود (WASI: لا عمليات) |
| الدخل والخرج القياسيان للعملية الابن (وراثة أو فارغ أو أنبوب)، والتقاط الخرج، وتمرير بايتات إلى الدخل | `Command::output`, `Stdio`; `Cmd.Output`, `CombinedOutput` | `std.process.command` (`اجمع_خرج_الأمر`, `شغل_الأمر`, `مصدر_تيار`) | موجود |
| أنابيب متدفقة إلى عملية ابن قيد التشغيل | `Command::spawn` مع `Stdio::piped`, `Child { stdin, stdout, stderr }`, `wait`, `try_wait`, `kill`, `wait_with_output`; `Cmd.StdinPipe/StdoutPipe/Start` | `std.process.command` (`ابدأ_الأمر`، و`طفل_أمر`، و`دخل_طفل` و`خرج_طفل` على شكل القارئ والكاتب في `std.io`، و`اكتب_إلى_الطفل`، و`اقرأ_خرج_الطفل`، و`أغلق_دخل_الطفل`، و`أغلق_تيارات_الطفل`، و`انتظر_الطفل`، و`تفقد_الطفل`، و`اقتل_الطفل`، و`انتظر_خرج_الطفل`) | موجود. يكون كل تدفق موروثًا أو فارغًا أو موصولًا بأنبوب. ولأن اللغة بلا دوال إتلاف (destructors)، يجب إغلاق الدخل والأنابيب المتبقية صراحةً، وتُغلق `انتظر_الطفل` الدخل كما يفعل Rust. والكتابة إلى عملية ابن أغلقت دخلها تعيد الخطأ EPIPE لا الإشارة SIGPIPE. WASI: غير مدعوم. |
| الوسائط، والخروج، والانهيار | `env::args`, `process::exit`, `panic!` | `std.process.arguments`، و`اخرج`، و`إنهيار` | موجود |
| تحليل سطر الأوامر | (حزمة clap)؛ `flag` | `std.process.cli` | موجود |

## البيئة

| الأداة | Rust / Go | السلطان | الحالة |
|---|---|---|---|
| قراءة متغير واحد | `env::var`; `os.Getenv/LookupEnv` | `std.env.environment` (`متغير_بيئة`) | موجود |
| كل المتغيرات | `env::vars`; `os.Environ` | `std.env.variables` (`متغيرات_البيئة`) | موجود |
| ضبط متغيرات العملية نفسها وإزالتها | `env::set_var/remove_var`; `os.Setenv/Unsetenv` | `std.env.variables` (`اضبط_متغير_بيئة`, `أزل_متغير_بيئة`) فوق `std.env.table`، ويرثها `std.process.command` | موجود. يُستعمل قفل واحد على مستوى العملية بمجرد وجود خيط، مثل `ENV_LOCK` في Rust. |
| قراءة المجلد الحالي وتغييره | `env::current_dir/set_current_dir`; `os.Getwd/Chdir` | `دليل_العمل`, `غير_دليل_العمل` | موجود (WASI: لا chdir) |
| المجلد الرئيسي والمجلد المؤقت | `env::home_dir/temp_dir`; `os.UserHomeDir/TempDir` | `الدليل_الرئيسي`, `الدليل_المؤقت` | موجود |

## الملفات والمسارات

| الأداة | Rust / Go | السلطان | الحالة |
|---|---|---|---|
| قراءة الملف كاملًا وكتابته، والنسخ | `fs::read/write/copy`; `os.ReadFile/WriteFile` | `std.fs.read_file`, `write_file`, `write_bytes` | موجود |
| المقابض: الفتح، والقراءة، والكتابة، والانتقال إلى موضع، والمزامنة، والإغلاق | `File`; `os.File` | `std.fs.handle` | موجود |
| الاقتطاع، وضبط الطول | `File::set_len`; `os.Truncate` | `std.fs.attributes` (`اقتطع_الملف`, `اقتطع_مقبض_ملف`) | موجود |
| البيانات الوصفية: النوع، والحجم، ووقت التعديل، والوجود | `fs::metadata`; `os.Stat` | `std.fs.metadata`, `directory` | موجود |
| ضبط أوقات الملف | `File::set_times`; `os.Chtimes` | `اضبط_أوقات_الملف` | موجود |
| قراءة الأذونات وتغييرها (chmod) | `Permissions`, `set_permissions`; `os.Chmod` | `std.fs.permissions` | موجود (Windows: بت «للقراءة فقط» وحده؛ WASI: لا شيء) |
| إنشاء الوصلات الرمزية وقراءتها، والوصلة الصلبة | `os::unix::fs::symlink`, `read_link`, `hard_link`; `os.Symlink/Readlink/Link` | `std.fs.links` | موجود |
| المسار المعياري (canonical)، والمسار المطلق | `fs::canonicalize`, `path::absolute`; `filepath.EvalSymlinks/Abs` | `std.fs.canonical` | موجود |
| create_dir_all، وremove_dir_all، ونسخ شجرة مجلدات، والمرور على الشجرة | `create_dir_all`, `remove_dir_all`; `os.MkdirAll/RemoveAll`, `filepath.WalkDir` | `std.fs.walk` | موجود |
| الملفات والمجلدات المؤقتة | (حزمة tempfile)؛ `os.CreateTemp/MkdirTemp` | `std.fs.temporary` | موجود |
| إعادة التسمية، والحذف، وإنشاء مجلد، وسرد محتويات مجلد | `fs::rename/remove_file/read_dir` | `std.fs.mutation`, `directory` | موجود |
| أقفال الملف بأكمله | `File::lock/lock_shared/try_lock/try_lock_shared/unlock` (Rust 1.89); `syscall.Flock` | `std.fs.lock` (`اقفل_الملف`، و`اقفل_الملف_مشتركا`، و`حاول_قفل_الملف`، و`حاول_قفل_الملف_مشتركا`، و`افتح_قفل_الملف`؛ و`خطأ_محاولة_قفل.ستحظر` يقابل `TryLockError::WouldBlock` في Rust) | موجود (flock على Linux وAndroid وFreeBSD وmacOS؛ و`LockFileEx` على Windows؛ WASI: غير مدعوم) |
| تغيير المالك (chown)، والمراقبة | `os::unix::fs::chown`; `os.Chown`, (fsnotify) | لا شيء | غير موجود |
| الضم، والمجلد الأب، واسم الملف، والامتداد، والمكوّنات، والتطبيع | `Path`; `path/filepath` | `std.fs.path` (صيغتا POSIX وWindows) | موجود |

## الدخل والخرج

| الأداة | Rust / Go | السلطان | الحالة |
|---|---|---|---|
| print، وeprint، وقراءة أسطر stdin | `println!`, `stdin().lines()`; `fmt.Println`, `bufio.Scanner` | `std.io.print`, `println`, `read_line` | موجود |
| القارئ والكاتب المزوّدان بتخزين مؤقت | `BufReader/BufWriter`; `bufio` | `std.io.read_line` (`قارئ_مخزن`), `buffered_writer` | موجود |
| القارئ والكاتب، والنسخ | `Read/Write`, `io::copy`; `io.Reader/Writer`, `io.Copy` | `std.io.reader`, `writer` | موجود |
| المحوّلات (take، وchain، والكاتب المتعدد)، والدخل والخرج المتجهي | `Read::take/chain`; `io.MultiWriter`, `TeeReader` | لا شيء | غير موجود |
| التسجيل | (حزمة log)؛ `log`, `slog` | `std.io.log` | موجود |

## التنسيق

| الأداة | Rust / Go | السلطان | الحالة |
|---|---|---|---|
| نصوص التنسيق، و`Display` و`Debug`، واللغة المصغّرة لمواصفات التنسيق | `format!`, `write!`, `print!`; `fmt.Sprintf`, `Fprintf`, `Printf` | `std.fmt.fmt` (`نسق`، و`نسق_في`، و`اكتب_منسقا`، و`اطبع_سطرا_منسقا` وصيغ الطباعة الأخرى، و`قيمة_تنسيق` مع حالة لدالة مغلقة (closure) مخصّصة، و`منسق`)؛ و`سجل_منسقا` في `std.io.log` | جزئي. يُفحص نص التنسيق الحرفي في وقت الترجمة مقابل قائمة وسائط مكتوبة متجهًا حرفيًّا (العدد، والمواضع، والأنواع، والأعداد)؛ أما نص التنسيق المبني في وقت التشغيل فيُفحص في وقت التشغيل. لا وسائط مسمّاة. ولا تلتقط الدوال المغلقة المخصّصة إلا قيمًا بسيطة. انظر [التنسيق](formatting.md). |

## الشبكات

| الأداة | Rust / Go | السلطان | الحالة |
|---|---|---|---|
| الاتصال في TCP، والاستماع، والقبول، والقراءة، والكتابة، والإيقاف (shutdown)، والخيارات | `TcpStream/TcpListener`; `net.Dial/Listen` | `std.net.tcp`, `options` | موجود (WASI: المقابس المفتوحة مسبقًا فقط) |
| الربط في UDP، والاتصال، والإرسال والاستقبال | `UdpSocket` | `std.net.udp` | موجود |
| send_to وrecv_from في UDP دون اتصال | `UdpSocket::send_to/recv_from`; `WriteTo/ReadFrom` | `std.net.socket_names` | موجود |
| العنوان المحلي وعنوان النظير | `local_addr/peer_addr`; `LocalAddr/RemoteAddr` | `std.net.socket_names` | موجود |
| عناوين IP وعناوين المقابس، وDNS | `IpAddr`, `ToSocketAddrs`; `net.ParseIP`, `LookupHost` | `std.net.ip`, `socket_address`, `dns` | موجود |
| المهل | `set_read_timeout` | `خيار_مقبس.مهلة_استقبال` و`مهلة_إرسال` | موجود |
| مقابس Unix: مستمع للتدفقات وتدفقات، ومقابس الرسائل (datagram)، وعناوين بمسار وعناوين مجرّدة | `std::os::unix::net::{UnixListener, UnixStream, UnixDatagram, SocketAddr}`; `net.UnixListener/UnixConn` | `std.net.unix` (`مستمع_يونكس`, `تدفق_يونكس`, `مقبس_رسائل_يونكس`, `عنوان_يونكس`) فوق `std.os.unix_socket` | موجود على Linux وAndroid وFreeBSD وmacOS؛ والأسماء المجرّدة على Linux وAndroid فقط؛ وعلى Windows 10 1803 فما بعد: التدفقات فقط؛ WASI: غير مدعوم. لا socketpair، ولا تمرير للواصفات، ولا بيانات اعتماد النظير. |
| استطلاع مقابس كثيرة، والبث المتعدد | (mio); `JoinGroup` | لا شيء | غير موجود |

## الوقت

| الأداة | Rust / Go | السلطان | الحالة |
|---|---|---|---|
| الساعة الرتيبة وساعة النظام، والنوم | `Instant`, `SystemTime`, `sleep`; `time.Now`, `Sleep` | `std.time.time` | موجود |
| المدد: الحساب، والمقارنة | `Duration`; `time.Duration` | `std.time.duration`، مع قيمة مطلقة وقسمة متحقَّق منهما | موجود |
| تمثيل المدة نصًّا | `Duration` (Debug)؛ `Duration.String`, `ParseDuration` | `std.time.duration_text` | موجود |
| التقويم، وRFC 3339 وISO 8601، وتواريخ HTTP | (chrono); `time.Parse/Format` | `std.time.calendar`, `date` | موجود |
| المناطق الزمنية (TZif) | (chrono-tz); `time.LoadLocation` | `std.time.timezone` | موجود |
| الثواني بأعداد عائمة | `Duration::as_secs_f64`, `try_from_secs_f64`; `Seconds()` | `std.time.duration_float` (`ثواني_المدة_ع64`، و`مدة_من_ثواني_ع64`، وصيغ ع32) | موجود |

## النصوص وUnicode

| الأداة | Rust / Go | السلطان | الحالة |
|---|---|---|---|
| find، وrfind، وcontains، والتحقق من البداية والنهاية | `str::find`; `strings.Index/Contains/HasPrefix` | `std.text.search` | موجود |
| trim، وsplit، وjoin، وreplace، وlines، وrepeat | `trim/split/join/replace`; `strings.TrimSpace/Split/Join` | `std.text.transform` | موجود |
| split_whitespace، وsplitn، وreplacen، وstrip_prefix وstrip_suffix، وحالة أحرف ASCII | `split_whitespace/splitn/replacen/strip_prefix`; `strings.Fields/SplitN/CutPrefix` | `std.text.transform` | موجود |
| count، وعدد المحارف، وeq_ignore_ascii_case | `matches().count`, `chars().count`; `strings.Count`, `EqualFold` | `std.text.search` | موجود |
| الباني، والأعداد، والأساس العددي، والمقارنة | `String`, `parse`; `strings.Builder`, `strconv` | `std.text.builder`, `number`, `radix`, `compare` | موجود |
| تنسيق الأعداد العائمة وتحليلها | `f64::to_string`, `parse`; `strconv.FormatFloat/ParseFloat` | `std.text.float`: `ع64_إلى_نص` (أقصر نص يعيد القيمة نفسها عند قراءته، مع مسار سريع بخوارزمية Grisu3)، و`حلل_ع64` (دقيق)، و`أرقام_ع64_الأقصر`، و`أرقام_ع64_الدقيقة`، و`أرقام_ع64_بخانات_كسرية`، و`أرقام_ع64_بخانات_معنوية` (أرقام تستعملها المنسِّقات)؛ وصيغ ع32 كذلك | موجود |
| الإعدادات المحلية (locales)، وعزل الاتجاه | (icu) | `std.text.language`, `bidi` | موجود |
| الفئات، والخصائص، وتحويل حالة الأحرف وطيّها، والتطبيع، وتقطيع النص إلى عناقيد محارف (graphemes) وكلمات وجمل | دوال `char` وحزم unicode؛ `unicode`, `x/text` | `std.unicode.**` (جداول Unicode مولَّدة) | موجود |
| UTF-8 وUTF-16 | `str`, `encode_utf16`; `unicode/utf8`, `utf16` | `std.encoding.utf8`, `utf16` | موجود |

## المجموعات

| الأداة | Rust / Go | السلطان | الحالة |
|---|---|---|---|
| خريطة التجزئة ومجموعتها، وخريطة B-tree ومجموعتها | `HashMap/HashSet/BTreeMap/BTreeSet`; `map` | `std.collections.map`, `set`, `btree_map`, `btree_set`, `text_index_map` | موجود |
| الطابور ثنائي الطرف، والكومة الثنائية، ومجموعة البتات | `VecDeque/BinaryHeap`; `container/heap` | `deque`, `heap`, `bitset` | موجود |
| الفرز، والبحث الثنائي | `sort_by`, `binary_search_by`; `slices.Sort` | `std.collections.sort` | موجود |
| عكس المتجه، والبحث فيه، وموضع أصغر عنصر وأكبر عنصر، والتدوير | `reverse/position/iter().max_by/rotate_left`; `slices.Reverse/IndexFunc` | `std.collections.vector` | موجود |
| الإزالة من المتجه: pop، وtruncate، وretain، وdedup، وremove، وswap_remove، وinsert، وclear | `Vec::pop/truncate/retain/dedup_by/remove/swap_remove/insert/clear`; `slices.DeleteFunc/CompactFunc/Delete/Insert` | العمليتان الأساسيتان في اللغة `أزل_الأخير` و`اقتطع`؛ و`std.collections.vector` (`احتفظ_في_المتجه`، و`أزل_المتكرر_المتتالي`، و`أزل_عند`، و`أزل_بالتبديل`، و`أدرج_عند`، و`امسح_المتجه`) | موجود |
| سعة المتجه: capacity، وreserve، وreserve_exact، وwith_capacity، وshrink_to_fit | `Vec::capacity/reserve/reserve_exact/with_capacity/shrink_to_fit`; `cap`, `make([]T, 0, n)`, `slices.Grow/Clip` | العمليتان الأساسيتان في اللغة `سعة` و`احجز_سعة`؛ و`std.collections.vector` (`متجه_بسعة`، و`احجز_في_المتجه`، و`قلص_المتجه`) | موجود. يبقى المخزن المؤقت محجوزًا بعد الاقتطاع، والإزالة حتى يفرغ المتجه، والمسح، كما في `Vec` في Rust. |
| القائمة المترابطة | `LinkedList`; `container/list` | لا شيء | غير موجود |

## الرياضيات والعشوائية

| الأداة | Rust / Go | السلطان | الحالة |
|---|---|---|---|
| الأعداد الصحيحة: العمليات المتحقَّق منها والمشبعة، وgcd، والقوى، والبتات | `checked_*`, `leading_zeros`; `math/bits` | `std.math.integer` | موجود |
| العمليات الملتفة والمتحقَّق منها والمشبعة لكل عرض من البتات | `wrapping_*`, `checked_*`, `saturating_*` لكل نوع عدد صحيح | `std.math.wrapping` | موجود |
| أنماط بتات الأعداد العائمة | `f64::to_bits/from_bits`; `math.Float64bits` | `std.math.float` (`بتات_ع64`, `ع64_من_بتات`) | موجود |
| الأعداد العائمة: sqrt، وexp، وln، وpow، والتقريب، والدوال الزائدية، والدوال المثلثية العكسية | دوال `f64`؛ `math` | `std.math.float`. تُنفَّذ sqrt، وabs، وmin، وmax، وcopysign بتعليمة واحدة، وكذلك floor، وceil، وtrunc، وround_ties_even، وmul_add حيث يوفّرها الهدف (وهي عمليات أساسية للأعداد العائمة في المترجم). | موجود |
| الأعداد العائمة: copysign، وround_ties_even، وmul_add | `f64::copysign`, `round_ties_even`, `mul_add`; `math.Copysign`, `math.RoundToEven`, `math.FMA` | `std.math.float` (`نسخ_إشارة64`، و`تقريب_زوجي64`، و`ضرب_جمع_مدمج64`، وصيغ ع32) | موجود |
| sin، وcos، وtan | `f64::sin/cos/tan`; `math.Sin/Cos/Tan` | `std.math.trigonometry` (binary64 وbinary32) | موجود |
| العشوائية | (rand); `math/rand`, `crypto/rand` | `std.random.prng`, `std.os.entropy` | موجود |
| الأعداد الصحيحة الكبيرة، والأعداد المركّبة | (num); `math/big`, `math/cmplx` | `std.crypto.bignum` (للتشفير فقط) | جزئي |

## الترميز وصيغ البيانات

| الأداة | Rust / Go | السلطان | الحالة |
|---|---|---|---|
| base64، وbase32، وhex، والصيغ الثنائية، وvarint، وPEM، وDER | (حزمة base64)؛ `encoding/*` | `std.encoding.*` | موجود |
| JSON، وTOML (قراءة وكتابة)، وCSV | (serde_json, toml); `encoding/json`, `encoding/csv` | `std.json`, `std.toml`, `std.csv` | موجود |
| URI، وMIME، وUUID، وsemver | (url, uuid); `net/url`, `mime` | `std.uri`، و`std.mime`، و`std.uuid` (الإصداران v4 وv7)، و`std.version` | موجود |
| XML، وCBOR، وMessagePack | (quick-xml); `encoding/xml` | لا شيء | غير موجود |

## التشفير والتجزئة

| الأداة | Rust / Go | السلطان | الحالة |
|---|---|---|---|
| SHA-1، وSHA-256، وSHA-384، وSHA-512، وHMAC، وHKDF، وPBKDF2 | (sha2, hmac); `crypto/sha256`, `crypto/hmac` | `std.crypto.*` | موجود |
| AES-GCM، وChaCha20-Poly1305، وX25519، وEd25519، وECDSA P-256 وP-384، وRSA PKCS#1 وPSS | (ring); `crypto/*` | `std.crypto.*` | موجود |
| تجزئة كلمات المرور (bcrypt، وscrypt، وargon2) | حزم خارجية؛ `x/crypto` | لا شيء | غير موجود |
| دوال التجزئة غير التشفيرية | `Hasher`; `hash/fnv` | `std.hash.fnv`, `std.hash.xxh64`, `std.hash.combine` | موجود |

## الضغط والأرشيفات

| الأداة | Rust / Go | السلطان | الحالة |
|---|---|---|---|
| DEFLATE، وgzip، ومجاميع التحقق | (flate2); `compress/flate`, `gzip` | `std.compression.deflate`، و`gzip`؛ و`std.checksum` (CRC-32 قائم على جدول مع صيغة للتحديث التدريجي `حدث_سي_ار_سي32`، وAdler-32، وCRC-64/XZ `احسب_سي_ار_سي64`) | موجود |
| zstd (RFC 8878) | (zstd); `klauspost/compress/zstd` | `std.compression.zstd`: مفكِّك ترميز يدعم الإطارات، والإطارات القابلة للتخطي، والكتل الخام وكتل RLE والكتل المضغوطة، والقيم الحرفية (literals) المرمَّزة بـ Huffman مع جدول الشجرة أو دونه (تدفق واحد أو 4 تدفقات)، وتسلسلات FSE بأنماطها الأربعة كلها، والإزاحات المكررة، وحد النافذة، ومجموع التحقق XXH64، والقواميس الخام والقواميس بصيغة zstd، مع حدود للخرج والنافذة؛ وضاغط على نمط المستوى 1 (LZ77 بجدول تجزئة، وقيم حرفية مرمَّزة بـ Huffman مع أوزان مضغوطة بـ FSE، وجداول FSE معرَّفة مسبقًا أو محسوبة)؛ وXXH64 في `std.hash.xxh64` | موجود |
| xz، وLZMA، وLZMA2 | (xz2, lzma-rs); `ulikunitz/xz` | `std.compression.xz` (التدفقات، والكتل، والفهرس، والحشو، والتسلسل؛ وفحوص CRC32 وCRC64 وSHA-256؛ وLZMA2 مع مرشّحي x86 BCJ وDelta؛ ومرمِّز يكتب كتلة LZMA2 واحدة مع CRC64) و`std.compression.lzma` (مفكِّكا ترميز لـ LZMA2 الخام ولصيغة `.lzma`، ومرمِّز LZMA بسلاسل تجزئة بأسلوبي البحث الجشع والكسول) | موجود |
| الأرشيفات tar، وzip، وar | (tar, zip); `archive/tar`, `archive/zip` | `std.archive.*` | موجود |
| bzip2، وlz4 | حزم خارجية؛ `compress/bzip2` (قراءة فقط) | لا شيء | غير موجود |
| الضواغط ومفكِّكات الضغط التدفقية (التدريجية) | محوّلات `Read` و`Write`؛ `io.Reader` | لا شيء: تعمل أدوات الترميز على مخازن كاملة | غير موجود |

## HTTP وTLS

| الأداة | Rust / Go | السلطان | الحالة |
|---|---|---|---|
| عميل وخادم HTTP/1.1، والموجّه، وملفات تعريف الارتباط، وmultipart، وchunked | (hyper); `net/http` | `std.http.**` | موجود |
| عميل وخادم TLS 1.3، والتحقق من شهادات X.509، وPKCS#12، وجذور الثقة في النظام | (rustls); `crypto/tls`, `crypto/x509` | `std.tls.**` | موجود |
| ALPN (RFC 7301) | `ClientConfig::alpn_protocols`; `tls.Config.NextProtos` | `std.tls.config` (`اضبط_بروتوكولات_عميل_تي_إل_إس`، و`اضبط_بروتوكولات_خادم_تي_إل_إس`)، و`std.tls.stream` (`بروتوكول_تدفق_تي_إل_إس`) | موجود |
| HTTP/2 (RFC 9113، وHPACK RFC 7541) | (h2); `net/http` (HTTP/2), `x/net/http2` | `std.http.http2.*`: الإطارات؛ وHPACK مع ترميز Huffman والجداول الديناميكية؛ واتصال يدير حالات التدفقات، والتحكم في التدفق على مستوى الاتصال وعلى مستوى التدفق، وSETTINGS، وPING، وGOAWAY، وRST_STREAM، والحدود (حجم الإطار، وقائمة الترويسات، وكتلة الترويسات، والتدفقات المتزامنة، والجسم)؛ وعميل بتدفقات متزامنة فوق h2c (بالمعرفة المسبقة) أو فوق TLS مع ALPN؛ وخادم يقدّم `خدمة_ويب` نفسها التي يقدّمها HTTP/1، ويُختار البروتوكول بحسب مقدمة العميل (`std.http.server`) أو بحسب ALPN (`شغل_خادم_ويب_آمن`). وتستعمل الطلبات والاستجابات الوحدة المشتركة `std.http.model`. | موجود |
| دفع الخادم في HTTP/2، وجدولة الأولويات، والترقية إلى h2c من HTTP/1.1 | `x/net/http2` | لا شيء: الدفع معطّل، والأولويات تُتجاهل، ولا يُدعم إلا وضع المعرفة المسبقة | غير موجود |
| HTTP/3 | (h3, quinn) | لا شيء | غير موجود |

## التزامن والخيوط

| الأداة | Rust / Go | السلطان | الحالة |
|---|---|---|---|
| الأعداد الصحيحة الذرية (القراءة، والكتابة، والتبديل، وCAS، وfetch_add/and/or/xor، وترتيبات الذاكرة، والسياج) | `std::sync::atomic`; `sync/atomic` | `std.sync.atomic`، تُخفض إلى تعليمات ذرية على x86_64 وarm64 وwasm32 وفي LLVM، وينفّذها المفسر ذو الخيط الواحد تنفيذًا عاديًّا؛ ومنشئات ترتيب الذاكرة (`ترتيب_ذاكرة.تسلسلي`، …) | موجود (متعدد الخيوط على Linux: `sync_atomic_register_pressure`) |
| خيوط نظام التشغيل: الإطلاق، والانضمام، والانضمام مع إعادة قيمة، والنوم، والتنازل، والمعرّف الحالي، والتوازي المتاح | `std::thread` (`spawn`, `JoinHandle<T>::join`, `sleep`, `yield_now`, `current().id()`, `available_parallelism`); goroutines | `std.thread` (`أطلق`، و`انضم`، و`أطلق_بقيمة`، و`انضم_بقيمة`، و`نم`، و`تنازل`، و`معرف_الخيط_الحالي`، و`معرف_المقبض`، و`عدد_المعالجات_المتاحة`) فوق `os.threads`. يتخلص الخيط المنتهي من دالته المغلقة ومما التقطته؛ ويحرر الانضمام ما بقي. يُستعمل `clone` وfutex مباشرةً على Linux وAndroid (كل المعماريات)؛ و`thr_new` و`_umtx_op` على FreeBSD؛ و`pthread_create` و`pthread_join` على macOS؛ و`CreateThread` و`WaitForSingleObject` على Windows. | موجود. WASI والمفسر: غير مدعومة، مع رسالة خطأ واضحة. |
| الانتظار على عنوان | `sys::futex` (داخلي)؛ `runtime.futex`, `sema` | `std.os.wait_address`: futex (Linux، وAndroid)، و`_umtx_op` (FreeBSD)، و`__ulock_wait2` و`__ulock_wake` (macOS)، و`WaitOnAddress` (Windows)، وبديل للخيط الواحد (WASI) | موجود |
| الملكية المشتركة | `Arc` (`strong_count`, `ptr_eq`) | `std.sync.arc` (`مشترك`): العدّاد الذري والإتلاف الذي يحدث مرة واحدة لبيئة دالة مغلقة تملك القيمة المغلّفة | موجود |
| الاستبعاد المتبادل | `Mutex`, `RwLock`; `sync.Mutex`, `sync.RWMutex` | `std.sync.mutex` (`قفل`: قفل futex بثلاث حالات، و`نفذ_مع_القفل`، و`حاول_القفل`، ولا تسميم (poisoning) لأن الانهيار يُنهي البرنامج) و`std.sync.rwlock` (`قفل_قراءة_كتابة`: `نفذ_مع_قراءة`، و`نفذ_مع_كتابة`، ولا أولوية للكاتب) | موجود |
| متغيرات الشرط | `Condvar`; `sync.Cond` | `std.sync.condvar` (`شرط_انتظار`: `انتظر_الشرط`، و`انتظر_الشرط_بمهلة`، و`انتظر_طالما`، و`نبه_واحدا`، و`نبه_الكل`) | موجود |
| التهيئة لمرة واحدة | `Once`, `OnceLock`; `sync.Once` | `std.sync.once` (`مرة`، و`خلية_مرة` مع `اقرأ_خلية_مرة`، و`اقرأ_أو_هيئ_خلية_مرة`، و`اضبط_خلية_مرة`) | موجود |
| القنوات | `mpsc::channel`, `sync_channel`; قنوات Go | `std.sync.channel`: قنوات غير محدودة ومحدودة، و`أرسل`، و`حاول_الإرسال`، و`استقبل`، و`حاول_الاستقبال`، و`استقبل_بمهلة`، وأخطاء الانقطاع في الاتجاهين؛ والقناة ذات الحد 0 تتسع لقيمة واحدة (لا تسليم متزامن rendezvous) | موجود |
| الخلو من سباقات البيانات | `Send`, `Sync` | فحص الالتقاط عند الإطلاق ([نظام الأنواع](type-system.md#قواعد-الالتقاط-في-الخيوط)): لا مؤشر خام خارج `غير_آمن`، ولا مرجع مستعار، ولا متغير وحدة قابل للكتابة في دالة مغلقة تُرسل إلى خيط آخر | جزئي: لا تُفحص الدوال المغلقة الملتقَطة ولا الاستدعاءات |
| الخيوط ذات النطاق، والمتغيرات المحلية للخيط، والحواجز | `thread::scope`, `thread_local!`, `Barrier`; `sync.WaitGroup` | لا شيء | غير موجود |

## الأخطاء

| الأداة | Rust / Go | السلطان | الحالة |
|---|---|---|---|
| قيمة الخطأ المشتركة، وسلاسل الأسباب، والسياق، والتمرير إلى المستدعي | `Error`, `?`, (anyhow); `errors`, `%w` | `std.error` (`فشل`, `؟`, `مع_سياق`, `إنهيار`)، مع محوّلات لأخطاء المكتبة القياسية محددة النوع، ومنها `خطأ_أمر` و`خطأ_مدة` و`خطأ_تنسيق` | موجود |

## الوحدات المساندة

| الوحدة | ما توفّره | Rust / Go |
|---|---|---|
| `std.bytes.builder`, `cursor`, `range`, `compare` | مخزن بايتات قابل للنمو، ومؤشر قراءة يقرأ أعدادًا صحيحة ثابتة العرض، ونطاقات ونسخ متحقَّق منها، والمساواة، والترتيب، واختبارات البادئة واللاحقة، والبحث | `bytes::BytesMut`, `Cursor`; `bytes.Buffer`, `bytes.Compare` |
| `std.chars.ascii` | تصنيف محارف ASCII إلى أرقام، وأرقام ست عشرية، وحروف، وفواصل | `u8::is_ascii_*` |
| `std.debug.assert` | `تأكد` و`تأكد_برسالة`: تأكيدات تبقى فعّالة في كل أنماط البناء وتوقف البرنامج كما يفعل `إنهيار` | `assert!` |
| `std.fuzz.core` | مزوّد حتمي يحوّل بايتات الدخل إلى اختيارات ذات أنواع قابلة لإعادة الإنتاج لأهداف اختبار fuzzing (أما التغطية وجدولة مجموعة المدخلات (corpus) فمن شأن الأدوات) | arbitrary |
| `std.meta.reflect` | حجم النوع، ومحاذاته، وعدد حقوله، وعدد منشئاته، وإفشال الترجمة عمدًا | `mem::size_of`, `align_of`; `reflect` |
| `std.memory.system` | حجم الصفحة وتعيينات الذاكرة في نظام التشغيل مع الحماية والإرشادات (advice) | memmap2; `syscall.Mmap` |
| `std.os.*` | طبقة المنصة: حجز الذاكرة، والملفات، والمقابس، والساعات، والعشوائية، والخيوط، والانتظار على عنوان، والذاكرة الافتراضية | `sys`, libc; `syscall`, `runtime` |

## خصائص اللغة التي تعتمد عليها المكتبة القياسية

تُبنى عدة وحدات في المكتبة القياسية على خصائص في اللغة لها قواعدها الخاصة.

- التحويل العددي: تحوّل `مثل` بين الأعداد العائمة والأعداد الصحيحة بدلالات Rust نفسها
  ([نظام الأنواع](type-system.md)، "التحويلات العددية"). أما أنماط البتات فتوفّرها `بتات_ع64` و`ع64_من_بتات` في
  `std.math.float`، وعليهما يُبنى تمثيل الأعداد العائمة نصًّا والثواني بأعداد عائمة.
- الحساب الملتف: دوال مدمجة (intrinsics) لا تُحدث اعتراضًا (non-trapping) في كل الخلفيات، و`std.math.wrapping`.
  وتستعملها الدوال المساعدة للكلمات ذات 64 بت في SHA-512 (`std.crypto.words64`).
- الحالة على مستوى العملية: يمكن استعمال المتغير العام القابل للكتابة الذي ليس قيمة مفردة (scalar) داخل نطاق
  غير آمن، مثل `static mut` في Rust. وتستعمله `std.env.table`.
- الإزالة من المتجه وسعته: العمليات الأساسية `أزل_الأخير`، و`اقتطع`، و`سعة`
  و`احجز_سعة` ([الملكية](ownership.md)، "الإزالة من الآخر والاقتطاع" و
  "السعة")، وعليها تبني `std.collections.vector` دوال الاحتفاظ، وإزالة
  المتكرر، والإزالة عند موضع، والإزالة بالتبديل، والإدراج، والمسح، والإنشاء بسعة، والحجز، والتقليص. ويبقى مخزن المتجه محجوزًا بعد إفراغه.

## الحدود

- لا تلتقط الدوال المغلقة إلا قيمًا بسيطة، فلا يستطيع منسِّق `std.fmt` مخصّص أن يستعير قيمة تملك نصوصًا أو
  متجهات. لذلك تُنسَّق مثل هذه القيمة أولًا في منسِّق، ثم تُمرَّر نصًّا.
- الوحدات التي يستوردها المترجم نفسه (`std.math.integer`، و`std.time.duration`، و`std.env.system`،
  و`std.crypto.words`، …) يبنيها مترجم الإقلاع الذاتي (bootstrap) Stage0 المكتوب بلغة C. ولهذا السبب جُعلت
  الوحدات المرافقة لها (`std.time.duration_float`، و`std.crypto.words64`، و`std.env.table`،
  و`std.collections.vector`) وحدات منفصلة.
- الثغرات المذكورة في جداول الحالة أعلاه (محوّلات التدفقات العامة، وتعدد الإرسال على المقابس، وXML، وbzip2،
  وlz4، وأدوات الترميز التدفقية، وHTTP/3، والخيوط ذات النطاق، وغيرها) غير منفّذة.

## انظر أيضًا

- [التزامن](concurrency.md): تصميم `std.sync` و`std.thread`.
- [التنسيق](formatting.md): `std.fmt`.
- [الأخطاء](errors.md): `std.error`.
- [الأهداف](targets.md) و[اعتماديات المنصات](platform-dependencies.md): الدعم لكل منصة.

</div>
