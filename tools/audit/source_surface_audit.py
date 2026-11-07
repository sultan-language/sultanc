#!/usr/bin/env python3
"""Audit the current SultanC source surface and generated compiler mirrors."""
from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
import re
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
GENERATE = ROOT / "tools" / "generate"
REGISTRY = GENERATE / "registry" / "token_registry.def"
TYPE_REGISTRY = GENERATE / "registry" / "type_registry.def"
COMPILER = ROOT / "compiler"
STD = ROOT / "library" / "std"


@dataclass(frozen=True)
class Token:
    identity: str
    constructor: str
    category: str
    lifecycle: str
    payload: str


def fail(message: str) -> None:
    raise SystemExit(f"surface audit: {message}")


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8")


def code_only(text: str) -> str:
    output: list[str] = []
    index = 0
    length = len(text)
    while index < length:
        if text.startswith("//", index):
            end = text.find("\n", index)
            if end < 0:
                output.append(" " * (length - index)); break
            output.append(" " * (end - index)); output.append("\n"); index = end + 1; continue
        if text.startswith("/*", index):
            end = text.find("*/", index + 2)
            if end < 0:
                output.append(" " * (length - index)); break
            end += 2
            segment = text[index:end]
            output.append("".join("\n" if char == "\n" else " " for char in segment))
            index = end; continue
        quote = text[index]
        if quote in ('"', "'", "«"):
            closing = "»" if quote == "«" else quote
            end = index + 1
            escaped = False
            while end < length:
                char = text[end]
                if quote != "«" and char == "\\" and not escaped:
                    escaped = True; end += 1; continue
                if char == closing and not escaped:
                    end += 1; break
                escaped = False; end += 1
            segment = text[index:end]
            output.append("".join("\n" if char == "\n" else " " for char in segment))
            index = end; continue
        output.append(text[index]); index += 1
    return "".join(output)


if not REGISTRY.is_file() or not TYPE_REGISTRY.is_file():
    fail("canonical generator registry is missing")

registry_text = read(REGISTRY)
token_rows = re.findall(
    r'^SULTANC__TOKEN__\(([A-Z0-9_]+),\s*"([^"]+)",\s*([A-Za-z_]+),\s*([A-Za-z_]+),\s*([A-Za-z_]+)\)$',
    registry_text,
    re.MULTILINE,
)
keyword_rows = re.findall(
    r'^SULTANC__KEYWORD__\(([A-Z0-9_]+),\s*"([^"]+)",\s*"([^"]+)"\)$',
    registry_text,
    re.MULTILINE,
)
bool_rows = re.findall(
    r'^SULTANC__BOOL_KEYWORD__\("([^"]+)",\s*"([^"]+)",\s*([01])\)$',
    registry_text,
    re.MULTILINE,
)
operator_rows = re.findall(
    r'^SULTANC__OPERATOR__\(([A-Z0-9_]+),\s*"([^"]+)"\)$',
    registry_text,
    re.MULTILINE,
)
if not token_rows:
    fail("canonical token registry is empty")

tokens = [Token(*row) for row in token_rows]
by_identity = {token.identity: token for token in tokens}
if len(by_identity) != len(tokens):
    fail("canonical token registry contains duplicate identities")
constructors = [token.constructor for token in tokens]
if len(constructors) != len(set(constructors)):
    fail("canonical token registry contains duplicate compiler constructors")

valid_categories = {"Keyword", "Operator", "Delimiter", "Value", "Identifier", "Eof"}
valid_lifecycles = {"Active", "Future_Reserved"}
valid_payloads = {"None", "Index", "Integer", "Text", "Character", "Boolean"}
for token in tokens:
    if token.category not in valid_categories:
        fail(f"{token.identity} has invalid lexical category {token.category}")
    if token.lifecycle not in valid_lifecycles:
        fail(f"{token.identity} has invalid lifecycle {token.lifecycle}")
    if token.payload not in valid_payloads:
        fail(f"{token.identity} has invalid payload category {token.payload}")

