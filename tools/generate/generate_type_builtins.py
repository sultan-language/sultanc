#!/usr/bin/env python3
from __future__ import annotations

import argparse
from dataclasses import dataclass
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[2]
REGISTRY = Path(__file__).resolve().parent / "registry/type_registry.def"
OUTPUT = ROOT / "compiler/frontend/types/builtins_generated.sn"
PARSER_OUTPUT = ROOT / "compiler/frontend/parser/type_builtins_generated.sn"
C_PARSER_OUTPUT = ROOT / "bootstrap/src/frontend/parser/parser_type_primitive.c"
CONTEXTUAL_OUTPUT = ROOT / "compiler/frontend/type_name_surface_generated.sn"
TYPE_NAME_OUTPUT = ROOT / "compiler/frontend/types/contextual_names_generated.sn"
C_TYPE_NAME_OUTPUT = ROOT / "bootstrap/include/private/kernel/type/type_contextual.generated.h"
C_REGISTRY_OUTPUT = ROOT / "bootstrap/include/private/kernel/type/type_registry.def"
TOKEN_REGISTRY = Path(__file__).resolve().parent / "registry/token_registry.def"
TOKEN_ENTRY = re.compile(r'^SULTANC__TOKEN__\(([^,]+),\s*"([^"]+)"')
ENTRY = re.compile(
    r'^SULTANC__TYPE_(BUILTIN|MACHINE)__\('
    r'([^,]+),\s*([^,]+),\s*([^,]+),\s*([^,]+),\s*([^,]+),\s*'
    r'([^,]+),\s*([^,]+),\s*"([^"]+)",\s*"([^"]+)"\)\s*$'
)

CONTEXTUAL_ENTRY = re.compile(
    r'^SULTANC__TYPE_CONTEXTUAL__\('
    r'([^,]+),\s*([^,]+),\s*"([^"]+)",\s*"([^"]+)",\s*"([^"]+)"\)\s*$'
)
DEPRECATED_CONTEXTUAL_ENTRY = re.compile(
    r'^SULTANC__TYPE_CONTEXTUAL_DEPRECATED__\(([^,]+),\s*"([^"]+)"\)\s*$'
)

@dataclass(frozen=True)
class Entry:
    form: str
    ident: str
    token: str
    ast_or_machine: str
    resolved: str
    bits: int
    scalar: bool
    signed: bool
    stage1: str
    diagnostic: str


@dataclass(frozen=True)
class Contextual:
    ident: str
    ast_kind: str
    stage1: str
    english: str
    arabic: str
    deprecated: tuple[str, ...] = ()


def token_names() -> dict[str, str]:
    out: dict[str, str] = {}
    for raw in TOKEN_REGISTRY.read_text(encoding="utf-8").splitlines():
        m = TOKEN_ENTRY.match(raw.strip())
        if m:
            out[m.group(1).strip()] = m.group(2)
    return out


def entries() -> list[Entry]:
    out: list[Entry] = []
    for raw in REGISTRY.read_text(encoding="utf-8").splitlines():
        line = raw.strip()
        m = ENTRY.match(line)
        if not m:
            continue
        out.append(
            Entry(
                form=m.group(1),
                ident=m.group(2).strip(),
                token=m.group(3).strip(),
                ast_or_machine=m.group(4).strip(),
                resolved=m.group(5).strip(),
                bits=int(m.group(6).strip()),
                scalar=m.group(7).strip() == "1",
                signed=m.group(8).strip() == "1",
                stage1=m.group(9),
                diagnostic=m.group(10),
            )
        )
    if not out:
        raise RuntimeError("type registry has no entries")
    return out


def contextual_entries() -> list[Contextual]:
    out: list[Contextual] = []
    deprecated: dict[str, list[str]] = {}
    for raw in REGISTRY.read_text(encoding="utf-8").splitlines():
        m = CONTEXTUAL_ENTRY.match(raw.strip())
        if m:
            out.append(Contextual(*[part.strip() for part in m.groups()]))
            continue
        m = DEPRECATED_CONTEXTUAL_ENTRY.match(raw.strip())
        if m:
            deprecated.setdefault(m.group(1).strip(), []).append(m.group(2))
    idents = {e.ident for e in out}
    for ident in deprecated:
        if ident not in idents:
            raise RuntimeError(f"deprecated contextual spelling targets an unknown Type name: {ident}")
    spellings = [x for e in out for x in (e.english, e.arabic)] + [x for xs in deprecated.values() for x in xs]
    if len(spellings) != len(set(spellings)):
        raise RuntimeError("duplicate contextual Type spelling in the type registry")
    return [
        Contextual(e.ident, e.ast_kind, e.stage1, e.english, e.arabic, tuple(deprecated.get(e.ident, ())))
        for e in out
    ]


