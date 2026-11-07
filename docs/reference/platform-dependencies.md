# Platform Dependencies of std

This page lists every function std imports from outside SultanC: the library it comes from, why
that library is the platform's stable boundary, the C declaration it was checked against, and how
mature toolchains handle the same boundary. It also lists what std implements in SultanC instead
of calling the platform. It is written for contributors who add or port platform code, and for
anyone auditing what a SultanC executable links.

The rule is the one Rust and Go follow. std depends only on the platform's supported system
interface. It does not use a C runtime for language features such as memory copy, formatting,
math, allocation, cryptography or DNS. Each import names its library at the declaration, in std,
and the compiler holds no table of std's symbols.

| Platform | Boundary std uses | Libraries imported |
|---|---|---|
| Linux, Android | raw kernel calls | none |
| FreeBSD | raw kernel calls | none |
| macOS | libSystem | `/usr/lib/libSystem.B.dylib` |
| Windows | Win32 API in system DLLs, no CRT | `kernel32.dll`, `ws2_32.dll`, `api-ms-win-core-synch-l1-2-0.dll`, `shell32.dll`, `bcryptprimitives.dll`, `crypt32.dll` |
| WASI preview 1 | WASI imports | `wasi_snapshot_preview1` |

## How a declaration names its library

```
[عام] خارجي [«library»] عرف name(params): Result [= «link_name»];
[public] extern ["library"] def name(params): Result [= "link_name"];
```

`«library»` is the library as the target's loader names it: a PE DLL (`«kernel32.dll»`), a Mach-O
install name (`«/usr/lib/libSystem.B.dylib»`) or a WebAssembly import module
(`«wasi_snapshot_preview1»`).

`= «link_name»` binds the declaration to an exact linker symbol when it differs from the source
name, for example `= «stat$INODE64»`. It plays the role of Rust's `link_name` and of a C asm label.
Both texts are nonempty and contain no NUL byte. External data (`خارجي دع x: T = «symbol»;`) takes
no library.

A link name that exists on only some architectures goes behind a target guard
(`لو (الهدف.المعمارية == «x86_64») { … } وإلا { … }`). Darwin's `$INODE64` names exist only on
x86_64. The interpreter's extern table (`tools/generate/generate_host_externs.py`) must respect
the same guard: linking `stat$INODE64` on every macOS target would stop the compiler loading on
arm64 Macs.

`tools/audit/macos_imports_audit.py --sdk <MacOSX.sdk> --arch arm64|x86_64 <binary>…` checks every
import of a Mach-O executable against the SDK's `.tbd` exports for that architecture. dyld rejects
a binary if any import is missing, and this check catches that without a Mac.

| Toolchain | Form | SultanC equivalent |
|---|---|---|
| Rust | `#[link(name = "kernel32")] extern "system" { ... }`; std's Windows bindings use `windows_targets::link!("kernel32.dll" "system" fn ...)` (raw-dylib); `#[link(wasm_import_module = "wasi_snapshot_preview1")]`; libc's `#[cfg_attr(all(target_os = "macos", target_arch = "x86_64"), link_name = "stat$INODE64")]` | library text plus `= «link_name»` |
| Go | Windows: `//sys GetLastError() ... = kernel32.GetLastError` (mkwinsyscall) and `//go:cgo_import_dynamic runtime._CloseHandle CloseHandle%1 "kernel32.dll"`; darwin: `//go:cgo_import_dynamic libc_read read "/usr/lib/libSystem.B.dylib"` | the same per-symbol library identity |

### How the library reaches the binary

The parser stores the library and the link name in the declaration
(`compiler/frontend/ast/declarations.sn`: `تصريح_دالة.المكتبة_الخارجية` and `اسم_الربط`). Lowering
carries them into MSIR as `دالة_خارجية_مسير.الاسم` (the linker symbol) and `.المكتبة`. Each target
then uses them as follows.

