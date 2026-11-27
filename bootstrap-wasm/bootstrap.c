/* bootstrap.c: build SultanC from source with only a C compiler.
 * Copyright 2026 SultanC contributors. MIT license (see repository LICENSE).
 *
 * Build (from the repository root; the result is build/sultanc, or
 * build/sultanc.exe on Windows, the same compiler ./build.sh gives):
 *
 *   cc -o build-bootstrap bootstrap-wasm/bootstrap.c && ./build-bootstrap   (POSIX)
 *   gcc -o build-bootstrap.exe bootstrap-wasm/bootstrap.c                   (Windows, MinGW)
 *   cl /Fe:build-bootstrap.exe bootstrap-wasm/bootstrap.c                   (Windows, MSVC)
 *
 * With a shell, ./build.sh does the same and more. The last line printed is
 *   build: compiler: build/sultanc
 *
 * Check: ./build-bootstrap --verify (with a shell: ./build.sh verify) builds
 * the compiler again with itself and requires the two to be identical.
 *
 * Maintainers: --check-seed and --update-seed compare and replace the
 * committed seed (./build.sh seed-check and ./build.sh seed-update);
 * ./build.sh chain is the release check of the frozen C Stage0.
 *
 * How it works: sultanc-wasm2c (wasm2c/) translates the committed,
 * zstd-compressed WASI seed bootstrap-wasm/seed/sultanc.wasm.zst to C; the C
 * is compiled with the runtime (runtime/w2c.c) and the WASI host
 * (runtime/wasi.c) into Stage1, the seed compiler; Stage1 compiles
 * compiler/main.sn into the native compiler (Stage2), which is copied to
 * build/sultanc. Stages are intermediates in build/bootstrap/seed/, the
 * directory ./build.sh uses for the seed too: it reuses that Stage1 (the
 * seed.stamp written here is the one it checks). No shell, LLVM or
 * WebAssembly runtime is needed: a C99 compiler with gcc-style options (gcc,
 * clang, MinGW, tcc...) or MSVC-style options (cl, clang-cl: chosen by the
 * compiler's name, or with --msvc).
 *
 * Options
 *   --cc=CC          C compiler command (default: $CC, else "cc")
 *   --cflags=FLAGS   C flags (default: $CFLAGS, else "-std=c99 -O2", or
 *                    "/nologo /O2" for an MSVC-style compiler)
 *   --msvc           the C compiler takes MSVC-style options (cl, clang-cl)
 *   --jobs=N         parallel C compilations (default 4)
 *   --output=PATH    where the compiler goes (default build/sultanc, or
 *                    build/cross/TRIPLE/sultanc for another --target)
 *   --out=DIR        directory of the intermediates (default
 *                    build/bootstrap/seed)
 *   --target=TRIPLE  target of the compiler (default: this host)
 *   --verbose        show each command as it runs, and its output
 *   --stage1-only    stop after the seed compiler (Stage1); it is copied to
 *                    --output only when --output is given
 *   --verify         build Stage3 with Stage2 and require Stage2 == Stage3: the
 *                    seed is current (the policy CI checks), so Stage2 is the
 *                    fixpoint
 *   --check-seed     rebuild the seed with Stage2 and compare it, byte for byte,
 *                    with the committed seed decompressed (exit status 3 when
 *                    they differ)
 *   --update-seed    rebuild the seed with Stage2 and replace the committed one
 *                    (needs the `zstd` command)
 *
 * Stage2 needs SultanC's Native backend for the host: x86_64-linux and
 * arm64-darwin today. On other hosts build/sultanc is Stage1, the seed
 * compiler, which works there and targets x86_64-linux, arm64-darwin and
 * wasm32-wasi; a host-native compiler there needs an LLVM-enabled compiler.
 *
 * The compilers are Native-only: compiler/main.sn is compiled with the LLVM
 * provider boundary bound to unavailable.sn (docs/en/internals/compiler-architecture.md),
 * always at an explicit -O2 (without -O the compiler optimizes nothing).
 */
#if defined(_WIN32)
#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#include <direct.h>
#include <windows.h>
#else
#if !defined(_POSIX_C_SOURCE)
#define _POSIX_C_SOURCE 200809L
#endif
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define SEED "bootstrap-wasm/seed/sultanc.wasm.zst"
#define SELF "bootstrap-wasm/bootstrap.c"
#define NATIVE_ONLY \
    "--module-source=compiler.backend.provider.llvm.llvm_codegen=compiler/backend/provider/llvm/unavailable.sn"
