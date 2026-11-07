# Standard Library Coverage

This page lists what SultanC's standard library (`library/std/**`) provides, module by module, and
compares each area with Rust's and Go's standard libraries. It is a reference for programmers
deciding whether std covers a need and for contributors looking for gaps.

A row's status has one of three values. **Present** means the common uses of the Rust and Go
facility have a std equivalent. **Partial** means some are covered and the gaps are named.
**Missing** means std has nothing. Names are given in Arabic with the English alias where one
exists; every English alias is listed in [Arabic terminology](arabic-terminology.md).

Platform notes name what a platform lacks. The supported platforms are Linux and Android (raw
kernel calls; 64-bit, i686 and armv7), FreeBSD (the same direct-kernel provider), macOS
(libSystem), Windows (kernel32, ws2_32) and WASI preview 1. See [Targets](targets.md) and
[Platform dependencies](platform-dependencies.md).

## Module index

| Module path | Provides | Closest in Rust / Go |
|---|---|---|
| `std.process.*` | spawn, wait, exit status, command builder, streaming children, arguments, `exit`, `panic`, command-line parsing | `std::process`, `env::args`; `os/exec`, `flag` |
| `std.env.*` | environment variables, working directory, home and temp directories | `std::env`; `os` |
| `std.fs.*` | files, directories, metadata, permissions, links, temporary files, locks, paths | `std::fs`, `std::path`; `os`, `path/filepath` |
| `std.io.*` | print and read functions, readers, writers, buffering, logging | `std::io`, `log`; `io`, `bufio`, `log` |
| `std.fmt.fmt` | format strings and the format-spec mini-language | `std::fmt`; `fmt` |
| `std.net.*` | TCP, UDP, Unix sockets, IP and socket addresses, DNS | `std::net`; `net` |
| `std.time.*` | clocks, durations, calendar dates, time zones | `std::time`, chrono; `time` |
| `std.text.*` | search, transform, builders, number and float text, language tags, bidi | `str`, `String`; `strings`, `strconv` |
| `std.unicode.*` | categories, properties, case mapping, normalization, segmentation | `char`, unicode crates; `unicode`, `x/text` |
| `std.collections.*` | maps, sets, B-trees, deque, heap, bit set, sorting, vector utilities | `std::collections`, `Vec`; `container/*`, `slices`, `sort` |
| `std.math.*` | integer and float math, wrapping arithmetic, trigonometry | `core` numeric methods; `math`, `math/bits` |
| `std.random.prng`, `std.os.entropy` | a seeded generator and OS entropy | rand; `math/rand`, `crypto/rand` |
| `std.encoding.*`, `std.json`, `std.toml`, `std.csv`, `std.uri`, `std.mime`, `std.uuid`, `std.version` | data formats and identifiers | serde_json, toml, csv, url, uuid, semver; `encoding/*`, `net/url`, `mime` |
| `std.crypto.*`, `std.hash.*`, `std.checksum` | hashes, MACs, KDFs, AEADs, signatures, big numbers, non-cryptographic hashes, CRCs | RustCrypto, ring; `crypto/*`, `hash/*` |
| `std.compression.*`, `std.archive.*` | deflate, gzip, zstd, xz, LZMA; tar, zip, ar | flate2, zstd, xz2, tar, zip; `compress/*`, `archive/*` |
| `std.http.*`, `std.tls.*` | HTTP/1.1 and HTTP/2 client and server, TLS 1.3, X.509 | hyper, rustls; `net/http`, `crypto/tls` |
| `std.sync.*`, `std.thread`, `std.os.wait_address` | threads, atomics, locks, channels | `std::sync`, `std::thread`; `sync`, goroutines |
| `std.error.*` | the common error value and converters | `Error`, anyhow; `errors` |
| `std.bytes.*`, `std.chars.ascii` | byte buffers, cursors, comparison; ASCII classification | `bytes`; `bytes` |
| `std.debug.assert`, `std.fuzz.core`, `std.meta.reflect`, `std.memory.system` | assertions, fuzz input, type reflection, OS memory mappings | `assert!`, arbitrary, `mem::size_of`, memmap; `reflect`, `syscall.Mmap` |
| `std.os.*` | the platform layer under the modules above | `sys`, libc; `syscall`, `runtime` |

