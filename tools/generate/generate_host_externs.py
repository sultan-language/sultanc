#!/usr/bin/env python3
"""Generate compiler/interpreter/execution/host_externs_generated.sn.

The interpreter's foreign-call table for hosts where std calls C functions
(Windows: kernel32, ws2_32, ...; macOS: libSystem). Every extern std
declares for that platform is re-declared here with the same library and the
same C type (a SultanC executable declares each import once, so the
declaration must agree with std's; every pointer is spelled `*ط8`, the same
C type as std's `*T`), and dispatched by its link name; a host address
becomes the pointer argument, and a pointer result its address, through
host_memory.sn. The
host loader resolves each symbol as for any SultanC executable.

The interpreter marshals every call (compiler/interpreter/execution/
foreign_calls.sn), as Miri does for the foreign functions it supports and
libffi/ctypes do for a resolved symbol: an integer passes as is, a null
pointer as 0, a pointer into a host region (memory the host returned) as
its host address, a pointer to interpreter storage as the address of a host
copy of the bytes it designates (copied back afterwards), and a pointer to a
table of pointers as a host table of such addresses. A pointer result maps
back to the argument it aliases, to a known host region, or to a new opaque
host region. Functions whose results point into host memory with an extent
only their contract gives (readdir's dirent, getaddrinfo's list, a CERT
context, errno) are shims in foreign_calls.sn, as in Miri.

The generator FAILS when std declares an extern for macOS or Windows the
interpreter cannot call: a parameter or result type that is neither an
integer nor a pointer (a float, a record by value), or a pointer result /
pointer-bearing record argument that no shim in foreign_calls.sn covers.

Audit (2026-11-06) of every std extern, per OS:
  - FreeBSD / Linux / Android: std issues raw syscalls, declares no extern.
  - macOS (libSystem): every extern is tabled; shims for __error (errno
    cell), opendir/readdir/closedir (interpreter DIR handles synthesising
    the 64-bit-inode dirent), getaddrinfo/freeaddrinfo (the list built in
    interpreter memory), dladdr (strings copied). getcwd's result aliases
    its buffer argument; posix_spawnp's argv/envp are marshalled as host
    pointer tables.
  - Windows: every extern is tabled; shims for GetLastError/SetLastError/
    WSAGetLastError (the interpreter's last-error cell), GetCommandLineW/
    CommandLineToArgvW/LocalFree (the program's own command line),
    GetEnvironmentStringsW (a host region up to the block's end; the
    FreeEnvironmentStringsW argument passes back its host address),
    GetAddrInfoW/FreeAddrInfoW (as getaddrinfo), CertEnumCertificatesInStore
    (a host region for the CERT_CONTEXT and its encoded bytes).
    PrefetchVirtualMemory takes a pointer to an all-integer record, staged
    with the C layout.

Usage: python3 tools/generate/generate_host_externs.py [--check]
"""
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parents[2]
OUT = ROOT / "compiler/interpreter/execution/host_externs_generated.sn"
STD = ROOT / "library/std/os/platform"
BRIDGE = ROOT / "compiler/interpreter/execution/foreign_calls.sn"

INTS = {"ص8": 8, "ص16": 16, "ص32": 32, "ص64": 64,
        "ط8": 8, "ط16": 16, "ط32": 32, "ط64": 64,
        "عدد": 64, "طبيعي": 64}

# Externs the interpreter answers itself (foreign_calls.sn), per platform.
SHIMS = {
    "macos": {"__error", "opendir", "opendir$INODE64", "readdir",
              "readdir$INODE64", "closedir", "getaddrinfo", "freeaddrinfo",
              "dladdr"},
    "windows": {"GetLastError", "SetLastError", "WSAGetLastError",
                "GetCommandLineW", "CommandLineToArgvW", "LocalFree",
                "GetEnvironmentStringsW", "GetAddrInfoW", "FreeAddrInfoW",
                "CertEnumCertificatesInStore"},
}
# Pointer results the generic bridge maps back without a shim: the result
# aliases a pointer argument (getcwd returns its buffer).
ALIASING_RESULTS = {"getcwd"}

DECL = re.compile(r"خارجي «([^»]*)» عرف (\w+)\s*\((.*?)\)\s*:\s*([^;{]+);", re.S)
RECORD = re.compile(r"نوع (\w+) = \{(.*?)\};", re.S)


def records():
    found = {}
    for f in sorted(STD.rglob("*.sn")):
        for m in RECORD.finditer(f.read_text(encoding="utf-8")):
            fields = []
            for part in m.group(2).split(","):
                if ":" in part:
                    fields.append(" ".join(part.split(":", 1)[1].split()))
            found[m.group(1)] = fields
    return found


