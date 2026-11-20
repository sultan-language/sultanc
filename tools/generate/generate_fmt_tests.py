#!/usr/bin/env python3
"""Generates the std.fmt spec tests from Rust's own output.

Every case is a format string and its arguments. The script writes one Rust
program that formats every case with format!, runs it (rustc must be on
PATH), and writes the SultanC tests that expect exactly Rust's text:

    tests/en/stdlib/fmt/spec_rust.sn
    tests/ar/stdlib/fmt/spec_rust.sn

Run from the repository root:  python3 tools/generate/generate_fmt_tests.py

It also writes tests/{en,ar}/stdlib/fmt/arabic.sn from ARABIC_CASES, whose
expected text is written by hand: there std.fmt deliberately differs from
Rust (widths in grapheme clusters, harakat kept by Debug, bytes as text).
"""

import os
import struct
import subprocess
import sys
import tempfile

# (template, [(kind, value), ...])
CASES = [
    # Positional arguments and escapes.
    ("{}", [("int", 42)]),
    ("{} {}", [("int", 1), ("text", "two")]),
    ("{0} {0}", [("text", "x")]),
    ("{1} {0}", [("int", 1), ("int", 2)]),
    ("{} {0} {}", [("text", "a"), ("text", "b")]),
    ("{{}}", []),
    ("{{{}}}", [("int", 7)]),
    ("a}}b{{c", []),
    ("{  }", [("int", 3)]),
    ("{:}", [("int", 3)]),
    # Fill and alignment.
    ("[{:<8}]", [("text", "abc")]),
    ("[{:^8}]", [("text", "abc")]),
    ("[{:>8}]", [("text", "abc")]),
    ("[{:*^9}]", [("text", "abc")]),
    ("[{:^7}]", [("text", "ab")]),
    ("[{:8}]", [("text", "abc")]),
    ("[{:6}]", [("int", 42)]),
    ("[{:<6}]", [("int", 42)]),
    ("[{:^6}]", [("int", 42)]),
    ("[{:>6}]", [("int", -42)]),
    ("[{:-^7}]", [("bool", True)]),
    ("[{:é>5}]", [("int", 9)]),
    ("[{:ـ^10}]", [("text", "سلام")]),
    ("[{:>>4}]", [("int", 1)]),
    ("[{:<<4}]", [("int", 1)]),
    ("[{:^^5}]", [("int", 1)]),
    ("[{:2}]", [("text", "abcdef")]),
    ("[{:>8}]", [("text", "سلام")]),
    ("[{:<8}]", [("text", "مرحبا")]),
    ("[{:^9}]", [("text", "عالم")]),
    # Sign.
    ("{:+}", [("int", 5)]),
    ("{:+}", [("int", -5)]),
    ("{:+}", [("int", 0)]),
    ("{:+}", [("u8", 7)]),
    ("{:-}", [("int", 5)]),
    ("{:+}", [("f64", 1.5)]),
    ("{:+}", [("f64", -0.0)]),
    ("{:+}", [("f64b", 0x7FF8000000000000)]),
    ("{:+}", [("f64b", 0x7FF0000000000000)]),
    ("{:+.2}", [("f64", 0.0)]),
    # Alternate forms.
    ("{:#x}", [("int", 255)]),
    ("{:#X}", [("int", 255)]),
    ("{:#o}", [("int", 8)]),
    ("{:#b}", [("int", 5)]),
    ("{:x}", [("i8", -1)]),
    ("{:X}", [("i16", -1)]),
    ("{:o}", [("u32", 4294967295)]),
    ("{:b}", [("i32", -2)]),
    ("{:x}", [("i64", -1)]),
    ("{:x}", [("int", -255)]),
    ("{:#010x}", [("int", 255)]),
    ("{:#010b}", [("int", 5)]),
    ("{:08x}", [("u16", 48879)]),
    ("[{:<#8x}]", [("int", 255)]),
    ("[{:^#10b}]", [("u8", 5)]),
    ("{:#x}", [("int", 0)]),
    ("{:+#x}", [("int", 10)]),
    ("{:#}", [("int", 10)]),
    # Zero padding.
    ("{:08}", [("int", -42)]),
    ("{:+08}", [("int", 42)]),
    ("{:08.3}", [("f64", -3.14159)]),
    ("{:<05}", [("int", 7)]),
    ("{:^05}", [("int", -7)]),
    ("[{:05}]", [("text", "ab")]),
    ("[{:06}]", [("bool", True)]),
    ("{:010}", [("f64b", 0x7FF8000000000000)]),
    ("{:010}", [("f64b", 0xFFF0000000000000)]),
    ("{:05}", [("int", 123456)]),
    ("{:00}", [("int", 5)]),
    ("{:03}", [("int", 0)]),
    # Width and precision.
    ("[{:10.3}]", [("f64", 3.14159)]),
    ("[{:<10.3}]", [("f64", 3.14159)]),
    ("{:.0}", [("f64", 0.5)]),
    ("{:.0}", [("f64", 1.5)]),
    ("{:.0}", [("f64", 2.5)]),
    ("{:.0}", [("f64", -0.5)]),
    ("{:.1}", [("f64", 0.25)]),
    ("{:.1}", [("f64", 0.35)]),
    ("{:.1}", [("f64", -0.04)]),
    ("{:.3}", [("f64", 1e-10)]),
    ("{:.2}", [("f64", 1e21)]),
    ("{:.2}", [("f64", 0.995)]),
    ("{:.2}", [("f64", 9.995)]),
    ("{:.3}", [("f64", 99.9995)]),
    ("{:.20}", [("f64", 0.1)]),
    ("{:.5}", [("f32", 0.1)]),
    ("{:.1}", [("f64", 0.05)]),
    ("{:.0}", [("f64", 0.0)]),
    ("{:.2}", [("f64", -0.0)]),
    ("{:.5}", [("text", "abcdefgh")]),
    ("[{:8.3}]", [("text", "abcdef")]),
    ("[{:>8.3}]", [("text", "abcdef")]),
    ("[{:.0}]", [("text", "abc")]),
    ("[{:.10}]", [("text", "abc")]),
    ("{:.2}", [("int", 5)]),
    ("{:5.2}", [("int", 5)]),
    ("[{:.2}]", [("text", "سلام")]),
    ("[{:6.2}]", [("text", "سلام")]),
    ("[{:1$}]", [("text", "x"), ("uint", 5)]),
    ("[{:>1$}]", [("text", "x"), ("uint", 5)]),
    ("{:.*}", [("uint", 2), ("f64", 3.14159)]),
    ("[{:>8.*}]", [("uint", 1), ("f64", 2.25)]),
    ("{0:1$.2$}", [("f64", 1.23456), ("uint", 9), ("uint", 2)]),
    ("{:.1$}", [("text", "abcdef"), ("uint", 3)]),
    ("[{:0$}]", [("uint", 5)]),
    ("[{:^2$.*}]", [("uint", 2), ("text", "abcdef"), ("uint", 8)]),
    ("[{1:.*}]", [("uint", 3), ("f64", 1.0)]),
    ("[{:w$}]", []),
    # Exponent forms.
    ("{:e}", [("f64", 1234.5)]),
    ("{:E}", [("f64", 0.00012)]),
    ("{:e}", [("f64", 0.0)]),
    ("{:e}", [("f64", -1e300)]),
    ("{:e}", [("f64b", 1)]),
    ("{:e}", [("f64", 1.0)]),
    ("{:.2e}", [("f64", 1234.5)]),
    ("{:.2e}", [("f64", 9.999)]),
    ("{:.0e}", [("f64", 2.5)]),
    ("{:.3e}", [("f64", 0.0)]),
    ("{:e}", [("f32", 0.1)]),
    ("{:e}", [("int", 1234)]),
    ("{:e}", [("int", 0)]),
    ("{:e}", [("int", -50)]),
    ("{:e}", [("int", 1200)]),
    ("{:.1e}", [("int", 1250)]),
    ("{:.1e}", [("int", 1350)]),
    ("{:.1e}", [("int", 1251)]),
    ("{:.3e}", [("int", 7)]),
    ("{:E}", [("u8", 150)]),
    ("{:010e}", [("int", 12)]),
    ("{:+e}", [("int", 7)]),
    ("{:e}", [("f64b", 0x7FF8000000000000)]),
    ("{:E}", [("f64b", 0xFFF0000000000000)]),
    ("[{:>12e}]", [("f64", -0.5)]),
    ("{:e}", [("u64", 18446744073709551615)]),
    # Floats: Display and Debug.
    ("{}", [("f64b", 0x3FD3333333333334)]),
    ("{}", [("f64", 1e21)]),
    ("{}", [("f64", 1e-7)]),
    ("{}", [("f64", 123.456)]),
    ("{}", [("f64", 1e100)]),
    ("{}", [("f64", -0.0)]),
    ("{}", [("f64", 0.0)]),
    ("{}", [("f64", 1.0)]),
    ("{}", [("f64", 100.0)]),
    ("{}", [("f32", 0.1)]),
    ("{}", [("f32", 16777216.0)]),
    ("{}", [("f64b", 1)]),
    ("{}", [("f64b", 0x7FEFFFFFFFFFFFFF)]),
    ("{}", [("f64b", 0x7FF8000000000000)]),
    ("{}", [("f64b", 0xFFF8000000000000)]),
    ("{}", [("f64b", 0x7FF0000000000000)]),
    ("{}", [("f64b", 0xFFF0000000000000)]),
    ("{:?}", [("f64", 1.0)]),
    ("{:?}", [("f64", 0.0)]),
    ("{:?}", [("f64", -0.0)]),
    ("{:?}", [("f32", 0.1)]),
    ("{:?}", [("f64", 1e16)]),
    ("{:?}", [("f64", 1e15)]),
    ("{:?}", [("f64", 1e-5)]),
    ("{:?}", [("f64", 1e-4)]),
    ("{:?}", [("f64", 1.5e-5)]),
    ("{:?}", [("f64", 123456789.0)]),
    ("{:?}", [("f64", 1e300)]),
    ("{:?}", [("f64", 1.25e17)]),
    ("{:?}", [("f64b", 0x7FF8000000000000)]),
    ("{:?}", [("f64b", 0xFFF0000000000000)]),
    ("{:.2?}", [("f64", 1.0)]),
    ("[{:8?}]", [("f64", 1.5)]),
    ("[{:+?}]", [("f64", 2.0)]),
    ("{:08?}", [("f64", -2.5)]),
    # Integers: extremes and widths.
    ("{}", [("u64", 18446744073709551615)]),
    ("{}", [("i64min", 0)]),
    ("{:x}", [("i64min", 0)]),
    ("{:b}", [("u8", 255)]),
    ("{:#b}", [("u8", 255)]),
    ("{:x}", [("i8", -128)]),
    ("{}", [("i8", -128)]),
    ("{}", [("i16", -32768)]),
    ("{}", [("u16", 65535)]),
    ("{}", [("i32", -2147483648)]),
    ("{:o}", [("i32", -1)]),
    ("{:X}", [("u64", 18446744073709551615)]),
    ("{}", [("uint", 0)]),
    ("{:?}", [("int", -3)]),
    ("[{:>8?}]", [("int", 5)]),
    ("{:x?}", [("int", 255)]),
    ("{:X?}", [("int", 255)]),
    ("{:#x?}", [("int", 255)]),
    ("{:#X?}", [("i8", -1)]),
    ("[{:#?}]", [("int", 7)]),
    # Characters and booleans.
    ("[{:>3}]", [("char", "ب")]),
    ("[{:<3}]", [("char", "a")]),
    ("[{:.0}]", [("char", "a")]),
    ("{}", [("char", "ب")]),
    ("{:?}", [("char", "a")]),
    ("{:?}", [("char", "ب")]),
    ("{:?}", [("char", "'")]),
    ("{:?}", [("char", '"')]),
    ("{:?}", [("char", "\n")]),
    ("{:?}", [("char", "\\")]),
    ("{:?}", [("char", "َ")]),
    ("{:?}", [("char", "\u200f")]),
    ("[{:>5?}]", [("char", "a")]),
    ("[{:>6}]", [("bool", True)]),
    ("[{:.2}]", [("bool", True)]),
    ("{:?}", [("bool", False)]),
    ("[{:>8?}]", [("bool", False)]),
    # Debug text.
    ("{:?}", [("text", "it's \"q\"\t\r\n\\")]),
    ("{:?}", [("text", "\x01\x1b\x7f")]),
    ("{:?}", [("text", "سلام")]),
    ("{:?}", [("text", "سلام\u200f")]),
    ("{:?}", [("text", "a\u00a0b\u2066c\u2069")]),
    ("{:?}", [("text", "َab")]),
    ("{:?}", [("text", "")]),
    ("[{:>10?}]", [("text", "ab")]),
    ("{:?}", [("text", "\u2028\ufeff\ue000")]),
    ("{:?}", [("text", "😀 ﻻ")]),
    # Lists.
    ("{:?}", [("ints", [1, -2, 3])]),
    ("{:?}", [("ints", [])]),
    ("{:#?}", [("ints", [1, 2])]),
    ("{:#?}", [("ints", [])]),
    ("{:5?}", [("ints", [1, 2])]),
    ("{:x?}", [("ints", [10, 255])]),
    ("{:+?}", [("ints", [1, -1])]),
    ("{:?}", [("texts", ["a", "ب", "c\"d"])]),
    ("{:#?}", [("texts", ["x", "y"])]),
    ("{:?}", [("bytes", [10, 255, 0])]),
    ("{:x?}", [("bytes", [10, 255])]),
    ("{:#x?}", [("bytes", [10])]),
    ("{:?}", [("lists", [[1, 2], [3], []])]),
    ("{:#?}", [("lists", [[1, 2], [3]])]),
    ("{:#?}", [("lists", [[]])]),
    # Mixed.
    ("{:>4}|{:<6}|{:^5}|{:+.1}", [("int", 1), ("text", "ab"), ("char", "c"), ("f64", 2.25)]),
    ("{0:>5}{0:<5}|", [("text", "x")]),
    ("x = {x:?}", []),
]