expected_payloads = {
    "UNDERSCORE_WITH_INDEX": "Index",
    "LIT_INT": "Integer",
    "LIT_STR": "Text",
    "LIT_CHAR": "Character",
    "LIT_BOOL": "Boolean",
}
for token in tokens:
    expected = expected_payloads.get(token.identity, "None")
    if token.payload != expected:
        fail(f"{token.identity} payload category must be {expected}, got {token.payload}")

keyword_targets = [identity for identity, _, _ in keyword_rows]
expected_keyword_targets = {
    token.identity for token in tokens
    if token.category == "Keyword" and token.identity != "LIT_BOOL"
}
if set(keyword_targets) != expected_keyword_targets or len(keyword_targets) != len(expected_keyword_targets):
    fail("keyword registry does not exactly cover keyword token identities")

aliases: dict[str, str] = {}
for identity, english, arabic in keyword_rows:
    token = by_identity.get(identity)
    if token is None or token.category != "Keyword":
        fail(f"keyword spelling targets non-keyword token {identity}")
    if not english.isascii():
        fail(f"English keyword spelling for {identity} is not ASCII: {english!r}")
    if arabic.isascii():
        fail(f"Arabic keyword spelling for {identity} is not Arabic-surface text: {arabic!r}")
    for spelling in (english, arabic):
        if spelling in aliases:
            fail(f"keyword spelling is duplicated: {spelling!r}")
        aliases[spelling] = identity
for english, arabic, _ in bool_rows:
    for spelling in (english, arabic):
        if spelling in aliases:
            fail(f"boolean keyword spelling is duplicated: {spelling!r}")
        aliases[spelling] = "LIT_BOOL"

for spelling, identity in {
    "def": "FUNCTION_DEF", "عرف": "FUNCTION_DEF",
    "fn": "FUNCTION_TYPE", "دالة": "FUNCTION_TYPE",
    "as": "AS", "مثل": "AS",
    "of": "OF", "بنوع": "OF",
    "any": "ANY", "أي": "ANY",
    "from": "FROM", "من": "FROM",
    "import": "IMPORT", "استورد": "IMPORT",
    "use": "USE", "استخدم": "USE",
    "extern": "EXTERN", "خارجي": "EXTERN",
    "asm": "ASSEMBLY", "string": "STRING", "public": "PUBLIC",
}.items():
    if aliases.get(spelling) != identity:
        fail(f"canonical spelling {spelling!r} must target {identity}")

removed_english = {"fof", "be", "foreign", "inline", "str", "pub", "module", "mod", "module_scope"}
removed_arabic_keyword_aliases = {
    "والا", "ارجع", "دالة_مركبة", "كن", "قابل_للتغيير", "غير_امن",
    "مضمن", "صحيح", "عدد_غير_موقع", "لا_نتيجة", "اي_نوع", "أي_نوع", "بقيمة",
}
for spelling in sorted(removed_english | removed_arabic_keyword_aliases):
    if spelling in aliases:
        fail(f"removed/non-canonical keyword spelling is still registered: {spelling!r}")

removed_identities = {
    "META", "FOR", "EACH", "DO", "META_DEFINE", "THEN", "INCLUDE", "END", "ALL_BACKENDS",
    "LPAREN_STAR", "STAR_RPAREN", "MODULE", "MOD", "BRACEQUOTE",
    "OPERATOR_ASSIGNMENT_PLUS", "OPERATOR_ASSIGNMENT_MINUS", "OPERATOR_ASSIGNMENT_STAR",
    "OPERATOR_ASSIGNMENT_SLASH", "OPERATOR_ASSIGNMENT_PERCENT", "OPERATOR_ASSIGNMENT_AND",
    "OPERATOR_ASSIGNMENT_OR", "OPERATOR_ASSIGNMENT_LEFT_SHIFT", "OPERATOR_ASSIGNMENT_RIGHT_SHIFT",
    "OPERATOR_ASSIGNMENT_ARITHMETIC_RIGHT_SHIFT", "OPERATOR_ASSIGNMENT_CARET",
    "POUND_OPERATOR", "AT_OPERATOR", "LEFT_ARROW_OPERATOR", "SEND_OPERATOR",
    "FOF", "BE", "DEFINE", "FOREIGN", "OPTION", "RESULT", "VECTOR",
}
returned = sorted(removed_identities & set(by_identity))
if returned:
    fail(f"dead token identities returned to the registry: {returned}")

