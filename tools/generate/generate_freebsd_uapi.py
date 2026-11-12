#!/usr/bin/env python3
"""Generate std's FreeBSD kernel ABI constants and record layouts.

std's direct-kernel provider (library/std/os/platform/unix/linux) serves
FreeBSD where FreeBSD's system calls take the same arguments as Linux's.
Its flag, option and errno values come from the selected kernel's tables:
library/std/os/platform/unix/linux/uapi.sn for Linux, and the file this
script writes, library/std/os/platform/unix/freebsd/uapi.sn, for FreeBSD.
The FreeBSD file defines

  * the provider's constant functions (the `..._لينكس` names it calls) with
    FreeBSD's values, and
  * FreeBSD-only values (`..._فري_بي_إس_دي`): the struct stat layout,
    wait(2) status, the fstatat flag, rlimit and sysctl names, and the
    classification of FreeBSD errno values.

Values are read with `clang -E -dM` and record offsets with offsetof (clang
emits them as an LLVM IR constant), one guarded block per architecture.

  tools/generate/generate_freebsd_uapi.py --src freebsd-src
"""
import argparse, os, re, subprocess, tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
OUT_FILE = ROOT / "library/std/os/platform/unix/freebsd/uapi.sn"

ARCHES = {"x86_64": "amd64", "aarch64": "arm64", "riscv64": "riscv"}
RELEASE = "freebsd14.2"
HEADERS = ["sys/types.h", "sys/fcntl.h", "sys/mman.h", "sys/socket.h", "sys/signal.h", "sys/errno.h",
           "sys/time.h", "sys/wait.h", "sys/resource.h", "sys/stat.h", "sys/dirent.h", "sys/poll.h",
           "sys/elf_common.h", "sys/sysctl.h", "sys/un.h", "netinet/in.h"]

