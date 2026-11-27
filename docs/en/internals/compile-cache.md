[العربية](../../ar/internals/compile-cache.md) | **English**

# The compilation cache

`sultanc` keeps the outputs of its compilations in a content-addressed cache, so the same compilation is
not done twice: a second build of an unchanged program, a test suite run again after a change elsewhere,
the same object built by two scripts. A hit writes exactly the bytes the stored compilation wrote. This
page says what is cached, what the key is, when nothing is stored, and what is not possible yet. The code
is `compiler/driver/compile_cache.sn` and the request key `مفتاح_خزن_طلب` in
`compiler/driver/process_driver.sn`.

## What is cached

One compilation: a build of an executable, an object (`-c`), a shared library, a relocatable object or a
target module written to a file. Assembly, LLVM IR and MSIR text (`--emit=asm`, `llvm-ir`, `msir`),
`run`, `check` and `test` are not cached, and neither is the executable `run` builds for a program with C
imports.

The compiler is a whole-program pipeline: every module of a program is loaded, analysed and lowered into
one MSIR program, which is optimized and generated as one unit. There is no per-module object or
interface to reuse, so the unit of the cache is the compilation, not the module (see "Limits").

## The key

A lookup has two parts.

**The request key** is the SHA-256 of:

- the compiler's identity: the content hash of the running `sultanc` executable and, when it contains the
  LLVM backend, of the libLLVM it runs with;
- the request, normalized: the source as given and the further sources, the target triple, the backend
  choice, the `-O` level, the output kind and path, the `--module-source` bindings, the `-l` names and `-L`
  directories, every link input by content (the direct ones and the file each `-l` resolves to), the
  package graph (`--package-graph`) by path and content, the language of the program's runtime failures
  and the compile-time budget;
- `SULTANC_STD`, `SULTANC_LINKER` and the working directory.

**The manifest** of a stored compilation lists what it read, from its own source map: every module source
with the content hash of the exact text compiled, the module listing (subdirectories, `.sn` files,
`Supax.toml`) of every directory those sources come from and of up to eight of its ancestors, each such
directory's folder root file (`<dir>/<dir>.sn`, module spellings) and `Supax.toml` by content or absence,
and the output's content hash and mode. One request key can have several manifests (the same command over
different source versions). A content hash is 128 bits, two XXH64 lanes with different seeds: every
compilation hashes every module it loads, so it is the fast hash; what it guards against is a stale
entry, not an adversary, since the cache is the user's own. Request keys and manifest names are SHA-256.

A lookup takes the manifests of the request key and accepts the first whose every line still holds,
hashing the files again. Its output is then written to the output path through a temporary file and a
rename, with the mode it had. Module lookup is deterministic and never searches directories
([Modules and imports](../reference/modules-and-imports.md)), so these inputs are everything a
compilation reads to resolve and compile its modules.

## When nothing is stored

A compilation is stored only when it succeeded, wrote its output file, and nothing outside the key and
the manifest could have decided it:

- it showed warnings (a hit would show none);
- a source is a C import (`.h`, `.c`): the C bridge reads headers the source map does not hold;
- a source uses `ضمن_ملف` / `embed_file` (the embedded file is read by the parser, outside the source map;
  any source containing the word is excluded);
- the output was linked by an external linker;
- a source's text is not its file's (an editor's overlay); the one generated source accepted is std's
  `process/failure_text.sn`, whose text is a function of the language in the request key;
- the package graph comes from standard input (`--package-graph=-`), or the request cannot be keyed (an
  unregistered target, a library `-l` does not find, a link input that cannot be read).

## The directory

`SULTANC_CACHE` names the cache directory, and `SULTANC_CACHE=off` turns the cache off. Without it the
cache is `sultanc/` in the user cache directory: `$XDG_CACHE_HOME`, else `~/Library/Caches` on macOS,
`%LOCALAPPDATA%` on Windows and `~/.cache` elsewhere. `--no-cache` turns it off for one command.
`SULTANC_CACHE_REPORT=1` writes what the cache did (hit, stored, not stored) with the request key to
standard error.

```text
objects/<2>/<hash>             output bytes, named by their content hash
manifests/<2>/<key>/<sha256>   manifests, named by the hash of their text
identities/<2>/<sha256>        the content hash of a large file (the compiler, a libLLVM, a link input),
                               by path, size and modification time
tmp/                           files being written
last-trim                      the time of the last size check
```

Every file is written in `tmp/` and renamed into place, so concurrent compilers never read half an entry;
two compilers that store the same compilation write the same names with the same bytes. Outputs are
content-addressed, so a manifest names its output by hash and a read checks the hash again.

The cache is bounded by `SULTANC_CACHE_LIMIT` MiB (default 2048). At most once an hour a store sums
`objects/`, `manifests/` and `identities/` and, above the limit, removes the oldest files (by modification
time) until the cache holds under 80% of it. A file another compiler removes meanwhile is a miss, never a
wrong output.

The identities of large files are kept by path, size and modification time: the compiler executable and
libLLVM are hashed once per version, not at every compilation. Sources are always hashed by content.

## Where caching is off

The checks that must compile for real run without the cache: the build driver sets `SULTANC_CACHE=off`
for `verify`, `seed-check`, `seed-update` and `chain` (every compiler they run, old or new, inherits it),
`bootstrap/chain.sh` sets it too, and the test runner passes `--no-cache` to the fixpoint, the RSS
measurements and the determinism check of objects. The runner's other builds use the cache, and its suite
`اختبر_خزن_التجميع` checks a store, a hit with the same bytes, `--no-cache` and the miss after a source
changes.

## Limits

- **No per-module entries.** The request key and the manifest cover the whole program, so a change to
  any module, a function body included, misses for every program that loads it. Splitting a module's
  interface hash from its implementation hash, so that a body change keeps its importers' entries, needs
  modules compiled separately: a module's analysed interface (its public declarations and types) written
  to a file and read back by its importers, and code generation per module. The Frontend keeps one type
  registry and one symbol space for the whole program, and MSIR is lowered and optimized as one program,
  so neither exists yet.
- **No cached frontend results.** Parsed, name-resolved or type-checked modules are not cached: their
  identities (sources, symbols, types) are positions in whole-program tables, and nothing serializes them.
- **No parallel module compilation.** The pipeline loads, analyses and lowers modules one after another
  into shared tables; `sultanc` has no `-j`. The build driver's `--jobs` runs the C compilations of Stage1
  and of the C Stage0 in parallel.
- **Order.** Eviction removes the oldest stored files, not the least recently used: a hit does not
  refresh an entry.

## See also

- [The `sultanc` command](../reference/command-line.md)
- [Bootstrap](bootstrap.md)
- [Compiler Architecture](compiler-architecture.md)