| Target format | Behavior |
|---|---|
| WebAssembly | Both the native wasm writer and the LLVM provider (`wasm-import-module` and `wasm-import-name`) import the function from the declared module, with an explicit import name. A declaration with no module imports from `env`, the default of wasm-ld, clang and rustc. The wasm linker imports undefined functions from the module their object names; wasm objects carry their modules themselves. |
| PE/COFF | The driver records each declared (symbol, DLL) pair in the link plan (`compiler/link/model.sn`, `استيراد_مكتبة_مصرح_ربط`). The COFF linker turns every undefined symbol with a declared DLL into an import of that DLL (`استورد_رموز_مصرحة_كوف_في_السياق`, `compiler/link/coff/directives.sn`). This needs no import library, like rustc's raw-dylib. |
| Mach-O | An undefined symbol binds to the ordinal of the dylib its declaration names (`ترتيب_مكتبة_رمز_ماتشو`, `compiler/link/macho/final_image.sn`). A declared dylib is loaded only when one of its functions is imported. Without a declared library a symbol binds to libSystem, always ordinal 1, which is sound because libSystem is the macOS system interface. |
| ELF | The declared library has no effect. Externals resolve against the link inputs (`-l c` and so on). std declares no extern on Linux, Android or FreeBSD. |

The compiler names one library itself: LLVM's (`LLVM-C.dll`, `مكتبة_إل_إل_في_إم_باسم_للهدف` in
`compiler/backend/target/runtime_symbol_query_context.sn`), because LLVM is a dependency of the
compiler, not of std. Adding an extern to std never requires editing the compiler.

### Calls and `غير_آمن`

A raw kernel call (`بدائي_نداء_نواة_خام`) must appear inside `غير_آمن`. A call of an external
function does not need `غير_آمن`, by the owner's decision: an extern declaration is a typed
signature, and std keeps its pointer arguments and results in bounded wrappers.

## Linux, Android and FreeBSD: raw kernel calls, no libc

std calls the kernel directly through `بدائي_نداء_نواة_خام`. Linux and Android use
`library/std/os/platform/unix/linux/**`, with syscall tables generated from each architecture's
UAPI headers. FreeBSD uses `platform/unix/freebsd`, which shares the direct-kernel provider.
Neither imports anything. This is Go's model: Go's `syscall` and `runtime` packages issue raw
system calls on Linux and FreeBSD and link no libc, because both kernels guarantee a stable
system-call ABI.

Kernel operations beyond basic file and process calls are in `syscalls/operations.sn`, with
interpreter signatures. Their numbers are generated for every Linux ABI and for FreeBSD.