# Constant functions of the shared direct-kernel provider:
# (std function, FreeBSD macro, result type, meaning)
SHARED = [
    ("علم_فتح_للكتابة_لينكس", "O_WRONLY", "ط64", "open for writing"),
    ("علم_فتح_للقراءة_والكتابة_لينكس", "O_RDWR", "ط64", "open for reading and writing"),
    ("علم_فتح_إنشاء_لينكس", "O_CREAT", "ط64", "create if missing"),
    ("علم_فتح_اقتطاع_لينكس", "O_TRUNC", "ط64", "truncate"),
    ("علم_فتح_إغلاق_عند_التنفيذ_لينكس", "O_CLOEXEC", "ط64", "close on execve"),
    ("علم_فتح_غير_حاصر_لينكس", "O_NONBLOCK", "ط64", "non-blocking"),
    ("علم_فتح_دليل_لينكس", "O_DIRECTORY", "ط64", "fail unless a directory"),
    ("علم_فتح_حصري_لينكس", "O_EXCL", "ط64", "with O_CREAT: fail if the path exists"),
    ("علم_مقبس_إغلاق_عند_التنفيذ_لينكس", "SOCK_CLOEXEC", "ط64", "socket type flag, unlike O_CLOEXEC"),
    ("علم_مقبس_غير_حاصر_لينكس", "SOCK_NONBLOCK", "ط64", "socket type flag, unlike O_NONBLOCK"),
    ("أمر_تكرار_واصف_بإغلاق_عند_التنفيذ_لينكس", "F_DUPFD_CLOEXEC", "ط64", "fcntl"),
    ("علم_حذف_دليل_لينكس", "AT_REMOVEDIR", "ط64", "unlinkat"),
    ("مستوى_مقبس_لينكس", "SOL_SOCKET", "ط64", "socket option level"),
    ("خيار_إعادة_استخدام_العنوان_لينكس", "SO_REUSEADDR", "ط64", ""),
    ("خيار_إبقاء_الاتصال_لينكس", "SO_KEEPALIVE", "ط64", ""),
    ("خيار_مخزن_الاستقبال_لينكس", "SO_RCVBUF", "ط64", ""),
    ("خيار_مخزن_الإرسال_لينكس", "SO_SNDBUF", "ط64", ""),
    ("خيار_خطأ_المقبس_لينكس", "SO_ERROR", "ط64", "pending socket error"),
    ("خيار_مهلة_الاستقبال_لينكس", "SO_RCVTIMEO", "ط64", "struct timeval, two 64-bit fields"),
    ("خيار_مهلة_الإرسال_لينكس", "SO_SNDTIMEO", "ط64", "struct timeval, two 64-bit fields"),
    ("خيار_عمر_آيبي4_لينكس", "IP_TTL", "ط64", "IPPROTO_IP level"),
    ("خيار_قفزات_آيبي6_لينكس", "IPV6_UNICAST_HOPS", "ط64", "IPPROTO_IPV6 level"),
    ("علم_إرسال_بلا_إشارة_لينكس", "MSG_NOSIGNAL", "ط64", "no SIGPIPE on a closed stream"),
    ("عائلة_آيبي6_لينكس", "AF_INET6", "ط64", ""),
    ("ربط_ذاكرة_خاص_لينكس", "MAP_PRIVATE", "ط64", ""),
    ("ربط_ذاكرة_مجهول_لينكس", "MAP_ANON", "ط64", ""),
    ("نصيحة_ذاكرة_استرداد_لينكس", "MADV_FREE", "ط64", "pages may be discarded; contents not kept"),
    ("ساعة_رتيبة_لينكس", "CLOCK_MONOTONIC", "ط64", ""),
    ("إشارة_انتهاء_طفل_لينكس", "SIGCHLD", "ط64", "fork(2) takes no arguments: std's clone arguments are ignored"),
    ("رمز_خطأ_قيد_التقدم_لينكس", "EINPROGRESS", "عدد", ""),
    ("رمز_خطأ_جار_بالفعل_لينكس", "EALREADY", "عدد", ""),
    ("رمز_خطأ_متصل_بالفعل_لينكس", "EISCONN", "عدد", ""),
    ("رمز_خطأ_انتهت_المهلة_لينكس", "ETIMEDOUT", "عدد", ""),
    ("نوع_مقبس_تدفق_لينكس", "SOCK_STREAM", "ط64", ""),
    ("نوع_مقبس_رسائل_لينكس", "SOCK_DGRAM", "ط64", ""),
    ("عائلة_يونكس_لينكس", "AF_UNIX", "ط64", "AF_LOCAL"),
    ("إشارة_أنبوب_مكسور_لينكس", "SIGPIPE", "ط64", "write to a pipe with no reader"),
    ("أمر_حجب_إشارات_لينكس", "SIG_BLOCK", "ط64", "sigprocmask: add to the mask"),
    ("أمر_ضبط_قناع_إشارات_لينكس", "SIG_SETMASK", "ط64", "sigprocmask: replace the mask"),
    ("قفل_ملف_مشترك_لينكس", "LOCK_SH", "ط64", "flock: shared lock"),
    ("قفل_ملف_حصري_لينكس", "LOCK_EX", "ط64", "flock: exclusive lock"),
    ("قفل_ملف_بلا_انتظار_لينكس", "LOCK_NB", "ط64", "flock: fail instead of waiting"),
    ("فك_قفل_ملف_لينكس", "LOCK_UN", "ط64", "flock: release"),
    ("رمز_خطأ_سيحظر_لينكس", "EAGAIN", "عدد", "also EWOULDBLOCK"),
    ("رمز_خطأ_أنبوب_مكسور_لينكس", "EPIPE", "عدد", ""),
]

