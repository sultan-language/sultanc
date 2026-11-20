[العربية](../../ar/book/appendix-04-standard-library.md) | **English**

# D. The Standard Library by Module

The standard library, `std` (Arabic `المكتبة`), is organized in folders of modules. This list is read from
its root files, which declare each module's Arabic spelling. Import a module with either spelling:
`import std.text.number;` or `استورد المكتبة.نصوص.أعداد;`.

[Standard library coverage](../reference/std-coverage.md) in the reference describes what each module
provides.

## `std.io`, `المكتبة.دخ`

Printing, reading lines, readers and writers.

| Module | Arabic |
| --- | --- |
| `std.io.buffered_writer` | `المكتبة.دخ.كاتب_مؤقت` |
| `std.io.log` | `المكتبة.دخ.سجل` |
| `std.io.print` | `المكتبة.دخ.اطبع` |
| `std.io.print_failure` | `المكتبة.دخ.اطبع_فشل` |
| `std.io.print_formatted` | `المكتبة.دخ.اطبع_منسقا` |
| `std.io.println` | `المكتبة.دخ.اطبع_سطر` |
| `std.io.read_line` | `المكتبة.دخ.اقرأ_سطر` |
| `std.io.reader` | `المكتبة.دخ.قارئ` |
| `std.io.writer` | `المكتبة.دخ.كاتب` |

## `std.fs`, `المكتبة.ملفات`

Files, folders, paths, permissions, walking a tree.

| Module | Arabic |
| --- | --- |
| `std.fs.attributes` | `المكتبة.ملفات.سمات` |
| `std.fs.canonical` | `المكتبة.ملفات.مسار_قانوني` |
| `std.fs.directory` | `المكتبة.ملفات.مجلد` |
| `std.fs.error` | `المكتبة.ملفات.أخطاء` |
| `std.fs.handle` | `المكتبة.ملفات.مقبض` |
| `std.fs.links` | `المكتبة.ملفات.وصلات` |
| `std.fs.lock` | `المكتبة.ملفات.قفل_ملف` |
| `std.fs.metadata` | `المكتبة.ملفات.بيانات_وصفية` |
| `std.fs.mutation` | `المكتبة.ملفات.تعديل` |
| `std.fs.path` | `المكتبة.ملفات.مسار` |
| `std.fs.permissions` | `المكتبة.ملفات.أذونات` |
| `std.fs.read_file` | `المكتبة.ملفات.قراءة_ملف` |
| `std.fs.temporary` | `المكتبة.ملفات.مؤقت` |
| `std.fs.walk` | `المكتبة.ملفات.اجتياز` |
| `std.fs.write_bytes` | `المكتبة.ملفات.كتابة_بايتات` |
| `std.fs.write_file` | `المكتبة.ملفات.كتابة_ملف` |

## `std.text`, `المكتبة.نصوص`

Joining, searching, splitting, numbers in text, builders.

| Module | Arabic |
| --- | --- |
| `std.text.system_language` | `المكتبة.نصوص.لغة_النظام` |
| `std.text.bidi` | `المكتبة.نصوص.اتجاه` |
| `std.text.builder` | `المكتبة.نصوص.باني` |
| `std.text.float` | `المكتبة.نصوص.عشري` |
| `std.text.format` | `المكتبة.نصوص.تنسيق` |
| `std.text.from_bytes` | `المكتبة.نصوص.من_بايتات` |
| `std.text.language` | `المكتبة.نصوص.لغة` |
| `std.text.messages` | `المكتبة.نصوص.رسائل` |
| `std.text.number` | `المكتبة.نصوص.أعداد` |
| `std.text.radix` | `المكتبة.نصوص.أساس` |
| `std.text.substring` | `المكتبة.نصوص.مقطع` |
| `std.text.transform` | `المكتبة.نصوص.تحويل` |
| `std.text.compare` | `المكتبة.نصوص.مقارنة` |
| `std.text.range` | `المكتبة.نصوص.نطاق` |
| `std.text.search` | `المكتبة.نصوص.بحث` |

## `std.memory`, `المكتبة.ذاكرة`

Allocation and raw memory.

| Module | Arabic |
| --- | --- |
| `std.memory.system` | `المكتبة.ذاكرة.نظام` |
| `std.memory.allocator` | `المكتبة.ذاكرة.مخصص` |

## `std.net`, `المكتبة.شبكة`

Ip addresses, dns, tcp and udp.