def f32_bits(v):
    return struct.unpack("<I", struct.pack("<f", v))[0]


def f64_bits(v):
    return struct.unpack("<Q", struct.pack("<d", v))[0]


BIDI = set(range(0x200E, 0x2010)) | set(range(0x202A, 0x202F)) | set(range(0x2066, 0x206A))


def rust_lit(s):
    out = []
    for ch in s:
        c = ord(ch)
        if c in BIDI:
            out.append("\\u{%x}" % c)
        elif ch == "\\":
            out.append("\\\\")
        elif ch == '"':
            out.append('\\"')
        elif c < 0x20 or c == 0x7F:
            out.append("\\u{%x}" % c)
        else:
            out.append(ch)
    return '"' + "".join(out) + '"'


def rust_char(ch):
    c = ord(ch)
    if ch == "\\":
        return "'\\\\'"
    if ch == "'":
        return "'\\''"
    if c < 0x20 or c == 0x7F or c in BIDI:
        return "'\\u{%x}'" % c
    return "'" + ch + "'"


def rust_arg(kind, v):
    if kind == "int":
        return "(%d as i64)" % v
    if kind == "uint":
        return "(%d as usize)" % v
    if kind in ("i8", "i16", "i32", "i64", "u8", "u16", "u32", "u64"):
        return "(%d as %s)" % (v, kind) if v >= 0 else "(%d%s)" % (v, kind)
    if kind == "i64min":
        return "i64::MIN"
    if kind == "f64":
        return "f64::from_bits(0x%x)" % f64_bits(v)
    if kind == "f64b":
        return "f64::from_bits(0x%x)" % v
    if kind == "f32":
        return "f32::from_bits(0x%x)" % f32_bits(v)
    if kind == "bool":
        return "true" if v else "false"
    if kind == "char":
        return rust_char(v)
    if kind == "text":
        return rust_lit(v)
    if kind == "bytes":
        return "vec![%s] as Vec<u8>" % ", ".join("%du8" % b for b in v)
    if kind == "ints":
        return "(vec![%s] as Vec<i64>)" % ", ".join("%di64" % b for b in v)
    if kind == "texts":
        return "(vec![%s] as Vec<&str>)" % ", ".join(rust_lit(t) for t in v)
    if kind == "lists":
        return "(vec![%s] as Vec<Vec<i64>>)" % ", ".join(
            "vec![%s] as Vec<i64>" % ", ".join("%di64" % b for b in inner) for inner in v)
    raise ValueError(kind)


