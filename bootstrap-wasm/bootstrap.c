/* bootstrap.c: build SultanC from source with only a C compiler.
 * Copyright 2026 SultanC contributors. MIT license (see repository LICENSE).
 *
 * The seed bootstrap:
 *   1. wasm2c.c translates the committed, zstd-compressed WASI seed
 *      bootstrap-wasm/seed/sultanc.wasm.zst to C;
 *   2. the C is compiled with wasi.c into Stage1, a native SultanC compiler;
 *   3. Stage1 compiles compiler/main.sn into the native Stage2 compiler.
 * No CMake, shell, LLVM or WebAssembly runtime is needed: a C99 compiler with
 * gcc-style options (gcc, clang, MinGW, tcc...). For MSVC use the main CMake
 * build (cmake/, -DSULTANC_BOOTSTRAP=wasm).
 *
 *   cc -o build-bootstrap bootstrap-wasm/bootstrap.c        (POSIX)
 *   gcc -o build-bootstrap.exe bootstrap-wasm/bootstrap.c   (Windows, MinGW)
 *   ./build-bootstrap [options]                              (repository root)
 *
 * Options
 *   --cc=CC          C compiler command (default: $CC, else "cc")
 *   --cflags=FLAGS   C flags (default: $CFLAGS, else "-std=c99 -O2")
 *   --jobs=N         parallel C compilations (default 4)
 *   --out=DIR        output directory (default build/bootstrap-wasm)
 *   --target=TRIPLE  target of the Stage2 compiler (default: this host)
 *   --stage1-only    stop after Stage1
 *   --verify         build Stage3 and Stage4 and require Stage3 == Stage4
 *   --check-seed     rebuild the seed with Stage2 and compare it, byte for byte,
 *                    with the committed seed decompressed (exit status 3 when
 *                    they differ)
 *   --update-seed    rebuild the seed with Stage2 and replace the committed one
 *                    (needs the `zstd` command)
 *
 * Stage2 needs SultanC's Native backend for the host: x86_64-linux and
 * arm64-darwin today. On other hosts Stage1 is still built and is a working
 * compiler (it targets x86_64-linux, arm64-darwin and wasm32-wasi); a
 * host-native Stage2 there needs an LLVM-enabled compiler.
 *
 * The compilers are Native-only: compiler/main.sn is compiled with the LLVM
 * provider boundary bound to unavailable.sn (docs/en/internals/compiler-architecture.md).
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
#define NATIVE_ONLY \
    "--module-source=compiler.backend.provider.llvm.llvm_codegen=compiler/backend/provider/llvm/unavailable.sn"
#define MAX_SHARDS 64

#if defined(_WIN32)
#define EXE ".exe"
#else
#define EXE ""
#endif

static const char *cc = "cc";
static const char *cflags = "-std=c99 -O2";
static const char *out_dir = "build/bootstrap-wasm";
static int jobs = 4;

static void fail(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    fputs("bootstrap: error: ", stderr);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputc('\n', stderr);
    exit(1);
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

static int run_status(const char *cmd) {
    int rc;
    printf("+ %s\n", cmd);
    fflush(stdout);
    rc = system(shell_line(cmd));
#if defined(_WIN32)
    return rc;
#else
    if (rc == -1) return -1;
    return WIFEXITED(rc) ? WEXITSTATUS(rc) : 128;
#endif
}
static void run(const char *cmd) {
    int rc = run_status(cmd);
    if (rc != 0) fail("command failed (status %d): %s", rc, cmd);
}

/* Runs `count` commands, at most `jobs` at a time. */
static void run_parallel(char **cmds, int count) {
#if defined(_WIN32)
    HANDLE running[MAXIMUM_WAIT_OBJECTS];
    int next = 0, n = 0, failed = 0, limit = jobs > MAXIMUM_WAIT_OBJECTS ? MAXIMUM_WAIT_OBJECTS : jobs;
    while (next < count || n > 0) {
        while (n < limit && next < count && !failed) {
            STARTUPINFOA si;
            PROCESS_INFORMATION pi;
            char *line = fmt("cmd.exe /c %s", shell_line(cmds[next]));
            memset(&si, 0, sizeof(si));
            si.cb = sizeof(si);
            printf("+ %s\n", cmds[next]);
            fflush(stdout);
            if (!CreateProcessA(NULL, line, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi))
                fail("cannot start: %s", cmds[next]);
            CloseHandle(pi.hThread);
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
            running[k] = running[--n];
            if (code != 0) failed = 1;
        }
        if (failed && n == 0) fail("a C compilation failed (see above)");
    }
#else
    int next = 0, running = 0, failed = 0;
    while (next < count || running > 0) {
        while (running < jobs && next < count && !failed) {
            pid_t pid;
            printf("+ %s\n", cmds[next]);
            fflush(stdout);
            pid = fork();
            if (pid < 0) fail("fork failed");
            if (pid == 0) {
                execl("/bin/sh", "sh", "-c", cmds[next], (char *)NULL);
                _exit(127);
            }
            next++;
            running++;
        }
        if (running > 0) {
            int status;
            if (wait(&status) < 0) fail("wait failed");
            running--;
            if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) failed = 1;
        }
        if (failed && running == 0) fail("a C compilation failed (see above)");
    }
