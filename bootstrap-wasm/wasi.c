/* WASI Preview 1 host for the translated SultanC seed (see wasm2c.c).
 * Copyright 2026 SultanC contributors. MIT license (see repository LICENSE).
 *
 * The host half of the seed bootstrap: wasm2c turns the seed into C that
 * calls the wasi_snapshot_preview1_* functions below, and this file
 * implements them on the host OS. It is a trusted build tool, not a sandbox:
 * the compiler runs with the invoking user's permissions.
 *
 * Hosts: every POSIX system (Linux, macOS, FreeBSD, NetBSD, OpenBSD,
 * DragonFly, illumos/Solaris, Haiku, Cygwin, ...) and Windows (MSVC, MinGW,
 * clang). One portable WASI layer sits on a small OS layer with two
 * implementations, `#if defined(_WIN32)` and POSIX. It is C99 and needs no
 * libraries beyond the C runtime (on Windows, kernel32 only; advapi32 and
 * shell32 are loaded at run time).
 *
 * Preopens (what std's WASI provider resolves paths against):
 *   fd 3 = "."  the current working directory
 *   fd 4 = "/"  the root (POSIX only)
 * std chooses the longest matching preopen and passes a path relative to it.
 * On Windows an absolute path such as C:\x or \\server\share arrives as a
 * path relative to "." and is used as is.
 *
 * Every guest pointer is bounds-checked against linear memory: an invalid
 * pointer returns WASI_EFAULT instead of touching host memory.
 *
 * Native stack: each nested guest call is one C frame. The guest runs with a
 * known native stack (Windows: a thread with a 256 MiB reservation; Linux: the
 * main thread with its soft limit raised to 64 MiB where the hard limit allows;
 * other POSIX: the main thread's limit), and the translated code checks it
 * (w2c_limit_native_stack), so deep recursion traps instead of crashing.
 */
#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <wchar.h>
#else
#if !defined(_POSIX_C_SOURCE)
#define _POSIX_C_SOURCE 200809L
#endif
/* 32-bit hosts: 64-bit file offsets, inode numbers and times. Without this,
 * stat() fails with EOVERFLOW on files with large inode numbers. */
#if !defined(_FILE_OFFSET_BITS)
#define _FILE_OFFSET_BITS 64
#endif
#if !defined(_TIME_BITS)
#define _TIME_BITS 64
#endif
#if defined(__APPLE__)
#define _DARWIN_C_SOURCE 1
#endif
#if defined(__NetBSD__)
#define _NETBSD_SOURCE 1
#endif
#if defined(__sun)
#define __EXTENSIONS__ 1
#endif
#define _DEFAULT_SOURCE 1
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#endif

#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern uint8_t *w2c_memory;
extern uint64_t w2c_memory_size;
extern void w2c_run(void);
extern void w2c_limit_native_stack(size_t budget);

enum {
    WASI_ESUCCESS = 0, WASI_E2BIG = 1, WASI_EACCES = 2, WASI_EAGAIN = 6, WASI_EBADF = 8,
    WASI_EBUSY = 10, WASI_EEXIST = 20, WASI_EFAULT = 21, WASI_EINTR = 27, WASI_EINVAL = 28,
    WASI_EIO = 29, WASI_EISDIR = 31, WASI_ELOOP = 32, WASI_EMFILE = 33, WASI_EMLINK = 34,
    WASI_ENAMETOOLONG = 37, WASI_ENFILE = 41, WASI_ENOENT = 44, WASI_ENOMEM = 48,
    WASI_ENOSPC = 51, WASI_ENOSYS = 52, WASI_ENOTDIR = 54, WASI_ENOTEMPTY = 55,
    WASI_ENOTSUP = 58, WASI_EOVERFLOW = 61, WASI_EPERM = 63, WASI_EPIPE = 64, WASI_EROFS = 69,
    WASI_ESPIPE = 70, WASI_EXDEV = 75, WASI_ENOTCAPABLE = 76
};

enum {
    WASI_FILETYPE_UNKNOWN = 0, WASI_FILETYPE_BLOCK_DEVICE = 1, WASI_FILETYPE_CHARACTER_DEVICE = 2,
    WASI_FILETYPE_DIRECTORY = 3, WASI_FILETYPE_REGULAR_FILE = 4, WASI_FILETYPE_SYMBOLIC_LINK = 7
};

typedef struct {
    uint64_t dev, ino, nlink, size, atim, mtim, ctim;
    uint8_t filetype;
} FileStat;

/* Open flags translated from WASI (oflags/fdflags/rights/lookupflags). */
typedef struct {
    int read, write, create, exclusive, truncate, append, follow;
} OpenFlags;

/* ============================================================================
 * OS layer: Windows
 * ========================================================================== */
#if defined(_WIN32)

typedef HANDLE HostFile;
#define HOST_FILE_NONE INVALID_HANDLE_VALUE

static uint32_t win_error(DWORD e) {
    switch (e) {
        case ERROR_SUCCESS: return WASI_ESUCCESS;
        case ERROR_FILE_NOT_FOUND: case ERROR_PATH_NOT_FOUND: case ERROR_INVALID_DRIVE:
        case ERROR_BAD_NETPATH: case ERROR_BAD_NET_NAME: case ERROR_INVALID_NAME: return WASI_ENOENT;
        case ERROR_ACCESS_DENIED: case ERROR_SHARING_VIOLATION: case ERROR_LOCK_VIOLATION:
        case ERROR_CURRENT_DIRECTORY: return WASI_EACCES;
        case ERROR_WRITE_PROTECT: return WASI_EROFS;
        case ERROR_FILE_EXISTS: case ERROR_ALREADY_EXISTS: return WASI_EEXIST;
        case ERROR_DIR_NOT_EMPTY: return WASI_ENOTEMPTY;
        case ERROR_DIRECTORY: return WASI_ENOTDIR;
        case ERROR_DISK_FULL: case ERROR_HANDLE_DISK_FULL: return WASI_ENOSPC;
        case ERROR_NOT_SAME_DEVICE: return WASI_EXDEV;
        case ERROR_FILENAME_EXCED_RANGE: return WASI_ENAMETOOLONG;
        case ERROR_INVALID_HANDLE: return WASI_EBADF;
        case ERROR_NOT_ENOUGH_MEMORY: case ERROR_OUTOFMEMORY: return WASI_ENOMEM;
        case ERROR_TOO_MANY_OPEN_FILES: return WASI_EMFILE;
        case ERROR_BROKEN_PIPE: case ERROR_NO_DATA: return WASI_EPIPE;
        case ERROR_NEGATIVE_SEEK: case ERROR_INVALID_PARAMETER: return WASI_EINVAL;
        case ERROR_CALL_NOT_IMPLEMENTED: case ERROR_NOT_SUPPORTED: return WASI_ENOTSUP;
        default: return WASI_EIO;
    }
}

/* UTF-8 -> UTF-16 (caller frees). Forward slashes are kept: Win32 accepts both. */
static wchar_t *win_wide(const char *s) {
    int n = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, s, -1, NULL, 0);
    wchar_t *w;
    if (n <= 0) return NULL;
    w = (wchar_t *)malloc((size_t)n * sizeof(wchar_t));
    if (w && MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, s, -1, w, n) != n) { free(w); w = NULL; }
    return w;
}
/* UTF-16 -> UTF-8 (caller frees). */
static char *win_utf8(const wchar_t *w) {
    int n = WideCharToMultiByte(CP_UTF8, 0, w, -1, NULL, 0, NULL, NULL);
    char *s;
    if (n <= 0) return NULL;
    s = (char *)malloc((size_t)n);
    if (s && WideCharToMultiByte(CP_UTF8, 0, w, -1, s, n, NULL, NULL) != n) { free(s); s = NULL; }
    return s;
}

