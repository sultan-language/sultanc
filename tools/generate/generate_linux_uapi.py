#!/usr/bin/env python3
"""Generate the per-architecture Linux UAPI constants of std.

Flag and option values that differ between Linux architectures (open flags,
socket levels/options, mmap flags, SIGCHLD) are read from each architecture's
kernel UAPI headers with `clang -E -dM`. std code calls the generated
zero-argument functions instead of spelling numbers. Same --include
arguments as generate_linux_syscalls.py.
"""
import argparse, re, subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
OUT_FILE = ROOT / "library/std/os/platform/unix/linux/uapi.sn"
HEADERS = ["asm/fcntl.h", "linux/mman.h", "asm/socket.h", "asm/signal.h", "asm/errno.h",
           "linux/fcntl.h", "linux/time.h", "linux/in.h", "linux/in6.h"]

# arch key: (clang triple, SultanC guard spelling of الهدف.المعمارية)
ARCHES = {
    "x86_64": ("x86_64-linux-gnu", "x86_64"),
    "aarch64": ("aarch64-linux-gnu", "aarch64"),
    "riscv64": ("riscv64-linux-gnu", "riscv64"),
    "loongarch64": ("loongarch64-linux-gnu", "loongarch64"),
    "powerpc64": ("powerpc64le-linux-gnu", "powerpc64"),
    "s390x": ("s390x-linux-gnu", "systemz"),
    "mips64": ("mips64el-linux-gnuabi64", "mips64"),
    "sparcv9": ("sparc64-linux-gnu", "sparcv9"),
    "x86": ("i386-linux-gnu", "x86"),
    "arm": ("arm-linux-gnueabihf", "arm"),
}

# 32-bit ABIs: 4-byte words, a 12-byte siginfo header, and the time64 /
# large-file entry points (see the ilp32: rows of linux_syscalls.def).
ILP32 = {"x86", "arm"}

# (std function, macro, meaning)
CONSTANTS = [
    ("علم_فتح_للكتابة_لينكس", "O_WRONLY", "open for writing"),
    ("علم_فتح_للقراءة_والكتابة_لينكس", "O_RDWR", "open for reading and writing"),
    ("علم_فتح_إنشاء_لينكس", "O_CREAT", "create if missing"),
    ("علم_فتح_اقتطاع_لينكس", "O_TRUNC", "truncate"),
    ("علم_فتح_إغلاق_عند_التنفيذ_لينكس", "O_CLOEXEC", "close on execve; also SOCK_CLOEXEC"),
    ("علم_فتح_غير_حاصر_لينكس", "O_NONBLOCK", "non-blocking; also SOCK_NONBLOCK"),
    ("علم_فتح_دليل_لينكس", "O_DIRECTORY", "fail unless a directory"),
    ("علم_فتح_حصري_لينكس", "O_EXCL", "with O_CREAT: fail if the path exists"),
    # Linux defines SOCK_CLOEXEC and SOCK_NONBLOCK as the O_ flags
    # (include/linux/net.h); FreeBSD's differ (freebsd/uapi.sn).
    ("علم_مقبس_إغلاق_عند_التنفيذ_لينكس", "O_CLOEXEC", "SOCK_CLOEXEC"),
    ("علم_مقبس_غير_حاصر_لينكس", "O_NONBLOCK", "SOCK_NONBLOCK"),
    ("أمر_تكرار_واصف_بإغلاق_عند_التنفيذ_لينكس", "F_DUPFD_CLOEXEC", "fcntl"),
    ("علم_حذف_دليل_لينكس", "AT_REMOVEDIR", "unlinkat"),
    ("مستوى_مقبس_لينكس", "SOL_SOCKET", "socket option level"),
    ("خيار_إعادة_استخدام_العنوان_لينكس", "SO_REUSEADDR", ""),
    ("خيار_إبقاء_الاتصال_لينكس", "SO_KEEPALIVE", ""),
    ("خيار_مخزن_الاستقبال_لينكس", "SO_RCVBUF", ""),
    ("خيار_مخزن_الإرسال_لينكس", "SO_SNDBUF", ""),
    ("خيار_خطأ_المقبس_لينكس", "SO_ERROR", "pending socket error"),
    ("خيار_مهلة_الاستقبال_لينكس", "SO_RCVTIMEO_OLD", "struct timeval of two longs"),
    ("خيار_مهلة_الإرسال_لينكس", "SO_SNDTIMEO_OLD", "struct timeval of two longs"),
    ("خيار_عمر_آيبي4_لينكس", "IP_TTL", "IPPROTO_IP level"),
    ("خيار_قفزات_آيبي6_لينكس", "IPV6_UNICAST_HOPS", "IPPROTO_IPV6 level"),
    ("ربط_ذاكرة_خاص_لينكس", "MAP_PRIVATE", ""),
    ("ربط_ذاكرة_مجهول_لينكس", "MAP_ANONYMOUS", ""),
    ("ربط_ذاكرة_بلا_حجز_لينكس", "MAP_NORESERVE", ""),
    ("نصيحة_ذاكرة_استرداد_لينكس", "MADV_DONTNEED", "anonymous pages read back as zero"),
    ("ساعة_رتيبة_لينكس", "CLOCK_MONOTONIC", ""),
    ("إشارة_انتهاء_طفل_لينكس", "SIGCHLD", "child status change, the clone exit signal"),
    ("إشارة_أنبوب_مكسور_لينكس", "SIGPIPE", "write to a pipe with no reader"),
    ("أمر_حجب_إشارات_لينكس", "SIG_BLOCK", "rt_sigprocmask: add to the mask"),
    ("أمر_ضبط_قناع_إشارات_لينكس", "SIG_SETMASK", "rt_sigprocmask: replace the mask"),
    ("قفل_ملف_مشترك_لينكس", "LOCK_SH", "flock: shared lock"),
    ("قفل_ملف_حصري_لينكس", "LOCK_EX", "flock: exclusive lock"),
    ("قفل_ملف_بلا_انتظار_لينكس", "LOCK_NB", "flock: fail instead of waiting"),
    ("فك_قفل_ملف_لينكس", "LOCK_UN", "flock: release"),
]