| Module | Arabic |
| --- | --- |
| `std.net.dns` | `المكتبة.شبكة.أسماء_النطاقات` |
| `std.net.ip` | `المكتبة.شبكة.آيبي` |
| `std.net.options` | `المكتبة.شبكة.خيارات` |
| `std.net.socket_address` | `المكتبة.شبكة.عنوان_مقبس` |
| `std.net.socket_names` | `المكتبة.شبكة.أسماء_المقابس` |
| `std.net.tcp` | `المكتبة.شبكة.تي_سي_بي` |
| `std.net.udp` | `المكتبة.شبكة.يو_دي_بي` |
| `std.net.unix` | `المكتبة.شبكة.يونكس` |

## `std.http`, `المكتبة.ويب`

Http messages, clients and servers.

| Module | Arabic |
| --- | --- |
| `std.http.body` | `المكتبة.ويب.جسم` |
| `std.http.client` | `المكتبة.ويب.عميل` |
| `std.http.cookies` | `المكتبة.ويب.كعكات` |
| `std.http.deadline` | `المكتبة.ويب.مهلة` |
| `std.http.handover` | `المكتبة.ويب.تسليم` |
| `std.http.headers` | `المكتبة.ويب.ترويسات` |
| `std.http.limits` | `المكتبة.ويب.حدود` |
| `std.http.locale` | `المكتبة.ويب.لغة_الطلب` |
| `std.http.media_type` | `المكتبة.ويب.نوع_الوسائط` |
| `std.http.model` | `المكتبة.ويب.نموذج` |
| `std.http.multipart` | `المكتبة.ويب.متعدد_الأجزاء` |
| `std.http.query` | `المكتبة.ويب.استعلام` |
| `std.http.route_table` | `المكتبة.ويب.جدول_المسارات` |
| `std.http.router` | `المكتبة.ويب.موجه` |
| `std.http.server` | `المكتبة.ويب.خادم` |
| `std.http.shutdown` | `المكتبة.ويب.إيقاف` |
| `std.http.transport` | `المكتبة.ويب.ناقل` |
| `std.http.http1` | `المكتبة.ويب.ويب1` |
| `std.http.http2` | `المكتبة.ويب.ويب2` |

## `std.debug`, `المكتبة.تصحيح`

Assertions.

| Module | Arabic |
| --- | --- |
| `std.debug.assert` | `المكتبة.تصحيح.تأكد` |
| `std.debug.location` | `المكتبة.تصحيح.موقع` |

## `std.error`, `المكتبة.أخطاء`

The standard `error` type.

| Module | Arabic |
| --- | --- |
| `std.error.from_command` | `المكتبة.أخطاء.من_أمر` |
| `std.error.from_net` | `المكتبة.أخطاء.من_شبكة` |
| `std.error.from_os` | `المكتبة.أخطاء.من_نظام` |
| `std.error.context` | `المكتبة.أخطاء.سياق` |
| `std.error.failure` | `المكتبة.أخطاء.فشل` |
| `std.error.from_duration` | `المكتبة.أخطاء.من_مدة` |
| `std.error.from_fmt` | `المكتبة.أخطاء.من_تنسيق` |
| `std.error.from_json` | `المكتبة.أخطاء.من_جسون` |
| `std.error.from_number` | `المكتبة.أخطاء.من_عدد` |

## `std.process`, `المكتبة.عمليات`

Arguments, exit, panic, running other programs.

| Module | Arabic |
| --- | --- |
| `std.process.argument` | `المكتبة.عمليات.وسيطة` |
| `std.process.argument_count` | `المكتبة.عمليات.عدد_الوسائط` |
| `std.process.arguments` | `المكتبة.عمليات.الوسائط` |
| `std.process.child` | `المكتبة.عمليات.عملية_ابن` |
| `std.process.cli` | `المكتبة.عمليات.سطر_أوامر` |
| `std.process.command` | `المكتبة.عمليات.أمر` |
| `std.process.exit` | `المكتبة.عمليات.اخرج` |
| `std.process.failure` | `المكتبة.عمليات.فشل` |
| `std.process.id` | `المكتبة.عمليات.معرف_العملية` |
| `std.process.panic_handler` | `المكتبة.عمليات.معالج_الانهيار` |
| `std.process.failure_text` | `المكتبة.عمليات.نص_الفشل` |
| `std.process.panic` | `المكتبة.عمليات.انهيار` |