operators = {spelling: identity for identity, spelling in operator_rows}
if len(operators) != len(operator_rows):
    fail("operator registry contains duplicate spellings")
if operators.get("=") != "DEFINITION_SEPARATOR" or operators.get("==") != "EQUALS_OPERATOR":
    fail("definition separator/equality token identity drifted")
for spelling in {"+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=", "<<=", ">>=", ">>>=", "#", "@", "<-", "<|", "(*", "*)"}:
    if spelling in operators:
        fail(f"parser-ownerless operator remains registered: {spelling!r}")

expected_future = {"USE", "FLOAT", "UNDERSCORE_WITH_INDEX", "LIT_FLO"}
actual_future = {token.identity for token in tokens if token.lifecycle == "Future_Reserved"}
if actual_future != expected_future:
    fail(f"future-reserved token disposition changed: {sorted(actual_future)!r}")

# Generated mirrors must be exactly reproducible from the three canonical registries.
for generator in (
    GENERATE / "generate_type_builtins.py",
    GENERATE / "generate_identifier_policy.py",
    GENERATE / "generate_token_surface.py",
):
    process = subprocess.run(
        [sys.executable, str(generator), "--check"], cwd=ROOT,
        text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
    )
    if process.returncode != 0:
        fail(process.stderr.strip() or process.stdout.strip() or f"{generator.name} check failed")

payload_registry = read(COMPILER / "frontend/lexer/payload_registry.sn")
for token in tokens:
    if f"رمز.{token.constructor}" not in payload_registry:
        fail(f"generated payload registry is missing {token.identity}")

keyword_registry = read(COMPILER / "frontend/lexer/keywords/keyword_registry.sn")
if "تطابق_هوية_معرف_وشريحة" not in keyword_registry or "identifier_identity.sn" not in keyword_registry:
    fail("generated keyword registry bypasses canonical identifier identity")
if "0x0640" in keyword_registry:
    fail("generated keyword registry independently owns Tatweel policy")

result_source = read(COMPILER / "frontend/lexer/result.sn")
if "عام عرف رفض_مصنف" not in result_source:
    fail("canonical classified token reject constructor is missing")
if "عرف نجاح_متحقق" not in result_source or "فئة_حمولة_الرمز(النوع)" not in result_source:
    fail("compiler token constructors do not enforce generated payload metadata")
for obsolete in ("رفض_بناء", "رفض_يوتيف8", "رفض_داخلي", "رمز_بلا_حمولة", "رمز_بحمولة", "نجاح_رمز"):
    if f"عام عرف {obsolete}" in result_source or obsolete in result_source:
        fail(f"obsolete token result helper remains: {obsolete}")

values_source = read(COMPILER / "frontend/lexer/values.sn")
for signature in (
    "قيمة_عدد_من_رمز(الرمز: &رمز_مصدر): اختياري عدد",
    "قيمة_محرف_من_رمز(الرمز: &رمز_مصدر): اختياري غ32",
    "قيمة_منطق_من_رمز(الرمز: &رمز_مصدر): اختياري منطقي",
    "نسخ_بايتات_رمز(الرمز: &رمز_مصدر): اختياري متجه غ8",
):
    if signature not in values_source:
        fail(f"checked payload accessor is missing: {signature}")