def run_rust():
    lines = ["#![allow(unused)]", "fn emit(s: String) { print!(\"{}\\n{}\\n\", s.len(), s); }", "fn main() {"]
    for template, args in CASES:
        if "{x" in template or "{:w$" in template:
            lines.append('    emit(String::from("ERROR"));')
            continue
        rargs = "".join(", " + rust_arg(k, v) for k, v in args)
        lines.append("    emit(format!(%s%s));" % (rust_lit(template), rargs))
    lines.append("}")
    with tempfile.TemporaryDirectory() as tmp:
        src = os.path.join(tmp, "cases.rs")
        exe = os.path.join(tmp, "cases")
        with open(src, "w", encoding="utf-8") as f:
            f.write("\n".join(lines) + "\n")
        subprocess.run(["rustc", "-O", "--edition", "2021", "-A", "warnings", src, "-o", exe], check=True)
        data = subprocess.run([exe], check=True, capture_output=True).stdout
    results = []
    pos = 0
    for _ in CASES:
        nl = data.index(b"\n", pos)
        n = int(data[pos:nl])
        results.append(data[nl + 1:nl + 1 + n].decode("utf-8"))
        pos = nl + 1 + n + 1
    return results


def sn_lit(s):
    out = []
    for ch in s:
        c = ord(ch)
        if ch == "\\":
            out.append("\\\\")
        elif ch == '"':
            out.append('\\"')
        elif ch == "\n":
            out.append("\\n")
        elif ch == "\t":
            out.append("\\t")
        elif ch == "\r":
            out.append("\\r")
        elif c < 0x20 or c == 0x7F:
            out.append("\\x%02x" % c)
        elif c in INVISIBLE or 0xE000 <= c <= 0xF8FF:
            out.append("\\u%04x" % c)
        else:
            out.append(ch)
    return '"' + "".join(out) + '"'


