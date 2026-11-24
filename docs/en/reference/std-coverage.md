[العربية](../../ar/reference/std-coverage.md) | **English**

# Standard Library Coverage

This page lists what SultanC's standard library (`library/std/**`) provides, module by module, and
how fully each area is covered. It is a reference for programmers deciding whether std covers a
need and for contributors looking for gaps.

A row's status has one of three values. **Present** means the common uses of the facility are
covered by std. **Partial** means some are covered and the gaps are named.
**Missing** means std has nothing. Names are given in their English spelling; the Arabic name of
each is listed in [Arabic terminology](arabic-terminology.md).

Platform notes name what a platform lacks. The supported platforms are Linux and Android (raw
kernel calls; 64-bit, i686 and armv7), FreeBSD (the same direct-kernel provider), macOS
(libSystem), Windows (kernel32, ws2_32) and WASI preview 1. See [Targets](targets.md) and
[Platform dependencies](platform-dependencies.md).

## Module index

| Module path | Provides |
|---|---|
| `std.process.*` | spawn, wait, exit status, command builder, streaming children, arguments, `exit`, `panic`, command-line parsing |
| `std.env.*` | environment variables, working directory, home and temp directories |
| `std.fs.*` | files, directories, metadata, permissions, links, temporary files, locks, paths |
| `std.io.*` | print and read functions, readers, writers, buffering, logging |
| `std.fmt.fmt` | format strings and the format-spec mini-language |
| `std.net.*` | TCP, UDP, Unix sockets, IP and socket addresses, DNS |
| `std.time.*` | clocks, durations, calendar dates, time zones |
| `std.text.*` | search, transform, builders, number and float text, language tags, bidi |
| `std.unicode.*` | categories, properties, case mapping, normalization, segmentation |
| `std.collections.*` | maps, sets, B-trees, deque, heap, bit set, sorting, vector utilities |
| `std.math.*` | integer and float math, wrapping arithmetic, trigonometry |
| `std.random.prng`, `std.os.entropy` | a seeded generator and OS entropy |
| `std.encoding.*`, `std.json`, `std.toml`, `std.csv`, `std.uri`, `std.mime`, `std.uuid`, `std.version` | data formats and identifiers |
| `std.crypto.*`, `std.hash.*`, `std.checksum` | hashes, MACs, KDFs, AEADs, signatures, big numbers, non-cryptographic hashes, CRCs |
| `std.compression.*`, `std.archive.*` | deflate, gzip, zstd, xz, LZMA; tar, zip, ar |
| `std.http.*`, `std.tls.*` | HTTP/1.1 and HTTP/2 client and server, TLS 1.3, X.509 |
| `std.sync.*`, `std.thread`, `std.os.wait_address` | threads, atomics, locks, channels |
| `std.error.*` | the common error value and converters |
| `std.bytes.*`, `std.chars.ascii` | byte buffers, cursors, comparison; ASCII classification |
| `std.debug.assert`, `std.fuzz.core`, `std.meta.reflect`, `std.memory.system` | assertions, fuzz input, type reflection, OS memory mappings |
| `std.os.*` | the platform layer under the modules above |

## Summary

| Area | Status | Main gaps |
|---|---|---|
| process | present | process groups, signals other than kill, hooks that run in the child before exec |
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
| encoding | present | XML, MessagePack, CBOR |
| crypto | present | a TLS-oriented set: no bcrypt, scrypt or argon2 |
| compression | partial | bzip2, lz4; streaming (incremental) codecs |
| http and tls | present | HTTP/3; HTTP/2 server push and priorities |
| sync and threads | present | scoped threads, thread-locals, barriers; weaker atomic orderings; see [Concurrency](concurrency.md) |
| error | present | |

## process

