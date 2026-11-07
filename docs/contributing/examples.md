# Examples

The `examples/` tree holds small, complete SultanC programs, one concept per file, that document how
the language and the standard library are meant to be used. This page explains how the tree is organized,
how to build and run an example on each supported path, what each category shows, and which examples
have limits. It is written for users learning the language and for contributors adding examples.

## Layout

The examples are mirrored by source language:

- `examples/en/` uses English SultanC syntax.
- `examples/ar/` uses Arabic SultanC syntax.

Both trees use the same categories and file names, so each example has a direct counterpart. A pair is
the same program token for token. The Arabic one uses the canonical Arabic keywords, standard-library
names and identifiers, and prints its messages in Arabic. Multi-file examples (`modules/*`) are
directories holding a `main.sn` and its helper modules. `examples/run-wasi.mjs` runs a WebAssembly
build under Node's WASI support.

## Running an example

Build and run one from the repository root with a built compiler (`build/sultanc`).

```sh
./build/sultanc examples/en/basics/hello.sn -o build/example
./build/example
```

Run it with the interpreter instead of building an executable:

```sh
./build/sultanc run examples/ar/basics/arithmetic.sn
```

Build for WebAssembly and run it with Node:

```sh
./build/sultanc examples/en/basics/hello.sn --target=wasm32-wasi -o build/hello.wasm
node --no-warnings examples/run-wasi.mjs build/hello.wasm
```

Build for another platform. No toolchain is needed because SultanC links internally:

```sh
./build/sultanc examples/en/basics/hello.sn --target=arm64-darwin -o build/hello-arm64
./build/sultanc examples/en/basics/hello.sn --target=x86_64-linux -o build/hello-x86_64
```

## Contents

| Category | Examples | Shows |
|---|---|---|
| `basics` | `arithmetic`, `hello`, `constants`, `integer_types` | printing, module-level constants, fixed-width integers, hex literals, `as` casts, hex and binary output with std.fmt |
| `control_flow` | `conditions`, `loops`, `for_loop`, `match_patterns` | `while`, C-style `for` with `break` and `continue`, `match` on integers, ranges, characters, strings |
| `functions` | `recursion`, `indirect_call`, `closures`, `higher_order` | function values, closures that capture and are returned, transform, filter and fold |
| `numbers` | `floating_point` | `f64` arithmetic |
| `math` | `float_math` | square roots, `hypot`, mean and standard deviation, tolerant comparison |
| `data` | `records`, `enums`, `shapes`, `tuples`, `type_aliases` | records, enums, ADTs carrying records, tuples, `type` and `alias` |
| `generics` | `identity`, `maybe`, `stack` | generic ADTs, generic records and functions, several instantiations |
| `comptime` | `compile_time_values`, `specialization`, `decorators`, `reflection`, `compile_error` | `comptime` expressions and blocks, `comptime` parameters, decorators, type reflection |
| `memory` | `box_and_pointers` | `box`, raw pointers inside `unsafe` |
| `ownership` | `references`, `moves_and_borrows` | moves, shared and `&mutable` borrows, `clone` |
| `collections` | `vector`, `slices`, `word_count`, `sorting`, `priority_queue`, `set`, `deque`, `ordered_map`, `bit_set` | slices, hash map and set, sort and binary search, heap, deque, B-tree map, bit set |
| `strings` | `text`, `builder`, `search_and_replace`, `unicode` | text builder, split, join, trim, replace and find, UTF-8 and Arabic text |
| `fmt` | `formatting`, `table`, `display` | std.fmt format strings (alignment, numbers, floats, Debug), an aligned table of Arabic names, Display and Debug for your own type |
| `chars` | `ascii` | classifying and converting ASCII bytes |
| `errors` | `results`, `assertions`, `error_values`, `propagation`, `languages` | `result` values with typed error cases, `assert`, `std.error` values with identity, causes and notes, the `?` operator and context helpers, optional message languages ([Errors](../reference/errors.md)) |
| `formats` | `json`, `csv`, `toml` | parsing JSON, CSV and TOML documents |
| `encoding` | `base64` | Base64 encode and decode |
| `checksum` | `checksums` | CRC-32 and Adler-32 |
| `compression` | `gzip` | gzip compress and decompress |
| `crypto` | `sha256`, `hashes`, `signatures`, `encryption` | SHA-256, SHA-512, HMAC-SHA-256, Ed25519, ChaCha20-Poly1305 |
| `random` | `seeded_random` | a seeded pseudo-random generator |
| `time` | `calendar`, `iso_dates` | leap years, month lengths, day counts, ISO 8601 dates |
| `ids` | `uuid` | parsing and printing UUIDs |
| `web` | `uri`, `mime_types`, `versions`, `http_response` | URIs and queries, media types, semantic versions, HTTP/1.1 responses |
| `net` | `ip_addresses` | parsing and printing IPv4 addresses |
| `io` | `read_input`, `logging`, `buffered_output` | reading standard input, a leveled logger with formatted entries, buffered writes |
| `env` | `environment` | environment variables, working directory |
| `fs` | `files`, `directories` | files; creating, listing, walking and removing directories |
| `process` | `arguments`, `exit_code`, `child_process` | arguments, `exit`, running another program |
| `ffi` | `extern_call` | calling a C function with `extern` and `unsafe` |
| `algorithms` | `binary_search`, `matrix_multiply`, `sieve`, `gcd`, `insertion_sort` | classic algorithms, checked against the standard library where it has one |
| `programs` | `stack_machine` | a small interpreter: an instruction ADT executed by `match` |
| `modules` | `basic`, `public_api`, `selective_import`, `import_alias` | multi-file programs, `public` declarations, `from ... import`, `import ... as` |

## Status

Every example is written the way the language and its documentation intend, and all of them build and
run on x86_64-linux. One needs an option.

| Example | Notes |
|---|---|
| `ffi/extern_call` | Links the C library, which SultanC never links implicitly: `sultanc examples/en/ffi/extern_call.sn -l c`. Without the option the link stops with `SULTANC-LINK-UNDEFINED-SYMBOL` for `labs`. `sultanc run` finds `labs` at run time where the compiler has a dynamic loader ([Interpreter](../internals/interpreter.md), "Dynamic C functions"). |

Some examples report their result through the exit code, for example 42 on success. `process/exit_code`
exits with status 3 on purpose.

### WebAssembly

On `wasm32-wasi`, every example that builds natively (`ffi/extern_call` excepted) also builds and prints
the same output, with two exceptions that come from the target:

- `process/child_process` reports that it cannot start a program, because WASI has no process creation.
- `errors/error_values` prints a different numeric status code for a missing file (the WASI error number
  instead of the Linux one).

### Language limits the examples work around

- English enum-variant aliases are accepted in expressions but not in patterns, and library records
  have Arabic field names only. English examples therefore spell library variants and fields with their
  canonical names (`قيمة_جسون.كائن`, `حالة_إنهاء.خرج`, `parsed.الاستجابة.الحالة`).
- A tuple cannot be a function's return type, so `tuples` passes tuples as parameters and locals only.

## Adding an example

Add the English and Arabic files together under the same category and name, keep them token-for-token
equivalent, and make the example print something observable. Check both with the compiler
(`sultanc check file.sn`) and run them. Examples are also part of the interpreter-versus-native sweep
(`tools/tests/interp_sweep.sh`), so an example must give the same output interpreted and native.
Behavior that needs to be guarded belongs in `tests/` ([Tests](tests.md)), not only in an example.

## See also

- [Tests](tests.md)
- [Contributor guide](README.md)
- [Targets](../reference/targets.md)