/* The translator's sources, and the translated seed's units. */
static const char *const wasm2c_sources[] = {"common.c", "stream.c", "module.c", "funcgen.c", "output.c", "main.c"};
#define UNITS 8

#if defined(_WIN32)
#define EXE ".exe"
#else
#define EXE ""
#endif

/* The compiler for this host, as ./build.sh leaves it, and the stamp the build
 * driver (tools/build) keeps beside it ("seed compiler" when it is Stage1, on
 * a host without a native backend). */
#define COMPILER "build/sultanc" EXE
#define COMPILER_STAMP "build/sultanc.stamp"

static const char *cc = "cc";
static const char *cflags = NULL;
/* The C compiler takes MSVC-style options (cl, clang-cl). */
static int msvc = 0;

/* Whether a C compiler command names cl or clang-cl (any directory, any case,
 * with or without .exe). */
static int msvc_name(const char *command) {
    const char *base = command, *p;
    char name[16];
    size_t n = 0;
    for (p = command; *p && *p != ' '; p++)
        if (*p == '/' || *p == '\\') base = p + 1;
    while (base[n] && base[n] != ' ' && n + 1 < sizeof(name)) {
        char c = base[n];
        name[n] = (char)(c >= 'A' && c <= 'Z' ? c - 'A' + 'a' : c);
        n++;
    }
    name[n] = 0;
    return strcmp(name, "cl") == 0 || strcmp(name, "cl.exe") == 0 || strcmp(name, "clang-cl") == 0 ||
           strcmp(name, "clang-cl.exe") == 0;
}
/* The intermediates: the same directory ./build.sh gets the seed compiler in. */
static const char *out_dir = "build/bootstrap/seed";
static int jobs = 4;
/* --verbose: each command is shown and its output goes to the terminal.
 * Otherwise a command's output goes to a log, shown only when it fails. */
static int verbose = 0;
static const char *command_log = NULL;

static void fail(const char *fmt, ...) {
    va_list ap;
    fflush(stdout);
    va_start(ap, fmt);
    fputs("build: error: ", stderr);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputc('\n', stderr);
    exit(1);
}

/* One line of progress. */
static void say(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    fputs("build: ", stdout);
    vprintf(fmt, ap);
    va_end(ap);
    fputc('\n', stdout);
    fflush(stdout);
}

static char *fmt(const char *f, ...) {
    va_list ap;
    int n;
    char *s;
    va_start(ap, f);
    n = vsnprintf(NULL, 0, f, ap);
    va_end(ap);
    if (n < 0) fail("formatting failed");
    s = (char *)malloc((size_t)n + 1);
    if (!s) fail("out of memory");
    va_start(ap, f);
    vsnprintf(s, (size_t)n + 1, f, ap);
    va_end(ap);
    return s;
}

/* ---- host commands -------------------------------------------------------------- */

#if defined(_WIN32)
/* cmd.exe: double quotes; paths may not contain them. */
static char *quote(const char *s) {
    if (strchr(s, '"')) fail("path contains a double quote: %s", s);
    return fmt("\"%s\"", s);
}
/* cmd /c strips one outer pair of quotes from a command that starts with one. */
static char *shell_line(const char *cmd) { return fmt("\"%s\"", cmd); }
#else
/* /bin/sh: single quotes. */
static char *quote(const char *s) {
    size_t n = 3, i, j = 0;
    char *q;
    for (i = 0; s[i]; i++) n += s[i] == '\'' ? 4 : 1;
    q = (char *)malloc(n);
    if (!q) fail("out of memory");
    q[j++] = '\'';
    for (i = 0; s[i]; i++) {
        if (s[i] == '\'') { memcpy(q + j, "'\\''", 4); j += 4; }
        else q[j++] = s[i];
    }
    q[j++] = '\'';
    q[j] = 0;
    return q;
}
static char *shell_line(const char *cmd) { return fmt("%s", cmd); }
#endif

/* The command as the shell runs it: without --verbose its output (standard
 * output and errors, the same for sh and cmd.exe) goes to log. */
static char *logged(const char *cmd, const char *log) {
    if (verbose || !log) return fmt("%s", cmd);
    return fmt("%s > %s 2>&1", cmd, quote(log));
}

/* Stops with the command that failed and, without --verbose, its output. */
static void command_failed(const char *cmd, const char *log, int status) {
    FILE *f;
    fflush(stdout);
    fprintf(stderr, "build: error: this command failed (status %d):\n  %s\n", status, cmd);
    if (!verbose && log && (f = fopen(log, "rb")) != NULL) {
        char buf[4096];
        size_t n;
        while ((n = fread(buf, 1, sizeof(buf), f)) > 0) fwrite(buf, 1, n, stderr);
        fclose(f);
    }
    exit(1);
}