# errno values beyond asm-generic/errno-base.h differ on MIPS and SPARC.
ERRNOS = [
    ("رمز_خطأ_قيد_التقدم_لينكس", "EINPROGRESS"),
    ("رمز_خطأ_جار_بالفعل_لينكس", "EALREADY"),
    ("رمز_خطأ_متصل_بالفعل_لينكس", "EISCONN"),
    ("رمز_خطأ_انتهت_المهلة_لينكس", "ETIMEDOUT"),
    ("رمز_خطأ_سيحظر_لينكس", "EAGAIN"),
    ("رمز_خطأ_أنبوب_مكسور_لينكس", "EPIPE"),
]


def macros(triple, includes):
    cmd = ["clang", f"--target={triple}", "-nostdinc", "-D__KERNEL_STRICT_NAMES"]
    for inc in includes:
        cmd += ["-I", inc]
    cmd += ["-E", "-dM"]
    for h in HEADERS:
        cmd += ["-include", h]
    cmd += ["-"]
    text = subprocess.run(cmd, input="", capture_output=True, text=True, check=True).stdout
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
    expr = re.sub(r"(\d+)[UuLl]+\b", r"\1", expr)
    try:
        return int(eval(expr, {"__builtins__": {}}))
    except Exception:
        return None


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--include", action="append", default=[], help="arch=dir[:dir]")
    args = ap.parse_args()
    lines = [
        "// GENERATED by tools/generate/generate_linux_uapi.py. Do not edit.",
        "// Linux UAPI constants that differ between architectures, read from each",
        "// architecture's kernel headers. One guarded block per architecture.",
        "",
    ]
    for spec in args.include:
        arch, dirs = spec.split("=", 1)
        triple, guard = ARCHES[arch]
        table = macros(triple, dirs.split(":"))
        lines.append(f"// {triple}")
        lines.append(f"لو ((الهدف.المنصة == «linux» || الهدف.المنصة == «android») && الهدف.المعمارية == «{guard}») {{")
        for func, macro, meaning in CONSTANTS:
            value = evaluate(macro, table)
            if value is None:
                raise SystemExit(f"{arch}: {macro} not found")
            note = f" // {macro}" + (f": {meaning}" if meaning else "")
            lines.append(f"    عام عرف {func}(): ط64 {{ أرجع {hex(value)} مثل ط64; }}{note}")
        for func, macro in ERRNOS:
            value = evaluate(macro, table)
            if value is None:
                raise SystemExit(f"{arch}: {macro} not found")
            lines.append(f"    عام عرف {func}(): عدد {{ أرجع {value}; }} // {macro}")
        # Socket types are not UAPI macros: MIPS alone swaps them
        # (arch/mips/include/asm/socket.h, ARCH_HAS_SOCKET_TYPES).
        stream, dgram = (2, 1) if arch == "mips64" else (1, 2)
        lines.append(f"    عام عرف نوع_مقبس_تدفق_لينكس(): ط64 {{ أرجع {stream} مثل ط64; }} // SOCK_STREAM")
        lines.append(f"    عام عرف نوع_مقبس_رسائل_لينكس(): ط64 {{ أرجع {dgram} مثل ط64; }} // SOCK_DGRAM")
        # s390 clone takes (stack, flags, ...); every other ABI (flags, stack, ...).
        first = "خطأ" if arch == "s390x" else "صواب"
        lines.append(f"    عام عرف نسخ_عملية_بالأعلام_أولا_لينكس(): منطقي {{ أرجع {first}; }} // clone argument order")
        # s390 __NR_mmap is old_mmap (i386's too, but std maps mmap2 there): one pointer to the six arguments.
        block = "صواب" if arch == "s390x" else "خطأ"
        lines.append(f"    عام عرف ربط_ذاكرة_بكتلة_وسائط_لينكس(): منطقي {{ أرجع {block}; }} // mmap argument block")
        # siginfo: MIPS orders si_code before si_errno (arch/mips/include/uapi/asm/siginfo.h).
        code_at = 4 if arch == "mips64" else 8
        lines.append(f"    عام عرف إزاحة_رمز_معلومات_إشارة_لينكس(): طبيعي {{ أرجع {code_at}; }} // siginfo si_code offset")
        big = "صواب" if arch in ("s390x", "sparcv9") else "خطأ"
        lines.append(f"    عام عرف ترتيب_بايت_كبير_لينكس(): منطقي {{ أرجع {big}; }} // kernel records are big-endian")
        narrow = arch in ILP32
        lines.append(f"    عام عرف حجم_كلمة_لينكس(): طبيعي {{ أرجع {4 if narrow else 8}; }} // long / pointer size (auxv entries)")
        # siginfo: three ints, then the union aligned to the word size.
        lines.append(f"    عام عرف إزاحة_حمولة_معلومات_إشارة_لينكس(): طبيعي {{ أرجع {12 if narrow else 16}; }} // siginfo si_pid offset")
        flag = "صواب" if narrow else "خطأ"
        lines.append(f"    عام عرف موضع_ملف_بنصفين_لينكس(): منطقي {{ أرجع {flag}; }} // _llseek(fd, high, low, &result, whence)")
        lines.append(f"    عام عرف نوم_بالساعة_لينكس(): منطقي {{ أرجع {flag}; }} // clock_nanosleep_time64 replaces nanosleep")
        # Kernel-internal on Linux (include/linux/socket.h), equal on every
        # architecture; FreeBSD's come from its headers (freebsd/uapi.sn).
        lines.append("    عام عرف علم_إرسال_بلا_إشارة_لينكس(): ط64 { أرجع 0x4000 مثل ط64; } // MSG_NOSIGNAL")
        lines.append("    عام عرف عائلة_آيبي6_لينكس(): ط64 { أرجع 0xa مثل ط64; } // AF_INET6")
        lines.append("    عام عرف عائلة_يونكس_لينكس(): ط64 { أرجع 0x1 مثل ط64; } // AF_UNIX")
        # struct sockaddr_un (include/uapi/linux/un.h): sa_family_t, then UNIX_PATH_MAX bytes.
        lines.append("    عام عرف حجم_مسار_مقبس_يونكس_لينكس(): طبيعي { أرجع 108; } // UNIX_PATH_MAX: sockaddr_un sun_path")
        # The kernel's sigset_t is _NSIG bits: 64, and 128 on MIPS
        # (arch/mips/include/uapi/asm/signal.h); rt_sigprocmask checks the size.
        nsig = 16 if arch == "mips64" else 8
        lines.append(f"    عام عرف حجم_مجموعة_إشارات_لينكس(): طبيعي {{ أرجع {nsig}; }} // sizeof(sigset_t): _NSIG / 8")
        # Facts that select the direct-kernel provider's code paths (FreeBSD differs).
        lines.append("    عام عرف عنوان_مقبس_بطول_لينكس(): منطقي { أرجع خطأ; } // sockaddr starts with sa_family (native u16)")
        lines.append("    عام عرف دليل_العمل_بطوله_لينكس(): منطقي { أرجع صواب; } // getcwd returns the length including NUL")
        lines.append("    عام عرف إزاحة_اسم_مدخل_دليل_لينكس(): طبيعي { أرجع 19; } // linux_dirent64 d_name")
        lines.append("    عام عرف تخطى_مداخل_دليل_بلا_رقم_لينكس(): منطقي { أرجع خطأ; } // getdents64 records are all live")
        # timeval.tv_usec is __kernel_suseconds_t: a long everywhere except
        # where asm/posix_types.h makes it an int (sparc64).
        usec32 = "خطأ"
        for inc in dirs.split(":"):
            pt = Path(inc) / "asm/posix_types.h"
            if pt.exists() and re.search(r"typedef\s+int\s+__kernel_suseconds_t", pt.read_text()):
                usec32 = "صواب"
        lines.append(f"    عام عرف ميكروثواني_زمن_32_لينكس(): منطقي {{ أرجع {usec32}; }} // timeval tv_usec is a 32-bit int")
        lines.append("}")
        lines.append("")
        print(arch)
    OUT_FILE.write_text("\n".join(lines), encoding="utf-8")


if __name__ == "__main__":
    main()