## Summary

| Area | Status | Main gaps |
|---|---|---|
| process | present | process groups, signals other than kill, `pre_exec`-style hooks |
| env | present | |
| fs | present | ownership (chown), directory watching, byte-range locks |
| path | present | |
| io | partial | generic stream adapters (take, chain, tee), stdin locking, vectored I/O |
| net | partial | non-blocking multiplexing (epoll, kqueue), multicast, descriptor passing over Unix sockets |
| time | present | |
| text and strings | present | named format arguments |
| unicode | present | |
| collections | present | linked list |
| math | present | big integers outside crypto, complex numbers |
| encoding | present | XML, gob, MessagePack, CBOR |
| crypto | present | a TLS-oriented set: no bcrypt, scrypt or argon2 |
| compression | partial | bzip2, lz4; streaming (incremental) codecs |
| http and tls | present | HTTP/3; HTTP/2 server push and priorities |
| sync and threads | present | scoped threads, thread-locals, barriers; weaker atomic orderings; see [Concurrency](concurrency.md) |
| error | present | |

## process

| Facility | Rust / Go | SultanC | Status |
|---|---|---|---|
| Spawn, wait, exit status | `Command::spawn`, `Child::wait`; `exec.Cmd.Start/Wait` | `std.process.child` (`شغل_عملية`, `انتظر_عملية`, `حالة_إنهاء`) | present |
| try_wait, kill, wait with timeout | `try_wait`, `kill` | `std.process.child` (`تفقد_عملية`, `اقتل_عملية`, `انتظر_عملية_بمهلة`) on Unix; `std.process.command` children (`تفقد_الطفل`, `اقتل_الطفل`) on every OS with processes, Windows included | present |
| Builder: args, env set/remove/clear, current dir | `Command::arg/env/env_remove/env_clear/current_dir` | `std.process.command` | present (WASI: no processes) |
| Child stdio inherit/null/piped, output capture, input bytes | `Command::output`, `Stdio`; `Cmd.Output`, `CombinedOutput` | `std.process.command` (`اجمع_خرج_الأمر`, `شغل_الأمر`, `مصدر_تيار`) | present |
| Streaming pipes to a live child | `Command::spawn` with `Stdio::piped`, `Child { stdin, stdout, stderr }`, `wait`, `try_wait`, `kill`, `wait_with_output`; `Cmd.StdinPipe/StdoutPipe/Start` | `std.process.command` (`ابدأ_الأمر`, `طفل_أمر`, `دخل_طفل` and `خرج_طفل` in the `std.io` reader and writer shapes, `اكتب_إلى_الطفل`, `اقرأ_خرج_الطفل`, `أغلق_دخل_الطفل`, `أغلق_تيارات_الطفل`, `انتظر_الطفل`, `تفقد_الطفل`, `اقتل_الطفل`, `انتظر_خرج_الطفل`) | present. Each stream is inherit, null or piped. With no destructors, closing the input and the leftover pipes is explicit, and `wait` closes the input as Rust does. A write to a child that closed its input is EPIPE, not SIGPIPE. WASI: unsupported. |
| Arguments, exit, panic | `env::args`, `process::exit`, `panic!` | `std.process.arguments`, `exit`, `panic` (`إنهيار`) | present |
| Command-line parsing | (clap crate); `flag` | `std.process.cli` | present |

## env

| Facility | Rust / Go | SultanC | Status |
|---|---|---|---|
| Read one variable | `env::var`; `os.Getenv/LookupEnv` | `std.env.environment` (`متغير_بيئة`) | present |
| All variables | `env::vars`; `os.Environ` | `std.env.variables` (`متغيرات_البيئة`) | present |
| Set and unset own variables | `env::set_var/remove_var`; `os.Setenv/Unsetenv` | `std.env.variables` (`اضبط_متغير_بيئة`, `أزل_متغير_بيئة`) over `std.env.table`, inherited by `std.process.command` | present. One process-wide lock once a thread exists, as Rust's `ENV_LOCK`. |
| Current directory get and set | `env::current_dir/set_current_dir`; `os.Getwd/Chdir` | `دليل_العمل`, `غير_دليل_العمل` | present (WASI: no chdir) |
| Home and temp dirs | `env::home_dir/temp_dir`; `os.UserHomeDir/TempDir` | `الدليل_الرئيسي`, `الدليل_المؤقت` | present |

