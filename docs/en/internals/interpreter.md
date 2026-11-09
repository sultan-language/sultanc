[العربية](../../ar/internals/interpreter.md) | **English**

# The SultanC Interpreter

`sultanc run file.sn [-- args]` (also `interpret`) runs a program without producing an executable.
The interpreter executes the same verified MSIR that the Native and LLVM backends compile,
including the same std sources: there is no interpreter-only std and no forked std path. Bounded
compile-time evaluation (CTFE) is the same loop with an instruction budget, a frame budget and no
host effects (`compiler/interpreter/interpreter.sn`). This page explains the interpreter's design,
its memory model and its bridges to the host, and is written for compiler contributors.

Where a program meets the host (system calls, C functions, startup data, raw machine memory), the
interpreter bridges it itself, as mature interpreters do. The sources are in
`compiler/interpreter/`.

| File | Role |
|---|---|
| `interpreter.sn` | the dispatch loop, CTFE boundary |
| `execution/execution_state.sn` | state, frame stack, startup data |
| `execution/calls.sn`, `control_flow.sn` | calls, returns, edges, switches |
| `operations/operations.sn` | one handler per MSIR operation |
| `operations/fast_paths.sn` | specialized handlers for the most executed instructions |
| `execution/primitives.sn` | compiler primitives, kernel and foreign-call bridges |
| `operations/float_software.sn` | the float primitives (sqrt, abs, floor, ceil, trunc, round-even, minimum, maximum, copysign, fma), computed on the raw bits and bit-identical to the hardware instructions ([Type System](../reference/type-system.md), "Float operations") |
| `memory/memory.sn` | the interpreter heap, places, byte views |
| `execution/host_calls.sn`, `host_kernel_generated.sn`, `host_externs_generated.sn`, `host_environment.sn` | the host side of the bridges |
| `execution/foreign_calls.sn`, `host_memory.sn`, `host_dynamic.sn` | the foreign-call bridge: marshalling, host memory, a program's own C functions found at run time |

## Design

### Dispatch

The loop borrows the verified program for the whole run, and each step borrows the current
instruction in place, with no copy of the instruction or of the block, as wasm3 and Wasmtime's
Pulley dispatch directly on their code. Frames live in a heap-allocated stack
(`حالة_تنفيذ_مفسر.المكدس`), so interpreted recursion never uses the host stack. A popped frame's slot
table is reused by the next call. Each frame owns one slot per MSIR value ID and a table of local
storage cells.

**Per-function facts.** The first call of a function builds its descriptor (`واصف_دالة_مفسر`,
`execution/execution_state.sn`): the slot count, the entry block, the parameter value IDs, and a
numeric descriptor per value ID with the width, signedness and kind (integer, character, float or
reference) resolved from its type through the registry. CPython keeps the like in a code object and
Lua in a `Proto`. Handlers read an operand's width and sign from the descriptor table instead of
walking the type registry on every arithmetic and comparison instruction, which profiles showed to
be a fifth of the work.

**Reused frames and registers.** A returned frame keeps its slot table and its local cells at its
stack position, and the next activation there reuses them (`ادخل_إطار_دالة_مفسر`), so a warm call
allocates nothing. Verified MSIR defines every value before its uses, so stale slots need no
clearing, just as a register VM's registers do not. A SultanC call reads each argument straight from
the caller's slot into the callee's parameter slot (`نفذ_نداء_سلطان_مباشر_مفسر`,
`execution/calls.sn`). An edge stages its operands in one vector that the state reuses
(`حجج_الحواف`) instead of allocating a vector per edge.

**Native integer arithmetic.** Multiply, divide and shift evaluate with the host's own 64-bit
operations (`ضرب_خام_مفسر`, `اقسم_خام_طبيعي`, `ازح_يسارا_خام_مفسر`), masked to the operand width,
instead of one shift-and-add or restoring-division step per bit of the width.