| Facility | SultanC | Status |
|---|---|---|
| Spawn, wait, exit status | `std.process.child` (`spawn`, `wait`, `exit_status`) | present |
| try_wait, kill, wait with timeout | `std.process.child` (`try_wait`, `kill_child`, `wait_timeout`) on Unix; `std.process.command` children (`child_try_wait`, `child_kill`) on every OS with processes, Windows included | present |
| Builder: args, env set/remove/clear, current dir | `std.process.command` | present (WASI: no processes) |
| Child stdio inherit/null/piped, output capture, input bytes | `std.process.command` (`command_output_of`, `command_status`, `stdio`) | present |
| Streaming pipes to a live child | `std.process.command` (`command_spawn`, `command_child`, `child_stdin` and `child_stdout` in the `std.io` reader and writer shapes, `child_write_stdin`, `child_read_stdout`, `child_close_stdin`, `child_close_streams`, `child_wait`, `child_try_wait`, `child_kill`, `child_wait_with_output`) | present. Each stream is inherit, null or piped. With no destructors, closing the input and the leftover pipes is explicit, and `wait` closes the input first. A write to a child that closed its input is EPIPE, not SIGPIPE. WASI: unsupported. |
| Arguments, exit, panic | `std.process.arguments`, `exit`, `panic` | present |
| Command-line parsing | `std.process.cli` | present |

## env

| Facility | SultanC | Status |
|---|---|---|
| Read one variable | `std.env.environment` (`get_environment_variable`) | present |
| All variables | `std.env.variables` (`environment_variables`) | present |
| Set and unset own variables | `std.env.variables` (`set_environment_variable`, `remove_environment_variable`) over `std.env.table`, inherited by `std.process.command` | present. One process-wide lock once a thread exists. |
| Current directory get and set | `current_directory`, `set_current_directory` | present (WASI: no chdir) |
| Home and temp dirs | `home_directory`, `temporary_directory` | present |

## fs and path

| Facility | SultanC | Status |
|---|---|---|
| Read and write whole files, copy | `std.fs.read_file`, `write_file`, `write_bytes` | present |
| Handles: open, read, write, seek, sync, close | `std.fs.handle` | present |
| Truncate, set length | `std.fs.attributes` (`truncate_file`, `file_handle_set_len`) | present |
| Metadata: type, size, mtime, exists | `std.fs.metadata`, `directory` | present |
| Set file times | `set_file_times` | present |
| Permissions read and chmod | `std.fs.permissions` | present (Windows: read-only bit; WASI: none) |
| Symlink create and read, hard link | `std.fs.links` | present |
| Canonicalize, absolute | `std.fs.canonical` | present |
| create_dir_all, remove_dir_all, copy tree, walk | `std.fs.walk` | present |
| Temporary files and directories | `std.fs.temporary` | present |
| Rename, remove, mkdir, list directory | `std.fs.mutation`, `directory` | present |
| Whole-file locks | `std.fs.lock` (`file_lock`, `file_lock_shared`, `file_try_lock`, `file_try_lock_shared`, `file_unlock`; `try_lock_error.would_block` reports a lock held elsewhere) | present (flock on Linux, Android, FreeBSD and macOS; `LockFileEx` on Windows; WASI: unsupported) |
| chown, watching | none | missing |
| Join, parent, file name, extension, components, normalize | `std.fs.path` (POSIX and Windows flavors) | present |

## io

| Facility | SultanC | Status |
|---|---|---|
| print, eprint, stdin lines | `std.io.print`, `println`, `read_line` | present |
| Buffered reader and writer | `std.io.read_line` (`buffered_reader`), `buffered_writer` | present |
| Reader and Writer, copy | `std.io.reader`, `writer` | present |
| Adapters (take, chain, multi-writer), vectored I/O | none | missing |
| Logging | `std.io.log` | present |

## fmt

| Facility | SultanC | Status |
|---|---|---|
| Format strings, `Display` and `Debug`, the format-spec mini-language | `std.fmt.fmt` (`format`, `format_into`, `write_fmt`, `println_fmt` and the other print forms, `fmt_arg` with a custom closure case, `formatter`); `std.io.log`'s `log_fmt` | partial. A literal format string is checked at compile time against a vector-literal argument list (count, positions, types, counts); a format string built at run time is checked at run time. No named arguments. Custom closures capture plain values only. See [Formatting](formatting.md). |

## net