def render_contextual_surface(es: list[Contextual]) -> str:
    lines = [
        "// GENERATED FILE — DO NOT EDIT.",
        "// Owner: tools/generate/registry/type_registry.def",
        "// Contextual Type names remain ordinary lexical identifiers; Type owns these spellings.",
        "",
        'استورد compiler.source.source_file;',
        'استورد compiler.source.span;',
        'استورد compiler.frontend.identifier_identity;',
        'استورد compiler.frontend.lexer.keywords.deprecated_registry;',
        "",
        "عام نوع صنف_اسم_نوع_سياقي =",
        "    | عادي",
    ]
    for e in es:
        lines.append(f"    | {e.stage1}")
    lines += [
        ";",
        "",
        "عام عرف صنف_اسم_نوع_سياقي_في_نطاق(المصدر: &مصدر, النطاق: نطاق): صنف_اسم_نوع_سياقي {",
        "    دع الأصل: مصدر = *المصدر;",
    ]
    for e in es:
        spellings = [e.english, e.arabic, *e.deprecated]
        lines.append(
            f"    إذا (تطابق_هوية_معرف_وشريحة(&الأصل.المحتوى, النطاق.البداية.البايت, النطاق.النهاية.البايت, «{spellings[0]}»)"
        )
        for spelling in spellings[1:]:
            lines.append(
                f"        || تطابق_هوية_معرف_وشريحة(&الأصل.المحتوى, النطاق.البداية.البايت, النطاق.النهاية.البايت, «{spelling}»)"
            )
        lines[-1] += ") {"
        lines += [
            f"        أرجع صنف_اسم_نوع_سياقي.{e.stage1}();",
            "    };",
        ]
    lines += ["    أرجع صنف_اسم_نوع_سياقي.عادي();", "}", ""]
    lines += [
        "// The canonical spelling that replaces a deprecated contextual Type name",
        "// (SULTANC__TYPE_CONTEXTUAL_DEPRECATED__), when the slice is one.",
        "عام عرف تهجئة_مهجورة_لاسم_نوع_سياقي(",
        "    النص: &نص,",
        "    البداية: طبيعي,",
        "    النهاية: طبيعي",
        "): اختياري تهجئة_كلمة_مهجورة {",
    ]
    for e in es:
        for spelling in e.deprecated:
            lines += [
                f"    إذا (تطابق_هوية_معرف_وشريحة(النص, البداية, النهاية, «{spelling}»)) {{",
                f"        دع التهجئة: تهجئة_كلمة_مهجورة = {{ القديمة: «{spelling}», الجديدة: «{e.arabic}» }};",
                "        أرجع اختياري.موجود(التهجئة);",
                "    };",
            ]
    lines += ["    أرجع اختياري.لا_شيء();", "}", ""]
    return "\n".join(lines)