# FreeBSD-only values: (std function, macro, result type, meaning)
OWN = [
    ("علم_بلا_تتبع_وصلة_فري_بي_إس_دي", "AT_SYMLINK_NOFOLLOW", "ط64", "fstatat"),
    ("حد_المكدس_فري_بي_إس_دي", "RLIMIT_STACK", "ط64", "getrlimit/setrlimit"),
    ("حالة_انتظار_متوقف_فري_بي_إس_دي", "_WSTOPPED", "ط64", "wait(2) status: low 7 bits, 0177 = stopped"),
    ("انتظار_بلا_حجب_فري_بي_إس_دي", "WNOHANG", "ط64", "wait4 option: return 0 for a live child"),
    ("متغير_عتاد_فري_بي_إس_دي", "CTL_HW", "ط64", "sysctl top-level hw"),
    ("متغير_حجم_الصفحة_فري_بي_إس_دي", "HW_PAGESIZE", "ط64", "sysctl hw.pagesize, an int"),
]

# Record layouts (std function, C expression, meaning).
LAYOUT = [
    ("حجم_سجل_معلومات_فري_بي_إس_دي", "sizeof(struct stat)", "struct stat (ino64 layout)"),
    ("إزاحة_نمط_معلومات_فري_بي_إس_دي", "__builtin_offsetof(struct stat, st_mode)", "st_mode, u16"),
    ("إزاحة_حجم_معلومات_فري_بي_إس_دي", "__builtin_offsetof(struct stat, st_size)", "st_size, i64"),
    ("إزاحة_تعديل_معلومات_فري_بي_إس_دي", "__builtin_offsetof(struct stat, st_mtim)", "st_mtim: tv_sec i64, tv_nsec i64"),
]
CHECKS = [
    # std's shared decoders assume these: fail generation instead of guessing.
    ("sizeof(((struct stat *)0)->st_mode)", 2), ("sizeof(((struct stat *)0)->st_size)", 8),
    ("sizeof(struct timespec)", 16), ("sizeof(struct timeval)", 16), ("sizeof(struct rlimit)", 16),
    ("sizeof(struct pollfd)", 8), ("__builtin_offsetof(struct pollfd, events)", 4),
    ("__builtin_offsetof(struct dirent, d_fileno)", 0), ("__builtin_offsetof(struct dirent, d_off)", 8),
    ("__builtin_offsetof(struct dirent, d_reclen)", 16), ("__builtin_offsetof(struct dirent, d_type)", 18),
    ("sizeof(struct sockaddr_in)", 16), ("__builtin_offsetof(struct sockaddr_in, sin_len)", 0),
    ("__builtin_offsetof(struct sockaddr_in, sin_family)", 1), ("sizeof(struct sockaddr_in6)", 28),
    ("sizeof(void *)", 8),
]

# FreeBSD errno -> std classification (os/error.sn صنف_خطأ_نظام).
ERRNO_CLASSES = [
    ("ENOENT", "غير_موجود"), ("EINTR", "عملية_انقطعت"), ("ENOMEM", "استنفاد_الموارد"),
    ("EACCES", "وصول_مرفوض"), ("EBUSY", "مورد_مشغول"), ("EEXIST", "موجود_مسبقا"),
    ("ENOTDIR", "ليس_دليلا"), ("EISDIR", "هو_دليل"), ("EINVAL", "وسيط_غير_صالح"),
    ("ENFILE", "استنفاد_الموارد"), ("EMFILE", "استنفاد_الموارد"), ("ENOSPC", "استنفاد_الموارد"),
    ("EAGAIN", "عملية_ستحظر"), ("ETIMEDOUT", "انتهت_المهلة"), ("ECONNREFUSED", "اتصال_مرفوض"),
]


def include_root(src, machine):
    root = tempfile.mkdtemp(prefix="freebsd-include-")
    links = {"sys": "sys/sys", "machine": f"sys/{machine}/include", "x86": "sys/x86/include",
             "netinet": "sys/netinet", "netinet6": "sys/netinet6"}
    for name, rel in links.items():
        if (Path(src) / rel).is_dir():
            os.symlink(Path(src).resolve() / rel, Path(root) / name)
    return root