INVISIBLE = BIDI | set(range(0x200B, 0x2010)) | {0x00A0, 0x00AD, 0x2028, 0x2029, 0xFEFF} | set(range(0x2060, 0x2070))


LANG = {
    "en": {
        "def": "def", "let": "let", "mutable": "mutable", "return": "return", "if": "if",
        "vector": "vector", "string": "string", "int": "int", "bool": "bool", "u8": "u8",
        "true": "true", "false": "false", "arg": "fmt_arg", "char": "char", "as": "as",
        "ctor": {
            "int": "int", "uint": "uint", "i8": "i8", "i16": "i16", "i32": "i32", "i64": "i64",
            "u8": "u8", "u16": "u16", "u32": "u32", "u64": "u64", "f32": "f32", "f64": "f64",
            "bool": "bool", "char": "char", "text": "text", "bytes": "bytes", "ints": "ints",
            "texts": "texts", "list": "list",
        },
        "f64_from_bits": "f64_from_bits", "f32_from_bits": "f32_from_bits",
        "check": "check", "case": "case_", "failures": "failures", "main": "main",
        "args": "args",
    },
    "ar": {
        "def": "عرف", "let": "دع", "mutable": "متغير", "return": "أرجع", "if": "إذا",
        "vector": "متجه", "string": "نص", "int": "عدد", "bool": "منطقي", "u8": "ط8",
        "true": "صواب", "false": "خطأ", "arg": "قيمة_تنسيق", "char": "حرف", "as": "مثل",
        "ctor": {
            "int": "عدد", "uint": "طبيعي", "i8": "ص8", "i16": "ص16", "i32": "ص32", "i64": "ص64",
            "u8": "ط8", "u16": "ط16", "u32": "ط32", "u64": "ط64", "f32": "ع32", "f64": "ع64",
            "bool": "منطقي", "char": "حرف", "text": "نص", "bytes": "بايتات", "ints": "أعداد",
            "texts": "نصوص", "list": "قائمة",
        },
        "f64_from_bits": "ع64_من_بتات", "f32_from_bits": "ع32_من_بتات",
        "check": "افحص", "case": "حالة_", "failures": "الإخفاقات", "main": "الرئيسية",
        "args": "الوسائط",
    },
}