## `std.env`, `المكتبة.بيئة`

Environment variables and directories.

| Module | Arabic |
| --- | --- |
| `std.env.environment` | `المكتبة.بيئة.بيئة_العمل` |
| `std.env.executable` | `المكتبة.بيئة.البرنامج_الحالي` |
| `std.env.system` | `المكتبة.بيئة.نظام` |
| `std.env.table` | `المكتبة.بيئة.جدول` |
| `std.env.variables` | `المكتبة.بيئة.متغيرات` |

## `std.time`, `المكتبة.وقت`

Clocks, sleeping, calendar, time zones.

| Module | Arabic |
| --- | --- |
| `std.time.time` | `المكتبة.وقت.الوقت` |
| `std.time.timezone` | `المكتبة.وقت.منطقة_زمنية` |
| `std.time.date` | `المكتبة.وقت.تاريخ` |
| `std.time.duration_text` | `المكتبة.وقت.نص_المدة` |
| `std.time.calendar` | `المكتبة.وقت.تقويم` |
| `std.time.duration` | `المكتبة.وقت.مدة` |
| `std.time.duration_float` | `المكتبة.وقت.مدة_عشرية` |

## `std.testing`, `المكتبة.اختبارات`

Helpers for tests.

| Module | Arabic |
| --- | --- |
| `std.testing.harness` | `المكتبة.اختبارات.مشغل` |

## `std.async`, `المكتبة.غير_متزامن`

Asynchronous tasks.

| Module | Arabic |
| --- | --- |
| `std.async.net` | `المكتبة.غير_متزامن.شبكة` |
| `std.async.runtime` | `المكتبة.غير_متزامن.وقت_التشغيل` |

## `std.os`, `المكتبة.نظام_التشغيل`

Operating-system details.

| Module | Arabic |
| --- | --- |
| `std.os.allocation` | `المكتبة.نظام_التشغيل.تخصيص` |
| `std.os.allocation_cache` | `المكتبة.نظام_التشغيل.مخبأ_التخصيص` |
| `std.os.allocator_platform` | `المكتبة.نظام_التشغيل.منصة_المخصص` |
| `std.os.arguments` | `المكتبة.نظام_التشغيل.وسائط` |
| `std.os.buffer` | `المكتبة.نظام_التشغيل.مخزن` |
| `std.os.capability` | `المكتبة.نظام_التشغيل.مرافق` |
| `std.os.child_stdio` | `المكتبة.نظام_التشغيل.قنوات_العملية_الابن` |
| `std.os.clock` | `المكتبة.نظام_التشغيل.ساعة` |
| `std.os.directory` | `المكتبة.نظام_التشغيل.مجلد` |
| `std.os.entropy` | `المكتبة.نظام_التشغيل.عشوائية` |
| `std.os.environment` | `المكتبة.نظام_التشغيل.بيئة` |
| `std.os.environment_list` | `المكتبة.نظام_التشغيل.قائمة_البيئة` |
| `std.os.environment_validation` | `المكتبة.نظام_التشغيل.تحقق_البيئة` |
| `std.os.error` | `المكتبة.نظام_التشغيل.أخطاء` |
| `std.os.error_translation` | `المكتبة.نظام_التشغيل.ترجمة_الأخطاء` |
| `std.os.executable` | `المكتبة.نظام_التشغيل.البرنامج` |
| `std.os.file` | `المكتبة.نظام_التشغيل.ملف` |
| `std.os.file_attributes` | `المكتبة.نظام_التشغيل.سمات_الملف` |
| `std.os.file_lock` | `المكتبة.نظام_التشغيل.قفل_الملف` |
| `std.os.io` | `المكتبة.نظام_التشغيل.دخل_خرج` |
| `std.os.path` | `المكتبة.نظام_التشغيل.مسار` |
| `std.os.poll` | `المكتبة.نظام_التشغيل.استطلاع` |
| `std.os.process` | `المكتبة.نظام_التشغيل.عملية` |
| `std.os.process_info` | `المكتبة.نظام_التشغيل.معلومات_العملية` |
| `std.os.signal` | `المكتبة.نظام_التشغيل.إشارة` |
| `std.os.socket` | `المكتبة.نظام_التشغيل.مقبس` |
| `std.os.socket_names` | `المكتبة.نظام_التشغيل.أسماء_المقابس` |
| `std.os.stack_limit` | `المكتبة.نظام_التشغيل.حد_المكدس` |
| `std.os.terminal` | `المكتبة.نظام_التشغيل.طرفية` |
| `std.os.threads` | `المكتبة.نظام_التشغيل.خيوط` |
| `std.os.trust_store` | `المكتبة.نظام_التشغيل.مخزن_الثقة` |
| `std.os.unix_socket` | `المكتبة.نظام_التشغيل.مقبس_يونكس` |
| `std.os.virtual_memory` | `المكتبة.نظام_التشغيل.ذاكرة_افتراضية` |
| `std.os.wait_address` | `المكتبة.نظام_التشغيل.انتظار_العنوان` |
| `std.os.platform` | `المكتبة.نظام_التشغيل.منصات` |