static void run(const char *cmd) {
    int rc;
    if (verbose) printf("+ %s\n", cmd);
    fflush(stdout);
    rc = system(shell_line(logged(cmd, command_log)));
#if !defined(_WIN32)
    if (rc != -1) rc = WIFEXITED(rc) ? WEXITSTATUS(rc) : 128;
#endif
    if (rc != 0) command_failed(cmd, command_log, rc);
}

/* Runs `count` commands, at most `jobs` at a time; command i logs to logs[i].
 * When one fails, the others already running finish, and the first failure is
 * reported. */
static void run_parallel(char **cmds, char **logs, int count) {
    int failed = -1, failed_status = 0;
#if defined(_WIN32)
    HANDLE running[MAXIMUM_WAIT_OBJECTS];
    int which[MAXIMUM_WAIT_OBJECTS];
    int next = 0, n = 0, limit = jobs > MAXIMUM_WAIT_OBJECTS ? MAXIMUM_WAIT_OBJECTS : jobs;
    while (next < count || n > 0) {
        while (n < limit && next < count && failed < 0) {
            STARTUPINFOA si;
            PROCESS_INFORMATION pi;
            char *line = fmt("cmd.exe /c %s", shell_line(logged(cmds[next], logs[next])));
            memset(&si, 0, sizeof(si));
            si.cb = sizeof(si);
            if (verbose) printf("+ %s\n", cmds[next]);
            fflush(stdout);
            if (!CreateProcessA(NULL, line, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi))
                fail("cannot start: %s", cmds[next]);
            CloseHandle(pi.hThread);
            which[n] = next;
            running[n++] = pi.hProcess;
            next++;
        }
        if (n > 0) {
            DWORD w = WaitForMultipleObjects((DWORD)n, running, FALSE, INFINITE), code = 1;
            int k;
            if (w >= WAIT_OBJECT_0 + (DWORD)n) fail("wait failed"); /* WAIT_OBJECT_0 is 0 */
            k = (int)(w - WAIT_OBJECT_0);
            GetExitCodeProcess(running[k], &code);
            CloseHandle(running[k]);
            if (code != 0 && failed < 0) { failed = which[k]; failed_status = (int)code; }
            --n;
            running[k] = running[n];
            which[k] = which[n];
        }
        if (failed >= 0 && n == 0) break;
    }
#else
    int next = 0, running = 0;
    pid_t *pids = (pid_t *)calloc((size_t)count, sizeof(*pids));
    if (!pids) fail("out of memory");
    while (next < count || running > 0) {
        while (running < jobs && next < count && failed < 0) {
            pid_t pid;
            char *line = logged(cmds[next], logs[next]);
            if (verbose) printf("+ %s\n", cmds[next]);
            fflush(stdout);
            pid = fork();
            if (pid < 0) fail("fork failed");
            if (pid == 0) {
                execl("/bin/sh", "sh", "-c", line, (char *)NULL);
                _exit(127);
            }
            pids[next] = pid;
            next++;
            running++;
        }
        if (running > 0) {
            int status, k;
            pid_t pid = wait(&status);
            if (pid < 0) fail("wait failed");
            running--;
            for (k = 0; k < next && pids[k] != pid; k++) {}
            if ((!WIFEXITED(status) || WEXITSTATUS(status) != 0) && failed < 0 && k < next) {
                failed = k;
                failed_status = WIFEXITED(status) ? WEXITSTATUS(status) : 128;
            }
        }
        if (failed >= 0 && running == 0) break;
    }
    free(pids);
#endif
    if (failed >= 0) command_failed(cmds[failed], logs[failed], failed_status);
}

/* ---- files ------------------------------------------------------------------------ */

static int file_exists(const char *path) {
    struct stat st;
    return stat(path, &st) == 0;
}

static void make_dir(const char *path) {
    char *copy = fmt("%s", path), *p;
    for (p = copy + 1;; p++) {
        if (*p == '/' || *p == '\\' || *p == 0) {
            char c = *p;
            int rc;
            *p = 0;
#if defined(_WIN32)
            rc = _mkdir(copy);
#else
            rc = mkdir(copy, 0777);
#endif
            if (rc != 0 && errno != EEXIST && !file_exists(copy)) fail("cannot create %s", copy);
            *p = c;
            if (c == 0) break;
        }
    }
    free(copy);
}