static uint64_t win_time_ns(FILETIME ft) {
    uint64_t t = (uint64_t)ft.dwHighDateTime << 32 | ft.dwLowDateTime; /* 100 ns since 1601 */
    const uint64_t epoch = UINT64_C(116444736000000000);
    return t > epoch ? (t - epoch) * 100u : 0;
}

static uint32_t win_stat_handle(HANDLE h, FileStat *st) {
    BY_HANDLE_FILE_INFORMATION info;
    DWORD type = GetFileType(h);
    memset(st, 0, sizeof(*st));
    if (type == FILE_TYPE_CHAR || type == FILE_TYPE_PIPE) {
        st->filetype = WASI_FILETYPE_CHARACTER_DEVICE;
        st->nlink = 1;
        return WASI_ESUCCESS;
    }
    if (!GetFileInformationByHandle(h, &info)) return win_error(GetLastError());
    st->dev = info.dwVolumeSerialNumber;
    st->ino = (uint64_t)info.nFileIndexHigh << 32 | info.nFileIndexLow;
    st->nlink = info.nNumberOfLinks;
    st->size = (uint64_t)info.nFileSizeHigh << 32 | info.nFileSizeLow;
    st->atim = win_time_ns(info.ftLastAccessTime);
    st->mtim = win_time_ns(info.ftLastWriteTime);
    st->ctim = win_time_ns(info.ftLastWriteTime);
    if (info.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) st->filetype = WASI_FILETYPE_SYMBOLIC_LINK;
    else if (info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) st->filetype = WASI_FILETYPE_DIRECTORY;
    else st->filetype = WASI_FILETYPE_REGULAR_FILE;
    return WASI_ESUCCESS;
}

static uint32_t os_stat_path(const char *path, int follow, FileStat *st) {
    wchar_t *w = win_wide(path);
    HANDLE h;
    uint32_t r;
    if (!w) return WASI_EINVAL;
    h = CreateFileW(w, 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING,
                    FILE_FLAG_BACKUP_SEMANTICS | (follow ? 0 : FILE_FLAG_OPEN_REPARSE_POINT), NULL);
    free(w);
    if (h == INVALID_HANDLE_VALUE) return win_error(GetLastError());
    r = win_stat_handle(h, st);
    CloseHandle(h);
    if (r == WASI_ESUCCESS && follow && st->filetype == WASI_FILETYPE_SYMBOLIC_LINK)
        st->filetype = WASI_FILETYPE_REGULAR_FILE;
    return r;
}
static uint32_t os_stat_file(HostFile f, FileStat *st) { return win_stat_handle(f, st); }

static int os_is_directory(const char *path) {
    wchar_t *w = win_wide(path);
    DWORD a;
    if (!w) return 0;
    a = GetFileAttributesW(w);
    free(w);
    return a != INVALID_FILE_ATTRIBUTES && (a & FILE_ATTRIBUTE_DIRECTORY);
}

static uint32_t os_open(const char *path, const OpenFlags *fl, HostFile *out) {
    wchar_t *w = win_wide(path);
    DWORD access = 0, disposition;
    HANDLE h;
    if (!w) return WASI_EINVAL;
    if (fl->read) access |= GENERIC_READ;
    if (fl->append) access |= FILE_APPEND_DATA;
    else if (fl->write) access |= GENERIC_WRITE;
    if (!access) access = GENERIC_READ;
    if (fl->create && fl->exclusive) disposition = CREATE_NEW;
    else if (fl->create && fl->truncate) disposition = CREATE_ALWAYS;
    else if (fl->create) disposition = OPEN_ALWAYS;
    else if (fl->truncate) disposition = TRUNCATE_EXISTING;
    else disposition = OPEN_EXISTING;
    h = CreateFileW(w, access, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, disposition,
                    FILE_ATTRIBUTE_NORMAL | (fl->follow ? 0 : FILE_FLAG_OPEN_REPARSE_POINT), NULL);
    free(w);
    if (h == INVALID_HANDLE_VALUE) return win_error(GetLastError());
    *out = h;
    return WASI_ESUCCESS;
}
static void os_close(HostFile f) { CloseHandle(f); }

static uint32_t os_read(HostFile f, void *buf, uint32_t len, uint32_t *n) {
    DWORD got = 0;
    if (!ReadFile(f, buf, len, &got, NULL)) {
        DWORD e = GetLastError();
        if (e == ERROR_BROKEN_PIPE || e == ERROR_HANDLE_EOF) { *n = 0; return WASI_ESUCCESS; }
        return win_error(e);
    }
    *n = got;
    return WASI_ESUCCESS;
}
static uint32_t os_write(HostFile f, const void *buf, uint32_t len, uint32_t *n) {
    DWORD put = 0;
    if (!WriteFile(f, buf, len, &put, NULL)) return win_error(GetLastError());
    *n = put;
    return WASI_ESUCCESS;
}
static uint32_t os_seek(HostFile f, int64_t off, int whence, uint64_t *result) {
    LARGE_INTEGER d, r;
    DWORD how = whence == 0 ? FILE_BEGIN : whence == 1 ? FILE_CURRENT : FILE_END;
    DWORD type = GetFileType(f);
    if (type == FILE_TYPE_CHAR || type == FILE_TYPE_PIPE) return WASI_ESPIPE;
    d.QuadPart = off;
    if (!SetFilePointerEx(f, d, &r, how)) return win_error(GetLastError());
    *result = (uint64_t)r.QuadPart;
    return WASI_ESUCCESS;
}
static uint32_t os_sync(HostFile f) {
    DWORD type = GetFileType(f);
    if (type != FILE_TYPE_DISK) return WASI_ESUCCESS;
    return FlushFileBuffers(f) ? WASI_ESUCCESS : win_error(GetLastError());
}

static uint32_t os_path_op(int op, const char *a, const char *b) {
    wchar_t *wa = win_wide(a), *wb = b ? win_wide(b) : NULL;
    BOOL ok = FALSE;
    if (!wa || (b && !wb)) { free(wa); free(wb); return WASI_EINVAL; }
    switch (op) {
        case 0: ok = CreateDirectoryW(wa, NULL); break;
        case 1: ok = RemoveDirectoryW(wa); break;
        case 2: ok = DeleteFileW(wa); break;
        case 3: ok = MoveFileExW(wa, wb, MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED); break;
    }
    free(wa);
    free(wb);
    return ok ? WASI_ESUCCESS : win_error(GetLastError());
}

typedef struct { HANDLE h; WIN32_FIND_DATAW data; int first, done; } HostDir;

