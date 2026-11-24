// freebsd_abi_emulator: run a static x86-64 FreeBSD executable built by
// SultanC on Linux x86-64, translating each FreeBSD system call to the Linux
// one with ptrace (tools/tests/freebsd_examples.sh). A test harness, not
// FreeBSD: it restates the FreeBSD ABI facts std relies on (system-call
// numbers, flag values, struct stat and struct dirent layouts, the
// carry-flag error convention, `_start`'s %rdi argument block) independently
// of std, so that std's FreeBSD code paths run. It covers the calls std
// issues and nothing else; an unexpected call is reported on stderr.
// usage: freebsd_abi_emulator [-v] <freebsd-exe> [args...]
#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ptrace.h>
#include <sys/uio.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/mman.h>
#include <time.h>

static int verbose;

typedef struct { pid_t pid; int in_call; long fnr; unsigned long a[6]; long aux; int freebsd; int fresh; } proc_t;
static proc_t procs[64];

static proc_t *P(pid_t pid) {
    for (int i = 0; i < 64; i++) if (procs[i].pid == pid) return &procs[i];
    for (int i = 0; i < 64; i++) if (procs[i].pid == 0) { memset(&procs[i], 0, sizeof procs[i]); procs[i].pid = pid; procs[i].freebsd = 1; procs[i].fresh = 1; return &procs[i]; }
    fprintf(stderr, "freebsd_abi_emulator: too many processes\n"); exit(99);
}

static void rd(pid_t pid, unsigned long addr, void *buf, size_t n) {
    struct iovec l = { buf, n }, r = { (void *)addr, n };
    if (process_vm_readv(pid, &l, 1, &r, 1, 0) != (ssize_t)n) { memset(buf, 0, n); }
}
static void wr(pid_t pid, unsigned long addr, const void *buf, size_t n) {
    struct iovec l = { (void *)buf, n }, r = { (void *)addr, n };
    if (process_vm_writev(pid, &l, 1, &r, 1, 0) != (ssize_t)n) fprintf(stderr, "freebsd_abi_emulator: write failed\n");
}

// FreeBSD sys/sys/fcntl.h -> Linux open flags.
static unsigned long open_flags(unsigned long f) {
    unsigned long o = f & 3;
    if (f & 0x0004) o |= O_NONBLOCK;
    if (f & 0x0008) o |= O_APPEND;
    if (f & 0x0200) o |= O_CREAT;
    if (f & 0x0400) o |= O_TRUNC;
    if (f & 0x0800) o |= O_EXCL;
    if (f & 0x20000) o |= O_DIRECTORY;
    if (f & 0x100000) o |= O_CLOEXEC;
    if (f & ~(0x3UL | 0x4 | 0x8 | 0x200 | 0x400 | 0x800 | 0x20000 | 0x100000)) fprintf(stderr, "freebsd_abi_emulator: open flags %#lx\n", f);
    return o;
}
static unsigned long open_flags_back(unsigned long o) {
    unsigned long f = o & 3;
    if (o & O_NONBLOCK) f |= 4;
    if (o & O_APPEND) f |= 8;
    return f;
}
static unsigned long sock_type(unsigned long t) {
    unsigned long o = t & 0xff;
    if (t & 0x10000000) o |= SOCK_CLOEXEC;
    if (t & 0x20000000) o |= SOCK_NONBLOCK;
    return o;
}
static int errno_back(int e) {   // Linux errno -> FreeBSD sys/sys/errno.h
    switch (e) {
    case EAGAIN: return 35; case EDEADLK: return 11; case ENAMETOOLONG: return 63; case ENOSYS: return 78;
    case ENOTEMPTY: return 66; case ELOOP: return 62; case EINPROGRESS: return 36; case EALREADY: return 37;
    case ENOTSOCK: return 38; case EADDRINUSE: return 48; case EADDRNOTAVAIL: return 49; case ENETUNREACH: return 51;
    case ECONNRESET: return 54; case EISCONN: return 56; case ENOTCONN: return 57; case ETIMEDOUT: return 60;
    case ECONNREFUSED: return 61; case EHOSTUNREACH: return 65; case EAFNOSUPPORT: return 47; case EOPNOTSUPP: return 45;
    case EPROTONOSUPPORT: return 43; case ERANGE: return 34; default: return e <= 34 ? e : 1000 + e;
    }
}