| Facility | SultanC | Status |
|---|---|---|
| TCP connect, listen, accept, read, write, shutdown, options | `std.net.tcp`, `options` | present (WASI: preopened sockets only) |
| UDP bind, connect, send and receive | `std.net.udp` | present |
| UDP unconnected send_to and recv_from | `std.net.socket_names` | present |
| Local and peer addresses | `std.net.socket_names` | present |
| IP and socket addresses, DNS | `std.net.ip`, `socket_address`, `dns` | present |
| Timeouts | `std.net.options` (`receive_timeout_ms` and `send_timeout_ms` socket options) | present |
| Unix domain sockets: stream listener and streams, datagrams, pathname and abstract addresses | `std.net.unix` (`unix_listener`, `unix_stream`, `unix_datagram`, `unix_socket_addr`) over `std.os.unix_socket` | present on Linux, Android, FreeBSD and macOS; abstract names on Linux and Android; Windows 10 1803+: streams only; WASI: unsupported. No socketpair, no descriptor passing, no peer credentials. |
| Polling many sockets, multicast | none | missing |

## time

| Facility | SultanC | Status |
|---|---|---|
| Monotonic and system clock, sleep | `std.time.time` | present |
| Durations: arithmetic, compare | `std.time.duration`, with checked abs and divide | present |
| Duration text | `std.time.duration_text` | present |
| Calendar, RFC 3339 and ISO 8601, HTTP dates | `std.time.calendar`, `date` | present |
| Time zones (TZif) | `std.time.timezone` | present |
| Float seconds | `std.time.duration_float` (`duration_seconds_f64`, `duration_from_seconds_f64`, and the `f32` forms) | present |

## text and unicode

| Facility | SultanC | Status |
|---|---|---|
| find, rfind, contains, starts and ends with | `std.text.search` | present |
| trim, split, join, replace, lines, repeat | `std.text.transform` | present |
| split_whitespace, splitn, replacen, strip_prefix and suffix, ASCII case | `std.text.transform` | present |
| count, char count, eq_ignore_ascii_case | `std.text.search` | present |
| Builder, numbers, radix, compare | `std.text.builder`, `number`, `radix`, `compare` | present |
| Float formatting and parsing | `std.text.float`: `f64_to_string` (shortest round trip, with a Grisu3 fast path), `parse_f64` (exact), and `f64_shortest_digits`, `f64_exact_digits`, `f64_fixed_digits`, `f64_significant_digits` (digits for formatters); the `f32` forms too | present |
| Locales, bidi isolation | `std.text.language`, `bidi` | present |
| Categories, properties, case mapping and folding, normalization, grapheme, word and sentence segmentation | `std.unicode.**` (Unicode tables generated) | present |
| UTF-8 and UTF-16 | `std.encoding.utf8`, `utf16` | present |

## collections

| Facility | SultanC | Status |
|---|---|---|
| Hash map and set, B-tree map and set | `std.collections.map`, `set`, `btree_map`, `btree_set`, `text_index_map` | present |
| Deque, binary heap, bit set | `deque`, `heap`, `bitset` | present |
| Sort, binary search | `std.collections.sort` | present |
| Vector reverse, search, min and max position, rotate | `std.collections.vector` | present |
| Removing from a vector: pop, truncate, retain, dedup, remove, swap_remove, insert, clear | the `pop` and `truncate` primitives; `std.collections.vector` (`retain`, `dedup_by`, `remove_at`, `swap_remove`, `insert_at`, `clear_vector`) | present |
| Vector capacity: capacity, reserve, reserve_exact, with_capacity, shrink_to_fit | the `capacity` and `reserve_exact` primitives; `std.collections.vector` (`with_capacity`, `reserve`, `shrink_to_fit`) | present. Truncate, pop to empty and clear keep the buffer. |
| Linked list | none | missing |

## math and random

| Facility | SultanC | Status |
|---|---|---|
| Integer: checked and saturating ops, gcd, powers, bits | `std.math.integer` | present |
| Wrapping, checked and saturating ops for every width | `std.math.wrapping` | present |
| Float bit patterns | `std.math.float` (`f64_to_bits`, `f64_from_bits`) | present |
| Float: sqrt, exp, ln, pow, rounding, hyperbolic, inverse trigonometry | `std.math.float`. sqrt, abs, min, max, copysign and, where the target has them, floor, ceil, trunc, round_ties_even and mul_add are single instructions (compiler float primitives). | present |
| Float: copysign, round_ties_even, mul_add | `std.math.float` (`copysign_f64`, `round_ties_even_f64`, `mul_add_f64`, and the `f32` forms) | present |
| sin, cos, tan | `std.math.trigonometry` (binary64 and binary32) | present |
| Random | `std.random.prng`, `std.os.entropy` | present |
| Big integers, complex numbers | `std.crypto.bignum` (crypto only) | partial |