# Removed public aliases must not silently reappear in active SultanC sources.
# `أي_نوع` and `بقيمة` remain valid internal token-constructor identifiers.
# Their removal applies to lexical keyword aliases, which is enforced above; they
# must not be rejected merely because compiler implementation code names a token
# constructor with that identifier.
removed_active_spellings = (removed_arabic_keyword_aliases - {"أي_نوع", "بقيمة"}) | {
    "أضف", "اضف", "اقرا_ملف", "اكتب_خطا", "نجاح", "فشل"
}
active_sources: list[Path] = []
for base in (COMPILER, STD, ROOT / "tests", ROOT / "examples"):
    if base.is_dir():
        active_sources.extend(sorted(base.rglob("*.sn")))
for path in active_sources:
    code = code_only(read(path))
    for spelling in removed_active_spellings:
        if re.search(rf"(?<![\w]){re.escape(spelling)}(?![\w])", code):
            fail(f"removed Arabic spelling {spelling!r} remains active in {path.relative_to(ROOT)}")

# Current production diagnostics are canonical in compiler/diagnostics/catalog.sn.
diagnostic_catalog = read(COMPILER / "diagnostics/catalog.sn")
codes = re.findall(r"«(SULTANC-E\d{4})»", diagnostic_catalog)
if not codes:
    fail("compiler diagnostic catalog is empty")
if len(codes) != len(set(codes)):
    fail("compiler diagnostic catalog contains duplicate external codes")

# The Type registry must remain non-empty and generated consumers current.
type_text = read(TYPE_REGISTRY)
type_rows = re.findall(r"^SULTANC__TYPE_(?:BUILTIN|MACHINE)__\(", type_text, re.MULTILINE)
if not type_rows:
    fail("canonical builtin type registry is empty")
contextual_rows = re.findall(
    r'^SULTANC__TYPE_CONTEXTUAL__\(([A-Z0-9_]+),\s*([^,]+),\s*"([^"]+)",\s*"([^"]+)",\s*"([^"]+)"\)$',
    type_text, re.MULTILINE,
)
contextual = {row[0]: row for row in contextual_rows}
for identity, english, arabic in (("OPTION", "option", "اختياري"), ("RESULT", "result", "نتيجة"), ("VECTOR", "vector", "متجه")):
    if identity not in contextual:
        fail(f"contextual Type owner is missing {identity}")
    row = contextual[identity]
    if row[3] != english or row[4] != arabic:
        fail(f"contextual Type spelling drifted for {identity}")
    if identity in by_identity:
        fail(f"{identity} must be Type/Name-owned, not a lexical token")

# Old grammar spellings/semantic conflations must be absent from active code.
for path in active_sources:
    code = code_only(read(path))
    if re.search(r"(?<![\w])كن(?![\w])", code):
        fail(f"old Arabic cast spelling remains active in {path.relative_to(ROOT)}")
    if re.search(r"(?<![\w])foreign(?![\w])", code):
        fail(f"old external-declaration spelling remains active in {path.relative_to(ROOT)}")
    if re.search(r"^\s*use\b", code, re.MULTILINE):
        fail(f"old import spelling 'use' remains active in {path.relative_to(ROOT)}")
    # Arabic `من` is the active FROM spelling only in selective-import grammar.
    # Remove canonical FROM statements before checking that the obsolete OF spelling
    # did not leak back into any other grammar position.
    without_selective_from = re.sub(
        r"^\s*من\s+[\w]+(?:\.[\w]+)*\s+استورد\s+[\w]+(?:\s*,\s*[\w]+)*\s*;?",
        "",
        code,
        flags=re.MULTILINE,
    )
    if re.search(r"(?<![\w])من(?![\w])", without_selective_from):
        fail(f"old Arabic OF spelling remains active in {path.relative_to(ROOT)}")

canonical_spelling_count = len(keyword_rows) * 2 + len(bool_rows) * 2
print(
    "source surface audit: PASS "
    f"({len(tokens)} tokens, {canonical_spelling_count} canonical keyword spellings, "
    f"{len(codes)} compiler diagnostics, {len(actual_future)} future-reserved)"
)