def sn_char(L, ch):
    c = ord(ch)
    if c < 0x80:
        return "((%d %s %s) %s %s)" % (c, L["as"], L["u8"], L["as"], L["char"])
    if c > 0xFFFF:
        return "'\\U%08x'" % c
    return "'\\u%04x'" % c


def sn_case(L, index, template, args, expected):
    """One function returning whether the case formats as expected."""
    lets = []
    vals = []
    a = L["arg"]
    c = L["ctor"]
    for k, (kind, v) in enumerate(args):
        name = "v%d" % k
        if kind in ("int", "uint", "i8", "i16", "i32", "i64", "u8", "u16", "u32", "u64"):
            vals.append("%s.%s(%d)" % (a, c[kind], v))
        elif kind == "i64min":
            vals.append("%s.%s(-9223372036854775807 - 1)" % (a, c["i64"]))
        elif kind == "f64":
            vals.append("%s.%s(%s(%d))" % (a, c["f64"], L["f64_from_bits"], f64_bits(v)))
        elif kind == "f64b":
            vals.append("%s.%s(%s(%d))" % (a, c["f64"], L["f64_from_bits"], v))
        elif kind == "f32":
            vals.append("%s.%s(%s(%d))" % (a, c["f32"], L["f32_from_bits"], f32_bits(v)))
        elif kind == "bool":
            vals.append("%s.%s(%s)" % (a, c["bool"], L["true"] if v else L["false"]))
        elif kind == "char":
            vals.append("%s.%s(%s)" % (a, c["char"], sn_char(L, v)))
        elif kind == "text":
            lets.append("    %s %s: %s = %s;" % (L["let"], name, L["string"], sn_lit(v)))
            vals.append("%s.%s(&%s)" % (a, c["text"], name))
        elif kind == "bytes":
            lets.append("    %s %s: %s %s = [%s];" % (L["let"], name, L["vector"], L["u8"], ", ".join(str(b) for b in v)))
            vals.append("%s.%s(&%s)" % (a, c["bytes"], name))
        elif kind == "ints":
            lets.append("    %s %s: %s %s = [%s];" % (L["let"], name, L["vector"], L["int"], ", ".join(str(b) for b in v)))
            vals.append("%s.%s(&%s)" % (a, c["ints"], name))
        elif kind == "texts":
            lets.append("    %s %s: %s %s = [%s];" % (L["let"], name, L["vector"], L["string"], ", ".join(sn_lit(t) for t in v)))
            vals.append("%s.%s(&%s)" % (a, c["texts"], name))
        elif kind == "lists":
            inner_vals = []
            for j, inner in enumerate(v):
                iname = "%s_%d" % (name, j)
                lets.append("    %s %s: %s %s = [%s];" % (L["let"], iname, L["vector"], L["int"], ", ".join(str(b) for b in inner)))
                inner_vals.append("%s.%s(&%s)" % (a, c["ints"], iname))
            lets.append("    %s %s: %s %s = [%s];" % (L["let"], name, L["vector"], a, ", ".join(inner_vals)))
            vals.append("%s.%s(&%s)" % (a, c["list"], name))
        else:
            raise ValueError(kind)
    body = lets + ["    %s %s: %s %s = [%s];" % (L["let"], L["args"], L["vector"], a, ", ".join(vals))]
    exp = "%s" % sn_lit(expected)
    if expected == "ERROR":
        body.append("    %s %s_error(%s, %s);" % (L["return"], L["check"], sn_lit(template), L["args"]))
    else:
        body.append("    %s %s(%s, %s, %s);" % (L["return"], L["check"], sn_lit(template), L["args"], exp))
    return "%s %s%d(): %s {\n%s\n}\n" % (L["def"], L["case"], index, L["bool"], "\n".join(body))