static uint32_t os_dir_open(const char *path, HostDir *d) {
    size_t n = strlen(path);
    char *pattern = (char *)malloc(n + 3);
    wchar_t *w;
    if (!pattern) return WASI_ENOMEM;
    memcpy(pattern, path, n);
    if (n && (path[n - 1] == '/' || path[n - 1] == '\\')) memcpy(pattern + n, "*", 2);
    else memcpy(pattern + n, "/*", 3);
    w = win_wide(pattern);
    free(pattern);
    if (!w) return WASI_EINVAL;
    d->h = FindFirstFileW(w, &d->data);
    free(w);
    if (d->h == INVALID_HANDLE_VALUE) {
        DWORD e = GetLastError();
        if (e == ERROR_FILE_NOT_FOUND) { d->done = 1; return WASI_ESUCCESS; }
        return win_error(e);
    }
    d->first = 1;
    d->done = 0;
    return WASI_ESUCCESS;
}
/* Next entry: UTF-8 name (caller frees), WASI file type, inode (0 = unknown). 0 at the end. */
static int os_dir_next(HostDir *d, const char *dir_path, char **name, uint8_t *type, uint64_t *ino) {
    (void)dir_path;
    if (d->done) return 0;
    if (!d->first && !FindNextFileW(d->h, &d->data)) { d->done = 1; return 0; }
    d->first = 0;
    *name = win_utf8(d->data.cFileName);
    if (!*name) return 0;
    if (d->data.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) *type = WASI_FILETYPE_SYMBOLIC_LINK;
    else if (d->data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) *type = WASI_FILETYPE_DIRECTORY;
    else *type = WASI_FILETYPE_REGULAR_FILE;
    *ino = 0;
    return 1;
}
static void os_dir_close(HostDir *d) {
    if (d->h != INVALID_HANDLE_VALUE) FindClose(d->h);
    d->h = INVALID_HANDLE_VALUE;
}

static uint32_t os_clock(uint32_t id, uint64_t *ns) {
    if (id == 0) {
        typedef VOID(WINAPI * PreciseFn)(LPFILETIME);
        static PreciseFn precise;
        static int looked;
        FILETIME ft;
        if (!looked) {
            HMODULE k = GetModuleHandleA("kernel32.dll");
            looked = 1;
            if (k) precise = (PreciseFn)(void (*)(void))GetProcAddress(k, "GetSystemTimePreciseAsFileTime");
        }
        if (precise) precise(&ft); else GetSystemTimeAsFileTime(&ft);
        *ns = win_time_ns(ft);
        return WASI_ESUCCESS;
    }
    if (id == 1) {
        LARGE_INTEGER c, f;
        if (!QueryPerformanceCounter(&c) || !QueryPerformanceFrequency(&f) || f.QuadPart <= 0) return WASI_EIO;
        *ns = (uint64_t)(c.QuadPart / f.QuadPart) * 1000000000u +
              (uint64_t)(c.QuadPart % f.QuadPart) * 1000000000u / (uint64_t)f.QuadPart;
        return WASI_ESUCCESS;
    }
    if (id == 2 || id == 3) {
        FILETIME a, b, k, u;
        BOOL ok = id == 2 ? GetProcessTimes(GetCurrentProcess(), &a, &b, &k, &u)
                          : GetThreadTimes(GetCurrentThread(), &a, &b, &k, &u);
        if (!ok) return win_error(GetLastError());
        *ns = (((uint64_t)k.dwHighDateTime << 32 | k.dwLowDateTime) +
               ((uint64_t)u.dwHighDateTime << 32 | u.dwLowDateTime)) * 100u;
        return WASI_ESUCCESS;
    }
    return WASI_EINVAL;
}

static uint32_t os_random(uint8_t *buf, uint32_t len) {
    /* RtlGenRandom (advapi32 SystemFunction036), loaded at run time. */
    typedef BOOLEAN(WINAPI * GenFn)(PVOID, ULONG);
    static GenFn gen;
    if (!gen) {
        HMODULE a = LoadLibraryA("advapi32.dll");
        if (a) gen = (GenFn)(void (*)(void))GetProcAddress(a, "SystemFunction036");
        if (!gen) return WASI_ENOSYS;
    }
    return gen(buf, len) ? WASI_ESUCCESS : WASI_EIO;
}

static void os_sleep_ns(uint64_t ns) {
    uint64_t ms = (ns + 999999u) / 1000000u;
    while (ms > 0) {
        DWORD step = ms > 0x7fffffffu ? 0x7fffffffu : (DWORD)ms;
        Sleep(step);
        ms -= step;
    }
}

/* Absolute Windows paths sent relative to the "." preopen (see top). */
static int os_absolute_guest_path(const char *p, size_t n) {
    if (n >= 3 && ((p[0] >= 'A' && p[0] <= 'Z') || (p[0] >= 'a' && p[0] <= 'z')) && p[1] == ':' &&
        (p[2] == '/' || p[2] == '\\'))
        return 1;
    return n >= 2 && (p[0] == '\\' || p[0] == '/') && (p[1] == '\\' || p[1] == '/');
}

/* Program arguments and environment as UTF-8 (argv from main is in the ANSI code page). */
static char **os_args(int argc, char **argv, int *count) {
    typedef LPWSTR *(WINAPI * SplitFn)(LPCWSTR, int *);
    HMODULE s = LoadLibraryA("shell32.dll");
    SplitFn split = s ? (SplitFn)(void (*)(void))GetProcAddress(s, "CommandLineToArgvW") : NULL;
    LPWSTR *wargv;
    char **out;
    int n, i;
    if (!split || !(wargv = split(GetCommandLineW(), &n))) { *count = argc; return argv; }
    out = (char **)calloc((size_t)n + 1, sizeof(char *));
    if (!out) { *count = argc; return argv; }
    for (i = 0; i < n; i++) {
        out[i] = win_utf8(wargv[i]);
        if (!out[i]) { *count = argc; return argv; }
    }
    LocalFree(wargv);
    *count = n;
    return out;
}
static char **os_environ(int *count) {
    LPWCH block = GetEnvironmentStringsW(), p;
    char **out;
    int n = 0, i = 0;
    if (!block) { *count = 0; return NULL; }
    for (p = block; *p; p += wcslen(p) + 1) if (*p != L'=') n++; /* skip "=C:=C:\" drive entries */
    out = (char **)calloc((size_t)n + 1, sizeof(char *));
    if (!out) { FreeEnvironmentStringsW(block); *count = 0; return NULL; }
    for (p = block; *p; p += wcslen(p) + 1) {
        if (*p == L'=') continue;
        out[i] = win_utf8(p);
        if (out[i]) i++;
    }
    FreeEnvironmentStringsW(block);
    *count = i;
    return out;
}

static HostFile os_std_handle(int i) {
    return GetStdHandle(i == 0 ? STD_INPUT_HANDLE : i == 1 ? STD_OUTPUT_HANDLE : STD_ERROR_HANDLE);
}
static void os_startup(void) {
    /* Text written by the compiler is UTF-8 (Arabic diagnostics). */
    SetConsoleOutputCP(CP_UTF8);
}

/* The main thread's stack is the image's reserve (1 MiB with link.exe), too
 * small for the compiler: run the guest on a thread with its own stack. */
#if defined(_WIN64)
#define GUEST_STACK_BYTES ((size_t)256 << 20)
#else
#define GUEST_STACK_BYTES ((size_t)64 << 20)
#endif
static DWORD WINAPI guest_thread(LPVOID unused) {
    (void)unused;
    w2c_limit_native_stack(GUEST_STACK_BYTES);
    w2c_run();
    return 0;
}
static void os_run_guest(void) {
    HANDLE t = CreateThread(NULL, GUEST_STACK_BYTES, guest_thread, NULL, STACK_SIZE_PARAM_IS_A_RESERVATION, NULL);
    if (!t) {
        /* No thread: stay on the main thread with a conservative bound. */
        w2c_limit_native_stack((size_t)512 << 10);
        w2c_run();
        return;
    }
    WaitForSingleObject(t, INFINITE);
    CloseHandle(t);
}