/* The directory part of a path ("" for a bare name). */
static char *dir_of(const char *path) {
    char *copy = fmt("%s", path), *p, *last = NULL;
    for (p = copy; *p; p++)
        if (*p == '/' || *p == '\\') last = p;
    if (last) *last = 0;
    else copy[0] = 0;
    return copy;
}

static int same_file_contents(const char *a, const char *b) {
    FILE *fa = fopen(a, "rb"), *fb = fopen(b, "rb");
    int same = 1;
    if (!fa || !fb) fail("cannot open %s or %s", a, b);
    for (;;) {
        int ca = getc(fa), cb = getc(fb);
        if (ca != cb) { same = 0; break; }
        if (ca == EOF) break;
    }
    fclose(fa);
    fclose(fb);
    return same;
}

static void set_executable(const char *path) {
#if defined(_WIN32)
    (void)path; /* the .exe suffix makes it runnable */
#else
    /* WASI Preview 1 cannot set permission bits; Stage1 writes a plain file. */
    if (chmod(path, 0755) != 0) fail("cannot make %s executable", path);
#endif
}

/* Renames from to to, replacing to (rename does not replace on Windows). */
static void replace_file(const char *from, const char *to) {
    if (rename(from, to) != 0) {
        remove(to);
        if (rename(from, to) != 0) fail("cannot replace %s", to);
    }
}

/* Copies the compiler `from` to `to` through a temporary beside it, so `to` is
 * never half written; the copy is executable. */
static void install(const char *from, const char *to) {
    char *dir = dir_of(to), *tmp = fmt("%s.new", to), buf[65536];
    FILE *in, *out;
    size_t n;
    if (*dir) make_dir(dir);
    in = fopen(from, "rb");
    if (!in) fail("cannot read %s", from);
    out = fopen(tmp, "wb");
    if (!out) fail("cannot write %s", tmp);
    while ((n = fread(buf, 1, sizeof(buf), in)) > 0)
        if (fwrite(buf, 1, n, out) != n) fail("cannot write %s", tmp);
    if (ferror(in)) fail("cannot read %s", from);
    fclose(in);
    if (fclose(out) != 0) fail("cannot write %s", tmp);
    set_executable(tmp);
    replace_file(tmp, to);
    /* The build driver's stamp describes the compiler it built there; this one
     * replaces it, so the next ./build.sh builds a step of its own. */
    if (strcmp(to, COMPILER) == 0) remove(COMPILER_STAMP);
}

/* ---- the seed stamp ---------------------------------------------------------------- */

/* SHA-256 (FIPS 180-4), 32-bit words in unsigned long. */
#define W32(x) ((x) & 0xffffffffUL)
#define ROR(x, n) W32(((x) >> (n)) | ((x) << (32 - (n))))
typedef struct {
    unsigned long h[8];
    unsigned char block[64];
    size_t used;
    unsigned long long total;
} sha256_state;

static const unsigned long sha256_k[64] = {
    0x428a2f98UL, 0x71374491UL, 0xb5c0fbcfUL, 0xe9b5dba5UL, 0x3956c25bUL, 0x59f111f1UL, 0x923f82a4UL, 0xab1c5ed5UL,
    0xd807aa98UL, 0x12835b01UL, 0x243185beUL, 0x550c7dc3UL, 0x72be5d74UL, 0x80deb1feUL, 0x9bdc06a7UL, 0xc19bf174UL,
    0xe49b69c1UL, 0xefbe4786UL, 0x0fc19dc6UL, 0x240ca1ccUL, 0x2de92c6fUL, 0x4a7484aaUL, 0x5cb0a9dcUL, 0x76f988daUL,
    0x983e5152UL, 0xa831c66dUL, 0xb00327c8UL, 0xbf597fc7UL, 0xc6e00bf3UL, 0xd5a79147UL, 0x06ca6351UL, 0x14292967UL,
    0x27b70a85UL, 0x2e1b2138UL, 0x4d2c6dfcUL, 0x53380d13UL, 0x650a7354UL, 0x766a0abbUL, 0x81c2c92eUL, 0x92722c85UL,
    0xa2bfe8a1UL, 0xa81a664bUL, 0xc24b8b70UL, 0xc76c51a3UL, 0xd192e819UL, 0xd6990624UL, 0xf40e3585UL, 0x106aa070UL,
    0x19a4c116UL, 0x1e376c08UL, 0x2748774cUL, 0x34b0bcb5UL, 0x391c0cb3UL, 0x4ed8aa4aUL, 0x5b9cca4fUL, 0x682e6ff3UL,
    0x748f82eeUL, 0x78a5636fUL, 0x84c87814UL, 0x8cc70208UL, 0x90befffaUL, 0xa4506cebUL, 0xbef9a3f7UL, 0xc67178f2UL};