def clang_args(src, arch):
    root = include_root(src, ARCHES[arch])
    return ["clang", f"--target={arch}-unknown-{RELEASE}", "-nostdinc",
            "-isystem", root, "-isystem", str(Path(src).resolve() / "include")]


def macros(src, arch):
    cmd = clang_args(src, arch) + ["-E", "-dM"]
    for h in HEADERS:
        cmd += ["-include", h]
    text = subprocess.run(cmd + ["-"], input="", capture_output=True, text=True, check=True).stdout
    table = {}
    for line in text.splitlines():
        m = re.match(r"#define (\w+) (.+)", line)
        if m:
            table[m.group(1)] = m.group(2).strip()
    return table


def evaluate(name, table, depth=0):
    if depth > 20 or name not in table:
        return None
    expr = table[name]
    expr = re.sub(r"\b([A-Za-z_]\w*)\b", lambda m: str(evaluate(m.group(1), table, depth + 1)), expr)
    expr = re.sub(r"\b0([0-7]+)\b", lambda m: str(int(m.group(1), 8)), expr)
    expr = re.sub(r"(0x[0-9a-fA-F]+|\d+)[UuLl]+\b", r"\1", expr)
    try:
        return int(eval(expr, {"__builtins__": {}}))
    except Exception:
        return None