def render_type_contextual_names(es: list[Contextual]) -> str:
    lines = [
        "// GENERATED FILE — DO NOT EDIT.",
        "// Owner: tools/generate/registry/type_registry.def",
        "// Canonical contextual Type source-name matching for Type semantic consumers.",
        "",
        'استورد compiler.frontend.names.identifiers;',
        'استورد compiler.frontend.names.name_context;',
        "",
        "عام عرف اسم_مقنن_يطابق_نصا_سياقيا(",
        "    سياق_الأسماء: &سياق_أسماء,",
        "    الاسم: معرف_اسم,",
        "    النص: نص",
        "): منطقي {",
        "    طابق هوية_اسم_لنص_إن_وجد(سياق_الأسماء, &النص) مع",
        "    | اختياري.موجود(الموجود) -> { أرجع معرفا_اسم_متساويان(الاسم, الموجود); }",
        "    | اختياري.لا_شيء -> { أرجع خطأ; };",
        "}",
        "",
    ]
    fn_names = {"OPTION": "اسم_اختيار_مدمج", "RESULT": "اسم_نتيجة_مدمجة", "VECTOR": "اسم_متجه_مدمج"}
    for e in es:
        fn = fn_names.get(e.ident, f"اسم_{e.stage1}_مدمج")
        lines += [
            f"عام عرف {fn}(سياق_الأسماء: &سياق_أسماء, الاسم: معرف_اسم): منطقي {{",
            f"    أرجع اسم_مقنن_يطابق_نصا_سياقيا(سياق_الأسماء, الاسم, «{e.arabic}»)",
        ]
        for spelling in e.deprecated:
            lines.append(f"        || اسم_مقنن_يطابق_نصا_سياقيا(سياق_الأسماء, الاسم, «{spelling}»)")
        lines += [
            f"        || اسم_مقنن_يطابق_نصا_سياقيا(سياق_الأسماء, الاسم, «{e.english}»);",
            "}",
            "",
            f"عام عرف تهجئة_عرض_{e.stage1}_مدمجة(): نص {{ أرجع «{e.arabic}»; }}",
            "",
            f"عام عرف تهجئة_عرض_{e.stage1}_مدمجة_إنجليزية(): نص {{ أرجع «{e.english}»; }}",
            "",
        ]
    return "\n".join(lines)


def render_c_type_contextual_header(es: list[Contextual]) -> str:
    lines = [
        "/* GENERATED FILE — DO NOT EDIT.",
        " * Owner: tools/generate/registry/type_registry.def",
        " */",
        "#ifndef SULTANC__KERNEL_TYPE_CONTEXTUAL_GENERATED_H__",
        "#define SULTANC__KERNEL_TYPE_CONTEXTUAL_GENERATED_H__",
        "",
        '#include "core/source.h"',
        "",
    ]
    for e in es:
        lines += [
            f"/* Checks whether the type contextual name is {e.english}. */",
            f"static inline int __Type_Contextual_Name_Is_{e.ident}__(__Text_Slice__ __Name__)",
            "{",
            f'    return __Text_Equals_Cstr__(__Name__, "{e.english}") ||',
            *[f'           __Text_Equals_Cstr__(__Name__, "{spelling}") ||' for spelling in e.deprecated],
            f'           __Text_Equals_Cstr__(__Name__, "{e.arabic}");',
            "}",
            "",
        ]
    lines += ["#endif", ""]
    return "\n".join(lines)

def render_arm(pattern: str, statement: str, last: bool = False) -> list[str]:
    return [
        f"    | {pattern} -> {{",
        f"        {statement}",
        "    };" if last else "    }",
    ]