#endif
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
    run(fmt("%s compiler/main.sn --backend=native --target=%s %s -o %s", quote(compiler), for_target, NATIVE_ONLY,
            quote(output)));
    set_executable(output);
}

/* ---- main ----------------------------------------------------------------------- */

int main(int argc, char **argv) {
    int verify = 0, check_seed = 0, update_seed = 0, stage1_only = 0, i, shards, runnable;
    const char *target = NULL, *host;
    char *w2c, *gen, *stage1, *stage2, **cmds, *objs;
    size_t objs_len = 0;

    if (getenv("CC") && *getenv("CC")) cc = getenv("CC");
    if (getenv("CFLAGS") && *getenv("CFLAGS")) cflags = getenv("CFLAGS");
    for (i = 1; i < argc; i++) {
        if (strncmp(argv[i], "--cc=", 5) == 0) cc = argv[i] + 5;
        else if (strncmp(argv[i], "--cflags=", 9) == 0) cflags = argv[i] + 9;
        else if (strncmp(argv[i], "--jobs=", 7) == 0) jobs = atoi(argv[i] + 7);
        else if (strncmp(argv[i], "--out=", 6) == 0) out_dir = argv[i] + 6;
        else if (strncmp(argv[i], "--target=", 9) == 0) target = argv[i] + 9;
        else if (strcmp(argv[i], "--stage1-only") == 0) stage1_only = 1;
        else if (strcmp(argv[i], "--verify") == 0) verify = 1;
        else if (strcmp(argv[i], "--check-seed") == 0) check_seed = 1;
        else if (strcmp(argv[i], "--update-seed") == 0) update_seed = 1;
        else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            puts("usage: build-bootstrap [--cc=CC] [--cflags=FLAGS] [--jobs=N] [--out=DIR] [--target=TRIPLE]\n"
                 "                       [--stage1-only] [--verify] [--check-seed] [--update-seed]");
            return 0;
        } else fail("unknown option %s (try --help)", argv[i]);
    }
    if (jobs < 1) jobs = 1;
    if (!file_exists("compiler/main.sn") || !file_exists(SEED))
        fail("run from the repository root (compiler/main.sn and %s must exist)", SEED);
    host = host_target();
    if (!target) target = host;
    if (!stage1_only && (!target || !native_capable(target))) {
        printf("bootstrap: note: no Native SultanC backend for %s yet (Native: x86_64-linux, arm64-darwin);\n"
               "           building Stage1 only. Stage1 is a working compiler on this host.\n",
               target ? target : "this host");
        if (verify || check_seed || update_seed) fail("--verify/--check-seed/--update-seed need a Stage2");
        stage1_only = 1;
    }
    runnable = target && host && strcmp(target, host) == 0;
    if ((verify || check_seed || update_seed) && !runnable)
        fail("--verify/--check-seed/--update-seed need Stage2 to run on this host (target %s, host %s)", target,
             host ? host : "unknown");

    make_dir(out_dir);
    gen = fmt("%s/gen", out_dir);
    make_dir(gen);

    /* 1. Build the translator and translate the seed. */
    w2c = fmt("%s/sultanc-wasm2c" EXE, out_dir);
    run(fmt("%s %s -o %s bootstrap-wasm/wasm2c.c", cc, cflags, quote(w2c)));
    shards = jobs * 4 > MAX_SHARDS ? MAX_SHARDS : jobs * 4;
    if (shards < 8) shards = 8;
    run(fmt("%s --shards=%d %s %s", quote(w2c), shards, SEED, quote(fmt("%s/sultanc", gen))));

    /* 2. Compile the translated C and the WASI host; link Stage1. */
    cmds = (char **)calloc((size_t)shards + 2, sizeof(*cmds));
    objs = (char *)calloc(1, 1);
    if (!cmds || !objs) fail("out of memory");
    for (i = 0; i <= shards + 1; i++) {
        char *src, *obj, *q;
        size_t n;
        if (i < shards) {
            src = fmt("%s/sultanc-%03d.c", gen, i);
            obj = fmt("%s/sultanc-%03d.o", gen, i);
        } else if (i == shards) {
            src = fmt("%s/sultanc-runtime.c", gen);
            obj = fmt("%s/sultanc-runtime.o", gen);
        } else {
            src = fmt("bootstrap-wasm/wasi.c");
            obj = fmt("%s/wasi.o", out_dir);
        }
        cmds[i] = fmt("%s %s -I%s -c %s -o %s", cc, cflags, quote(gen), quote(src), quote(obj));
        q = quote(obj);
        n = strlen(q);
        objs = (char *)realloc(objs, objs_len + n + 2);
        if (!objs) fail("out of memory");
        objs[objs_len++] = ' ';
        memcpy(objs + objs_len, q, n + 1);
        objs_len += n;
    }
    run_parallel(cmds, shards + 2);
    stage1 = fmt("%s/sultanc-stage1" EXE, out_dir);
    run(fmt("%s -o %s%s", cc, quote(stage1), objs));
    printf("bootstrap: Stage1 compiler: %s\n", stage1);
    if (stage1_only) return 0;

    /* 3. Stage1 (the translated seed) builds the native compiler. */
    stage2 = fmt("%s/sultanc-stage2%s", out_dir, runnable ? EXE : "");
    build_compiler(stage1, stage2, target);
    printf("bootstrap: Stage2 compiler: %s\n", stage2);

    if (verify) {
        char *stage3 = fmt("%s/sultanc-stage3" EXE, out_dir), *stage4 = fmt("%s/sultanc-stage4" EXE, out_dir);
        build_compiler(stage2, stage3, target);
        build_compiler(stage3, stage4, target);
        if (!same_file_contents(stage3, stage4)) fail("convergence failed: Stage3 and Stage4 differ");
        printf("bootstrap: convergence: Stage3 == Stage4\n");
        printf("bootstrap: the seed %s the current sources (Stage2 %s Stage3)\n",
               same_file_contents(stage2, stage3) ? "matches" : "predates",
               same_file_contents(stage2, stage3) ? "==" : "!=");
    }
    if (check_seed || update_seed) {
        char *seed = fmt("%s/sultanc.wasm", out_dir), *committed = fmt("%s/sultanc.committed.wasm", out_dir);
        int same;
        run(fmt("%s compiler/main.sn --backend=native --target=wasm32-wasi %s -o %s", quote(stage2), NATIVE_ONLY,
                quote(seed)));
        /* The committed seed, decompressed by the in-tree decoder. */
        run(fmt("%s --decompress %s %s", quote(w2c), SEED, quote(committed)));
        same = same_file_contents(seed, committed);
        if (same) {
            printf("bootstrap: seed check: the committed seed is reproduced exactly\n");
        } else if (check_seed && !update_seed) {
            printf("bootstrap: seed check: the current sources give a different seed\n"
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
            if (rename(zst, SEED) != 0) {
                remove(SEED);
                if (rename(zst, SEED) != 0) fail("cannot replace %s", SEED);
            }
            printf("bootstrap: seed updated: %s\n"
                   "  Review the change, then run build-bootstrap --verify --check-seed from a clean output directory.\n",
                   SEED);
        }
    }
    return 0;
}