## `std.sync`, `المكتبة.تزامن`

Arc, mutex, channels, atomics, locks.

| Module | Arabic |
| --- | --- |
| `std.sync.arc` | `المكتبة.تزامن.مشترك` |
| `std.sync.cancel` | `المكتبة.تزامن.إلغاء` |
| `std.sync.channel` | `المكتبة.تزامن.قناة` |
| `std.sync.condvar` | `المكتبة.تزامن.شرط_انتظار` |
| `std.sync.mutex` | `المكتبة.تزامن.قفل` |
| `std.sync.once` | `المكتبة.تزامن.مرة` |
| `std.sync.rwlock` | `المكتبة.تزامن.قفل_قراءة_كتابة` |
| `std.sync.atomic` | `المكتبة.تزامن.ذري` |

## `std.thread`, `المكتبة.خيوط`

Starting and joining threads.

| Module | Arabic |
| --- | --- |
| `std.thread.thread` | `المكتبة.خيوط.خيط` |

## `std.tls`, `المكتبة.تي_إل_إس`

Tls 1.3.

| Module | Arabic |
| --- | --- |
| `std.tls.client` | `المكتبة.تي_إل_إس.عميل` |
| `std.tls.config` | `المكتبة.تي_إل_إس.إعداد` |
| `std.tls.errors` | `المكتبة.تي_إل_إس.أخطاء` |
| `std.tls.key_exchange` | `المكتبة.تي_إل_إس.تبادل_المفاتيح` |
| `std.tls.pkcs12` | `المكتبة.تي_إل_إس.بي_كي_سي_إس12` |
| `std.tls.record` | `المكتبة.تي_إل_إس.سجل` |
| `std.tls.server` | `المكتبة.تي_إل_إس.خادم` |
| `std.tls.signature` | `المكتبة.تي_إل_إس.توقيع` |
| `std.tls.stream` | `المكتبة.تي_إل_إس.مجرى` |
| `std.tls.trust` | `المكتبة.تي_إل_إس.ثقة` |
| `std.tls.verify` | `المكتبة.تي_إل_إس.تحقق` |
| `std.tls.wire` | `المكتبة.تي_إل_إس.سلك` |
| `std.tls.x509` | `المكتبة.تي_إل_إس.إكس509` |

## `std.uri`, `المكتبة.عنوان_موحد`

Uris.

| Module | Arabic |
| --- | --- |
| `std.uri.uri` | `المكتبة.عنوان_موحد.عنوان_موحد` |

## `std.uuid`, `المكتبة.معرف_فريد`

Uuids.

| Module | Arabic |
| --- | --- |
| `std.uuid.generate` | `المكتبة.معرف_فريد.توليد` |
| `std.uuid.uuid` | `المكتبة.معرف_فريد.معرف_فريد` |

## `std.archive`, `المكتبة.أرشيف`

Tar and zip archives.

| Module | Arabic |
| --- | --- |
| `std.archive.tar` | `المكتبة.أرشيف.تار` |
| `std.archive.zip` | `المكتبة.أرشيف.زيب` |
| `std.archive.ar` | `المكتبة.أرشيف.آر` |

## `std.bytes`, `المكتبة.بايتات`

Byte buffers.

| Module | Arabic |
| --- | --- |
| `std.bytes.builder` | `المكتبة.بايتات.باني` |
| `std.bytes.cursor` | `المكتبة.بايتات.مؤشر` |
| `std.bytes.range` | `المكتبة.بايتات.نطاق` |
| `std.bytes.compare` | `المكتبة.بايتات.مقارنة` |

## `std.checksum`, `المكتبة.مجاميع_تحقق`

Crc and other checksums.

| Module | Arabic |
| --- | --- |
| `std.checksum.checksum` | `المكتبة.مجاميع_تحقق.مجاميع_تحقق` |