/* ============================================================================
 * OS layer: POSIX
 * ========================================================================== */
#else

typedef int HostFile;
#define HOST_FILE_NONE (-1)

extern char **environ;

static uint32_t posix_error(int e) {
    switch (e) {
        case 0: return WASI_ESUCCESS;
        case E2BIG: return WASI_E2BIG;
        case EACCES: return WASI_EACCES;
        case EAGAIN: return WASI_EAGAIN;
        case EBADF: return WASI_EBADF;
        case EBUSY: return WASI_EBUSY;
        case EEXIST: return WASI_EEXIST;
        case EFAULT: return WASI_EFAULT;
        case EINTR: return WASI_EINTR;
        case EINVAL: return WASI_EINVAL;
        case EIO: return WASI_EIO;
        case EISDIR: return WASI_EISDIR;
        case ELOOP: return WASI_ELOOP;
        case EMFILE: return WASI_EMFILE;
        case EMLINK: return WASI_EMLINK;
        case ENAMETOOLONG: return WASI_ENAMETOOLONG;
        case ENFILE: return WASI_ENFILE;
        case ENOENT: return WASI_ENOENT;
        case ENOMEM: return WASI_ENOMEM;
        case ENOSPC: return WASI_ENOSPC;
        case ENOSYS: return WASI_ENOSYS;
        case ENOTDIR: return WASI_ENOTDIR;
#if defined(ENOTEMPTY) && ENOTEMPTY != EEXIST
        case ENOTEMPTY: return WASI_ENOTEMPTY;
#endif
#if defined(ENOTSUP)
        case ENOTSUP: return WASI_ENOTSUP;
#endif
#if defined(EOPNOTSUPP) && (!defined(ENOTSUP) || EOPNOTSUPP != ENOTSUP)
        case EOPNOTSUPP: return WASI_ENOTSUP;
#endif
        case EOVERFLOW: return WASI_EOVERFLOW;
        case EPERM: return WASI_EPERM;
        case EPIPE: return WASI_EPIPE;
        case EROFS: return WASI_EROFS;
        case ESPIPE: return WASI_ESPIPE;
        case EXDEV: return WASI_EXDEV;
        default: return WASI_EIO;
    }
}

/* Nanosecond timestamps: POSIX.1-2008 st_*tim everywhere except Apple. */
#if defined(__APPLE__)
#define POSIX_NS(st, f) ((uint64_t)(st).f##timespec.tv_sec * 1000000000u + (uint64_t)(st).f##timespec.tv_nsec)
#else
#define POSIX_NS(st, f) ((uint64_t)(st).f##tim.tv_sec * 1000000000u + (uint64_t)(st).f##tim.tv_nsec)
#endif

static uint8_t posix_filetype(mode_t mode) {
    if (S_ISREG(mode)) return WASI_FILETYPE_REGULAR_FILE;
    if (S_ISDIR(mode)) return WASI_FILETYPE_DIRECTORY;
    if (S_ISLNK(mode)) return WASI_FILETYPE_SYMBOLIC_LINK;
    if (S_ISCHR(mode)) return WASI_FILETYPE_CHARACTER_DEVICE;
    if (S_ISBLK(mode)) return WASI_FILETYPE_BLOCK_DEVICE;
    return WASI_FILETYPE_UNKNOWN;
}
static void posix_fill(const struct stat *s, FileStat *st) {
    st->dev = (uint64_t)s->st_dev;
    st->ino = (uint64_t)s->st_ino;
    st->nlink = (uint64_t)s->st_nlink;
    st->size = (uint64_t)s->st_size;
    st->atim = POSIX_NS(*s, st_a);
    st->mtim = POSIX_NS(*s, st_m);
    st->ctim = POSIX_NS(*s, st_c);
    st->filetype = posix_filetype(s->st_mode);
}

static uint32_t os_stat_path(const char *path, int follow, FileStat *st) {
    struct stat s;
    if ((follow ? stat(path, &s) : lstat(path, &s)) != 0) return posix_error(errno);
    posix_fill(&s, st);
    return WASI_ESUCCESS;
}
static uint32_t os_stat_file(HostFile f, FileStat *st) {
    struct stat s;
    if (fstat(f, &s) != 0) return posix_error(errno);
    posix_fill(&s, st);
    return WASI_ESUCCESS;
}
static int os_is_directory(const char *path) {
    struct stat s;
    return stat(path, &s) == 0 && S_ISDIR(s.st_mode);
}

static uint32_t os_open(const char *path, const OpenFlags *fl, HostFile *out) {
    int flags = fl->read && (fl->write || fl->append) ? O_RDWR : (fl->write || fl->append) ? O_WRONLY : O_RDONLY;
    int fd;
    if (fl->create) flags |= O_CREAT;
    if (fl->exclusive) flags |= O_EXCL;
    if (fl->truncate) flags |= O_TRUNC;
    if (fl->append) flags |= O_APPEND;
#if defined(O_NOFOLLOW)
    if (!fl->follow) flags |= O_NOFOLLOW;
#endif
#if defined(O_CLOEXEC)
    flags |= O_CLOEXEC;
#endif
#if defined(O_BINARY)
    flags |= O_BINARY; /* Cygwin, MSYS */
#endif
    do fd = open(path, flags, 0666); while (fd < 0 && errno == EINTR);
    if (fd < 0) return posix_error(errno);
    *out = fd;
    return WASI_ESUCCESS;
}
static void os_close(HostFile f) { close(f); }

static uint32_t os_read(HostFile f, void *buf, uint32_t len, uint32_t *n) {
    ssize_t r;
    if (len > (uint32_t)INT_MAX) len = (uint32_t)INT_MAX;
    do r = read(f, buf, (size_t)len); while (r < 0 && errno == EINTR);
    if (r < 0) return posix_error(errno);
    *n = (uint32_t)r;
    return WASI_ESUCCESS;
}
static uint32_t os_write(HostFile f, const void *buf, uint32_t len, uint32_t *n) {
    ssize_t r;
    if (len > (uint32_t)INT_MAX) len = (uint32_t)INT_MAX;
    do r = write(f, buf, (size_t)len); while (r < 0 && errno == EINTR);
    if (r < 0) return posix_error(errno);
    *n = (uint32_t)r;
    return WASI_ESUCCESS;
}
static uint32_t os_seek(HostFile f, int64_t off, int whence, uint64_t *result) {
    off_t r;
    if ((int64_t)(off_t)off != off) return WASI_EOVERFLOW; /* 32-bit off_t hosts */
    r = lseek(f, (off_t)off, whence == 0 ? SEEK_SET : whence == 1 ? SEEK_CUR : SEEK_END);
    if (r < 0) return posix_error(errno);
    *result = (uint64_t)r;
    return WASI_ESUCCESS;
}
static uint32_t os_sync(HostFile f) {
    if (fsync(f) == 0) return WASI_ESUCCESS;
    /* Pipes, terminals and some special files cannot be synced. */
    return errno == EINVAL || errno == EROFS || errno == ENOTSUP ? WASI_ESUCCESS : posix_error(errno);
}

static uint32_t os_path_op(int op, const char *a, const char *b) {
    int r = -1;
    switch (op) {
        case 0: r = mkdir(a, 0777); break;
        case 1: r = rmdir(a); break;
        case 2: r = unlink(a); break;
        case 3: r = rename(a, b); break;
    }
    return r == 0 ? WASI_ESUCCESS : posix_error(errno);
}