**Fast paths.** The dispatch loop runs the instructions that a program executes most through
specialized handlers (`operations/fast_paths.sn`), after CPython 3.11's adaptive interpreter and
Lua's integer opcodes. They cover integer constants, arithmetic, comparison and conversion, loads
and stores through the common places, element-place formation, index extraction, the length and
append primitives, reference and place conversions, and branches. Each runs in one step: operands
are read in place, width and sign come from the descriptor table, the result is written straight
into its slot and the instruction pointer advances. A whole block's covered instructions and its
terminator run in an inner loop that does not re-derive the active frame and block each step.

An instruction that a fast path does not cover (another operand shape, a value not where it expects
it, an operation that would trap) changes nothing and falls through to the checked handler, which
executes it and reports any failure. CTFE always takes the checked path, which counts every
instruction against its budget.

**Recursion limit.** Unbounded recursion stops at 250 000 live frames with
`SULTANC-INTERPRETER-RUNTIME-FAILURE` (reason `interpreter.runtime.call-depth-exceeded`), as
CPython's recursion limit and Lua's `LUAI_MAXCCALLS` stop runaway recursion, instead of growing
until the host runs out of memory. A native program overflows its 8 MiB stack at a comparable depth.

### Memory

Values are semantic (integers are raw bit patterns, records and tuples are member lists), but
storage follows the native model.

- A vector, slice or text value is a handle to one heap buffer (`قيمة_تشغيل.مخزن`), as a native value
  is a pointer to its storage. Copying the value copies the handle, and MSIR ownership (drop
  elaboration and the per-type glue every backend runs, [Ownership and Destruction](../reference/ownership.md))
  decides who releases it. Buffers of `u8` and text store raw bytes.