// Linux struct stat (asm/stat.h, x86-64) -> FreeBSD struct stat (sys/sys/stat.h, ino64).
static void convert_stat(pid_t pid, unsigned long buf) {
    struct stat ls; rd(pid, buf, &ls, sizeof ls);
    unsigned char f[224]; memset(f, 0, sizeof f);
    uint64_t v;
    v = ls.st_dev; memcpy(f + 0, &v, 8); v = ls.st_ino; memcpy(f + 8, &v, 8); v = ls.st_nlink; memcpy(f + 16, &v, 8);
    uint16_t m = ls.st_mode; memcpy(f + 24, &m, 2);
    uint32_t u = ls.st_uid; memcpy(f + 28, &u, 4); u = ls.st_gid; memcpy(f + 32, &u, 4);
    v = ls.st_rdev; memcpy(f + 40, &v, 8);
    int64_t t;
    t = ls.st_atim.tv_sec; memcpy(f + 48, &t, 8); t = ls.st_atim.tv_nsec; memcpy(f + 56, &t, 8);
    t = ls.st_mtim.tv_sec; memcpy(f + 64, &t, 8); t = ls.st_mtim.tv_nsec; memcpy(f + 72, &t, 8);
    t = ls.st_ctim.tv_sec; memcpy(f + 80, &t, 8); t = ls.st_ctim.tv_nsec; memcpy(f + 88, &t, 8);
    t = ls.st_size; memcpy(f + 112, &t, 8); t = ls.st_blocks; memcpy(f + 120, &t, 8);
    int32_t b = ls.st_blksize; memcpy(f + 128, &b, 4);
    wr(pid, buf, f, sizeof f);
}

// linux_dirent64 records -> FreeBSD struct dirent records (d_name at 24).
static long convert_dirents(pid_t pid, unsigned long buf, long n, unsigned long cap) {
    unsigned char *in = malloc(n), *out = calloc(1, cap);
    rd(pid, buf, in, n);
    long ip = 0, op = 0;
    while (ip < n) {
        uint64_t ino, off; uint16_t reclen; uint8_t type;
        memcpy(&ino, in + ip, 8); memcpy(&off, in + ip + 8, 8); memcpy(&reclen, in + ip + 16, 2); type = in[ip + 18];
        const char *name = (const char *)in + ip + 19; size_t len = strlen(name);
        size_t rec = (24 + len + 1 + 7) & ~7UL;
        if (op + rec > cap) { fprintf(stderr, "freebsd_abi_emulator: dirent overflow\n"); break; }
        uint16_t r16 = rec, nl = len;
        memcpy(out + op, &ino, 8); memcpy(out + op + 8, &off, 8); memcpy(out + op + 16, &r16, 2);
        out[op + 18] = type; memcpy(out + op + 20, &nl, 2); memcpy(out + op + 24, name, len + 1);
        op += rec; ip += reclen;
    }
    wr(pid, buf, out, op);
    free(in); free(out);
    return op;
}

// BSD sockaddr (sin_len, sin_family u8) -> Linux (sa_family u16); returns the original two bytes.
static unsigned short fix_sockaddr(pid_t pid, unsigned long addr) {
    unsigned char h[2]; rd(pid, addr, h, 2);
    unsigned short fam = h[1] == 28 ? AF_INET6 : h[1];
    unsigned char l[2] = { fam & 0xff, fam >> 8 };
    wr(pid, addr, l, 2);
    return h[0] | (h[1] << 8);
}

static long sockopt_level(long l) { return l == 0xffff ? SOL_SOCKET : l; }
static long sockopt_name(long l, long n) {
    if (l == 0xffff) switch (n) {
        case 4: return SO_REUSEADDR; case 8: return SO_KEEPALIVE; case 0x1001: return SO_SNDBUF;
        case 0x1002: return SO_RCVBUF; case 0x1005: return SO_SNDTIMEO; case 0x1006: return SO_RCVTIMEO;
        case 0x1007: return SO_ERROR; }
    if (l == 0 && n == 4) return 2;      // IP_TTL
    if (l == 41 && n == 4) return 16;    // IPV6_UNICAST_HOPS
    return n;
}

enum { POST_NONE, POST_STAT, POST_DIRENT, POST_GETCWD, POST_SYSCTL, POST_FCNTL_GETFL, POST_SOCKADDR };