typedef struct { DIR *dir; } HostDir;

static uint32_t os_dir_open(const char *path, HostDir *d) {
    d->dir = opendir(path);
    return d->dir ? WASI_ESUCCESS : posix_error(errno);
}
static int os_dir_next(HostDir *d, const char *dir_path, char **name, uint8_t *type, uint64_t *ino) {
    struct dirent *e;
    size_t dl, nl;
    char *full;
    struct stat s;
    if (!d->dir) return 0;
    e = readdir(d->dir);
    if (!e) return 0;
    nl = strlen(e->d_name);
    *name = (char *)malloc(nl + 1);
    if (!*name) return 0;
    memcpy(*name, e->d_name, nl + 1);
    *ino = (uint64_t)e->d_ino;
    *type = WASI_FILETYPE_UNKNOWN;
    /* d_type is not POSIX; lstat gives the type portably. */
    dl = strlen(dir_path);
    full = (char *)malloc(dl + nl + 2);
    if (full) {
        memcpy(full, dir_path, dl);
        full[dl] = '/';
        memcpy(full + dl + 1, e->d_name, nl + 1);
        if (lstat(full, &s) == 0) *type = posix_filetype(s.st_mode);
        free(full);
    }
    return 1;
}
static void os_dir_close(HostDir *d) {
    if (d->dir) closedir(d->dir);
    d->dir = NULL;
}

static uint32_t os_clock(uint32_t id, uint64_t *ns) {
    struct timespec ts;
    clockid_t c;
    switch (id) {
        case 0: c = CLOCK_REALTIME; break;
        case 1:
#if defined(CLOCK_MONOTONIC)
            c = CLOCK_MONOTONIC;
#else
            c = CLOCK_REALTIME;
#endif
            break;
#if defined(CLOCK_PROCESS_CPUTIME_ID)
        case 2: c = CLOCK_PROCESS_CPUTIME_ID; break;
#endif
#if defined(CLOCK_THREAD_CPUTIME_ID)
        case 3: c = CLOCK_THREAD_CPUTIME_ID; break;
#endif
        default: return WASI_EINVAL;
    }
    if (clock_gettime(c, &ts) != 0) return posix_error(errno);
    *ns = (uint64_t)ts.tv_sec * 1000000000u + (uint64_t)ts.tv_nsec;
    return WASI_ESUCCESS;
}

static uint32_t os_random(uint8_t *buf, uint32_t len) {
    int fd;
    uint32_t done = 0;
    do fd = open("/dev/urandom", O_RDONLY); while (fd < 0 && errno == EINTR);
    if (fd < 0) return posix_error(errno);
    while (done < len) {
        ssize_t n = read(fd, buf + done, (size_t)(len - done));
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) { close(fd); return WASI_EIO; }
        done += (uint32_t)n;
    }
    close(fd);
    return WASI_ESUCCESS;
}

static void os_sleep_ns(uint64_t ns) {
    struct timespec req;
    req.tv_sec = (time_t)(ns / 1000000000u);
    req.tv_nsec = (long)(ns % 1000000000u);
    while (nanosleep(&req, &req) != 0 && errno == EINTR) {}
}

static int os_absolute_guest_path(const char *p, size_t n) { (void)p; (void)n; return 0; }

static char **os_args(int argc, char **argv, int *count) { *count = argc; return argv; }
static char **os_environ(int *count) {
    int n = 0;
    while (environ && environ[n]) n++;
    *count = n;
    return environ;
}
static HostFile os_std_handle(int i) { return i; }
static void os_startup(void) {}

/* The guest runs on the main thread. Linux grows the main stack up to the
 * current soft limit, so raise it to 64 MiB (as the native compiler does);
 * elsewhere the stack was sized at exec time and the limit is what it is. */
#define GUEST_STACK_BYTES ((size_t)64 << 20)
static void os_run_guest(void) {
    size_t budget = (size_t)8 << 20;
    struct rlimit rl;
    if (getrlimit(RLIMIT_STACK, &rl) == 0) {
#if defined(__linux__)
        if (rl.rlim_cur != RLIM_INFINITY && rl.rlim_cur < (rlim_t)GUEST_STACK_BYTES) {
            struct rlimit raised = rl;
            raised.rlim_cur = (rl.rlim_max == RLIM_INFINITY || rl.rlim_max >= (rlim_t)GUEST_STACK_BYTES)
                                  ? (rlim_t)GUEST_STACK_BYTES : rl.rlim_max;
            if (setrlimit(RLIMIT_STACK, &raised) == 0) rl = raised;
        }
#endif
        if (rl.rlim_cur == RLIM_INFINITY || rl.rlim_cur > (rlim_t)GUEST_STACK_BYTES) budget = GUEST_STACK_BYTES;
        else budget = (size_t)rl.rlim_cur;
    }
    w2c_limit_native_stack(budget);
    w2c_run();
}

#endif

/* ============================================================================
 * WASI layer (portable)
 * ========================================================================== */

/* ---- guest memory --------------------------------------------------------------- */