## `std.collections`, `المكتبة.مجموعات`

Maps, sets, queues, sorting.

| Module | Arabic |
| --- | --- |
| `std.collections.bitset` | `المكتبة.مجموعات.مجموعة_بتات` |
| `std.collections.btree_map` | `المكتبة.مجموعات.خريطة_شجرية` |
| `std.collections.btree_set` | `المكتبة.مجموعات.مجموعة_شجرية` |
| `std.collections.map` | `المكتبة.مجموعات.خريطة` |
| `std.collections.set` | `المكتبة.مجموعات.مجموعة` |
| `std.collections.sort` | `المكتبة.مجموعات.ترتيب` |
| `std.collections.text_index_map` | `المكتبة.مجموعات.خريطة_نص_فهرس` |
| `std.collections.deque` | `المكتبة.مجموعات.رتل_مزدوج` |
| `std.collections.heap` | `المكتبة.مجموعات.كومة` |
| `std.collections.vector` | `المكتبة.مجموعات.متجهات` |

## `std.compression`, `المكتبة.ضغط`

Deflate, gzip, zlib.

| Module | Arabic |
| --- | --- |
| `std.compression.deflate` | `المكتبة.ضغط.ديفليت` |
| `std.compression.gzip` | `المكتبة.ضغط.جي_زيب` |
| `std.compression.lzma` | `المكتبة.ضغط.إل_زد_إم_إيه` |
| `std.compression.xz` | `المكتبة.ضغط.إكس_زد` |
| `std.compression.zstd` | `المكتبة.ضغط.زد_ستد` |

## `std.crypto`, `المكتبة.تشفير`

Hashes, ciphers, signatures.

| Module | Arabic |
| --- | --- |
| `std.crypto.aes` | `المكتبة.تشفير.أيس` |
| `std.crypto.aes_gcm` | `المكتبة.تشفير.أيس_جي_سي_إم` |
| `std.crypto.bignum` | `المكتبة.تشفير.أعداد_كبيرة` |
| `std.crypto.chacha20_poly1305` | `المكتبة.تشفير.تشاتشا20_بولي1305` |
| `std.crypto.des` | `المكتبة.تشفير.دي_إي_إس` |
| `std.crypto.digest` | `المكتبة.تشفير.ملخص` |
| `std.crypto.ecdsa` | `المكتبة.تشفير.إي_سي_دي_إس_إيه` |
| `std.crypto.ed25519` | `المكتبة.تشفير.إد25519` |
| `std.crypto.field25519` | `المكتبة.تشفير.حقل25519` |
| `std.crypto.rc2` | `المكتبة.تشفير.آر_سي2` |
| `std.crypto.rsa` | `المكتبة.تشفير.آر_إس_إيه` |
| `std.crypto.sha1` | `المكتبة.تشفير.شا1` |
| `std.crypto.sha256` | `المكتبة.تشفير.شا256` |
| `std.crypto.sha512` | `المكتبة.تشفير.شا512` |
| `std.crypto.words` | `المكتبة.تشفير.كلمات` |
| `std.crypto.x25519` | `المكتبة.تشفير.إكس25519` |
| `std.crypto.words64` | `المكتبة.تشفير.كلمات64` |

## `std.csv`, `المكتبة.سي_اس_في`

Csv files.

| Module | Arabic |
| --- | --- |
| `std.csv.csv` | `المكتبة.سي_اس_في.سي_اس_في` |

## `std.encoding`, `المكتبة.ترميز`

Utf-8, base64, hexadecimal.

| Module | Arabic |
| --- | --- |
| `std.encoding.base32` | `المكتبة.ترميز.قاعدة32` |
| `std.encoding.base64` | `المكتبة.ترميز.قاعدة64` |
| `std.encoding.binary` | `المكتبة.ترميز.ثنائي` |
| `std.encoding.der` | `المكتبة.ترميز.دي_إي_آر` |
| `std.encoding.hex` | `المكتبة.ترميز.سداسي` |
| `std.encoding.pem` | `المكتبة.ترميز.بي_إي_إم` |
| `std.encoding.positions` | `المكتبة.ترميز.مواضع` |
| `std.encoding.utf16` | `المكتبة.ترميز.يوتيف16` |
| `std.encoding.utf8` | `المكتبة.ترميز.يوتيف8` |
| `std.encoding.varint` | `المكتبة.ترميز.فارنت` |