static void sha256_init(sha256_state *s) {
    static const unsigned long iv[8] = {0x6a09e667UL, 0xbb67ae85UL, 0x3c6ef372UL, 0xa54ff53aUL,
                                        0x510e527fUL, 0x9b05688cUL, 0x1f83d9abUL, 0x5be0cd19UL};
    memcpy(s->h, iv, sizeof(iv));
    s->used = 0;
    s->total = 0;
}

static void sha256_block(sha256_state *s, const unsigned char *p) {
    unsigned long w[64], v[8], t1, t2;
    int i;
    for (i = 0; i < 16; i++)
        w[i] = ((unsigned long)p[4 * i] << 24) | ((unsigned long)p[4 * i + 1] << 16) |
               ((unsigned long)p[4 * i + 2] << 8) | (unsigned long)p[4 * i + 3];
    for (i = 16; i < 64; i++) {
        unsigned long s0 = ROR(w[i - 15], 7) ^ ROR(w[i - 15], 18) ^ (w[i - 15] >> 3);
        unsigned long s1 = ROR(w[i - 2], 17) ^ ROR(w[i - 2], 19) ^ (w[i - 2] >> 10);
        w[i] = W32(w[i - 16] + s0 + w[i - 7] + s1);
    }
    for (i = 0; i < 8; i++) v[i] = s->h[i];
    for (i = 0; i < 64; i++) {
        t1 = W32(v[7] + (ROR(v[4], 6) ^ ROR(v[4], 11) ^ ROR(v[4], 25)) + ((v[4] & v[5]) ^ (W32(~v[4]) & v[6])) +
                 sha256_k[i] + w[i]);
        t2 = W32((ROR(v[0], 2) ^ ROR(v[0], 13) ^ ROR(v[0], 22)) + ((v[0] & v[1]) ^ (v[0] & v[2]) ^ (v[1] & v[2])));
        v[7] = v[6];
        v[6] = v[5];
        v[5] = v[4];
        v[4] = W32(v[3] + t1);
        v[3] = v[2];
        v[2] = v[1];
        v[1] = v[0];
        v[0] = W32(t1 + t2);
    }
    for (i = 0; i < 8; i++) s->h[i] = W32(s->h[i] + v[i]);
}

static void sha256_update(sha256_state *s, const unsigned char *p, size_t n) {
    s->total += n;
    while (n > 0) {
        size_t take = 64 - s->used;
        if (take > n) take = n;
        memcpy(s->block + s->used, p, take);
        s->used += take;
        p += take;
        n -= take;
        if (s->used == 64) {
            sha256_block(s, s->block);
            s->used = 0;
        }
    }
}

/* The digest as 64 lowercase hexadecimal digits (as sha256sum prints it). */
static void sha256_hex(sha256_state *s, char out[65]) {
    static const char digits[] = "0123456789abcdef";
    unsigned long long bits = s->total * 8;
    unsigned char pad = 0x80, len[8];
    int i;
    for (i = 0; i < 8; i++) len[i] = (unsigned char)(bits >> (56 - 8 * i));
    sha256_update(s, &pad, 1);
    pad = 0;
    while (s->used != 56) sha256_update(s, &pad, 1);
    sha256_update(s, len, 8);
    for (i = 0; i < 32; i++) {
        unsigned long b = (s->h[i / 4] >> (24 - 8 * (i % 4))) & 0xff;
        out[2 * i] = digits[b >> 4];
        out[2 * i + 1] = digits[b & 15];
    }
    out[64] = 0;
}

static void sha256_file(const char *path, char out[65]) {
    sha256_state s;
    unsigned char buf[65536];
    size_t n;
    FILE *f = fopen(path, "rb");
    if (!f) fail("cannot read %s", path);
    sha256_init(&s);
    while ((n = fread(buf, 1, sizeof(buf), f)) > 0) sha256_update(&s, buf, n);
    fclose(f);
    sha256_hex(&s, out);
}

/* The stamp ./build.sh and the build driver check before they reuse Stage1
 * from out_dir: the SHA-256 of the lines "<SHA-256 of the file>  <path>\n" of
 * the seed and of this file (sha256sum's format; tools/build/support.sn). */