## encoding and data formats

| Facility | SultanC | Status |
|---|---|---|
| base64, base32, hex, binary, varint, PEM, DER | `std.encoding.*` | present |
| JSON, TOML (read and write), CSV | `std.json`, `std.toml`, `std.csv` | present |
| URI, MIME, UUID, semver | `std.uri`, `std.mime`, `std.uuid` (v4 and v7), `std.version` | present |
| XML, CBOR, MessagePack | none | missing |

## crypto and hashing

| Facility | SultanC | Status |
|---|---|---|
| SHA-1, SHA-256, SHA-384, SHA-512, HMAC, HKDF, PBKDF2 | `std.crypto.*` | present |
| AES-GCM, ChaCha20-Poly1305, X25519, Ed25519, ECDSA P-256 and P-384, RSA PKCS#1 and PSS | `std.crypto.*` | present |
| Password hashing (bcrypt, scrypt, argon2) | none | missing |
| Non-cryptographic hashes | `std.hash.fnv`, `std.hash.xxh64`, `std.hash.combine` | present |

## compression and archives

| Facility | SultanC | Status |
|---|---|---|
| DEFLATE, gzip, checksums | `std.compression.deflate`, `gzip`; `std.checksum` (table-driven CRC-32 with an update form `crc32_update`, Adler-32, CRC-64/XZ `crc64`) | present |
| zstd (RFC 8878) | `std.compression.zstd`: a decoder for frames, skippable frames, raw, RLE and compressed blocks, Huffman and treeless literals (1 or 4 streams), FSE sequences in all four modes, repeat offsets, window limit, XXH64 checksum, and raw and zstd-format dictionaries, with output and window limits; a level-1-style compressor (hash-table LZ77, Huffman literals with FSE-compressed weights, predefined or computed FSE tables); XXH64 in `std.hash.xxh64` | present |
| xz, LZMA, LZMA2 | `std.compression.xz` (streams, blocks, index, padding, concatenation; CRC32, CRC64 and SHA-256 checks; LZMA2 with x86 BCJ and Delta filters; an encoder that writes one LZMA2 block with CRC64) and `std.compression.lzma` (raw LZMA2 and `.lzma` decoders, a greedy and lazy hash-chain LZMA encoder) | present |
| Archives tar, zip, ar | `std.archive.*` | present |
| bzip2, lz4 | none | missing |
| Streaming (incremental) compressors and decompressors | none: the codecs work on whole buffers | missing |

## http and tls

| Facility | SultanC | Status |
|---|---|---|
| HTTP/1.1 client and server, router, cookies, multipart, chunked | `std.http.**` | present |
| TLS 1.3 client and server, X.509 verification, PKCS#12, system trust roots | `std.tls.**` | present |
| ALPN (RFC 7301) | `std.tls.config` (`tls_client_config_set_protocols`, `tls_server_config_set_protocols`), `std.tls.stream` (`tls_negotiated_protocol`) | present |
| HTTP/2 (RFC 9113, HPACK RFC 7541) | `std.http.http2.*`: frames; HPACK with Huffman and dynamic tables; a connection with stream states, connection and stream flow control, SETTINGS, PING, GOAWAY and RST_STREAM, and limits (frame size, header list, header block, concurrent streams, body); a client with concurrent streams over h2c (prior knowledge) or TLS with ALPN; a server serving the same `http_service` as HTTP/1, dispatched by the client preface (`std.http.server`) or by ALPN (`run_https_server`). Requests and responses use the shared `std.http.model`. | present |
| HTTP/2 server push, priority scheduling, h2c upgrade from HTTP/1.1 | none: push is disabled, priorities are ignored, and only prior knowledge is supported | missing |
| HTTP/3 | none | missing |

## sync and threads