static uint8_t *guest(uint32_t addr, uint64_t len) {
    if ((uint64_t)addr + len > w2c_memory_size) return NULL;
    return w2c_memory + addr;
}
static int get32(uint32_t addr, uint32_t *v) {
    uint8_t *p = guest(addr, 4);
    if (!p) return 0;
    *v = (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
    return 1;
}
static int put32(uint32_t addr, uint32_t v) {
    uint8_t *p = guest(addr, 4);
    unsigned i;
    if (!p) return 0;
    for (i = 0; i < 4; i++) p[i] = (uint8_t)(v >> (8 * i));
    return 1;
}
static void put64_at(uint8_t *p, uint64_t v) {
    unsigned i;
    for (i = 0; i < 8; i++) p[i] = (uint8_t)(v >> (8 * i));
}
static int put64(uint32_t addr, uint64_t v) {
    uint8_t *p = guest(addr, 8);
    if (!p) return 0;
    put64_at(p, v);
    return 1;
}

/* ---- descriptor table ---------------------------------------------------------- */

/* One directory entry of a cached listing (fd_readdir). */
typedef struct {
    char *name;
    uint64_t ino;
    uint8_t type;
} DirEntry;

typedef struct {
    int active;
    int is_dir;
    HostFile file;  /* files and standard streams */
    char *dir_path; /* host path (UTF-8) for directories */
    char *preopen;  /* guest-visible preopen name, or NULL */
    DirEntry *entries; /* directories: listing taken by fd_readdir at cookie 0 */
    uint32_t entry_count;
    int listed;
} Descriptor;

static Descriptor *fds;
static uint32_t fd_count;

static Descriptor *lookup(uint32_t fd) {
    if (fd >= fd_count || !fds[fd].active) return NULL;
    return &fds[fd];
}

static uint32_t add_descriptor(Descriptor d, uint32_t *out) {
    uint32_t i;
    for (i = 3; i < fd_count; i++)
        if (!fds[i].active) break;
    if (i == fd_count) {
        Descriptor *next;
        if (fd_count >= 1u << 20) return WASI_EMFILE;
        next = (Descriptor *)realloc(fds, ((size_t)fd_count + 1) * sizeof(*fds));
        if (!next) return WASI_ENOMEM;
        fds = next;
        fd_count++;
    }
    d.active = 1;
    fds[i] = d;
    *out = i;
    return WASI_ESUCCESS;
}

/* Host path for `len` guest bytes at `ptr`, relative to directory fd `dirfd`. */
static char *resolve(uint32_t dirfd, uint32_t ptr, uint32_t len, uint32_t *err) {
    Descriptor *d = lookup(dirfd);
    const uint8_t *src;
    size_t root, n;
    char *path;
    if (!d) { *err = WASI_EBADF; return NULL; }
    if (!d->is_dir) { *err = WASI_ENOTDIR; return NULL; }
    if (len > 1u << 20) { *err = WASI_ENAMETOOLONG; return NULL; }
    src = guest(ptr, len);
    if (!src) { *err = WASI_EFAULT; return NULL; }
    if (len && memchr(src, 0, len)) { *err = WASI_EINVAL; return NULL; }
    if (os_absolute_guest_path((const char *)src, len)) {
        path = (char *)malloc((size_t)len + 1);
        if (!path) { *err = WASI_ENOMEM; return NULL; }
        memcpy(path, src, len);
        path[len] = 0;
        return path;
    }
    /* std's resolver never sends an absolute POSIX path; refuse one rather
     * than let it escape the directory it was resolved against. */
    if (len && src[0] == '/') { *err = WASI_ENOTCAPABLE; return NULL; }
    root = strlen(d->dir_path);
    n = root + 1 + len + 1;
    path = (char *)malloc(n);
    if (!path) { *err = WASI_ENOMEM; return NULL; }
    memcpy(path, d->dir_path, root);
    if (root == 0 || path[root - 1] != '/') path[root++] = '/';
    memcpy(path + root, src, len);
    path[root + len] = 0;
    return path;
}

/* filestat: dev @0, ino @8, filetype u8 @16, nlink @24, size @32, atim @40, mtim @48, ctim @56. */
static uint32_t write_filestat(uint32_t addr, const FileStat *st) {
    uint8_t *p = guest(addr, 64);
    if (!p) return WASI_EFAULT;
    memset(p, 0, 64);
    put64_at(p + 0, st->dev);
    put64_at(p + 8, st->ino);
    p[16] = st->filetype;
    put64_at(p + 24, st->nlink);
    put64_at(p + 32, st->size);
    put64_at(p + 40, st->atim);
    put64_at(p + 48, st->mtim);
    put64_at(p + 56, st->ctim);
    return WASI_ESUCCESS;
}

/* ---- arguments and environment -------------------------------------------------- */

static char **host_argv, **host_env;
static int host_argc, host_envc;

static uint32_t strings_sizes(char **v, int count, uint32_t count_ptr, uint32_t size_ptr) {
    uint64_t total = 0;
    int i;
    for (i = 0; i < count; i++) total += strlen(v[i]) + 1;
    if (total > UINT32_MAX) return WASI_EOVERFLOW;
    if (!put32(count_ptr, (uint32_t)count) || !put32(size_ptr, (uint32_t)total)) return WASI_EFAULT;
    return WASI_ESUCCESS;
}
static uint32_t strings_get(char **v, int count, uint32_t ptrs, uint32_t buf) {
    int i;
    for (i = 0; i < count; i++) {
        size_t n = strlen(v[i]) + 1;
        uint8_t *dst = guest(buf, n);
        if (!dst || !put32(ptrs + 4u * (uint32_t)i, buf)) return WASI_EFAULT;
        memcpy(dst, v[i], n);
        buf += (uint32_t)n;
    }
    return WASI_ESUCCESS;
}

uint32_t wasi_snapshot_preview1_args_sizes_get(uint32_t count_ptr, uint32_t size_ptr) {
    return strings_sizes(host_argv, host_argc, count_ptr, size_ptr);
}
uint32_t wasi_snapshot_preview1_args_get(uint32_t ptrs, uint32_t buf) {
    return strings_get(host_argv, host_argc, ptrs, buf);
}
uint32_t wasi_snapshot_preview1_environ_sizes_get(uint32_t count_ptr, uint32_t size_ptr) {
    return strings_sizes(host_env, host_envc, count_ptr, size_ptr);
}
uint32_t wasi_snapshot_preview1_environ_get(uint32_t ptrs, uint32_t buf) {
    return strings_get(host_env, host_envc, ptrs, buf);
}

/* ---- descriptors ----------------------------------------------------------------- */

uint32_t wasi_snapshot_preview1_fd_read(uint32_t fd, uint32_t iovs, uint32_t count, uint32_t nread_ptr) {
    Descriptor *d = lookup(fd);
    uint64_t total = 0;
    uint32_t i;
    if (!d) return WASI_EBADF;
    if (d->is_dir) return WASI_EISDIR;
    for (i = 0; i < count; i++) {
        uint32_t buf, len, n = 0, r;
        uint8_t *dst;
        if (!get32(iovs + 8 * i, &buf) || !get32(iovs + 8 * i + 4, &len)) return WASI_EFAULT;
        dst = guest(buf, len);
        if (!dst) return WASI_EFAULT;
        if (!len) continue;
        r = os_read(d->file, dst, len, &n);
        if (r) return r;
        total += n;
        if (n < len) break;
    }
    if (total > UINT32_MAX) return WASI_EOVERFLOW;
    return put32(nread_ptr, (uint32_t)total) ? WASI_ESUCCESS : WASI_EFAULT;
}

uint32_t wasi_snapshot_preview1_fd_write(uint32_t fd, uint32_t iovs, uint32_t count, uint32_t nwritten_ptr) {
    Descriptor *d = lookup(fd);
    uint64_t total = 0;
    uint32_t i;
    if (!d) return WASI_EBADF;
    if (d->is_dir) return WASI_EISDIR;
    for (i = 0; i < count; i++) {
        uint32_t buf, len;
        const uint8_t *src;
        if (!get32(iovs + 8 * i, &buf) || !get32(iovs + 8 * i + 4, &len)) return WASI_EFAULT;
        src = guest(buf, len);
        if (!src) return WASI_EFAULT;
        while (len) {
            uint32_t n = 0, r = os_write(d->file, src, len, &n);
            if (r) return r;
            if (n == 0) return WASI_EIO;
            src += n;
            len -= n;
            total += n;
        }
    }
    if (total > UINT32_MAX) return WASI_EOVERFLOW;
    return put32(nwritten_ptr, (uint32_t)total) ? WASI_ESUCCESS : WASI_EFAULT;
}

static void free_listing(Descriptor *d) {
    uint32_t i;
    for (i = 0; i < d->entry_count; i++) free(d->entries[i].name);
    free(d->entries);
    d->entries = NULL;
    d->entry_count = 0;
    d->listed = 0;
}

uint32_t wasi_snapshot_preview1_fd_close(uint32_t fd) {
    Descriptor *d = lookup(fd);
    if (!d) return WASI_EBADF;
    if (!d->is_dir && fd > 2) os_close(d->file);
    free_listing(d);
    free(d->dir_path);
    free(d->preopen);
    memset(d, 0, sizeof(*d));
    return WASI_ESUCCESS;
}

uint32_t wasi_snapshot_preview1_fd_sync(uint32_t fd) {
    Descriptor *d = lookup(fd);
    if (!d) return WASI_EBADF;
    return d->is_dir ? WASI_ESUCCESS : os_sync(d->file);
}
uint32_t wasi_snapshot_preview1_fd_datasync(uint32_t fd) { return wasi_snapshot_preview1_fd_sync(fd); }

uint32_t wasi_snapshot_preview1_fd_seek(uint32_t fd, uint64_t offset, uint32_t whence, uint32_t result_ptr) {
    Descriptor *d = lookup(fd);
    uint64_t pos = 0;
    uint32_t r;
    if (!d) return WASI_EBADF;
    if (d->is_dir) return WASI_EISDIR;
    if (whence > 2) return WASI_EINVAL;
    /* offset is a signed 64-bit value carried in an unsigned slot. */
    r = os_seek(d->file, (int64_t)offset, (int)whence, &pos);
    if (r) return r;
    return put64(result_ptr, pos) ? WASI_ESUCCESS : WASI_EFAULT;
}
uint32_t wasi_snapshot_preview1_fd_tell(uint32_t fd, uint32_t result_ptr) {
    return wasi_snapshot_preview1_fd_seek(fd, 0, 1, result_ptr);
}

uint32_t wasi_snapshot_preview1_fd_filestat_get(uint32_t fd, uint32_t stat_ptr) {
    Descriptor *d = lookup(fd);
    FileStat st;
    uint32_t r;
    if (!d) return WASI_EBADF;
    r = d->is_dir ? os_stat_path(d->dir_path, 1, &st) : os_stat_file(d->file, &st);
    return r ? r : write_filestat(stat_ptr, &st);
}

/* prestat: tag u8 @0 (0 = directory), name_len u32 @4. */
uint32_t wasi_snapshot_preview1_fd_prestat_get(uint32_t fd, uint32_t prestat_ptr) {
    Descriptor *d = lookup(fd);
    uint8_t *p;
    if (!d || !d->preopen) return WASI_EBADF;
    p = guest(prestat_ptr, 8);
    if (!p) return WASI_EFAULT;
    memset(p, 0, 8);
    put32(prestat_ptr + 4, (uint32_t)strlen(d->preopen));
    return WASI_ESUCCESS;
}
uint32_t wasi_snapshot_preview1_fd_prestat_dir_name(uint32_t fd, uint32_t path_ptr, uint32_t path_len) {
    Descriptor *d = lookup(fd);
    size_t n;
    uint8_t *dst;
    if (!d || !d->preopen) return WASI_EBADF;
    n = strlen(d->preopen);
    if (path_len < n) return WASI_ENAMETOOLONG;
    dst = guest(path_ptr, n);
    if (!dst) return WASI_EFAULT;
    memcpy(dst, d->preopen, n);
    return WASI_ESUCCESS;
}

/* Directory listings are sorted by name (bytewise, as strcmp), so a listing
 * is the same on every host and file system, and the compiler's output never
 * depends on host directory order. */
static int compare_entries(const void *a, const void *b) {
    return strcmp(((const DirEntry *)a)->name, ((const DirEntry *)b)->name);
}

/* Reads the whole directory into d->entries, sorted. */
static uint32_t take_listing(Descriptor *d) {
    HostDir dir;
    uint32_t r, cap = 0;
    char *name;
    uint8_t type;
    uint64_t ino;
    free_listing(d);
    r = os_dir_open(d->dir_path, &dir);
    if (r) return r;
    while (os_dir_next(&dir, d->dir_path, &name, &type, &ino)) {
        if (d->entry_count == cap) {
            DirEntry *next;
            if (cap >= 1u << 24) { free(name); os_dir_close(&dir); free_listing(d); return WASI_ENOMEM; }
            cap = cap ? cap * 2 : 64;
            next = (DirEntry *)realloc(d->entries, (size_t)cap * sizeof(*next));
            if (!next) { free(name); os_dir_close(&dir); free_listing(d); return WASI_ENOMEM; }
            d->entries = next;
        }
        d->entries[d->entry_count].name = name;
        d->entries[d->entry_count].ino = ino;
        d->entries[d->entry_count].type = type;
        d->entry_count++;
    }
    os_dir_close(&dir);
    if (d->entry_count > 1) qsort(d->entries, d->entry_count, sizeof(*d->entries), compare_entries);
    d->listed = 1;
    return WASI_ESUCCESS;
}

/* dirent: d_next u64 @0, d_ino u64 @8, d_namlen u32 @16, d_type u8 @20, name; header 24 bytes.
 * WASI semantics: whenever more entries remain, fill the buffer completely,
 * truncating the last entry; a shorter result means the end was reached.
 * Cookie 0 takes a fresh listing of the directory and caches it on the
 * descriptor; cookie N resumes after the N-th entry of that listing, so a
 * listing read in many calls costs one directory scan (O(n), not O(n^2)). */
uint32_t wasi_snapshot_preview1_fd_readdir(uint32_t fd, uint32_t buf, uint32_t buf_len, uint64_t cookie,
                                           uint32_t used_ptr) {
    Descriptor *d = lookup(fd);
    uint8_t *out;
    uint64_t index;
    uint32_t used = 0, r;
    if (!d) return WASI_EBADF;
    if (!d->is_dir) return WASI_ENOTDIR;
    out = guest(buf, buf_len);
    if (!out) return WASI_EFAULT;
    if (cookie == 0 || !d->listed) {
        r = take_listing(d);
        if (r) return r;
    }
    for (index = cookie; index < d->entry_count && used < buf_len; index++) {
        const DirEntry *e = &d->entries[index];
        uint8_t header[24];
        size_t name_len = strlen(e->name);
        uint32_t take;
        put64_at(header, index + 1);
        put64_at(header + 8, e->ino);
        header[16] = (uint8_t)name_len;
        header[17] = (uint8_t)(name_len >> 8);
        header[18] = (uint8_t)(name_len >> 16);
        header[19] = (uint8_t)(name_len >> 24);
        header[20] = e->type;
        header[21] = header[22] = header[23] = 0;
        take = buf_len - used < 24 ? buf_len - used : 24;
        memcpy(out + used, header, take);
        used += take;
        if (take == 24) {
            take = buf_len - used < name_len ? buf_len - used : (uint32_t)name_len;
            memcpy(out + used, e->name, take);
            used += take;
        }
    }
    return put32(used_ptr, used) ? WASI_ESUCCESS : WASI_EFAULT;
}

/* ---- paths ------------------------------------------------------------------------ */

/* oflags: creat 1, directory 2, excl 4, trunc 8. fdflags: append 1.
 * rights: fd_read 1<<1, fd_write 1<<6. lookupflags: symlink_follow 1. */
uint32_t wasi_snapshot_preview1_path_open(uint32_t dirfd, uint32_t lookupflags, uint32_t path_ptr,
                                          uint32_t path_len, uint32_t oflags, uint64_t rights_base,
                                          uint64_t rights_inheriting, uint32_t fdflags, uint32_t fd_ptr) {
    uint32_t err = 0, result;
    char *path = resolve(dirfd, path_ptr, path_len, &err);
    int want_dir = (oflags & 2) != 0;
    Descriptor nd;
    OpenFlags fl;
    (void)rights_inheriting;
    if (!path) return err;
    memset(&nd, 0, sizeof(nd));
    if (!want_dir && !(oflags & 1) && os_is_directory(path)) want_dir = 1;
    if (want_dir) {
        FileStat st;
        err = os_stat_path(path, (lookupflags & 1) != 0, &st);
        if (err) { free(path); return err; }
        if (st.filetype != WASI_FILETYPE_DIRECTORY) { free(path); return WASI_ENOTDIR; }
        nd.is_dir = 1;
        nd.file = HOST_FILE_NONE;
        nd.dir_path = path;
        err = add_descriptor(nd, &result);
        if (err) { free(path); return err; }
        return put32(fd_ptr, result) ? WASI_ESUCCESS : WASI_EFAULT;
    }
    fl.read = (rights_base & (UINT64_C(1) << 1)) != 0;
    fl.write = (rights_base & (UINT64_C(1) << 6)) != 0;
    fl.create = (oflags & 1) != 0;
    fl.exclusive = (oflags & 4) != 0;
    fl.truncate = (oflags & 8) != 0;
    fl.append = (fdflags & 1) != 0;
    fl.follow = (lookupflags & 1) != 0;
    err = os_open(path, &fl, &nd.file);
    free(path);
    if (err) return err;
    err = add_descriptor(nd, &result);
    if (err) { os_close(nd.file); return err; }
    return put32(fd_ptr, result) ? WASI_ESUCCESS : WASI_EFAULT;
}

uint32_t wasi_snapshot_preview1_path_filestat_get(uint32_t dirfd, uint32_t lookupflags, uint32_t path_ptr,
                                                  uint32_t path_len, uint32_t stat_ptr) {
    uint32_t err = 0;
    char *path = resolve(dirfd, path_ptr, path_len, &err);
    FileStat st;
    if (!path) return err;
    err = os_stat_path(path, (lookupflags & 1) != 0, &st);
    free(path);
    return err ? err : write_filestat(stat_ptr, &st);
}

static uint32_t path_call(int op, uint32_t dirfd, uint32_t ptr, uint32_t len) {
    uint32_t err = 0;
    char *path = resolve(dirfd, ptr, len, &err);
    if (!path) return err;
    err = os_path_op(op, path, NULL);
    free(path);
    return err;
}
uint32_t wasi_snapshot_preview1_path_create_directory(uint32_t dirfd, uint32_t ptr, uint32_t len) {
    return path_call(0, dirfd, ptr, len);
}
uint32_t wasi_snapshot_preview1_path_remove_directory(uint32_t dirfd, uint32_t ptr, uint32_t len) {
    return path_call(1, dirfd, ptr, len);
}
uint32_t wasi_snapshot_preview1_path_unlink_file(uint32_t dirfd, uint32_t ptr, uint32_t len) {
    return path_call(2, dirfd, ptr, len);
}
uint32_t wasi_snapshot_preview1_path_rename(uint32_t from_dir, uint32_t from_ptr, uint32_t from_len,
                                            uint32_t to_dir, uint32_t to_ptr, uint32_t to_len) {
    uint32_t err = 0;
    char *from = resolve(from_dir, from_ptr, from_len, &err), *to;
    if (!from) return err;
    to = resolve(to_dir, to_ptr, to_len, &err);
    if (!to) { free(from); return err; }
    err = os_path_op(3, from, to);
    free(from);
    free(to);
    return err;
}

/* ---- clocks, randomness, polling, exit ---------------------------------------------- */

uint32_t wasi_snapshot_preview1_clock_time_get(uint32_t id, uint64_t precision, uint32_t time_ptr) {
    uint64_t ns = 0;
    uint32_t r;
    (void)precision;
    r = os_clock(id, &ns);
    if (r) return r;
    return put64(time_ptr, ns) ? WASI_ESUCCESS : WASI_EFAULT;
}

uint32_t wasi_snapshot_preview1_random_get(uint32_t buf, uint32_t len) {
    uint8_t *dst = guest(buf, len);
    if (!dst) return WASI_EFAULT;
    return len ? os_random(dst, len) : WASI_ESUCCESS;
}

/* subscription (48 bytes): userdata u64 @0, tag u8 @8; clock: id u32 @16,
 * timeout u64 @24, precision u64 @32, flags u16 @40 (1 = absolute).
 * event (32 bytes): userdata u64 @0, error u16 @8, type u8 @10.
 * Clock subscriptions sleep until the earliest deadline; descriptor
 * subscriptions are reported ready at once (files are always ready). */
uint32_t wasi_snapshot_preview1_poll_oneoff(uint32_t in, uint32_t out, uint32_t n, uint32_t nevents_ptr) {
    uint8_t *subs = guest(in, (uint64_t)n * 48), *evs = guest(out, (uint64_t)n * 32);
    uint64_t best = UINT64_MAX;
    uint32_t i, ready = 0, best_i = 0;
    int have_fd = 0;
    if (!subs || !evs) return WASI_EFAULT;
    if (n == 0) return WASI_EINVAL;
    for (i = 0; i < n; i++) {
        const uint8_t *s = subs + 48u * i;
        if (s[8] == 0) {
            uint64_t timeout = 0, now = 0;
            uint32_t clk = (uint32_t)s[16] | (uint32_t)s[17] << 8 | (uint32_t)s[18] << 16 | (uint32_t)s[19] << 24;
            unsigned b, flags = (unsigned)s[40] | (unsigned)s[41] << 8;
            for (b = 0; b < 8; b++) timeout |= (uint64_t)s[24 + b] << (8 * b);
            if (flags & 1) {
                if (os_clock(clk, &now)) return WASI_EINVAL;
                timeout = timeout > now ? timeout - now : 0;
            }
            if (timeout < best) { best = timeout; best_i = i; }
        } else {
            have_fd = 1;
        }
    }
    if (!have_fd && best != UINT64_MAX) os_sleep_ns(best);
    for (i = 0; i < n; i++) {
        const uint8_t *s = subs + 48u * i;
        uint8_t *e = evs + 32u * ready;
        if (s[8] == 0 && (have_fd || i != best_i)) continue;
        memset(e, 0, 32);
        memcpy(e, s, 8);
        e[10] = s[8];
        ready++;
    }
    return put32(nevents_ptr, ready) ? WASI_ESUCCESS : WASI_EFAULT;
}

void wasi_snapshot_preview1_proc_exit(uint32_t code) {
    fflush(stdout);
    fflush(stderr);
    exit((int)code);
}

/* ---- startup ------------------------------------------------------------------------ */

static void preopen(uint32_t fd, const char *host_dir, const char *guest_name) {
    size_t a = strlen(host_dir) + 1, b = strlen(guest_name) + 1;
    fds[fd].active = 1;
    fds[fd].is_dir = 1;
    fds[fd].file = HOST_FILE_NONE;
    fds[fd].dir_path = (char *)malloc(a);
    fds[fd].preopen = (char *)malloc(b);
    if (!fds[fd].dir_path || !fds[fd].preopen) {
        fputs("sultanc (wasm bootstrap): out of memory\n", stderr);
        exit(1);
    }
    memcpy(fds[fd].dir_path, host_dir, a);
    memcpy(fds[fd].preopen, guest_name, b);
}

int main(int argc, char **argv) {
    int i;
    os_startup();
    host_argv = os_args(argc, argv, &host_argc);
    host_env = os_environ(&host_envc);
#if defined(_WIN32)
    fd_count = 4;
#else
    fd_count = 5;
#endif
    fds = (Descriptor *)calloc(fd_count, sizeof(*fds));
    if (!fds) {
        fputs("sultanc (wasm bootstrap): out of memory\n", stderr);
        return 1;
    }
    for (i = 0; i < 3; i++) {
        fds[i].active = 1;
        fds[i].file = os_std_handle(i);
    }
    preopen(3, ".", ".");
#if !defined(_WIN32)
    preopen(4, "/", "/");
#endif
    os_run_guest();
    fflush(stdout);
    return 0;
}