static void write_seed_stamp(const char *path) {
    const char *const inputs[2] = {SEED, SELF};
    sha256_state s;
    char hex[65], *line;
    FILE *f;
    int i;
    sha256_init(&s);
    for (i = 0; i < 2; i++) {
        sha256_file(inputs[i], hex);
        line = fmt("%s  %s\n", hex, inputs[i]);
        sha256_update(&s, (const unsigned char *)line, strlen(line));
        free(line);
    }
    sha256_hex(&s, hex);
    f = fopen(path, "wb");
    if (!f) fail("cannot write %s", path);
    fputs(hex, f);
    if (fclose(f) != 0) fail("cannot write %s", path);
}
/* ---- host ----------------------------------------------------------------------- */

/* SultanC target triple of this host (see `sultanc targets`). */
static const char *host_target(void) {
#if defined(_WIN32)
#if defined(_M_ARM64) || defined(__aarch64__)
    return "aarch64-windows-msvc";
#else
    return "x86_64-windows-msvc";
#endif
#elif defined(__APPLE__)
#if defined(__aarch64__) || defined(__arm64__)
    return "arm64-darwin";
#else
    return "x86_64-darwin";
#endif
#elif defined(__FreeBSD__)
#if defined(__aarch64__)
    return "aarch64-freebsd";
#elif defined(__riscv) && __riscv_xlen == 64
    return "riscv64-freebsd";
#else
    return "x86_64-freebsd";
#endif
#elif defined(__ANDROID__)
#if defined(__aarch64__)
    return "aarch64-android";
#elif defined(__x86_64__)
    return "x86_64-android";
#else
    return "armv7-android";
#endif
#elif defined(__linux__)
#if defined(__x86_64__)
    return "x86_64-linux";
#elif defined(__aarch64__)
    return "aarch64-linux-gnu";
#elif defined(__i386__)
    return "i686-linux-gnu";
#elif defined(__arm__)
    return "armv7-linux-gnueabihf";
#elif defined(__riscv) && __riscv_xlen == 64
    return "riscv64-linux-gnu";
#elif defined(__s390x__)
    return "s390x-linux-gnu";
#elif defined(__powerpc64__) && defined(__LITTLE_ENDIAN__)
    return "powerpc64le-linux-gnu";
#elif defined(__loongarch64)
    return "loongarch64-linux-gnu";
#elif defined(__mips64)
    return "mips64el-linux-gnuabi64";
#else
    return NULL;
#endif
#else
    return NULL;
#endif
}

/* Targets SultanC's Native backend can produce a compiler for. */
static int native_capable(const char *t) {
    return strcmp(t, "x86_64-linux") == 0 || strcmp(t, "arm64-darwin") == 0;
}

static void build_compiler(const char *compiler, const char *output, const char *for_target) {
    run(fmt("%s compiler/main.sn -O2 --backend=native --target=%s %s -o %s", quote(compiler), for_target, NATIVE_ONLY,
            quote(output)));
    set_executable(output);
}

/* ---- main ----------------------------------------------------------------------- */