- `append` appends in place (amortized O(1)), and the release primitive frees the buffer. Each buffer
  keeps the vector's capacity as native code keeps it in the header: the growth policy's capacity for
  a literal, grown by append, raised by `reserve_exact`, and kept by pop and truncate.
  `capacity` therefore answers as it does in an executable. The length store of pop and
  truncate shortens a buffer by moving its kept elements into a new SultanC vector (O(kept), because
  the interpreter's own vectors only grow); a static buffer is copied first.
- A text literal is interned once per constant site and is static. It is never freed (the native
  allocator likewise ignores a literal's bytes), is copied before it grows, and is read-only.
- Boxes and closure environments are cells reused through free lists. Closure environments are
  reference counted exactly as the native header (count, destructor) counts them:
  `زيادة_مراجع_إغلاق`, `إنقاص_مراجع_إغلاق` and `مدمر_بيئة_إغلاق` run the same glue, so a captured owner is
  dropped when the last closure goes.
- Every slot has a generation. A stale handle (use after free, double release) is reported instead of
  reading reused storage, as Miri's allocation IDs report it.
- A place is a root (frame cell, global, box cell or buffer element) and a projection path.
  Projections never cross a handle: an element place is re-rooted at its buffer when it is formed, as
  native code reads the data pointer. Writes mutate in place.

Long-running programs therefore have bounded memory. An allocate-and-drop loop and a
closure-creating loop keep a flat resident set.

### Startup data

The program's `argv` is its source path followed by its arguments; the first `--` after the source
only separates them. On Unix hosts its `envp` is the interpreter's own startup environment, entry for
entry, as an `exec` would pass it. Both are materialized in interpreter buffers behind the canonical
startup symbols (`sultanc.bootstrap.argc/argv/envp`) that std reads. std's auxv use is
`/proc/self/auxv` through ordinary system calls. On Windows std reads the command line through
`GetCommandLineW` and `CommandLineToArgvW`, which the foreign-call bridge answers with the program's
own arguments.

The program's current executable (`std.env.executable`, Rust's `env::current_exe`) is the
interpreter's own path when interpreted. A program that starts a copy of itself
(`tests/*/stdlib/behavior/fs_lock.sn`) therefore runs that path with `interpret`, its source and `--`
before its own arguments, on every host.

## Host bridges

### The kernel bridge (Linux)

std's Linux code issues raw system calls. A call that std's contract table
(`std/os/platform/unix/linux/syscalls/operations.sn`) describes keeps its authenticated path: every
buffer is checked against the attested signature, staged, and only the transfer the contract allows
is copied back.

Any other raw call (a user program's own, `clone`, `execve`, `waitid`, `mmap`, socket options, `poll`
and so on) is marshalled from its runtime values, the way Miri's shims and libffi marshal a foreign
call. Integers pass as words and a null pointer as 0. A pointer to integer storage (a `u8` or integer
vector from the referenced element on, or one integer variable) passes as the address of a host copy
of those bytes, with slack after it. A pointer to a table of string pointers (`execve`'s argv and
envp) passes as a host pointer table. Copies that the kernel changed are written back.

A fork-like `clone` therefore forks the interpreter. The child interpreter continues with the child's
path (close descriptors, `execve`, report a failure through the pipe, `exit`), so `std.process.child`
works as in an executable, and so does a streaming child of `std.process.command`: its pipes are
ordinary descriptors, and the SIGPIPE mask calls around a write to its input are marshalled like
`clone`. `exit_group` ends the interpretation cleanly with the status.

Raw machine-memory words (`بدائي_اقرأ_ذاكرة_الآلة`, used by std's allocation provider on pages it
mapped) are read and written on the host through `/proc/self/mem`, as native code does. Like native
code, nothing checks such an address. The compiler's own sources cannot use the machine-memory
intrinsic, because the C Stage0 that compiles them has none. The wasm linear-memory primitives exist
only in wasm32 std paths and are never reached by a host-target program.

### The foreign-call bridge

On Windows and macOS, and for a program's own C functions on every host with a loader, an extern call
runs the host function through a generated table (`tools/generate/generate_host_externs.py` to
`execution/host_externs_generated.sn`). Every extern that std declares for the platform is
re-declared with its library and the same C type std gives it. A SultanC executable declares each
import once, and the LLVM backend rejects a second declaration of a different type, so every pointer
is spelled `*u8`, which is the same C type. The host loader binds the functions as for any
executable. This is CPython's and Lua's model of statically registered C functions, and wasm3's
host-function table.

The marshalling (`execution/foreign_calls.sn`) follows Miri's foreign-call support and
libffi/ctypes. Each argument becomes one word.

- An integer passes as is (a boolean as 0 or 1), and a null pointer as 0.
- A pointer into a host region passes the region's host address.
- A pointer to interpreter storage passes the address of a host copy of the bytes it designates:
  integer elements, or a record in its C layout (natural alignment, 8-byte pointers). The copy is
  read back when the callee changed it.
- A pointer to a table of pointers (`posix_spawnp`'s argv and envp) passes a host table of the
  addresses of each pointee's copy.
- A pointer result is the argument it aliases (`getcwd` returns its buffer), a pointer into a known
  host region, or a new opaque host region.

Host addresses and pointers are converted with `memmove` and `memcpy` and a zero count, which return
their first argument. Host copies live in host memory (`execution/host_memory.sn`: `HeapAlloc` on
Windows, `calloc` on macOS).

**Host regions** (`memory.sn`, `منطقة_مضيف_مفسر`) are memory that the host returned to the program, as
Miri gives every foreign allocation a provenance-tracked place. The bytes are mirrored in one
interpreter buffer of fixed length (`u8` elements as bytes, wider integers as values, one record per
element for a record pointee), so every read and write through a pointer into the region is
bounds-checked like any buffer access. A read-only region's mirror is static, so a write through it
fails as a write to a literal does. Before a call that receives a region, its changed mirror bytes
are written to the host, and afterwards the mirror is read back. When the host frees a region
(`FreeEnvironmentStringsW`, the previous context given to `CertEnumCertificatesInStore`), its buffer
is freed with it, so a later use is a stale-handle report. An opaque region, a pointer whose extent no
contract gives, has no elements: it can be compared and passed back to the host, and any dereference
is out of bounds.

**errno and the last-error code.** The interpreter keeps errno (macOS) and the Win32 last-error code
(Windows) in a cell of its own. It hands the cell to the host before every foreign call and takes it
back afterwards, as Miri keeps its own errno place, so the interpreter's own host work between the
program's call and its errno read cannot change the value.

**Shims** answer the externs whose results point into host memory with an extent that only their
contract gives.

| Host | Extern | Shim |
|---|---|---|
| macOS | `__error` | a pointer to the errno cell |
| macOS | `opendir`, `readdir`, `closedir` (and the `$INODE64` names) | an interpreter DIR stream; `readdir` copies the host's 64-bit-inode `struct dirent` (the layout `directories.sn` reads) into a buffer valid until the next `readdir` |
| macOS | `getaddrinfo`, `freeaddrinfo` | the list rebuilt in interpreter memory (one record per node, the name and socket address copied); the host list is freed at once |
| macOS | `dladdr` | the host's answer for an address in a host region, the names copied; interpreter storage is in no image (0) |
| Windows | `GetLastError`, `SetLastError`, `WSAGetLastError` | the last-error cell |
| Windows | `GetCommandLineW`, `CommandLineToArgvW`, `LocalFree` | the interpreted program's own arguments |
| Windows | `GetEnvironmentStringsW` | a read-only host region up to the empty string that ends the block |
| Windows | `GetAddrInfoW`, `FreeAddrInfoW` | as `getaddrinfo` (the record type std declares gives each member's offset) |
| Windows | `CertEnumCertificatesInStore` | a read-only record region for the `CERT_CONTEXT`, whose encoded-bytes member points to a region of `cbCertEncoded` bytes |

**The generator rule.** `generate_host_externs.py` fails, naming the declaration, when std declares
an extern for macOS or Windows that the interpreter cannot call:

- a parameter or result that is neither an integer nor a pointer (a float, a record by value);
- a pointer result or a pointer-bearing record argument without a shim;
- a shim that the generator lists and `foreign_calls.sn` does not implement;
- an extern outside `library/std/os/platform/`, where it would escape the audit.

`generate_host_externs.py --check` also fails when the generated table is out of date.

### Dynamic C functions

A C function that the program declares itself (`extern "libm.so.6" def pow(…)`,
`extern def labs(…)`) is not in the generated table. The bridge finds it at run time and calls it at
its address (`execution/host_dynamic.sn`, wired in `foreign_calls.sn`), as Python's ctypes opens a
library with dlopen and finds a function with dlsym (`Modules/_ctypes/callproc.c`), Miri's native-lib
mode loads the shared object a user names, and libffi's `ffi_call` loads the argument registers from
a prepared call interface.

**Lookup.** The declared library is opened once and kept: `dlopen(name, RTLD_NOW)` on Linux and
macOS, `LoadLibraryW` on Windows. The function is found with `dlsym` or `GetProcAddress` once per
extern and cached by its ID. A declaration without a library is looked up in the process's global
scope (`dlsym(RTLD_DEFAULT)`), then in the platform C libraries: `libc.so.6` and `libm.so.6`;
libSystem; `msvcrt.dll`, `ucrtbase.dll` and `kernel32.dll`.

**The call.** `raw_c_call` is the compiler's one primitive for a call at a
run-time address with the platform C calling convention (Rust's call through an `extern "C" fn`
pointer). It takes a fixed register image: a result witness (`u64` for the integer return register,
`f64` for the floating-point one), the address, eight integer words and eight floating-point words.

| ABI | Lowering |
|---|---|
| System V x86-64 | Native: the extern-call plan of `(u64 x8, f64 x8)`, `CALL r11` with `AL = 8`. LLVM: every word a variadic argument (the same registers as a prototyped call: six integer registers then the stack, `xmm0` to `xmm7`; `AL` set) |
| AAPCS64 (Linux, Apple, Windows on Arm) | Native (Darwin) and LLVM: the prototyped `(i64 x8, double x8)` call, `BLR x16`. No variadic argument is passed, so Apple's rule that variadic arguments go on the stack never arises: a variadic callee (`snprintf`) receives its fixed arguments in registers |
| Windows x64 | LLVM: positional words, the first four as `double` variadic arguments (the Win64 variadic rule puts each in `xmm k` and copies it to `rcx`/`rdx`/`r8`/`r9`, so an integer or a float parameter finds the same bits), the next four on the stack |
| wasm32 | rejected at selection (no callable addresses) |
| Stage0 | System V x86-64 and AAPCS64, as LLVM |

The primitive needs `unsafe`, and only the interpreter uses it.

**Supported signatures.** Every parameter and the result is an integer of any width (narrower ones
are sign- or zero-extended to 64 bits as C passes them, and a narrower result is masked), a boolean,
a pointer, `f32` or `f64`, or the result is void. At most eight integer and eight floating-point
arguments are allowed (Windows x64: eight in all, by position). A pointer argument is marshalled by
the bridge's rules above (host copies of interpreter storage, host regions, pointer tables), and a
pointer result is an argument it aliases, a known host region or a new opaque region. errno is
exchanged around each call (Linux: through `__errno_location`, which is also shimmed to the
interpreter's errno cell).

A record passed or returned by value, or more arguments, is reported as
`SULTANC-INTERPRETER-UNSUPPORTED-EXTERNAL-CALL` with reason
`interpreter.runtime.external-signature-unsupported`. A library that cannot be loaded and a missing
function have their own reasons: `external-library-not-loaded`, with the loader's message, and
`external-symbol-not-found`. Variadic arguments cannot be declared, so `snprintf` is called with its
fixed arguments only.

**Linux and the static compiler.** The native Linux compiler is a static executable without the C
library, and a process that the kernel started without a dynamic loader cannot load glibc soundly:
`libc.so.6` needs ld.so's own state (its TLS setup, IFUNC resolution from ld.so's CPU features, the
link map). A small ELF loader in SultanC, as Go's linker reads shared objects for cgo-free dynamic
imports, would work only for objects with no such dependency, never for libc or libm, and would be a
second dynamic linker to maintain. The interpreter therefore calls dynamic C functions only where the
host provides a loader, as a static Go binary cannot dlopen either.

- A compiler that the system's dynamic loader started (an LLVM-enabled build, linked against libLLVM,
  so the C library is in the process) finds `dlopen`, `dlsym`, `dlerror` and `__errno_location` in
  the C library already mapped into it (`/proc/self/maps`, then that library's ELF dynamic symbol
  table: glibc 2.34 and later and musl define them in libc, older glibc in libdl). Host blocks are
  anonymous mappings and bytes move through `/proc/self/mem`, so none of the bridge's own work needs
  the C library.
- A static compiler reports `SULTANC-INTERPRETER-UNSUPPORTED-EXTERNAL-CALL` with reason
  `interpreter.runtime.no-dynamic-loader`. Build the program, or interpret it with a compiler linked
  against the C library.

macOS and Windows executables always have a loader (libSystem's `dlopen`, kernel32's
`LoadLibraryW`). FreeBSD and Android compilers are static like the Linux one and report the same.

`tools/tests/interp_ffi_check.sh` runs `examples/{en,ar}/ffi/extern_call.sn` and
`tests/{en,ar}/runtime/ffi/libm_libc_calls.sn` (libm's `pow`, `ldexp`, `frexp` with an out-pointer
and `sqrtf`; libc's `strlen` and `snprintf`) natively and interpreted on Linux. It also runs them
under wine (`MODE=wine`, msvcrt) and checks their macOS imports (`MODE=macos`). In CI, whose compiler
is static, the test pair is an accept test.

`tools/tests/mac_interp_check.sh` runs the stdlib tests that meet host memory natively and
interpreted with `./build/sultanc` and prints a table of matches and differences, with the
interpreted stderr of each difference. It is plain POSIX sh for macOS's bash 3.2 and zsh, and writes
only under `build/tmp/`.

## Performance reference

The figures below were measured with `/usr/bin/time` on x86_64-linux on an idle host, as wall-clock
seconds of the current dispatch loop. They show the order of magnitude, not a guarantee.

| Program | Time |
|---|---|
| 10^6-iteration integer loop (`acc = (acc*31+i) mod p`) | 3.3 s |
| 200 GF(2^255-19) multiplications (Ed25519 inner loop) | 1.2 s |
| SHA-256 of 64 KiB, then a 32-bit FNV-1a over it | 10.0 s |
| sort 20 000 pseudo-random ints, then fold them | 14.9 s |
| gzip round trip over 16 KiB | 18.6 s |
| parse and re-stringify 1 000 JSON objects | 11.2 s |
| `examples/en/crypto/signatures.sn` (Ed25519; native: 1 s) | 283 s |

A tight loop costs roughly 0.3 to 0.5 microseconds per MSIR instruction. Programs that loop 10^5
times through the std allocator counters (`tests/*/language/propagation/drops.sn`,
`tests/*/stdlib/error/*rss_flat.sn`) take minutes when interpreted.

## Checking the interpreter against native code

`tools/tests/interp_sweep.sh` builds every example (`examples/{ar,en}`) and every run or differential
test (`tests/{ar,en}`) natively, runs it, and runs the same source with `sultanc run`, with the same
stdin (`/dev/null`), arguments and environment. It compares stdout, stderr and the exit status.

```sh
SC=build/sultanc bash tools/tests/interp_sweep.sh            # everything
SC=build/sultanc bash tools/tests/interp_sweep.sh examples/en/basics/hello.sn
```

The sweep takes about 30 minutes on 3 jobs, too long for `ci_run.sh`. The runner's differential tests
(`// اختبار: تفاضلي`) are the CI subset ([Tests](../contributing/tests.md)). The programs that do not
match natively are only those over the time limit: the slow allocator-counter loops
(`*rss_flat.sn`, the drop loops) and `examples/*/crypto/signatures.sn`. The sweep also skips the
files that are not programs, such as the modules of multi-file examples and the C extern example.

The Windows interpreter (`--target=x86_64-windows-msvc`, run under wine) agrees with the native Windows
executable on the `examples/en` programs except `signatures.sn` (time). On `tests/{ar,en}/stdlib`,
with the slow `*rss_flat.sn` loops left out, it agrees except `process_kill.sn`, which uses Unix-only
std names and does not build for Windows.

## Limitations

- Speed. The dispatch loop runs the common instructions through the fast paths, but each fast-path
  handler is still a function call that re-reads its operands' descriptors and re-matches the
  operation. A pre-decoded instruction stream (one record per instruction, operands resolved to slot
  indices once) or threaded dispatch would remove that cost; the interpreter has neither.
- macOS. The foreign-call bridge, including dynamic C functions, is built and import-audited for
  `arm64-darwin` and `x86_64-darwin`. The import audit does not run the bridge; Windows under wine
  exercises the same marshalling, host regions and dynamic calls on Linux.
- Variadic arguments and records passed by value are not supported. SultanC declares neither, and a
  record by value is reported.
- A program's own C functions need a dynamic loader in the interpreter's process. A static Linux,
  FreeBSD or Android compiler reports them (see "Dynamic C functions"). The CI compiler is static, so
  CI only checks the test pair, and `tools/tests/interp_ffi_check.sh` runs it.

## See also

- [MSIR and Native Code Generation](msir-and-codegen.md)
- [Compiler Architecture](compiler-architecture.md)
- [Safety Flow](safety-flow.md)
- [Platform Dependencies](../reference/platform-dependencies.md)
- [Tests](../contributing/tests.md)