## `std.fmt`, `المكتبة.تنسيق`

Format strings.

| Module | Arabic |
| --- | --- |
| `std.fmt.fmt` | `المكتبة.تنسيق.تنسيق` |

## `std.fuzz`, `المكتبة.فز`

Fuzz testing.

| Module | Arabic |
| --- | --- |
| `std.fuzz.core` | `المكتبة.فز.نواة` |

## `std.json`, `المكتبة.جسون`

Json.

| Module | Arabic |
| --- | --- |
| `std.json.derive` | `المكتبة.جسون.اشتقاق` |
| `std.json.escape` | `المكتبة.جسون.هروب` |
| `std.json.parse` | `المكتبة.جسون.تحليل` |
| `std.json.stringify` | `المكتبة.جسون.إلى_نص` |
| `std.json.value` | `المكتبة.جسون.قيمة` |

## `std.math`, `المكتبة.رياضيات`

Math functions, wrapping and checked arithmetic.

| Module | Arabic |
| --- | --- |
| `std.math.trigonometry` | `المكتبة.رياضيات.مثلثات` |
| `std.math.float` | `المكتبة.رياضيات.عشري` |
| `std.math.integer` | `المكتبة.رياضيات.صحيح` |
| `std.math.wrapping` | `المكتبة.رياضيات.التفاف` |

## `std.mime`, `المكتبة.أنواع_وسائط`

Media types.

| Module | Arabic |
| --- | --- |
| `std.mime.mime` | `المكتبة.أنواع_وسائط.أنواع_وسائط` |

## `std.toml`, `المكتبة.تومل`

Toml.

| Module | Arabic |
| --- | --- |
| `std.toml.parser` | `المكتبة.تومل.محلل` |
| `std.toml.writer` | `المكتبة.تومل.كاتب` |

## `std.unicode`, `المكتبة.يونيكود`

Unicode properties, case, normalization.

| Module | Arabic |
| --- | --- |
| `std.unicode.case_folding` | `المكتبة.يونيكود.طي_الحالة` |
| `std.unicode.case_mapping` | `المكتبة.يونيكود.تحويل_الحالة` |
| `std.unicode.code_point_text` | `المكتبة.يونيكود.نص_نقطة` |
| `std.unicode.general_category` | `المكتبة.يونيكود.الفئة_العامة` |
| `std.unicode.grapheme` | `المكتبة.يونيكود.عناقيد` |
| `std.unicode.normalization` | `المكتبة.يونيكود.تطبيع` |
| `std.unicode.properties` | `المكتبة.يونيكود.خصائص` |
| `std.unicode.sentence_boundary` | `المكتبة.يونيكود.حدود_الجمل` |
| `std.unicode.unicode` | `المكتبة.يونيكود.يونيكود` |
| `std.unicode.word_boundary` | `المكتبة.يونيكود.حدود_الكلمات` |
| `std.unicode.algorithms` | `المكتبة.يونيكود.خوارزميات` |
| `std.unicode.data` | `المكتبة.يونيكود.بيانات` |
| `std.unicode.scalar` | `المكتبة.يونيكود.قيم_قياسية` |
| `std.unicode.version` | `المكتبة.يونيكود.إصدار` |

## `std.version`, `المكتبة.إصدار`

Semantic versions.

| Module | Arabic |
| --- | --- |
| `std.version.semver` | `المكتبة.إصدار.إصدار_دلالي` |

## `std.chars`, `المكتبة.حروف`

Characters and ascii.

| Module | Arabic |
| --- | --- |
| `std.chars.ascii` | `المكتبة.حروف.أسكي` |

## `std.hash`, `المكتبة.تجزئة`

Hash functions for collections.

| Module | Arabic |
| --- | --- |
| `std.hash.combine` | `المكتبة.تجزئة.دمج` |
| `std.hash.fnv` | `المكتبة.تجزئة.فنڤ` |
| `std.hash.xxh64` | `المكتبة.تجزئة.إكس_إكس_إتش64` |

## `std.meta`, `المكتبة.انعكاس`

Reflection.

| Module | Arabic |
| --- | --- |
| `std.meta.reflect` | `المكتبة.انعكاس.انعكاس_الأنواع` |

## `std.random`, `المكتبة.عشوائي`

Pseudorandom numbers.

| Module | Arabic |
| --- | --- |
| `std.random.prng` | `المكتبة.عشوائي.مولد` |