def kind(t, recs):
    """('int', type) | ('ptr', pointee-description) | ('void',) | None."""
    t = " ".join(t.split())
    if t in INTS:
        return ("int", t)
    if t == "فراغ":
        return ("void",)
    m = re.match(r"^\*(متغير )?(.+)$", t)
    if m:
        inner = m.group(2).strip()
        if inner in INTS:
            return ("ptr", "int")
        if inner.startswith("*"):
            return ("ptr", "table")
        if inner in recs:
            bearing = any(f.startswith("*") for f in recs[inner])
            return ("ptr", "record-with-pointers" if bearing else "record")
        return ("ptr", "other")
    return None


def collect(platform_dir, plat, recs, errors):
    seen = {}
    for f in sorted((STD / platform_dir).rglob("*.sn")):
        text = f.read_text(encoding="utf-8")
        for m in DECL.finditer(text):
            lib, name, params, ret = m.group(1), m.group(2), m.group(3), m.group(4).strip()
            alias = None
            am = re.match(r"^(.*?)\s*=\s*«([^»]*)»$", ret)
            if am:
                ret, alias = am.group(1).strip(), am.group(2)
            link = alias or name
            where = f"{f.relative_to(ROOT)}: {link}"
            ps = []
            if params.strip():
                for p in params.split(","):
                    if ":" not in p:
                        errors.append(f"{where}: unparsed parameter {p.strip()!r}")
                        continue
                    t = " ".join(p.split(":", 1)[1].split())
                    k = kind(t, recs)
                    if k is None or k[0] == "void":
                        errors.append(f"{where}: parameter type {t} cannot be called by the interpreter")
                        continue
                    ps.append((t, k))
            rk = kind(ret, recs)
            if rk is None:
                errors.append(f"{where}: result type {ret} cannot be called by the interpreter")
                continue
            shimmed = link in SHIMS[plat]
            if rk[0] == "ptr" and not shimmed and link not in ALIASING_RESULTS:
                errors.append(f"{where}: pointer result without a shim in foreign_calls.sn")
            for t, k in ps:
                if k[0] == "ptr" and k[1] in ("record-with-pointers", "other") and not shimmed:
                    errors.append(f"{where}: argument {t} needs a shim in foreign_calls.sn")
            if link in seen:
                continue
            seen[link] = (lib, ps, ret, rk)
    return seen


def local(link):
    return re.sub(r"[^0-9A-Za-z_]", "_", link)


def word_type(t, k):
    # Every pointer is the same C (and LLVM) type, so `*ط8` declares the
    # same import as std's own `*T` declaration of the symbol.
    return "*ط8" if k[0] == "ptr" else t


def emit_platform(cond, entries):
    out = [f"لو ({cond}) {{"]
    for link, (lib, ps, ret, rk) in sorted(entries.items()):
        params = ", ".join(f"م{i}: {word_type(t, k)}" for i, (t, k) in enumerate(ps))
        result = "*ط8" if rk[0] == "ptr" else ret
        decl = f"خارجي «{lib}» عرف مضيف_{local(link)}({params}): {result}"
        if "$INODE64" in link:
            # Darwin's $INODE64 symbols exist only on x86_64 (Rust's libc puts
            # link_name = "stat$INODE64" behind not(target_arch = "aarch64")).
            # arm64 has only the 64-bit-inode ABI, under the plain name.
            plain = link.replace("$INODE64", "")
            out.append("    لو (الهدف.المعمارية == «x86_64») {")
            out.append(f"        {decl} = «{link}»;")
            out.append("    } وإلا {")
            out.append(f"        {decl} = «{plain}»;")
            out.append("    }")
            continue
        out.append(f"    {decl} = «{link}»;")
    out.append("")
    out.append("    عام عرف دالة_خارجية_مضيف_معروفة(الاسم: &متجه ط8): منطقي {")
    for link in sorted(entries):
        out.append(f"        لو (بايتات_تساوي_نص_خارجية(الاسم, «{link}»)) {{ أرجع صواب; }};")
    out.append("        أرجع خطأ;")
    out.append("    }")
    out.append("")
    out.append("    // Calls the extern named الاسم with one word per argument (an integer")
    out.append("    // extended to 64 bits, or a host address) and returns its result word")
    out.append("    // (0 for فراغ); لا_شيء for an unknown name or a wrong argument count.")
    out.append("    عام عرف نفذ_دالة_خارجية_مضيف(الاسم: &متجه ط8, الكلمات: &متجه ط64): اختياري ط64 {")
    for link, (lib, ps, ret, rk) in sorted(entries.items()):
        out.append(f"        لو (بايتات_تساوي_نص_خارجية(الاسم, «{link}»)) {{")
        out.append(f"            لو ((الطول(*الكلمات) مثل طبيعي) != {len(ps)}) {{ أرجع اختياري.لا_شيء(); }};")
        args = []
        for i, (t, k) in enumerate(ps):
            if k[0] == "int":
                args.append(f"(*الكلمات)[{i}] مثل {t}")
            else:
                args.append(f"مؤشر_من_عنوان_مضيف((*الكلمات)[{i}])")
        call = f"مضيف_{local(link)}({', '.join(args)})"
        if rk[0] == "void":
            out.append(f"            {call};")
            out.append("            أرجع اختياري.موجود(0 مثل ط64);")
        else:
            if rk[0] == "ptr":
                out.append(f"            دع الناتج: *ط8 = {call};")
                out.append("            أرجع اختياري.موجود(عنوان_من_مؤشر_مضيف(الناتج));")
            else:
                out.append(f"            دع الناتج: {rk[1]} = {call};")
                out.append("            أرجع اختياري.موجود(الناتج مثل ط64);")
        out.append("        };")
    out.append("        أرجع اختياري.لا_شيء();")
    out.append("    }")
    out.append("}")
    return out