HEADER = {
    "en": """// اختبار: تفاضلي
// أهداف: arm64-darwin,x86_64-linux
// خروج: 42
// تفاضلي: نعم
// نظير: tests/ar/stdlib/fmt/spec_rust.sn
// Generated by tools/generate/generate_fmt_tests.py: every format-spec
// feature of std.fmt against the text Rust's format! produces for the same
// format string and arguments (rustc %(rustc)s).
import std.fmt.fmt;
import std.math.float;
import std.text.compare;
import std.io.print;

def check(template: string, args: vector fmt_arg, expected: string): bool {
    match format(template, args) with
    | result.ok(got) -> {
        if (texts_equal(&got, &expected)) { return true; };
        let r: result int, int = eprint(got);
        let r2: result int, int = eprint("\\n");
        return false;
    }
    | result.err(_) -> { let r: result int, int = eprint("error\\n"); return false; };
}

def check_error(template: string, args: vector fmt_arg): bool {
    match format(template, args) with
    | result.ok(_) -> { return false; }
    | result.err(_) -> { return true; };
}

""",
    "ar": """// اختبار: تفاضلي
// أهداف: arm64-darwin,x86_64-linux
// خروج: 42
// تفاضلي: نعم
// نظير: tests/en/stdlib/fmt/spec_rust.sn
// مولد بالأداة tools/generate/generate_fmt_tests.py: كل ميزة من لغة مواصفات
// التنسيق في المكتبة.تنسيق مقارنة بالنص الذي ينتجه format! في Rust لقالب
// التنسيق والوسائط نفسها (rustc %(rustc)s).
استورد المكتبة.تنسيق.fmt;
استورد std.math.float;
استورد std.text.compare;
استورد std.io.print;

عرف افحص(القالب: نص, الوسائط: متجه قيمة_تنسيق, المتوقع: نص): منطقي {
    طابق نسق(القالب, الوسائط) مع
    | نتيجة.نجح(الناتج) -> {
        إذا (نصان_متساويان(&الناتج, &المتوقع)) { أرجع صواب; };
        دع ر: نتيجة عدد, عدد = اطبع_خطأ(الناتج);
        دع ر2: نتيجة عدد, عدد = اطبع_خطأ("\\n");
        أرجع خطأ;
    }
    | نتيجة.خطأ(_) -> { دع ر: نتيجة عدد, عدد = اطبع_خطأ("خطأ\\n"); أرجع خطأ; };
}

عرف افحص_error(القالب: نص, الوسائط: متجه قيمة_تنسيق): منطقي {
    طابق نسق(القالب, الوسائط) مع
    | نتيجة.نجح(_) -> { أرجع خطأ; }
    | نتيجة.خطأ(_) -> { أرجع صواب; };
}

""",
}