def layouts(src, arch, exprs):
    """Values of constant C expressions, read back from clang's LLVM IR."""
    body = "".join(f"#include <{h}>\n" for h in HEADERS)
    body += "unsigned long sultanc_values[] = {" + ", ".join(exprs) + "};\n"
    with tempfile.NamedTemporaryFile("w", suffix=".c", delete=False) as f:
        f.write(body)
    ir = subprocess.run(clang_args(src, arch) + ["-S", "-emit-llvm", "-o", "-", f.name],
                        capture_output=True, text=True, check=True).stdout
    os.unlink(f.name)
    m = re.search(r"@sultanc_values = \S+ global \[\d+ x i64\] \[(.*?)\]", ir)
    return [int(v.split()[1]) for v in m.group(1).split(",")]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--src", required=True, help="FreeBSD source tree (sys/, include/)")
    args = ap.parse_args()
    lines = [
        "// GENERATED by tools/generate/generate_freebsd_uapi.py. Do not edit.",
        "// FreeBSD kernel ABI values read from FreeBSD's headers (releng/14.2), one",
        "// guarded block per architecture. The `..._لينكس` functions are the constant",
        "// interface of std's direct-kernel provider (platform/unix/linux), which",
        "// FreeBSD shares for the calls that take Linux's arguments; the",
        "// `..._فري_بي_إس_دي` ones serve FreeBSD's own code paths.",
        "استورد os.error;",
        "",
    ]
    for arch, machine in ARCHES.items():
        table = macros(args.src, arch)
        checks = layouts(args.src, arch, [c for c, _ in CHECKS])
        for (expr, want), got in zip(CHECKS, checks):
            if want != got:
                raise SystemExit(f"{arch}: {expr} is {got}, std assumes {want}")
        values = layouts(args.src, arch, [e for _, e, _ in LAYOUT])
        lines.append(f"// {arch}-unknown-freebsd (sys/{machine})")
        lines.append(f"إذا (الهدف.المنصة == «freebsd» && الهدف.المعمارية == «{arch}») {{")
        lines.append("    // The direct-kernel provider's constants.")

        def const(func, macro, kind, meaning):
            value = evaluate(macro, table)
            if value is None:
                raise SystemExit(f"{arch}: {macro} not found")
            note = f" // {macro}" + (f": {meaning}" if meaning else "")
            if kind == "عدد":
                lines.append(f"    عام عرف {func}(): عدد {{ أرجع {value}; }}{note}")
            else:
                lines.append(f"    عام عرف {func}(): ط64 {{ أرجع {hex(value)} مثل ط64; }}{note}")

        for row in SHARED:
            const(*row)
        # Not macros: facts of FreeBSD's ABI that select provider code paths.
        facts = [
            ("ربط_ذاكرة_بلا_حجز_لينكس", "ط64", "0 مثل ط64", "no MAP_NORESERVE (mman.h); PROT_NONE mappings are not charged to swap"),
            ("نسخ_عملية_بالأعلام_أولا_لينكس", "منطقي", "صواب", "clone argument order: unused, fork(2) takes none"),
            ("ربط_ذاكرة_بكتلة_وسائط_لينكس", "منطقي", "خطأ", "mmap takes its six arguments in registers"),
            ("ترتيب_بايت_كبير_لينكس", "منطقي", "خطأ", "kernel records are little-endian"),
            ("حجم_كلمة_لينكس", "طبيعي", "8", "long / pointer size (auxv entries)"),
            ("موضع_ملف_بنصفين_لينكس", "منطقي", "خطأ", "lseek takes a 64-bit offset"),
            ("نوم_بالساعة_لينكس", "منطقي", "خطأ", "nanosleep takes a 64-bit timespec"),
            ("عنوان_مقبس_بطول_لينكس", "منطقي", "صواب", "sockaddr starts with sa_len (u8), then sa_family (u8)"),
            ("دليل_العمل_بطوله_لينكس", "منطقي", "خطأ", "__getcwd returns 0, not the length"),
            ("إزاحة_اسم_مدخل_دليل_لينكس", "طبيعي", str(layouts(args.src, arch, ["__builtin_offsetof(struct dirent, d_name)"])[0]), "struct dirent d_name"),
            ("تخطى_مداخل_دليل_بلا_رقم_لينكس", "منطقي", "صواب", "d_fileno 0 marks an unused record (libc readdir skips it)"),
            ("حجم_مجموعة_إشارات_لينكس", "طبيعي", str(layouts(args.src, arch, ["sizeof(sigset_t)"])[0]), "sizeof(sigset_t): 128 bits"),
            ("حجم_مسار_مقبس_يونكس_لينكس", "طبيعي", str(layouts(args.src, arch, ["sizeof(((struct sockaddr_un *)0)->sun_path)"])[0]), "struct sockaddr_un sun_path, after sun_len and sun_family"),
            ("ميكروثواني_زمن_32_لينكس", "منطقي", "صواب" if layouts(args.src, arch, ["sizeof(((struct timeval *)0)->tv_usec)"])[0] == 4 else "خطأ", "timeval tv_usec is a 32-bit int (suseconds_t)"),
        ]
        for func, kind, value, meaning in facts:
            lines.append(f"    عام عرف {func}(): {kind} {{ أرجع {value}; }} // {meaning}")
        lines.append("    // FreeBSD's own values.")
        for row in OWN:
            const(*row)
        for (func, _, meaning), value in zip(LAYOUT, values):
            lines.append(f"    عام عرف {func}(): طبيعي {{ أرجع {value}; }} // {meaning}")
        lines.append("    // sys/errno.h; EAGAIN (35) and every value above 34 differ from Linux.")
        lines.append("    عام عرف صنف_خطأ_نواة_فري_بي_إس_دي_أساسيا(الرمز: عدد): صنف_خطأ_نظام {")
        for macro, cls in ERRNO_CLASSES:
            value = evaluate(macro, table)
            if value is None:
                raise SystemExit(f"{arch}: {macro} not found")
            lines.append(f"        إذا (الرمز == {value}) {{ أرجع صنف_خطأ_نظام.{cls}(); }}; // {macro}")
        lines.append("        أرجع صنف_خطأ_نظام.غير_معروف();")
        lines.append("    }")
        lines.append("}")
        lines.append("")
        print(arch)
    OUT_FILE.parent.mkdir(parents=True, exist_ok=True)
    OUT_FILE.write_text("\n".join(lines), encoding="utf-8")


if __name__ == "__main__":
    main()