def main():
    recs = records()
    errors = []
    win = collect("windows", "windows", recs, errors)
    mac = collect("unix/apple", "macos", recs, errors)
    # An extern outside the per-OS directories could reach macOS or Windows
    # without being audited here: std keeps every extern in os/platform.
    for f in sorted((ROOT / "library/std").rglob("*.sn")):
        rel = f.relative_to(ROOT / "library/std").as_posix()
        if rel.startswith("os/platform/"):
            continue
        if DECL.search(f.read_text(encoding="utf-8")):
            errors.append(f"library/std/{rel}: extern outside os/platform/ (not audited for the interpreter)")
    bridge = BRIDGE.read_text(encoding="utf-8") if BRIDGE.exists() else ""
    for plat, names in SHIMS.items():
        for name in sorted(names):
            if f"«{name}»" not in bridge:
                errors.append(f"{plat}: shim for {name} missing from {BRIDGE.relative_to(ROOT)}")
    if errors:
        for e in errors:
            print(f"generate_host_externs: {e}", file=sys.stderr)
        return 1
    lines = [
        "// GENERATED by tools/generate/generate_host_externs.py. Do not edit.",
        "// The foreign-call table of the interpreter's host bridge: std's C",
        "// externs for this host, re-declared with their libraries and C types,",
        "// dispatched by link name (see the generator for the contract).",
        "",
        "استورد compiler.interpreter.execution.host_memory;",
        "",
        "عرف بايتات_تساوي_نص_خارجية(الأولى: &متجه ط8, الثاني: نص): منطقي {",
        "    لو ((الطول(*الأولى) مثل طبيعي) != (الطول(الثاني) مثل طبيعي)) { أرجع خطأ; };",
        "    دع ف: متغير طبيعي = 0;",
        "    طالما (ف < (الطول(الثاني) مثل طبيعي)) {",
        "        لو ((*الأولى)[ف] != الثاني[ف]) { أرجع خطأ; };",
        "        ف := ف + 1;",
        "    }",
        "    أرجع صواب;",
        "}",
        "",
    ]
    lines += emit_platform("الهدف.المنصة == «windows»", win)
    lines.append("")
    lines += emit_platform("الهدف.المنصة == «macos»", mac)
    lines.append("")
    lines.append("لو (الهدف.المنصة != «windows» && الهدف.المنصة != «macos») {")
    lines.append("    عام عرف دالة_خارجية_مضيف_معروفة(الاسم: &متجه ط8): منطقي { أرجع خطأ; }")
    lines.append("    عام عرف نفذ_دالة_خارجية_مضيف(الاسم: &متجه ط8, الكلمات: &متجه ط64): اختياري ط64 {")
    lines.append("        أرجع اختياري.لا_شيء();")
    lines.append("    }")
    lines.append("}")
    text = "\n".join(lines) + "\n"
    if "--check" in sys.argv:
        if OUT.read_text(encoding="utf-8") != text:
            print(f"{OUT} is out of date", file=sys.stderr)
            return 1
        return 0
    OUT.write_text(text, encoding="utf-8")
    print(f"wrote {OUT}: windows {len(win)}, macos {len(mac)} externs")
    return 0


if __name__ == "__main__":
    sys.exit(main())