MAIN = {
    "en": "def main(): int {\n    let failures: mutable int = 0;\n%s\n    if (failures != 0) { return 1; };\n    return 42;\n}\n",
    "ar": "عرف الرئيسية(): عدد {\n    دع الإخفاقات: متغير عدد = 0;\n%s\n    إذا (الإخفاقات != 0) { أرجع 1; };\n    أرجع 42;\n}\n",
}


ARABIC_CASES = [
    ("[{:>6}]", [("text", "مُحَمَّد")], "[  مُحَمَّد]"),
    ("[{:<6}]", [("text", "مُحَمَّد")], "[مُحَمَّد  ]"),
    ("[{:^8}]", [("text", "سَلَام")], "[  سَلَام  ]"),
    ("[{:.2}]", [("text", "مُحَمَّد")], "[مُحَ]"),
    ("[{:5.3}]", [("text", "مُحَمَّد")], "[مُحَمَّ  ]"),
    ("[{:ـ>7}]", [("text", "عَلِيّ")], "[ــــعَلِيّ]"),
    ("[{:>3}]", [("text", "\U0001F468\u200d\U0001F469\u200d\U0001F467")], "[  \U0001F468\u200d\U0001F469\u200d\U0001F467]"),
    ("[{:>4}]", [("text", "a\r\nb")], "[ a\r\nb]"),
    ("[{:>4}]", [("text", "لا")], "[  لا]"),
    ("[{:>5}]", [("text", "١٢٣")], "[  ١٢٣]"),
    ("{:?}", [("text", "مُحَمَّد")], "\"مُحَمَّد\""),
    ("{:?}", [("text", "مرحبا\u200f")], "\"مرحبا\\u{200f}\""),
    ("{} {}", [("text", "سلام"), ("text", "world")], "سلام world"),
    ("{:<8}|{:>5}", [("text", "أحمد"), ("int", 42)], "أحمد    |   42"),
    ("{:<8}|{:>5}", [("text", "فَاطِمَة"), ("int", 7)], "فَاطِمَة   |    7"),
    ("{}", [("bytes", list("سلام".encode()))], "سلام"),
    ("[{:>6}]", [("bytes", list("سلام".encode()))], "[  سلام]"),
    ("{}", [("bytes", [0x61, 0xFF, 0x62])], "a�b"),
    ("{}", [("bytes", [0xD8])], "�"),
    ("{:x}", [("bytes", [0xDE, 0xAD, 0x01])], "dead01"),
    ("{:#X}", [("bytes", [0xDE, 0xAD])], "0xDEAD"),
    ("[{:>8x}]", [("bytes", [0xAB])], "[      ab]"),
    ("[{:>4}]", [("char", "ب")], "[   ب]"),
    ("[{:-^9}]", [("text", "نَصّ")], "[---نَصّ----]"),
]