def render(es: list[Entry]) -> str:
    class_names = {
        "Unknown": "غير_معروف",
        "Boolean": "منطقي",
        "Signed_Integer": "عدد_موقع",
        "Unsigned_Integer": "عدد_صحيح_طبيعي",
        "Float": "عشري",
        "Character": "حرف",
        "String": "نص",
        "Void": "فراغ",
    }
    lines = [
        "// GENERATED FILE — DO NOT EDIT.",
        "// Owner: tools/generate/registry/type_registry.def",
        "// Regenerate with: python3 tools/generate/generate_type_builtins.py",
        "",
        "عام نوع نوع_بدائي_مقنن =",
    ]
    for e in es:
        lines.append(f"    | {e.stage1}")
    lines += [
        ";",
        "",
        "عام نوع صنف_نوع_بدائي =",
        "    | غير_معروف",
        "    | منطقي",
        "    | عدد_موقع",
        "    | عدد_صحيح_طبيعي",
        "    | عشري",
        "    | حرف",
        "    | نص",
        "    | فراغ",
        ";",
        "",
        "عام عرف صنف_النوع_البدائي(النوع: نوع_بدائي_مقنن): صنف_نوع_بدائي {",
        "    طابق النوع مع",
    ]
    for i, e in enumerate(es):
        lines.extend(render_arm(
            f"نوع_بدائي_مقنن.{e.stage1}",
            f"أرجع صنف_نوع_بدائي.{class_names[e.resolved]}();",
            i == len(es) - 1,
        ))

    lines += ["}", "", "عام عرف رقم_نوع_بدائي(النوع: نوع_بدائي_مقنن): طبيعي {", "    طابق النوع مع"]
    for i, e in enumerate(es):
        lines.extend(render_arm(
            f"نوع_بدائي_مقنن.{e.stage1}",
            f"أرجع {i};",
            i == len(es) - 1,
        ))
    lines.append("}")

    def property_fn(name: str, typ: str, value):
        lines.extend(["", f"عام عرف {name}(النوع: نوع_بدائي_مقنن): {typ} {{", "    طابق النوع مع"])
        for i, e in enumerate(es):
            lines.extend(render_arm(
                f"نوع_بدائي_مقنن.{e.stage1}",
                f"أرجع {value(e)};",
                i == len(es) - 1,
            ))
        lines.append("}")

    property_fn("بتات_نوع_بدائي", "طبيعي", lambda e: str(e.bits))
    property_fn("هل_نوع_بدائي_قياسي", "منطقي", lambda e: "صواب" if e.scalar else "خطأ")
    property_fn("هل_نوع_بدائي_موقع", "منطقي", lambda e: "صواب" if e.signed else "خطأ")
    lines += ["", "عام عرف تهجئة_عرض_نوع_بدائي(النوع: نوع_بدائي_مقنن): نص {", "    طابق النوع مع"]
    for i, e in enumerate(es):
        lines.extend(render_arm(
            f"نوع_بدائي_مقنن.{e.stage1}",
            f"أرجع «{e.stage1}»;",
            i == len(es) - 1,
        ))
    lines.append("}")
    lines += ["", "عام عرف تهجئة_عرض_نوع_بدائي_إنجليزية(النوع: نوع_بدائي_مقنن): نص {", "    طابق النوع مع"]
    for i, e in enumerate(es):
        lines.extend(render_arm(
            f"نوع_بدائي_مقنن.{e.stage1}",
            f"أرجع «{e.diagnostic}»;",
            i == len(es) - 1,
        ))
    lines.append("}")
    return "\n".join(lines) + "\n"


def render_parser_bridge(es: list[Entry]) -> str:
    tokens = token_names()
    lines = [
        "// GENERATED FILE — DO NOT EDIT.",
        "// Owner: tools/generate/registry/type_registry.def",
        "// Frontend-owned Stage-1 token-to-AST primitive-type syntax adapter.",
        "",
        'استورد compiler.frontend.lexer.token;',
        'استورد compiler.source.span;',
        'استورد compiler.frontend.ast.name;',
        'استورد compiler.frontend.ast.types;',
        "",
        "عام عرف عقدة_نوع_بدائي_من_رمز(",
        "    الرمز: رمز,",
        "    النطاق: نطاق",
        "): اختياري نوع_تركيبي {",
        "    طابق الرمز مع",
    ]
    for e in es:
        token_name = tokens.get(e.token)
        if token_name is None:
            raise RuntimeError(f"type registry token has no token owner: {e.token}")
        if e.form == "MACHINE":
            expr = f"نوع_تركيبي.آلة(النطاق, نوع_آلة.{e.stage1}())"
        else:
            expr = f"نوع_تركيبي.{e.stage1}(النطاق)"
        lines.extend(render_arm(
            f"رمز.{token_name}",
            f"أرجع اختياري.موجود({expr});",
        ))
    lines.extend(render_arm("_", "أرجع اختياري.لا_شيء();", True))
    lines.append("}")
    return "\n".join(lines) + "\n"