static void on_entry(pid_t pid, proc_t *p, struct user_regs_struct *r) {
    long n = r->orig_rax; unsigned long *a = p->a;
    a[0] = r->rdi; a[1] = r->rsi; a[2] = r->rdx; a[3] = r->r10; a[4] = r->r8; a[5] = r->r9;
    p->fnr = n; p->aux = POST_NONE;
    long ln = -1;
    switch (n) {
    case 3: ln = SYS_read; break;
    case 4: ln = SYS_write; break;
    case 6: ln = SYS_close; break;
    case 499: ln = SYS_openat; r->rdx = open_flags(a[2]); break;
    case 552: ln = SYS_newfstatat; if (a[3] & ~0x200UL) fprintf(stderr, "freebsd_abi_emulator: fstatat flag\n"); r->r10 = (a[3] & 0x200) ? AT_SYMLINK_NOFOLLOW : 0; p->aux = POST_STAT; break;
    case 554: ln = SYS_getdents64; r->rdx = a[2] / 2; p->aux = POST_DIRENT; if (a[3]) fprintf(stderr, "freebsd_abi_emulator: basep\n"); break;
    case 496: ln = SYS_mkdirat; break;
    case 503: ln = SYS_unlinkat; r->rdx = (a[2] & 0x800) ? AT_REMOVEDIR : 0; break;
    case 501: ln = SYS_renameat; break;
    case 478: ln = SYS_lseek; break;
    case 95: ln = SYS_fsync; break;
    case 550: ln = SYS_fdatasync; break;
    case 124: ln = SYS_fchmod; break;
    case 92: ln = SYS_fcntl;
        if (a[1] == 17) r->rsi = F_DUPFD_CLOEXEC;
        else if (a[1] == 3) p->aux = POST_FCNTL_GETFL;
        else if (a[1] == 4) r->rdx = open_flags(a[2]) & ~3UL;
        else fprintf(stderr, "freebsd_abi_emulator: fcntl cmd %lu\n", a[1]);
        break;
    case 326: ln = SYS_getcwd; p->aux = POST_GETCWD; break;
    case 1: ln = SYS_exit_group; break;
    case 7: ln = SYS_wait4; break;
    case 575: ln = SYS_close_range; break;
    case 542: ln = SYS_pipe2; r->rsi = open_flags(a[1]) & ~3UL; break;
    case 545: ln = SYS_ppoll; r->r8 = 8; break;
    case 37: ln = SYS_kill; break;
    case 2: ln = SYS_fork; break;
    case 59: ln = SYS_execve; break;
    case 477: ln = SYS_mmap;
        if (a[3] & ~(0x1002UL)) fprintf(stderr, "freebsd_abi_emulator: mmap flags %#lx\n", a[3]);
        r->r10 = ((a[3] & 2) ? MAP_PRIVATE : 0) | ((a[3] & 0x1000) ? MAP_ANONYMOUS : 0); break;
    case 73: ln = SYS_munmap; break;
    case 74: ln = SYS_mprotect; break;
    case 203: ln = SYS_mlock; break;
    case 204: ln = SYS_munlock; break;
    case 75: ln = SYS_madvise; if (a[2] == 5) r->rdx = MADV_FREE; break;
    case 97: ln = SYS_socket; r->rdi = a[0] == 28 ? AF_INET6 : a[0]; r->rsi = sock_type(a[1]); break;
    case 98: ln = SYS_connect; p->aux = POST_SOCKADDR; p->a[5] = fix_sockaddr(pid, a[1]); break;
    case 104: ln = SYS_bind; p->aux = POST_SOCKADDR; p->a[5] = fix_sockaddr(pid, a[1]); break;
    case 106: ln = SYS_listen; break;
    case 541: ln = SYS_accept4; r->r10 = sock_type(a[3]); break;
    case 133: ln = SYS_sendto; r->r10 = (a[3] & ~0x20000UL) | ((a[3] & 0x20000) ? MSG_NOSIGNAL : 0); break;
    case 29: ln = SYS_recvfrom; break;
    case 134: ln = SYS_shutdown; break;
    case 105: ln = SYS_setsockopt; r->rsi = sockopt_level(a[1]); r->rdx = sockopt_name(a[1], a[2]); break;
    case 118: ln = SYS_getsockopt; r->rsi = sockopt_level(a[1]); r->rdx = sockopt_name(a[1], a[2]); break;
    case 232: ln = SYS_clock_gettime; r->rdi = a[0] == 4 ? CLOCK_MONOTONIC : a[0]; if (a[0] != 0 && a[0] != 4) fprintf(stderr, "freebsd_abi_emulator: clock %lu\n", a[0]); break;
    case 240: ln = SYS_nanosleep; break;
    case 244: ln = SYS_clock_nanosleep; r->rdi = a[0] == 4 ? CLOCK_MONOTONIC : a[0]; break;
    case 500: ln = SYS_readlinkat; break;
    case 563: ln = SYS_getrandom; break;
    case 194: ln = SYS_getrlimit; break;
    case 195: ln = SYS_setrlimit; break;
    case 202: ln = -1; p->aux = POST_SYSCTL; break;   // emulated below
    default: fprintf(stderr, "freebsd_abi_emulator: unexpected FreeBSD syscall %ld\n", n); ln = -1; break;
    }
    if (verbose) fprintf(stderr, "[%d] fbsd %ld -> linux %ld (%#lx, %#lx, %#lx, %#lx)\n", pid, n, ln, a[0], a[1], a[2], a[3]);
    r->orig_rax = ln;
    ptrace(PTRACE_SETREGS, pid, 0, r);
}