| Operation | Linux | FreeBSD | Used by |
|---|---|---|---|
| `وصلة_رمزية_عند` | `symlinkat` | `symlinkat` | `linux/file_attributes.sn` |
| `وصلة_صلبة_عند` | `linkat` | `linkat` | `linux/file_attributes.sn` |
| `قراءة_وصلة_عند` | `readlinkat` | `readlinkat` | `linux/file_attributes.sn` |
| `أذونات_مسار_عند` | `fchmodat` | `fchmodat` | `linux/file_attributes.sn` |
| `اقتطاع_واصف` | `ftruncate` (32-bit length on i386 and ARM: larger lengths are refused) | `ftruncate` | `linux/file_attributes.sn` |
| `أوقات_مسار_عند` | `utimensat` (`utimensat_time64` on i386 and ARM) | `utimensat` | `linux/file_attributes.sn` |
| `تغيير_دليل_العمل` | `chdir` | `chdir` | `linux/file_attributes.sn` |
| `مضاعفة_واصف` | `dup3` | `dup2` | the child of `شغل_طفلا_لينكس_بإعداد` |
| `اسم_مقبس_محلي` / `اسم_مقبس_نظير` | `getsockname` / `getpeername` | same | `linux/socket_names.sn` |
| `إرسال_إلى_عنوان` / `استقبال_من_عنوان` | `sendto` / `recvfrom` with an address | same | `linux/socket_names.sn` |
| `قفل_ملف` | `flock` | `flock` | `os/file_lock.sn` (`std.fs.lock`) |
| `حجب_إشارات` | `rt_sigprocmask` | `sigprocmask` | `linux/child_stdio.sn`: SIGPIPE blocked around a write to a child's input |
| `انتظر_إشارة` | `rt_sigtimedwait` (`rt_sigtimedwait_time64` on i386 and ARM) | `sigtimedwait` | the same write: takes the SIGPIPE that an EPIPE write left pending |
| `مستقبل` | `futex` (WAIT and WAKE, `_PRIVATE`) | `_umtx_op` (`UMTX_OP_WAIT_UINT_PRIVATE` and `UMTX_OP_WAKE_PRIVATE`) | `linux/wait_address.sn`, `freebsd/wait_address.sn` (`std.os.wait_address`); the Linux thread join; the FreeBSD thread join waits with `UMTX_OP_WAIT` on `thr_exit`'s state word |
| `مستقبل_بمهلة` | `futex` with a relative timeout (`futex_time64` on i386 and ARM) | none | `linux/wait_address.sn` |
| `مستقبل_بمهلة_بي_إس_دي` | none | `_umtx_op` with a relative `struct timespec` in `uaddr2` | `freebsd/wait_address.sn` |
| `نسخ_عملية` (thread form) | `clone(VM\|FS\|FILES\|SIGHAND\|THREAD\|SYSVSEM\|PARENT_SETTID\|CHILD_CLEARTID)`, issued by `بدائي_استنسخ_خيط_نواة` | none | `linux/threads.sn` (`std.thread`) |
| `إنشاء_خيط` | none | `thr_new` | `freebsd/threads.sn` |
| `إنهاء_خيط` | `exit` (the clone child's tail) | `thr_exit` | the thread's last call |
| `تنازل_جدولة` | `sched_yield` | `sched_yield` | `std.thread.تنازل` |
| `معرف_الخيط` | `gettid` | `thr_self` | `std.thread.معرف_الخيط_الحالي` |
| `ألفة_المعالج` | `sched_getaffinity` | `cpuset_getaffinity`, then `sysctl hw.ncpu` | `std.thread.عدد_المعالجات_المتاحة` |

`flock` has an interpreter signature of two integers, and the two signal calls are marshalled like
`clone` and `waitid`. The UAPI generators supply SIGPIPE, SIG_BLOCK and SIG_SETMASK (1 and 3 on
MIPS and FreeBSD, 1 and 4 on SPARC), `LOCK_SH/EX/NB/UN`, `EAGAIN`, `EPIPE`, `AF_UNIX`, `O_EXCL`,
`O_RDWR`, the size of the kernel's `sigset_t` (8 bytes; 16 on MIPS and FreeBSD) and of
`sockaddr_un`'s `sun_path` (108; 104 on FreeBSD). Child stdio uses the existing `pipe2` and
`ppoll` operations. Unix domain sockets need no dedicated call: `socket(AF_UNIX, ...)`, `bind`,
`connect`, `accept4`, `sendto`, `recvfrom` and the name calls take a `sockaddr_un` built by
`os/unix_socket.sn`.

### Writing to a child's input without SIGPIPE

A write to a pipe can raise SIGPIPE, which would end the process when the child has closed its
input. Sockets avoid it with `MSG_NOSIGNAL` (Linux, FreeBSD) or `SO_NOSIGPIPE` (macOS); pipes have
neither. Rust's std ignores SIGPIPE for the whole process and resets it in each child before exec.
std instead blocks SIGPIPE in the calling thread for the one write and, after EPIPE, takes the
pending signal with a zero-timeout `sigtimedwait`, as PostgreSQL's libpq does for its socket
writes (`pq_block_sigpipe`, `pq_reset_sigpipe`). No process-wide signal state changes, and a child
inherits nothing.

## macOS: libSystem

Apple does not support direct system calls: the syscall ABI is private and changes between
releases. libSystem (`/usr/lib/libSystem.B.dylib`, which re-exports libsystem_kernel, libsystem_c,
libdyld and others) is the stable boundary. Go has gone through libSystem on darwin since Go 1.12,
and Rust's std links it through the libc crate. std names it on every declaration, as Go does.

The signatures were checked against the macOS SDK headers. On both Darwin ABIs, `mode_t` is u16,
`off_t` and `ssize_t` are i64, `size_t` is u64 and `socklen_t` is u32. All modules below are under
`library/std/os/platform/unix/apple/darwin/`.

| Module | Functions and C declarations |
|---|---|
| `file_descriptors.sn` | `int *__error(void)`; `int open(const char *, int, ...)` (called with fixed arguments only, because on arm64 the variadic part travels on the stack); `int creat(const char *, mode_t)`; `ssize_t read(int, void *, size_t)`; `ssize_t write(int, const void *, size_t)`; `int close(int)`; `off_t lseek(int, off_t, int)`; `int fsync(int)`; `int fchmod(int, mode_t)`; `int rename(const char *, const char *)`; `int unlink(const char *)`; `int stat(const char *, struct stat *)`, bound to `stat$INODE64` on x86_64; `char *getcwd(char *, size_t)` |
| `directories.sn` | `DIR *opendir(const char *)` (`opendir$INODE64` on x86_64); `struct dirent *readdir(DIR *)` (`readdir$INODE64` on x86_64); `int closedir(DIR *)`; `int mkdir(const char *, mode_t)`; `int rmdir(const char *)` |
| `process_control.sn` | `int posix_spawnp(pid_t *, const char *, const posix_spawn_file_actions_t *, const posix_spawnattr_t *, char *const[], char *const[])`; `posix_spawnattr_init`, `posix_spawnattr_destroy`, `posix_spawnattr_setflags(posix_spawnattr_t *, short)`; `posix_spawn_file_actions_init`, `_destroy`, `_addinherit_np(…, int)`; `int posix_spawn_file_actions_adddup2(posix_spawn_file_actions_t *, int, int)`; `int posix_spawn_file_actions_addchdir_np(posix_spawn_file_actions_t *, const char *)` (macOS 10.15+); `pid_t waitpid(pid_t, int *, int)`; `int kill(pid_t, int)`; `void _exit(int)` |
| `sockets.sn` | `socket`, `connect`, `bind`, `listen`, `accept`, `ssize_t send(int, const void *, size_t, int)`, `recv`, `shutdown`, `setsockopt(int, int, int, const void *, socklen_t)`, `int getaddrinfo(const char *, const char *, const struct addrinfo *, struct addrinfo **)`, `void freeaddrinfo(struct addrinfo *)`. bind and connect take a `sockaddr_in`, `sockaddr_in6` or `sockaddr_un` (`sun_len`, `AF_UNIX` 1, a 104-byte `sun_path`). |
| `socket_names.sn` | `int getsockname(int, struct sockaddr *, socklen_t *)`; `int getpeername(...)`; `ssize_t sendto(int, const void *, size_t, int, const struct sockaddr *, socklen_t)`; `ssize_t recvfrom(int, void *, size_t, int, struct sockaddr *, socklen_t *)` |
| `virtual_memory.sn` | `void *mmap(void *, size_t, int, int, int, off_t)`, `int mprotect(void *, size_t, int)`, `int munmap(void *, size_t)`, `int mlock(const void *, size_t)`, `int munlock(const void *, size_t)`, `int madvise(void *, size_t, int)`, `int getpagesize(void)`. These are the only declarations of these functions: `os/virtual_memory.sn` calls them from here. |
| `clocks.sn` | `int clock_gettime(clockid_t, struct timespec *)`; `uint64_t mach_absolute_time(void)`; `kern_return_t mach_timebase_info(mach_timebase_info_t)`; `uint64_t mach_continuous_time(void)`; `kern_return_t mach_wait_until(uint64_t)` |
| `entropy.sn` | `int getentropy(void *, size_t)` |
| `dynamic_loader.sn` | `int dladdr(const void *, Dl_info *)` |
| `file_attributes.sn` | `int symlink(const char *, const char *)`; `ssize_t readlink(const char *, char *, size_t)`; `int link(const char *, const char *)`; `int chmod(const char *, mode_t)`; `int ftruncate(int, off_t)`; `int utimensat(int, const char *, const struct timespec[2], int)` (`AT_FDCWD` is -2); `int chdir(const char *)`. Exclusive creation avoids `open`'s variadic mode: `creat` a uniquely named sibling, `link` it to the requested path (EEXIST is atomic), `unlink` the sibling, then `open(O_RDWR\|O_CLOEXEC)`. |
| `child_stdio.sn` | `int pipe(int[2])`; `int poll(struct pollfd *, nfds_t, int)` (`nfds_t` is u32); `int pthread_sigmask(int, const sigset_t *, sigset_t *)`, `int sigpending(sigset_t *)`, `int sigwait(const sigset_t *, int *)` (`sigset_t` is u32). These give the SIGPIPE-safe write to a child's input, libpq's Darwin path; `F_SETNOSIGPIPE` would need fcntl's variadic argument. |
| `file_locks.sn` | `int flock(int, int)` (`<sys/file.h>`) |
| `wait_address.sn` | `int __ulock_wait2(uint32_t op, void *addr, uint64_t value, uint64_t timeout_ns, uint64_t value2)` (macOS 11+) and `int __ulock_wake(uint32_t op, void *addr, uint64_t wake_value)`, from libsystem_kernel through libSystem. They are the futex layer under `std.os.wait_address` and `std.sync`, Rust's choice before `os_sync_wait_on_address` (macOS 14.4). |
| `threads.sn` | `int pthread_create(pthread_t *, const pthread_attr_t *, void *(*)(void *), void *)`; `int pthread_join(pthread_t, void **)`; `pthread_attr_init`, `pthread_attr_setstacksize(pthread_attr_t *, size_t)`, `pthread_attr_destroy` (`pthread_attr_t` is 64 bytes); `int pthread_threadid_np(pthread_t, uint64_t *)`; `void pthread_yield_np(void)`; `int sysctlbyname(const char *, void *, size_t *, void *, size_t)` (`hw.logicalcpu`). The start routine is a closure's C entry (`بدائي_مدخل_سي_لدالة`), as Rust's std and Go's `newosproc` start threads with `pthread_create`. |

### Darwin ABI details

- **x86_64 inode ABI.** std reads the 64-bit-inode `struct stat` (144 bytes) and `struct dirent`
  (`d_namlen` u16 at offset 18). On x86_64 the plain `stat`, `opendir` and `readdir` are the legacy
  32-bit-inode functions, which `<sys/stat.h>` and `<dirent.h>` map to `stat$INODE64`,
  `opendir$INODE64` and `readdir$INODE64` (`__DARWIN_INODE64`, `__DARWIN_ALIAS_I`). The
  declarations are bound by link name under an architecture guard, as Rust's libc does. arm64 uses
  the plain names.
- **`struct addrinfo` field order.** Apple's `<netdb.h>` uses the BSD order: `ai_canonname` comes
  before `ai_addr`, the reverse of glibc. std's record follows the Darwin order. Reading the
  address from `ai_canonname` would fail every lookup, because that field is NULL without
  `AI_CANONNAME`.
- **One declaration per function.** Each libSystem function is declared once: `close` in
  `file_descriptors.sn` (used by `sockets.sn`), and `mmap`, `mprotect` and `madvise` only in
  `virtual_memory.sn`.

## Windows: system DLLs, no CRT

The Win32 API in the system DLLs is the supported boundary on Windows. The NT syscall numbers are
private. std links no C runtime: there is no `mainCRTStartup`, and no `memcpy`, `malloc` or
`printf`. The DLL is named at each declaration, as Rust's std does through
`windows_targets::link!`.

Signatures were checked against the MSDN declarations. `BOOL` and `int` are i32; `DWORD`, `UINT` and
`ULONG` (also `u_long`, since Windows `long` is 32-bit) are u32; `HANDLE`, `SOCKET`, `SIZE_T` and
the pointer types are 64-bit; and `LARGE_INTEGER` is an i64 passed by value or by pointer. Modules
are under `library/std/os/platform/windows/`.

### kernel32.dll

| Area (module under `win32/`) | Functions |
|---|---|
| Files (`file_handles.sn`, `file_paths.sn`, `directory_streams.sn`) | `GetStdHandle`, `CreateFileW`, `ReadFile`, `WriteFile`, `CloseHandle`, `FlushFileBuffers`, `GetFileTime`, `GetFileSizeEx`, `SetFilePointerEx`, `CreateDirectoryW`, `DeleteFileW`, `GetFileAttributesW`, `GetFileAttributesExW`, `RemoveDirectoryW`, `MoveFileExW`, `FindFirstFileW`, `FindNextFileW`, `FindClose`. Files are opened with `FILE_SHARE_READ \| WRITE \| DELETE`, Rust's default share mode, so another process can open (and lock) a file this one has open. |
| Links and attributes (`file_attributes.sn`) | `BOOLEAN CreateSymbolicLinkW(LPCWSTR, LPCWSTR, DWORD)`, `CreateHardLinkW`, `SetFileAttributesW`, `DeviceIoControl` (`FSCTL_GET_REPARSE_POINT`), `SetFileInformationByHandle` (`FileEndOfFileInfo`), `SetFileTime`, `GetFinalPathNameByHandleW` |
| File locks (`file_locks.sn`) | `LockFileEx`, `UnlockFileEx` over the whole file |
| Processes (`process_control.sn`) | `CreateProcessW`, `WaitForSingleObject`, `GetExitCodeProcess`, `TerminateProcess`, `ExitProcess`, `GetCommandLineW`, `LocalFree`, `GetProcessId` (a child's ID). try_wait is `WaitForSingleObject(h, 0)` and kill is `TerminateProcess(h, 1)`. |
| Child stdio (`child_stdio.sn`) | `CreatePipe`, `PeekNamedPipe`, `DuplicateHandle` (inheritable copies for `STARTF_USESTDHANDLES`), and `WriteFile` to a child's input, with `CreateProcessW`'s environment block (`CREATE_UNICODE_ENVIRONMENT`) and `lpCurrentDirectory`. The child inherits only those three copies, named in a `PROC_THREAD_ATTRIBUTE_HANDLE_LIST` (`STARTUPINFOEXW`, `EXTENDED_STARTUPINFO_PRESENT`), as Go's `syscall.StartProcess` does: `InitializeProcThreadAttributeList`, `UpdateProcThreadAttribute`, `DeleteProcThreadAttributeList`, with the list and the handle array it points to in one `HeapAlloc` block (`GetProcessHeap`, `HeapFree`). The array is copied in with `RtlMoveMemory` because the list keeps its address until it is deleted. |
| Memory (`virtual_memory.sn`) | `VirtualAlloc`, `VirtualFree`, `VirtualProtect`, `VirtualLock`, `VirtualUnlock`, `GetSystemInfo`, `GetCurrentProcess`, `PrefetchVirtualMemory` (Windows 8+) |
| Time (`clocks.sn`) | `GetSystemTimePreciseAsFileTime`, `QueryPerformanceCounter`, `QueryPerformanceFrequency`, `GetTickCount64`, `Sleep`, `CreateWaitableTimerW`, `SetWaitableTimer` |
| Environment (`environment.sn`, `environment_block.sn`) | `GetEnvironmentVariableW`, `SetEnvironmentVariableW`, `GetCurrentDirectoryW`, `GetTempPathW`, `SetCurrentDirectoryW`, `GetEnvironmentStringsW`, `FreeEnvironmentStringsW` |
| Errors (`error.sn`) | `GetLastError`, `SetLastError` |
| Threads (`threads.sn`, as Rust's std) | `CreateThread` (2 MiB `STACK_SIZE_PARAM_IS_A_RESERVATION`; the start routine is a closure's C entry), `WaitForSingleObject`, `GetExitCodeThread`, `CloseHandle`, `SwitchToThread`, `GetCurrentThreadId`, `GetSystemInfo` (`dwNumberOfProcessors`) |

### Other DLLs

| DLL | Functions (module under `win32/` or `winsock/`) |
|---|---|
| `ws2_32.dll` | `winsock/sockets.sn`: `socket` (returns `SOCKET`, u64; `AF_UNIX` 1 for `std.net.unix`, stream sockets on Windows 10 1803+), `closesocket`, `connect`, `bind`, `listen`, `send` and `recv` (`int` lengths), `accept`, `shutdown`, `ioctlsocket(SOCKET, long, u_long *)`, `setsockopt`, `getsockopt`, `getsockname`, `getpeername`, `sendto`, `recvfrom`, `WSAStartup`, `WSACleanup`, `WSAGetLastError`. `winsock/name_resolution.sn`: `GetAddrInfoW`, `FreeAddrInfoW`. These are native Winsock functions, not the CRT's `getaddrinfo`. `ADDRINFOW` puts `ai_canonname` before `ai_addr`. |
| `api-ms-win-core-synch-l1-2-0.dll` | `wait_address.sn`: `BOOL WaitOnAddress(volatile VOID *, PVOID, SIZE_T, DWORD)`, `VOID WakeByAddressSingle(PVOID)`, `VOID WakeByAddressAll(PVOID)` (Windows 8+). This is the synchronization API set that Rust's std links for its futex layer, used by `std.os.wait_address` and everything in `std.sync`. |
| `shell32.dll` | `CommandLineToArgvW(LPCWSTR, int *)` gives the standard Windows argument splitting, and std frees its single allocation with `LocalFree`. Rust's std and Go parse `GetCommandLineW` in their own code instead, so this import could be replaced by SultanC code. |
| `bcryptprimitives.dll` | `BOOL ProcessPrng(PBYTE, SIZE_T)`, the system CSPRNG that Rust's std (1.78+) and Go's runtime use |
| `crypt32.dll` | `certificates.sn`: `CertOpenSystemStoreW(HCRYPTPROV_LEGACY, LPCWSTR)`, `PCCERT_CONTEXT CertEnumCertificatesInStore(HCERTSTORE, PCCERT_CONTEXT)` and `CertCloseStore(HCERTSTORE, DWORD)`. They read the ROOT trust store that TLS verification uses, as Go's crypto/x509 and rustls-native-certs do. `CERT_CONTEXT` is declared as a record and its DER bytes are read through its pointer inside `غير_آمن`. |

## WASI preview 1

WASI preview 1 is the system interface of a `wasm32-wasi` module. Every function is imported from
the module `wasi_snapshot_preview1`, as Rust's `wasi` crate does with
`#[link(wasm_import_module = "wasi_snapshot_preview1")]`. Signatures follow the witx definitions:
handles, sizes and pointers are u32 on wasm32, flags are u16 or u8, offsets and rights are u64, and
every function returns `errno` as i32. Modules are under `library/std/os/platform/wasi/preview1/`.

| Module | Functions |
|---|---|
| `file_descriptors.sn` | `fd_read`, `fd_write`, `fd_close`, `fd_seek(fd, filedelta i64, whence u8, *filesize)`, `fd_sync`, `fd_prestat_get`, `fd_prestat_dir_name`, `path_open(fd, lookupflags u32, path, path_len, oflags u16, rights u64, rights u64, fdflags u16, *fd)`, `path_filestat_get`, `path_unlink_file`, `path_rename`, `path_create_directory`, `path_remove_directory`, `fd_readdir(fd, buf, len, cookie u64, *bufused)` |
| `sockets.sn` | `sock_accept(fd, fdflags u16, *fd)`, `sock_recv`, `sock_send`, `sock_shutdown(fd, sdflags u8)` |
| `clocks.sn` | `clock_time_get(clockid u32, precision u64, *timestamp)`, `poll_oneoff` |
| `process_startup.sn` | `args_sizes_get`, `args_get`, `environ_sizes_get`, `environ_get` |
| `file_attributes.sn` | `path_symlink(old_path, old_len, fd, new_path, new_len)`, `path_readlink(fd, path, path_len, buf, buf_len, *bufused)`, `path_link(old_fd, old_flags u32, old_path, old_len, new_fd, new_path, new_len)`, `fd_filestat_set_size(fd, size u64)`, `path_filestat_set_times(fd, flags u32, path, path_len, atim u64, mtim u64, fst_flags u16)`. Exclusive creation is `path_open` with oflags `creat\|excl`. |
| `process_control.sn`, `random.sn` | `proc_exit(u32)`, `random_get(buf, len)` |

WASI has no permission bits, working directory, processes, pipes, unconnected sockets, file locks
or Unix domain sockets. Those std functions report ENOTSUP (58).

## What std implements in SultanC

None of these call the platform.

| Capability | Where | Rust or Go equivalent |
|---|---|---|
| Heap allocation | `os/allocation.sn`. The allocator gets pages from `mmap`, `VirtualAlloc` or wasm `memory.grow` and never calls `malloc`. | Go's runtime allocator. Rust's default is the system malloc. |
| Memory copy and fill | Compiled code. The LLVM provider emits `llvm.memcpy.inline` so that no call to libc `memcpy` appears ([MSIR and code generation](../internals/msir-and-codegen.md)). | Go's runtime `memmove`. Rust uses compiler-builtins `mem` on no-libc targets. |
| Text formatting and parsing (numbers, radix, UTF-8) | `std/text/**`, `std/unicode` | Go `strconv`, Rust `core::fmt` |
| Floating-point and integer math (`sqrt`, `hypot`, …) | `std/math/**` | Go `math` is pure Go. Rust calls libm. |
| Cryptography and TLS 1.3 | `std/crypto/**`, `std/tls/**` ([TLS](../internals/tls.md)) | Go `crypto/tls` |
| DNS on Linux | `os/platform/unix/linux/name_resolution.sn` implements the DNS protocol over UDP and TCP to the configured nameserver. It does not use the libc resolver. | Go's pure-Go resolver |
| Process start-up on Unix | `_start` and the argument and environment vectors come from the backend (`sultanc.bootstrap.*` data symbols), with no crt1.o | Go's `rt0` |
| Compression, JSON, TOML, CSV, HTTP, URI, UUID | `std/**` | |

A grep confirms that no libc or CRT language function is imported. Among the externs declared
under `library/`, none is `memcpy`, `memmove`, `memset`, `memcmp`, `strlen`, `strcmp`, `printf`,
`snprintf`, `puts`, `malloc`, `calloc`, `realloc`, `free`, `sin`, `cos`, `sqrt`, `pow`, `exp`,
`log`, `floor` or `fmod`, and none comes from a C runtime DLL (`ucrtbase`, `msvcrt`, `vcruntime`).

```sh
grep -rnE '(خارجي|extern)' library | grep -wE 'memcpy|memmove|memset|strlen|printf|malloc|free|sin|cos|sqrt|pow'
```

The command prints nothing. The import tables of built examples agree: a Windows executable
imports only `kernel32`, `ws2_32`, `crypt32`, `shell32` and `bcryptprimitives` functions.

## Dependencies outside std

Two dependencies remain outside the rule above, and neither is std.

- The C Stage0 bootstrap compiler (`bootstrap/`) is a C program. The Stage1 it produces uses the
  host C library (`malloc`, `memcpy`); Stage2 and later stages do not.
- The compiler's LLVM provider imports the LLVM C API (`compiler/backend/provider/llvm/ffi`),
  including `LLVMBuildFPExt`, `LLVMBuildFPTrunc` and the `LLVMBuildFPToSI` family, and uses the
  `llvm.fptosi.sat` and `llvm.fptoui.sat` intrinsics.

## Limitations

- `shell32.dll`'s `CommandLineToArgvW` could be replaced by SultanC code, which would remove that
  import.
- `ve-linux-gnu` has no std OS layer; see [Targets](targets.md).
- Applications can still request libc through `-l c` on ELF targets, but std itself never does.

## See also

- [Targets](targets.md): the target matrix and how each target is linked and verified.
- [Concurrency](concurrency.md): the wait/wake and thread layers built on these calls.
- [MSIR and code generation](../internals/msir-and-codegen.md): how declarations reach the backends.
- [Standard library coverage](std-coverage.md): what std provides on top of these interfaces.