| Facility | SultanC | Status |
|---|---|---|
| Atomic integers (load, store, swap, CAS, fetch_add/and/or/xor, orderings, fence) | `std.sync.atomic`, lowered to atomic instructions on x86_64, arm64, wasm32 and LLVM and run plainly by the one-threaded interpreter; English ordering constructors (`memory_ordering.seq_cst`, …) | present (multi-threaded on Linux: `sync_atomic_register_pressure`) |
| OS threads: spawn, join, join with a value, sleep, yield, current id, available parallelism | `std.thread` (`spawn`, `join`, `spawn_value`, `join_value`, `sleep`, `yield_now`, `current_id`, `join_handle_id`, `available_parallelism`) over `os.threads`. A finished thread drops its closure and what it captured; join releases the rest. Raw `clone` and futex on Linux and Android (every architecture); `thr_new` and `_umtx_op` on FreeBSD; `pthread_create` and `pthread_join` on macOS; `CreateThread` and `WaitForSingleObject` on Windows. | present. WASI and the interpreter: unsupported, with a clear error. |
| Waiting on an address | `std.os.wait_address`: futex (Linux, Android), `_umtx_op` (FreeBSD), `__ulock_wait2` and `__ulock_wake` (macOS), `WaitOnAddress` (Windows), a single-threaded fallback (WASI) | present |
| Shared ownership | `std.sync.arc` (`arc`): the atomic count and the single drop of a closure environment that owns the boxed value | present |
| Mutual exclusion | `std.sync.mutex` (`mutex`: three-state futex mutex, `with_lock`, `try_lock`, no poisoning because a panic aborts) and `std.sync.rwlock` (`rwlock`: `with_read`, `with_write`, no writer preference) | present |
| Condition variables | `std.sync.condvar` (`condvar`: `wait`, `wait_timeout`, `wait_while`, `notify_one`, `notify_all`) | present |
| One-time initialization | `std.sync.once` (`once`, `once_lock` with `get`, `get_or_init`, `set`) | present |
| Channels | `std.sync.channel`: unbounded and bounded, `send`, `try_send`, `recv`, `try_recv`, `recv_timeout`, disconnection errors both ways; a bound of 0 holds one value (no rendezvous) | present |
| Data-race freedom | the spawn capture check ([Type system](type-system.md#thread-capture-rules)): no raw pointer outside `unsafe`, no borrowed reference, no writable module variable in a closure sent to another thread | partial: captured closures and calls are not inspected |
| Scoped threads, thread-locals, barriers | none | missing |

## error

| Facility | SultanC | Status |
|---|---|---|
| Common error value, cause chains, context, propagation | `std.error` (`Error`, `?`, `with_context`, `panic`), with converters for typed std errors including `command_error`, `duration_error` and `fmt_error` | present |

## Supporting modules

| Module | Provides |
|---|---|
| `std.bytes.builder`, `cursor`, `range`, `compare` | a growable byte buffer, a reading cursor with fixed-width integer reads, checked ranges and copies, equality, ordering, prefix and suffix tests and search |
| `std.chars.ascii` | ASCII digit, hex, letter and separator classification |
| `std.debug.assert` | `assert` and `assert_with_message`: assertions that stay active in every build mode and stop the program like `panic` |
| `std.fuzz.core` | a deterministic provider that maps input bytes to reproducible typed choices for fuzz targets (coverage and corpus scheduling belong to tooling) |
| `std.meta.reflect` | size, alignment, field count and constructor count of a type, and a compile-time failure |
| `std.memory.system` | page size and OS memory mappings with protection and advice |
| `std.os.*` | the platform layer: allocation, files, sockets, clocks, entropy, threads, wait-address, virtual memory |

## Language features std relies on

Several std modules build on language features with their own rules.

- Numeric conversion: `as` converts between floats and integers, saturating out-of-range values
  ([Type system](type-system.md), "Numeric casts"). The bit patterns are `std.math.float`'s
  `f64_to_bits` and `f64_from_bits`, on which float text and float seconds are built.
- Wrapping arithmetic: non-trapping intrinsics on every backend and `std.math.wrapping`.
  SHA-512's 64-bit word helpers use them (`std.crypto.words64`).
- Process-wide state: a writable global that is not a scalar is usable in an unsafe scope.
  `std.env.table` uses it.
- Vector removal and capacity: the `pop`, `truncate`, `capacity`
  and `reserve_exact` primitives ([Ownership](ownership.md), "Pop and truncate" and
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