## fs and path

| Facility | Rust / Go | SultanC | Status |
|---|---|---|---|
| Read and write whole files, copy | `fs::read/write/copy`; `os.ReadFile/WriteFile` | `std.fs.read_file`, `write_file`, `write_bytes` | present |
| Handles: open, read, write, seek, sync, close | `File`; `os.File` | `std.fs.handle` | present |
| Truncate, set length | `File::set_len`; `os.Truncate` | `std.fs.attributes` (`اقتطع_الملف`, `اقتطع_مقبض_ملف`) | present |
| Metadata: type, size, mtime, exists | `fs::metadata`; `os.Stat` | `std.fs.metadata`, `directory` | present |
| Set file times | `File::set_times`; `os.Chtimes` | `اضبط_أوقات_الملف` | present |
| Permissions read and chmod | `Permissions`, `set_permissions`; `os.Chmod` | `std.fs.permissions` | present (Windows: read-only bit; WASI: none) |
| Symlink create and read, hard link | `os::unix::fs::symlink`, `read_link`, `hard_link`; `os.Symlink/Readlink/Link` | `std.fs.links` | present |
| Canonicalize, absolute | `fs::canonicalize`, `path::absolute`; `filepath.EvalSymlinks/Abs` | `std.fs.canonical` | present |
| create_dir_all, remove_dir_all, copy tree, walk | `create_dir_all`, `remove_dir_all`; `os.MkdirAll/RemoveAll`, `filepath.WalkDir` | `std.fs.walk` | present |
| Temporary files and directories | (tempfile crate); `os.CreateTemp/MkdirTemp` | `std.fs.temporary` | present |
| Rename, remove, mkdir, list directory | `fs::rename/remove_file/read_dir` | `std.fs.mutation`, `directory` | present |
| Whole-file locks | `File::lock/lock_shared/try_lock/try_lock_shared/unlock` (Rust 1.89); `syscall.Flock` | `std.fs.lock` (`اقفل_الملف`, `اقفل_الملف_مشتركا`, `حاول_قفل_الملف`, `حاول_قفل_الملف_مشتركا`, `افتح_قفل_الملف`; `خطأ_محاولة_قفل.ستحظر` is Rust's `TryLockError::WouldBlock`) | present (flock on Linux, Android, FreeBSD and macOS; `LockFileEx` on Windows; WASI: unsupported) |
| chown, watching | `os::unix::fs::chown`; `os.Chown`, (fsnotify) | none | missing |
| Join, parent, file name, extension, components, normalize | `Path`; `path/filepath` | `std.fs.path` (POSIX and Windows flavors) | present |

## io

| Facility | Rust / Go | SultanC | Status |
|---|---|---|---|
| print, eprint, stdin lines | `println!`, `stdin().lines()`; `fmt.Println`, `bufio.Scanner` | `std.io.print`, `println`, `read_line` | present |
| Buffered reader and writer | `BufReader/BufWriter`; `bufio` | `std.io.read_line` (`قارئ_مخزن`), `buffered_writer` | present |
| Reader and Writer, copy | `Read/Write`, `io::copy`; `io.Reader/Writer`, `io.Copy` | `std.io.reader`, `writer` | present |
| Adapters (take, chain, multi-writer), vectored I/O | `Read::take/chain`; `io.MultiWriter`, `TeeReader` | none | missing |
| Logging | (log crate); `log`, `slog` | `std.io.log` | present |

## fmt

| Facility | Rust / Go | SultanC | Status |
|---|---|---|---|
| Format strings, `Display` and `Debug`, the format-spec mini-language | `format!`, `write!`, `print!`; `fmt.Sprintf`, `Fprintf`, `Printf` | `std.fmt.fmt` (`نسق` / `format`, `نسق_في` / `format_into`, `اكتب_منسقا` / `write_fmt`, `اطبع_سطرا_منسقا` / `println_fmt` and the other print forms, `قيمة_تنسيق` / `fmt_arg` with a custom closure case, `منسق` / `formatter`); `std.io.log`'s `سجل_منسقا` / `log_fmt` | partial. A literal format string is checked at compile time against a vector-literal argument list (count, positions, types, counts); a format string built at run time is checked at run time. No named arguments. Custom closures capture plain values only. See [Formatting](formatting.md). |

## net

| Facility | Rust / Go | SultanC | Status |
|---|---|---|---|
| TCP connect, listen, accept, read, write, shutdown, options | `TcpStream/TcpListener`; `net.Dial/Listen` | `std.net.tcp`, `options` | present (WASI: preopened sockets only) |
| UDP bind, connect, send and receive | `UdpSocket` | `std.net.udp` | present |
| UDP unconnected send_to and recv_from | `UdpSocket::send_to/recv_from`; `WriteTo/ReadFrom` | `std.net.socket_names` | present |
| Local and peer addresses | `local_addr/peer_addr`; `LocalAddr/RemoteAddr` | `std.net.socket_names` | present |
| IP and socket addresses, DNS | `IpAddr`, `ToSocketAddrs`; `net.ParseIP`, `LookupHost` | `std.net.ip`, `socket_address`, `dns` | present |
| Timeouts | `set_read_timeout` | `خيار_مقبس.مهلة_استقبال` and `مهلة_إرسال` | present |
| Unix domain sockets: stream listener and streams, datagrams, pathname and abstract addresses | `std::os::unix::net::{UnixListener, UnixStream, UnixDatagram, SocketAddr}`; `net.UnixListener/UnixConn` | `std.net.unix` (`مستمع_يونكس`, `تدفق_يونكس`, `مقبس_رسائل_يونكس`, `عنوان_يونكس`) over `std.os.unix_socket` | present on Linux, Android, FreeBSD and macOS; abstract names on Linux and Android; Windows 10 1803+: streams only; WASI: unsupported. No socketpair, no descriptor passing, no peer credentials. |
| Polling many sockets, multicast | (mio); `JoinGroup` | none | missing |

## time

| Facility | Rust / Go | SultanC | Status |
|---|---|---|---|
| Monotonic and system clock, sleep | `Instant`, `SystemTime`, `sleep`; `time.Now`, `Sleep` | `std.time.time` | present |
| Durations: arithmetic, compare | `Duration`; `time.Duration` | `std.time.duration`, with checked abs and divide | present |
| Duration text | `Duration` Debug; `Duration.String`, `ParseDuration` | `std.time.duration_text` | present |
| Calendar, RFC 3339 and ISO 8601, HTTP dates | (chrono); `time.Parse/Format` | `std.time.calendar`, `date` | present |
| Time zones (TZif) | (chrono-tz); `time.LoadLocation` | `std.time.timezone` | present |
| Float seconds | `Duration::as_secs_f64`, `try_from_secs_f64`; `Seconds()` | `std.time.duration_float` (`ثواني_المدة_ع64`, `مدة_من_ثواني_ع64`, and ع32) | present |

## text and unicode

| Facility | Rust / Go | SultanC | Status |
|---|---|---|---|
| find, rfind, contains, starts and ends with | `str::find`; `strings.Index/Contains/HasPrefix` | `std.text.search` | present |
| trim, split, join, replace, lines, repeat | `trim/split/join/replace`; `strings.TrimSpace/Split/Join` | `std.text.transform` | present |
| split_whitespace, splitn, replacen, strip_prefix and suffix, ASCII case | `split_whitespace/splitn/replacen/strip_prefix`; `strings.Fields/SplitN/CutPrefix` | `std.text.transform` | present |
| count, char count, eq_ignore_ascii_case | `matches().count`, `chars().count`; `strings.Count`, `EqualFold` | `std.text.search` | present |
| Builder, numbers, radix, compare | `String`, `parse`; `strings.Builder`, `strconv` | `std.text.builder`, `number`, `radix`, `compare` | present |
| Float formatting and parsing | `f64::to_string`, `parse`; `strconv.FormatFloat/ParseFloat` | `std.text.float`: `ع64_إلى_نص` (shortest round trip, with a Grisu3 fast path), `حلل_ع64` (exact), and `أرقام_ع64_الأقصر`, `أرقام_ع64_الدقيقة`, `أرقام_ع64_بخانات_كسرية`, `أرقام_ع64_بخانات_معنوية` (digits for formatters); ع32 too | present |
| Locales, bidi isolation | (icu) | `std.text.language`, `bidi` | present |
| Categories, properties, case mapping and folding, normalization, grapheme, word and sentence segmentation | `char` methods and unicode crates; `unicode`, `x/text` | `std.unicode.**` (Unicode tables generated) | present |
| UTF-8 and UTF-16 | `str`, `encode_utf16`; `unicode/utf8`, `utf16` | `std.encoding.utf8`, `utf16` | present |

## collections

| Facility | Rust / Go | SultanC | Status |
|---|---|---|---|
| Hash map and set, B-tree map and set | `HashMap/HashSet/BTreeMap/BTreeSet`; `map` | `std.collections.map`, `set`, `btree_map`, `btree_set`, `text_index_map` | present |
| Deque, binary heap, bit set | `VecDeque/BinaryHeap`; `container/heap` | `deque`, `heap`, `bitset` | present |
| Sort, binary search | `sort_by`, `binary_search_by`; `slices.Sort` | `std.collections.sort` | present |
| Vector reverse, search, min and max position, rotate | `reverse/position/iter().max_by/rotate_left`; `slices.Reverse/IndexFunc` | `std.collections.vector` | present |
| Removing from a vector: pop, truncate, retain, dedup, remove, swap_remove, insert, clear | `Vec::pop/truncate/retain/dedup_by/remove/swap_remove/insert/clear`; `slices.DeleteFunc/CompactFunc/Delete/Insert` | the `أزل_الأخير` / `pop` and `اقتطع` / `truncate` primitives; `std.collections.vector` (`احتفظ_في_المتجه` / `retain`, `أزل_المتكرر_المتتالي` / `dedup_by`, `أزل_عند` / `remove_at`, `أزل_بالتبديل` / `swap_remove`, `أدرج_عند` / `insert_at`, `امسح_المتجه` / `clear_vector`) | present |
| Vector capacity: capacity, reserve, reserve_exact, with_capacity, shrink_to_fit | `Vec::capacity/reserve/reserve_exact/with_capacity/shrink_to_fit`; `cap`, `make([]T, 0, n)`, `slices.Grow/Clip` | the `سعة` / `capacity` and `احجز_سعة` / `reserve_exact` primitives; `std.collections.vector` (`متجه_بسعة` / `with_capacity`, `احجز_في_المتجه` / `reserve`, `قلص_المتجه` / `shrink_to_fit`) | present. Truncate, pop to empty and clear keep the buffer, as Rust's `Vec` does. |
| Linked list | `LinkedList`; `container/list` | none | missing |

## math and random

| Facility | Rust / Go | SultanC | Status |
|---|---|---|---|
| Integer: checked and saturating ops, gcd, powers, bits | `checked_*`, `leading_zeros`; `math/bits` | `std.math.integer` | present |
| Wrapping, checked and saturating ops for every width | `wrapping_*`, `checked_*`, `saturating_*` on each integer type | `std.math.wrapping` | present |
| Float bit patterns | `f64::to_bits/from_bits`; `math.Float64bits` | `std.math.float` (`بتات_ع64`, `ع64_من_بتات`) | present |
| Float: sqrt, exp, ln, pow, rounding, hyperbolic, inverse trigonometry | `f64` methods; `math` | `std.math.float`. sqrt, abs, min, max, copysign and, where the target has them, floor, ceil, trunc, round_ties_even and mul_add are single instructions (compiler float primitives). | present |
| Float: copysign, round_ties_even, mul_add | `f64::copysign`, `round_ties_even`, `mul_add`; `math.Copysign`, `math.RoundToEven`, `math.FMA` | `std.math.float` (`نسخ_إشارة64`, `تقريب_زوجي64`, `ضرب_جمع_مدمج64`, and ع32) | present |
| sin, cos, tan | `f64::sin/cos/tan`; `math.Sin/Cos/Tan` | `std.math.trigonometry` (binary64 and binary32) | present |
| Random | (rand); `math/rand`, `crypto/rand` | `std.random.prng`, `std.os.entropy` | present |
| Big integers, complex numbers | (num); `math/big`, `math/cmplx` | `std.crypto.bignum` (crypto only) | partial |

## encoding and data formats

| Facility | Rust / Go | SultanC | Status |
|---|---|---|---|
| base64, base32, hex, binary, varint, PEM, DER | (base64 crate); `encoding/*` | `std.encoding.*` | present |
| JSON, TOML (read and write), CSV | (serde_json, toml); `encoding/json`, `encoding/csv` | `std.json`, `std.toml`, `std.csv` | present |
| URI, MIME, UUID, semver | (url, uuid); `net/url`, `mime` | `std.uri`, `std.mime`, `std.uuid` (v4 and v7), `std.version` | present |
| XML, CBOR, MessagePack | (quick-xml); `encoding/xml` | none | missing |

## crypto and hashing

| Facility | Rust / Go | SultanC | Status |
|---|---|---|---|
| SHA-1, SHA-256, SHA-384, SHA-512, HMAC, HKDF, PBKDF2 | (sha2, hmac); `crypto/sha256`, `crypto/hmac` | `std.crypto.*` | present |
| AES-GCM, ChaCha20-Poly1305, X25519, Ed25519, ECDSA P-256 and P-384, RSA PKCS#1 and PSS | (ring); `crypto/*` | `std.crypto.*` | present |
| Password hashing (bcrypt, scrypt, argon2) | crates; `x/crypto` | none | missing |
| Non-cryptographic hashes | `Hasher`; `hash/fnv` | `std.hash.fnv`, `std.hash.xxh64`, `std.hash.combine` | present |

## compression and archives

| Facility | Rust / Go | SultanC | Status |
|---|---|---|---|
| DEFLATE, gzip, checksums | (flate2); `compress/flate`, `gzip` | `std.compression.deflate`, `gzip`; `std.checksum` (table-driven CRC-32 with an update form `حدث_سي_ار_سي32` / `crc32_update`, Adler-32, CRC-64/XZ `احسب_سي_ار_سي64` / `crc64`) | present |
| zstd (RFC 8878) | (zstd); `klauspost/compress/zstd` | `std.compression.zstd`: a decoder for frames, skippable frames, raw, RLE and compressed blocks, Huffman and treeless literals (1 or 4 streams), FSE sequences in all four modes, repeat offsets, window limit, XXH64 checksum, and raw and zstd-format dictionaries, with output and window limits; a level-1-style compressor (hash-table LZ77, Huffman literals with FSE-compressed weights, predefined or computed FSE tables); XXH64 in `std.hash.xxh64` | present |
| xz, LZMA, LZMA2 | (xz2, lzma-rs); `ulikunitz/xz` | `std.compression.xz` (streams, blocks, index, padding, concatenation; CRC32, CRC64 and SHA-256 checks; LZMA2 with x86 BCJ and Delta filters; an encoder that writes one LZMA2 block with CRC64) and `std.compression.lzma` (raw LZMA2 and `.lzma` decoders, a greedy and lazy hash-chain LZMA encoder) | present |
| Archives tar, zip, ar | (tar, zip); `archive/tar`, `archive/zip` | `std.archive.*` | present |
| bzip2, lz4 | crates; `compress/bzip2` (read) | none | missing |
| Streaming (incremental) compressors and decompressors | `Read` and `Write` adapters; `io.Reader` | none: the codecs work on whole buffers | missing |

## http and tls

| Facility | Rust / Go | SultanC | Status |
|---|---|---|---|
| HTTP/1.1 client and server, router, cookies, multipart, chunked | (hyper); `net/http` | `std.http.**` | present |
| TLS 1.3 client and server, X.509 verification, PKCS#12, system trust roots | (rustls); `crypto/tls`, `crypto/x509` | `std.tls.**` | present |
| ALPN (RFC 7301) | `ClientConfig::alpn_protocols`; `tls.Config.NextProtos` | `std.tls.config` (`اضبط_بروتوكولات_عميل_تي_إل_إس` / `tls_client_config_set_protocols`, `اضبط_بروتوكولات_خادم_تي_إل_إس` / `tls_server_config_set_protocols`), `std.tls.stream` (`بروتوكول_تدفق_تي_إل_إس` / `tls_negotiated_protocol`) | present |
| HTTP/2 (RFC 9113, HPACK RFC 7541) | (h2); `net/http` (HTTP/2), `x/net/http2` | `std.http.http2.*`: frames; HPACK with Huffman and dynamic tables; a connection with stream states, connection and stream flow control, SETTINGS, PING, GOAWAY and RST_STREAM, and limits (frame size, header list, header block, concurrent streams, body); a client with concurrent streams over h2c (prior knowledge) or TLS with ALPN; a server serving the same `خدمة_ويب` as HTTP/1, dispatched by the client preface (`std.http.server`) or by ALPN (`شغل_خادم_ويب_آمن` / `run_https_server`). Requests and responses use the shared `std.http.model`. | present |
| HTTP/2 server push, priority scheduling, h2c upgrade from HTTP/1.1 | `x/net/http2` | none: push is disabled, priorities are ignored, and only prior knowledge is supported | missing |
| HTTP/3 | (h3, quinn) | none | missing |

## sync and threads

| Facility | Rust / Go | SultanC | Status |
|---|---|---|---|
| Atomic integers (load, store, swap, CAS, fetch_add/and/or/xor, orderings, fence) | `std::sync::atomic`; `sync/atomic` | `std.sync.atomic`, lowered to atomic instructions on x86_64, arm64, wasm32 and LLVM and run plainly by the one-threaded interpreter; English ordering constructors (`memory_ordering.seq_cst`, …) | present (multi-threaded on Linux: `sync_atomic_register_pressure`) |
| OS threads: spawn, join, join with a value, sleep, yield, current id, available parallelism | `std::thread` (`spawn`, `JoinHandle<T>::join`, `sleep`, `yield_now`, `current().id()`, `available_parallelism`); goroutines | `std.thread` (`أطلق` / `spawn`, `انضم` / `join`, `أطلق_بقيمة` / `spawn_value`, `انضم_بقيمة` / `join_value`, `نم` / `sleep`, `تنازل` / `yield_now`, `معرف_الخيط_الحالي` / `current_id`, `معرف_المقبض` / `join_handle_id`, `عدد_المعالجات_المتاحة` / `available_parallelism`) over `os.threads`. A finished thread drops its closure and what it captured; join releases the rest. Raw `clone` and futex on Linux and Android (every architecture); `thr_new` and `_umtx_op` on FreeBSD; `pthread_create` and `pthread_join` on macOS; `CreateThread` and `WaitForSingleObject` on Windows. | present. WASI and the interpreter: unsupported, with a clear error. |
| Waiting on an address | `sys::futex` (internal); `runtime.futex`, `sema` | `std.os.wait_address`: futex (Linux, Android), `_umtx_op` (FreeBSD), `__ulock_wait2` and `__ulock_wake` (macOS), `WaitOnAddress` (Windows), a single-threaded fallback (WASI) | present |
| Shared ownership | `Arc` (`strong_count`, `ptr_eq`) | `std.sync.arc` (`مشترك` / `arc`): the atomic count and the single drop of a closure environment that owns the boxed value | present |
| Mutual exclusion | `Mutex`, `RwLock`; `sync.Mutex`, `sync.RWMutex` | `std.sync.mutex` (`قفل` / `mutex`: three-state futex mutex, `with_lock`, `try_lock`, no poisoning because a panic aborts) and `std.sync.rwlock` (`قفل_قراءة_كتابة` / `rwlock`: `with_read`, `with_write`, no writer preference) | present |
| Condition variables | `Condvar`; `sync.Cond` | `std.sync.condvar` (`شرط_انتظار` / `condvar`: `wait`, `wait_timeout`, `wait_while`, `notify_one`, `notify_all`) | present |
| One-time initialization | `Once`, `OnceLock`; `sync.Once` | `std.sync.once` (`مرة` / `once`, `خلية_مرة` / `once_lock` with `get`, `get_or_init`, `set`) | present |
| Channels | `mpsc::channel`, `sync_channel`; Go channels | `std.sync.channel`: unbounded and bounded, `send`, `try_send`, `recv`, `try_recv`, `recv_timeout`, disconnection errors both ways; a bound of 0 holds one value (no rendezvous) | present |
| Data-race freedom | `Send`, `Sync` | the spawn capture check ([Type system](type-system.md#thread-capture-rules)): no raw pointer outside `غير_آمن`, no borrowed reference, no writable module variable in a closure sent to another thread | partial: captured closures and calls are not inspected |
| Scoped threads, thread-locals, barriers | `thread::scope`, `thread_local!`, `Barrier`; `sync.WaitGroup` | none | missing |

## error

| Facility | Rust / Go | SultanC | Status |
|---|---|---|---|
| Common error value, cause chains, context, propagation | `Error`, `?`, (anyhow); `errors`, `%w` | `std.error` (`فشل`, `؟`, `مع_سياق`, `إنهيار`), with converters for typed std errors including `خطأ_أمر`, `خطأ_مدة` and `خطأ_تنسيق` | present |

## Supporting modules

| Module | Provides | Rust / Go |
|---|---|---|
| `std.bytes.builder`, `cursor`, `range`, `compare` | a growable byte buffer, a reading cursor with fixed-width integer reads, checked ranges and copies, equality, ordering, prefix and suffix tests and search | `bytes::BytesMut`, `Cursor`; `bytes.Buffer`, `bytes.Compare` |
| `std.chars.ascii` | ASCII digit, hex, letter and separator classification | `u8::is_ascii_*` |
| `std.debug.assert` | `تأكد` and `تأكد_برسالة`: assertions that stay active in every build mode and stop the program like `إنهيار` | `assert!` |
| `std.fuzz.core` | a deterministic provider that maps input bytes to reproducible typed choices for fuzz targets (coverage and corpus scheduling belong to tooling) | arbitrary |
| `std.meta.reflect` | size, alignment, field count and constructor count of a type, and a compile-time failure | `mem::size_of`, `align_of`; `reflect` |
| `std.memory.system` | page size and OS memory mappings with protection and advice | memmap2; `syscall.Mmap` |
| `std.os.*` | the platform layer: allocation, files, sockets, clocks, entropy, threads, wait-address, virtual memory | `sys`, libc; `syscall`, `runtime` |

## Language features std relies on

Several std modules build on language features with their own rules.

- Numeric conversion: `مثل` converts between floats and integers with Rust's semantics
  ([Type system](type-system.md), "Numeric casts"). The bit patterns are `std.math.float`'s
  `بتات_ع64` and `ع64_من_بتات`, on which float text and float seconds are built.
- Wrapping arithmetic: non-trapping intrinsics on every backend and `std.math.wrapping`.
  SHA-512's 64-bit word helpers use them (`std.crypto.words64`).
- Process-wide state: a writable global that is not a scalar is usable in an unsafe scope, as
  Rust's `static mut`. `std.env.table` uses it.
- Vector removal and capacity: the `أزل_الأخير` / `pop`, `اقتطع` / `truncate`, `سعة` / `capacity`
  and `احجز_سعة` / `reserve_exact` primitives ([Ownership](ownership.md), "Pop and truncate" and
  "Capacity"), on which `std.collections.vector` builds retain, dedup, remove, swap_remove, insert,
  clear, with_capacity, reserve and shrink_to_fit. An emptied vector keeps its buffer.

## Limitations

- Closures capture plain values only, so a custom `std.fmt` formatter cannot borrow a value that
  owns text or vectors. Such a value is formatted into a formatter first and passed as text.
- The modules that the compiler itself imports (`std.math.integer`, `std.time.duration`,
  `std.env.system`, `std.crypto.words`, …) are built by the C Stage0 bootstrap compiler. Their
  companions (`std.time.duration_float`, `std.crypto.words64`, `std.env.table`,
  `std.collections.vector`) are separate modules for that reason.
- The gaps in the status tables above (generic stream adapters, multiplexed sockets, XML, bzip2,
  lz4, streaming codecs, HTTP/3, scoped threads and the rest) are not implemented.

## See also

- [Concurrency](concurrency.md): the `std.sync` and `std.thread` design.
- [Formatting](formatting.md): `std.fmt`.
- [Errors](errors.md): `std.error`.
- [Targets](targets.md) and [Platform dependencies](platform-dependencies.md): per-platform support.