def render_c_parser_bridge(es: list[Entry]) -> str:
    lines = [
        "/* GENERATED FILE — DO NOT EDIT.",
        " * Owner: tools/generate/registry/type_registry.def",
        " * Frontend-owned Stage-0 token-to-AST primitive-type syntax adapter.",
        " * Regenerate with: python3 tools/generate/generate_type_builtins.py",
        " */",
        "",
        '#include "frontend/parser/type_internal.h"',
        '#include "core/source.h"',
        "",
        "/* Returns the parser type primitive. */",
        "int __Parser_Type_Primitive__(__Token_Kind__ __Token_Value__,",
        "                              __Ast_Type_Kind__ *__Out_Kind__,",
        "                              __Machine_Type__ *__Out_Machine__)",
        "{",
        "    if (__Out_Kind__ == NULL || __Out_Machine__ == NULL)",
        "    {",
        "        return 0;",
        "    }",
        "",
        "    switch (__Token_Value__)",
        "    {",
    ]
    for e in es:
        lines.append(f"        case __Token_{e.token}__:")
        if e.form == "MACHINE":
            lines.append("            *__Out_Kind__ = __Ast_Type_Machine__;")
            lines.append(f"            *__Out_Machine__ = __Machine_{e.ast_or_machine}__;")
        else:
            lines.append(f"            *__Out_Kind__ = __Ast_Type_{e.ast_or_machine}__;")
        lines.append("            return 1;")
    lines += [
        "        default:",
        "            return 0;",
        "    }",
        "}",
        "",
        "/* Returns the parser type contextual. */",
        "int __Parser_Type_Contextual__(__Text_Slice__ __Name__, __Ast_Type_Kind__ *__Out_Kind__)",
        "{",
        "    if (__Out_Kind__ == NULL)",
        "    {",
        "        return 0;",
        "    }",
    ]
    for e in contextual_entries():
        lines += [
            f'    if (__Text_Equals_Cstr__(__Name__, "{e.english}") ||',
            *[f'        __Text_Equals_Cstr__(__Name__, "{spelling}") ||' for spelling in e.deprecated],
            f'        __Text_Equals_Cstr__(__Name__, "{e.arabic}"))',
            "    {",
            f"        *__Out_Kind__ = __Ast_Type_{e.ast_kind}__;",
            "        return 1;",
            "    }",
        ]
    lines += ["    return 0;", "}"]
    return "\n".join(lines) + "\n"


def render_c_registry() -> str:
    """Stage0's copy of the builtin and machine rows, generated so the two
    registries cannot drift."""
    rows = [
        raw for raw in REGISTRY.read_text(encoding="utf-8").splitlines()
        if ENTRY.match(raw.strip())
    ]
    builtin = [row for row in rows if row.startswith("SULTANC__TYPE_BUILTIN__")]
    machine = [row for row in rows if row.startswith("SULTANC__TYPE_MACHINE__")]
    lines = [
        "/* Canonical built-in type registry shared by generated frontend and type-system views.",
        " * GENERATED FILE — DO NOT EDIT. Owner: tools/generate/registry/type_registry.def",
        " * (builtin and machine rows; contextual names are in type_contextual.generated.h).",
        " * Regenerate with: python3 tools/generate/generate_type_builtins.py",
        " */",
        "",
        *builtin,
        "",
        *machine,
    ]
    return "\n".join(lines) + "\n"


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    es = entries()
    contextual = contextual_entries()
    content = render(es)
    parser_content = render_parser_bridge(es)
    c_parser_content = render_c_parser_bridge(es)
    contextual_content = render_contextual_surface(contextual)
    type_name_content = render_type_contextual_names(contextual)
    c_type_name_content = render_c_type_contextual_header(contextual)
    c_registry_content = render_c_registry()
    if args.check:
        stale = []
        for path, expected in (
            (OUTPUT, content),
            (PARSER_OUTPUT, parser_content),
            (C_PARSER_OUTPUT, c_parser_content),
            (CONTEXTUAL_OUTPUT, contextual_content),
            (TYPE_NAME_OUTPUT, type_name_content),
            (C_TYPE_NAME_OUTPUT, c_type_name_content),
            (C_REGISTRY_OUTPUT, c_registry_content),
        ):
            if not path.is_file() or path.read_text(encoding="utf-8") != expected:
                stale.append(path.relative_to(ROOT))
        if stale:
            print(f"generated Type Kernel files are stale: {', '.join(map(str, stale))}", file=sys.stderr)
            return 1
        print("Type Kernel generated builtins: PASS")
        return 0
    for path, expected in (
        (OUTPUT, content),
        (PARSER_OUTPUT, parser_content),
        (C_PARSER_OUTPUT, c_parser_content),
        (CONTEXTUAL_OUTPUT, contextual_content),
        (TYPE_NAME_OUTPUT, type_name_content),
        (C_TYPE_NAME_OUTPUT, c_type_name_content),
        (C_REGISTRY_OUTPUT, c_registry_content),
    ):
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(expected, encoding="utf-8")
        print(path.relative_to(ROOT))
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