ARABIC_HEADER = {
    "en": """// اختبار: تفاضلي
// أهداف: arm64-darwin,x86_64-linux
// خروج: 42
// تفاضلي: نعم
// نظير: tests/ar/stdlib/fmt/arabic.sn
// std.fmt and Arabic text: width and precision count extended grapheme
// clusters (UAX #29), so a letter with its harakat is one character and a
// precision never separates a letter from its marks; Rust counts Unicode
// scalar values and would give these texts less padding. Debug keeps the
// harakat (Rust escapes them) and escapes direction marks. No direction
// marks are inserted. Bytes format as text (ill-formed UTF-8 as U+FFFD) or
// as hex digits.
import std.fmt.fmt;
import std.text.compare;
import std.io.print;

def check(template: string, args: vector fmt_arg, expected: string): bool {
    match format(template, args) with
    | result.ok(got) -> {
        if (texts_equal(&got, &expected)) { return true; };
        let r: result int, int = eprint(got);
        return false;
    }
    | result.err(_) -> { return false; };
}

""",
    "ar": """// اختبار: تفاضلي
// أهداف: arm64-darwin,x86_64-linux
// خروج: 42
// تفاضلي: نعم
// نظير: tests/en/stdlib/fmt/arabic.sn
// المكتبة.تنسيق والنص العربي: يعد العرض والدقة عناقيد المحارف الموسعة
// (UAX #29)، فالحرف مع حركاته محرف واحد، ولا تفصل الدقة حرفا عن حركاته؛
// ويعد Rust القيم القياسية فيعطي هذه النصوص حشوا أقل. ويبقي التصحيح الحركات
// (يهربها Rust) ويهرب علامات الاتجاه. ولا تدرج علامات اتجاه. وتنسق
// البايتات نصا (والترميز غير الصالح U+FFFD) أو أرقاما ست عشرية.
استورد المكتبة.تنسيق.fmt;
استورد المكتبة.نصوص.compare;
استورد المكتبة.دخ.print;

عرف افحص(القالب: نص, الوسائط: متجه قيمة_تنسيق, المتوقع: نص): منطقي {
    طابق نسق(القالب, الوسائط) مع
    | نتيجة.نجح(الناتج) -> {
        إذا (نصان_متساويان(&الناتج, &المتوقع)) { أرجع صواب; };
        دع ر: نتيجة عدد, عدد = اطبع_خطأ(الناتج);
        أرجع خطأ;
    }
    | نتيجة.خطأ(_) -> { أرجع خطأ; };
}

""",
}


def write_arabic():
    for lang in ("en", "ar"):
        L = LANG[lang]
        parts = [ARABIC_HEADER[lang]]
        calls = []
        for i, (t, a, e) in enumerate(ARABIC_CASES):
            parts.append(sn_case(L, i, t, a, e))
            calls.append("    %s (!%s%d()) { %s := %s + 1; };" % (L["if"], L["case"], i, L["failures"], L["failures"]))
        parts.append(MAIN[lang] % "\n".join(calls))
        path = os.path.join("tests", lang, "stdlib", "fmt", "arabic.sn")
        with open(path, "w", encoding="utf-8") as f:
            f.write("\n".join(parts))
        print("wrote", path, len(ARABIC_CASES), "cases")


def main():
    write_arabic()
    results = run_rust()
    rustc = subprocess.run(["rustc", "--version"], capture_output=True, text=True).stdout.split()[1]
    for lang in ("en", "ar"):
        L = LANG[lang]
        parts = [HEADER[lang] % {"rustc": rustc}]
        calls = []
        for i, ((template, args), expected) in enumerate(zip(CASES, results)):
            parts.append(sn_case(L, i, template, args, expected))
            calls.append("    %s (!%s%d()) { %s := %s + 1; };" % (L["if"], L["case"], i, L["failures"], L["failures"]))
        parts.append(MAIN[lang] % "\n".join(calls))
        text = "\n".join(parts)
        if lang == "ar":
            text = text.replace("افحص_error", "افحص_خطأ")
        path = os.path.join("tests", lang, "stdlib", "fmt", "spec_rust.sn")
        os.makedirs(os.path.dirname(path), exist_ok=True)
        with open(path, "w", encoding="utf-8") as f:
            f.write(text)
        print("wrote", path, len(CASES), "cases")


if __name__ == "__main__":
    sys.exit(main())