int main(int argc, char **argv) {
    int verify = 0, check_seed = 0, update_seed = 0, stage1_only = 0, no_native = 0, i, runnable, count;
    const char *target = NULL, *host, *output = NULL;
    char *w2c, *gen, *stage1, *stage2, **cmds, **logs, *objs, *srcs;
    size_t objs_len = 0;

    if (getenv("CC") && *getenv("CC")) cc = getenv("CC");
    if (getenv("CFLAGS") && *getenv("CFLAGS")) cflags = getenv("CFLAGS");
    for (i = 1; i < argc; i++) {
        if (strncmp(argv[i], "--cc=", 5) == 0) cc = argv[i] + 5;
        else if (strncmp(argv[i], "--cflags=", 9) == 0) cflags = argv[i] + 9;
        else if (strncmp(argv[i], "--jobs=", 7) == 0) jobs = atoi(argv[i] + 7);
        else if (strncmp(argv[i], "--out=", 6) == 0) out_dir = argv[i] + 6;
        else if (strncmp(argv[i], "--output=", 9) == 0) output = argv[i] + 9;
        else if (strncmp(argv[i], "--target=", 9) == 0) target = argv[i] + 9;
        else if (strcmp(argv[i], "--stage1-only") == 0) stage1_only = 1;
        else if (strcmp(argv[i], "--msvc") == 0) msvc = 1;
        else if (strcmp(argv[i], "--verbose") == 0) verbose = 1;
        else if (strcmp(argv[i], "--verify") == 0) verify = 1;
        else if (strcmp(argv[i], "--check-seed") == 0) check_seed = 1;
        else if (strcmp(argv[i], "--update-seed") == 0) update_seed = 1;
        else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            puts("usage: build-bootstrap [--cc=CC] [--cflags=FLAGS] [--msvc] [--jobs=N] [--output=PATH] [--out=DIR]\n"
                 "                       [--target=TRIPLE] [--verbose] [--stage1-only] [--verify] [--check-seed]\n"
                 "                       [--update-seed]\n"
                 "Builds the compiler from the committed seed with only a C compiler -> build/sultanc" EXE "\n"
                 "(the options are described at the top of bootstrap-wasm/bootstrap.c).");
            return 0;
        } else fail("unknown option %s (try --help)", argv[i]);
    }
    if (jobs < 1) jobs = 1;
    if (output && !*output) fail("--output needs a path");
    if (msvc_name(cc)) msvc = 1;
    if (!cflags) cflags = msvc ? "/nologo /O2" : "-std=c99 -O2";
    if (!file_exists("compiler/main.sn") || !file_exists(SEED))
        fail("run from the repository root (compiler/main.sn and %s must exist)", SEED);
    host = host_target();
    if (!target) target = host;
    if (!stage1_only && (!target || !native_capable(target))) {
        if (verify || check_seed || update_seed)
            fail("--verify, --check-seed and --update-seed need a native backend for %s (x86_64-linux, "
                 "arm64-darwin)",
                 target ? target : "this host");
        stage1_only = 1;
        no_native = 1;
    }
    runnable = target && host && strcmp(target, host) == 0;
    if ((verify || check_seed || update_seed) && !runnable)
        fail("--verify, --check-seed and --update-seed run the compiler they build: they need this host's target "
             "(target %s, host %s)",
             target, host ? host : "unknown");

    make_dir(out_dir);
    gen = fmt("%s/gen", out_dir);
    make_dir(gen);
    command_log = fmt("%s/command.log", out_dir);
    /* Until the seed compiler is built again, the stamp describes none. */
    remove(fmt("%s/seed.stamp", out_dir));

    /* 1. Build the translator and translate the seed. */
    say("translating the seed to C (%s)", SEED);
    w2c = fmt("%s/sultanc-wasm2c" EXE, out_dir);
    srcs = fmt("%s", "");
    for (i = 0; i < (int)(sizeof(wasm2c_sources) / sizeof(wasm2c_sources[0])); i++)
        srcs = fmt("%s bootstrap-wasm/wasm2c/%s", srcs, wasm2c_sources[i]);
    if (msvc)
        run(fmt("%s %s /D_CRT_SECURE_NO_WARNINGS /Fe%s%s", cc, cflags, quote(w2c), srcs));
    else
        run(fmt("%s %s -o %s%s", cc, cflags, quote(w2c), srcs));
    run(fmt("%s --units=%d %s %s", quote(w2c), UNITS, SEED, quote(fmt("%s/sultanc", gen))));

    /* 2. Compile the translated C, the runtime and the WASI host; link Stage1. */
    count = UNITS + 3;
    say("compiling C (%d files)", count);
    cmds = (char **)calloc((size_t)count, sizeof(*cmds));
    logs = (char **)calloc((size_t)count, sizeof(*logs));
    objs = (char *)calloc(1, 1);
    if (!cmds || !logs || !objs) fail("out of memory");
    for (i = 0; i < count; i++) {
        char *src, *obj, *q;
        size_t n;
        if (i < UNITS) {
            src = fmt("%s/sultanc-%d.c", gen, i + 1);
            obj = fmt("%s/sultanc-%d.%s", gen, i + 1, msvc ? "obj" : "o");
        } else if (i == UNITS) {
            src = fmt("%s/sultanc-module.c", gen);
            obj = fmt("%s/sultanc-module.%s", gen, msvc ? "obj" : "o");
        } else if (i == UNITS + 1) {
            src = fmt("bootstrap-wasm/runtime/w2c.c");
            obj = fmt("%s/w2c.%s", out_dir, msvc ? "obj" : "o");
        } else {
            src = fmt("bootstrap-wasm/runtime/wasi.c");
            obj = fmt("%s/wasi.%s", out_dir, msvc ? "obj" : "o");
        }
        if (msvc)
            /* The generated units are large and machine-written: no warnings, big objects. */
            cmds[i] = fmt("%s %s%s /D_CRT_SECURE_NO_WARNINGS /Ibootstrap-wasm/runtime /I%s /c %s /Fo%s", cc, cflags,
                          i <= UNITS ? " /w /bigobj" : "", quote(gen), quote(src), quote(obj));
        else
            cmds[i] = fmt("%s %s -Ibootstrap-wasm/runtime -I%s -c %s -o %s", cc, cflags, quote(gen), quote(src),
                          quote(obj));
        logs[i] = fmt("%s.log", obj);
        q = quote(obj);
        n = strlen(q);
        objs = (char *)realloc(objs, objs_len + n + 2);
        if (!objs) fail("out of memory");
        objs[objs_len++] = ' ';
        memcpy(objs + objs_len, q, n + 1);
        objs_len += n;
    }
    run_parallel(cmds, logs, count);
    stage1 = fmt("%s/sultanc-stage1" EXE, out_dir);
    if (msvc)
        run(fmt("%s /nologo /Fe%s%s", cc, quote(stage1), objs));
    else
        run(fmt("%s -o %s%s", cc, quote(stage1), objs));
    write_seed_stamp(fmt("%s/seed.stamp", out_dir));

    if (stage1_only) {
        /* --stage1-only alone leaves build/sultanc as it is (./build.sh runs it
         * so), and so does a --target of another system without a native
         * backend; on a host without one, the seed compiler is the compiler. */
        int for_host = runnable || !target;
        if (!output && !(no_native && for_host)) {
            if (no_native)
                say("no native backend for %s: a compiler for it is generated by an LLVM-enabled compiler "
                    "(docs/en/internals/bootstrap.md, \"Hosts\")",
                    target);
            say("seed compiler: %s", stage1);
            return 0;
        }
        if (!output) output = COMPILER;
        install(stage1, output);
        if (no_native && strcmp(output, COMPILER) == 0) {
            /* ./build.sh refreshes a build/sultanc marked so from the seed. */
            FILE *f = fopen(COMPILER_STAMP, "wb");
            if (!f || fputs("seed compiler\n", f) < 0 || fclose(f) != 0) fail("cannot write %s", COMPILER_STAMP);
        }
        if (no_native)
            say("no native backend for %s; %s is the seed compiler (targets x86_64-linux, arm64-darwin, wasm32-wasi)",
                for_host ? "this host" : target, output);
        say("compiler: %s", output);
        return 0;
    }

    /* 3. Stage1 (the seed compiler) builds the native compiler. */
    say("building the compiler from compiler/ with the seed compiler");
    stage2 = fmt("%s/sultanc-stage2%s", out_dir, runnable ? EXE : "");
    build_compiler(stage1, stage2, target);

    if (verify) {
        char *stage3 = fmt("%s/sultanc-stage3" EXE, out_dir);
        say("verifying: building the compiler again with itself; the two must be identical");
        build_compiler(stage2, stage3, target);
        if (!same_file_contents(stage2, stage3))
            fail("fixpoint failed: %s and %s differ (the seed predates the sources: ./build.sh seed-update)", stage2,
                 stage3);
        say("fixpoint: Stage2 == Stage3");
    }
    if (!output) output = runnable ? COMPILER : fmt("build/cross/%s/sultanc", target);
    install(stage2, output);
    say("compiler: %s", output);

    if (check_seed || update_seed) {
        char *seed = fmt("%s/sultanc.wasm", out_dir), *committed = fmt("%s/sultanc.committed.wasm", out_dir);
        int same;
        say("rebuilding the seed from compiler/");
        run(fmt("%s compiler/main.sn -O2 --backend=native --target=wasm32-wasi %s -o %s", quote(stage2), NATIVE_ONLY,
                quote(seed)));
        /* The committed seed, decompressed by the in-tree decoder. */
        run(fmt("%s --decompress %s %s", quote(w2c), SEED, quote(committed)));
        same = same_file_contents(seed, committed);
        if (same) {
            say("seed check: the committed seed is reproduced exactly");
        } else if (check_seed && !update_seed) {
            printf("build: seed check: the current sources give a different seed\n"
                   "  committed %s (decompressed to %s)\n  rebuilt   %s\n"
                   "  To replace the committed seed: build-bootstrap --update-seed\n",
                   SEED, committed, seed);
            return 3;
        }
        if (update_seed && !same) {
            char *zst = fmt("%s/sultanc.wasm.zst", out_dir), *back = fmt("%s/sultanc.wasm.decoded", out_dir);
            run(fmt("zstd -q -f -19 %s -o %s", quote(seed), quote(zst)));
            /* The in-tree decoder must reproduce the module exactly. */
            run(fmt("%s --decompress %s %s", quote(w2c), quote(zst), quote(back)));
            if (!same_file_contents(back, seed)) fail("the compressed seed does not decode back to the module");
            replace_file(zst, SEED);
            printf("build: seed updated: %s\n"
                   "  Review the change, then run build-bootstrap --verify --check-seed from a clean output directory.\n",
                   SEED);
        }
    }
    return 0;
}