static void on_exit_(pid_t pid, proc_t *p, struct user_regs_struct *r) {
    long res = r->rax;
    if (p->aux == POST_SYSCTL) {
        int mib[2]; rd(pid, p->a[0], mib, 8);
        if (p->a[1] == 2 && mib[0] == 6 && mib[1] == 7) {
            int v = 4096; uint64_t len = 4; wr(pid, p->a[2], &v, 4); wr(pid, p->a[3], &len, 8); res = 0;
        } else res = -ENOENT;
    }
    if (p->aux == POST_SOCKADDR) { unsigned short h = p->a[5]; unsigned char b[2] = { h & 0xff, h >> 8 }; wr(pid, p->a[1], b, 2); }
    if (res < 0 && res > -4096) {
        r->rax = errno_back(-res); r->eflags |= 1;
    } else {
        if (p->aux == POST_STAT) convert_stat(pid, p->a[2]);
        if (p->aux == POST_DIRENT) res = convert_dirents(pid, p->a[1], res, p->a[2]);
        if (p->aux == POST_GETCWD) res = 0;
        if (p->aux == POST_FCNTL_GETFL) res = open_flags_back(res);
        r->rax = res; r->eflags &= ~1UL;
    }
    if (verbose) fprintf(stderr, "[%d]   = %#llx cf=%llu\n", pid, r->rax, r->eflags & 1);
    ptrace(PTRACE_SETREGS, pid, 0, r);
}

static int is_freebsd_image(pid_t pid) {
    char path[64]; snprintf(path, sizeof path, "/proc/%d/exe", pid);
    unsigned char h[8] = {0}; int fd = open(path, O_RDONLY); if (fd < 0) return 0;
    ssize_t got = read(fd, h, 8); close(fd); return got == 8 && h[7] == 9;
}

int main(int argc, char **argv) {
    int ai = 1;
    if (argc > 1 && !strcmp(argv[1], "-v")) { verbose = 1; ai = 2; }
    if (ai >= argc) { fprintf(stderr, "usage: freebsd_abi_emulator [-v] exe args\n"); return 2; }
    pid_t child = fork();
    if (child == 0) {
        ptrace(PTRACE_TRACEME, 0, 0, 0);
        execv(argv[ai], argv + ai);
        perror("exec"); _exit(127);
    }
    int status; waitpid(child, &status, 0);   // exec stop
    ptrace(PTRACE_SETOPTIONS, child, 0, PTRACE_O_TRACESYSGOOD | PTRACE_O_TRACEFORK | PTRACE_O_TRACEEXEC | PTRACE_O_EXITKILL);
    // FreeBSD exec_setregs: %rdi = the argument block (argc at its start).
    struct user_regs_struct r; ptrace(PTRACE_GETREGS, child, 0, &r);
    r.rdi = r.rsp; ptrace(PTRACE_SETREGS, child, 0, &r);
    P(child)->fresh = 0;
    ptrace(PTRACE_SYSCALL, child, 0, 0);
    int exit_code = 0;
    for (;;) {
        pid_t pid = waitpid(-1, &status, __WALL);
        if (pid < 0) break;
        proc_t *p = P(pid);
        if (WIFEXITED(status) || WIFSIGNALED(status)) {
            if (pid == child) exit_code = WIFEXITED(status) ? WEXITSTATUS(status) : 128 + WTERMSIG(status);
            p->pid = 0; continue;
        }
        if (!WIFSTOPPED(status)) continue;
        int sig = WSTOPSIG(status), ev = status >> 16, inject = 0;
        if (sig == (SIGTRAP | 0x80)) {
            ptrace(PTRACE_GETREGS, pid, 0, &r);
            if (!p->in_call) { p->in_call = 1; on_entry(pid, p, &r); }
            else { p->in_call = 0; on_exit_(pid, p, &r); }
        } else if (ev == PTRACE_EVENT_FORK) {
            unsigned long np; ptrace(PTRACE_GETEVENTMSG, pid, 0, &np);
            (void)P((pid_t)np);   // the child returns from fork without a syscall stop
        } else if (ev == PTRACE_EVENT_EXEC) {
            if (!is_freebsd_image(pid)) { p->pid = 0; ptrace(PTRACE_DETACH, pid, 0, 0); continue; }
            p->in_call = 0;
            ptrace(PTRACE_GETREGS, pid, 0, &r); r.rdi = r.rsp; ptrace(PTRACE_SETREGS, pid, 0, &r);
        } else if (sig == SIGSTOP && p->fresh) {
            p->fresh = 0;   // a fork child's initial stop
        } else if (sig != SIGTRAP) inject = sig;
        ptrace(PTRACE_SYSCALL, pid, 0, inject);
    }
    return exit_code;
}
