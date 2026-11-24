#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[2]
PATTERN = ROOT / "compiler/frontend/pattern"
MODEL = PATTERN / "pattern_model.sn"
REGISTRY = PATTERN / "pattern_registry.sn"
CONSTRUCTION = PATTERN / "pattern_construction.sn"
ANALYSIS = PATTERN / "pattern_analysis.sn"
TYPE_VALIDATION = PATTERN / "type_validation.sn"
TYPES = ROOT / "compiler/frontend/types"
AST_PATTERNS = ROOT / "compiler/frontend/ast/patterns.sn"
NAME_CONTEXT = ROOT / "compiler/frontend/names/name_context.sn"


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8")


def code_only(source: str) -> str:
    result: list[str] = []
    i = 0
    state = "code"
    while i < len(source):
        c = source[i]
        n = source[i + 1] if i + 1 < len(source) else ""
        if state == "code":
            if c == "/" and n == "/":
                result.extend("  "); i += 2; state = "line"; continue
            if c == "/" and n == "*":
                result.extend("  "); i += 2; state = "block"; continue
            if c == '"':
                result.append(" "); i += 1; state = "string"; continue
            if c == "'":
                result.append(" "); i += 1; state = "char"; continue
            if c == "«":
                result.append(" "); i += 1; state = "guillemet"; continue
            result.append(c); i += 1; continue
        if c == "\n":
            result.append("\n"); i += 1
            if state == "line":
                state = "code"
            continue
        if state == "block" and c == "*" and n == "/":
            result.extend("  "); i += 2; state = "code"; continue
        if state in {"string", "char"} and c == "\\" and n:
            result.extend("  "); i += 2; continue
        if state == "string" and c == '"':
            result.append(" "); i += 1; state = "code"; continue
        if state == "char" and c == "'":
            result.append(" "); i += 1; state = "code"; continue
        if state == "guillemet" and c == "»":
            result.append(" "); i += 1; state = "code"; continue
        result.append(" "); i += 1
    return "".join(result)


def definitions(paths: list[Path], name: str) -> list[Path]:
    pattern = re.compile(
        rf"(?<!\w)(?:عام\s+)?(?:نوع|عرف)\s+{re.escape(name)}(?=\s*(?:=|\())"
    )
    found: list[Path] = []
    for path in paths:
        found.extend([path] * len(pattern.findall(code_only(read(path)))))
    return found


def main() -> int:
    problems: list[str] = []
    required = [MODEL, REGISTRY, CONSTRUCTION, ANALYSIS, TYPE_VALIDATION]
    for path in required:
        if not path.is_file():
            problems.append(f"{path.relative_to(ROOT)}: required Pattern owner missing")
    if problems:
        for item in problems:
            print(item)
        print(f"Pattern architecture audit: FAIL ({len(problems)} issue(s))", file=sys.stderr)
        return 1

    files = sorted(PATTERN.glob("*.sn"))
    texts = {path: read(path) for path in files}
    codes = {path: code_only(value) for path, value in texts.items()}
    all_code = "\n".join(codes.values())

    # Canonical semantic ownership: each core semantic fact has one production definition.
    for owner in (
        "معرف_نمط",
        "حقل_نمط_دلالي",
        "مرجع_منشئ_نمط",
        "نمط_دلالي",
        "ربط_نمط",
        "سجل_أنماط",
        "ابن_نمطا",
        "حلل_نمطا_دلاليا",
    ):
        found = definitions(files, owner)
        if len(found) != 1:
            locations = [str(path.relative_to(ROOT)) for path in found]
            problems.append(
                f"Pattern canonical owner {owner}: expected 1 definition, found {len(found)} in {locations}"
            )

    # Dependency direction after Type↔Pattern integration:
    # Pattern may consume the public Type contract only from type_validation.sn.
    # Type must never import Pattern, and later kernels remain forbidden.
    forbidden_import_fragments = (
        "/layout/", "../layout/", "/memory/", "../memory/",
        "/ssf/", "../ssf/", "/msir/", "../msir/",
    )
    for path, source in texts.items():
        for match in re.finditer(r'^استورد\s+"([^"]+)"\s*;', source, re.MULTILINE):
            target = match.group(1).lower()
            if any(fragment in target for fragment in forbidden_import_fragments):
                problems.append(f"{path.relative_to(ROOT)}: forbidden dependency {match.group(1)!r}")
            if ("/types/" in target or "../types/" in target) and path != TYPE_VALIDATION:
                problems.append(
                    f"{path.relative_to(ROOT)}: Type dependency must remain isolated in type_validation.sn"
                )
            if path == TYPE_VALIDATION and target.endswith("/types/type_registry.sn"):
                problems.append(
                    "compiler/frontend/pattern/type_validation.sn: private Type registry import forbidden; "
                    "consume public Type semantic queries"
                )

    for path in sorted(TYPES.glob("*.sn")):
        source = read(path)
        for match in re.finditer(r'^استورد\s+"([^"]+)"\s*;', source, re.MULTILINE):
            target = match.group(1).lower()
            if "/pattern/" in target or "../pattern/" in target:
                problems.append(f"{path.relative_to(ROOT)}: forbidden Type→Pattern dependency")

    for token in (
        "تعريف_هيكل", "تعريف_تعداد", "إزاحة_حقل",
        "ملكية", "استعارة", "نقل", "SSF", "Layout", "MSIR",
    ):
        if re.search(rf"(?<!\w){re.escape(token)}(?!\w)", all_code):
            problems.append(f"compiler/frontend/pattern: forbidden later-kernel semantic fact {token!r}")

    type_validation_code = codes[TYPE_VALIDATION]
    for token in (".الأنواع", ".المسماة"):
        if token in type_validation_code:
            problems.append(f"compiler/frontend/pattern/type_validation.sn: Type private state accessed via {token!r}")
    for query in (
        "معرف_نوع_دلالي_صالح",
        "عدد_عناصر_صف_دلالي",
        "نوع_عنصر_صف_دلالي_عند",
        "عدد_حقول_سجل_دلالي",
        "نوع_حقل_سجل_دلالي_بالاسم",
        "عدد_منشئات_نوع_دلالي",
        "وصف_منشئ_نوع_دلالي_بالهوية",
        "اسم_منشئ_نوع_دلالي_بالهوية",
        "ترتيب_منشئ_نوع_دلالي_بالهوية",
        "نوع_حمولة_منشئ_دلالي_بالهوية_عند",
    ):
        if f"{query}(" not in type_validation_code:
            problems.append(
                f"compiler/frontend/pattern/type_validation.sn: canonical Type query not consumed: {query}"
            )
    for hardcoded in (
        "هوية_منشئ_نوع.اختيار_لا_شيء",
        "هوية_منشئ_نوع.اختيار_بعض",
        "هوية_منشئ_نوع.نتيجة_نجاح",
        "هوية_منشئ_نوع.نتيجة_خطأ",
    ):
        if hardcoded in type_validation_code:
            problems.append(
                f"compiler/frontend/pattern/type_validation.sn: builtin constructor special-case forbidden: {hardcoded}"
            )

    # Name owns normalization and lookup. Pattern may only consume canonical identity APIs.
    for token in ("0x0640", "ـ", "محرف_تهجئة_معرف", "انسخ_هوية_معرف"):
        if token in all_code:
            problems.append(f"compiler/frontend/pattern: identifier/Tatweel policy duplicated via {token!r}")
    for token in (".الأسماء", ".الرموز", ".النطاقات"):
        if token in all_code:
            problems.append(f"compiler/frontend/pattern: Name private state accessed via {token!r}")
    if "قنن_اسما_مصدريا" not in codes[CONSTRUCTION]:
        problems.append("compiler/frontend/pattern/pattern_construction.sn: canonical Name identity contract not consumed")
    public_name_contract = re.findall(
        r"(?<!\w)عام\s+عرف\s+قنن_اسما_مصدريا\s*\(", code_only(read(NAME_CONTEXT))
    )
    if len(public_name_contract) != 1:
        problems.append(
            "compiler/frontend/names/name_context.sn: Pattern requires exactly one public canonical source-name identity query"
        )

    # Representation separation: AST is consumed only at the construction boundary.
    for path in (MODEL, REGISTRY, ANALYSIS):
        if "/ast/" in texts[path] or "../frontend/ast/" in texts[path]:
            problems.append(f"{path.relative_to(ROOT)}: semantic Pattern owner depends on AST representation")
    if "../ast/patterns.sn" not in texts[CONSTRUCTION]:
        problems.append("compiler/frontend/pattern/pattern_construction.sn: AST-to-semantic boundary missing")

    # Source owns bytes/text; semantic Pattern records hold only spans and canonical IDs.
    if "source/source.sn" in texts[MODEL] or re.search(r"(?<!\w)مصدر(?!\w)", codes[MODEL]):
        problems.append("compiler/frontend/pattern/pattern_model.sn: semantic Pattern model must not own Source objects")
    model_code = codes[MODEL]
    for field in ("الاسم: معرف_اسم", "نطاق_الاسم: نطاق", "النمط: معرف_نمط", "النطاق: نطاق"):
        if field not in model_code:
            problems.append(f"compiler/frontend/pattern/pattern_model.sn: coherent record-field semantic member missing: {field}")
    if "سجل من (نطاق, متجه حقل_نمط_دلالي)" not in model_code:
        problems.append("compiler/frontend/pattern/pattern_model.sn: record Pattern must use one coherent field-record vector")
    parallel_vectors = ("أسماء_الحقول", "أنماط_الحقول", "نطاقات_الحقول")
    for token in parallel_vectors:
        if token in all_code:
            problems.append(f"compiler/frontend/pattern: parallel-vector field state introduced via {token!r}")

    # Recursive binding traversal and duplicate detection must have one owner.
    gather_defs = definitions(files, "اجمع_روابط_من")
    if len(gather_defs) != 1 or gather_defs[0] != ANALYSIS:
        problems.append("compiler/frontend/pattern: recursive binding extraction must have exactly one analysis owner")
    if codes[ANALYSIS].count("اجمع_روابط_من(") < 4:
        problems.append("compiler/frontend/pattern/pattern_analysis.sn: canonical recursive binding traversal is incomplete")
    if "ربط_مكرر" not in codes[ANALYSIS]:
        problems.append("compiler/frontend/pattern/pattern_analysis.sn: duplicate-binding classification missing")

    # Untyped analysis still defers Type facts; the integration owner completes them
    # through one recursive Type-directed traversal rather than guessing structure.
    if "يتطلب_دلالات_نوع" not in model_code or "يتطلب_دلالات_نوع" not in codes[ANALYSIS]:
        problems.append("compiler/frontend/pattern: structured untyped Type-deferral state missing")
    for variant in ("صف", "سجل", "منشئ"):
        pattern = rf"نمط_دلالي\.{variant}\([^\n]*\).*?يتطلب_دلالات_نوع"
        if not re.search(pattern, codes[ANALYSIS], re.DOTALL):
            problems.append(f"compiler/frontend/pattern/pattern_analysis.sn: untyped structural {variant} must defer Type facts")

    recursive_type_defs = definitions(files, "تحقق_عقدة_نوعيا")
    if len(recursive_type_defs) != 1 or recursive_type_defs[0] != TYPE_VALIDATION:
        problems.append(
            "compiler/frontend/pattern: Type-directed recursive validation must have exactly one integration owner"
        )
    if codes[TYPE_VALIDATION].count("تحقق_عقدة_نوعيا(") < 4:
        problems.append(
            "compiler/frontend/pattern/type_validation.sn: recursive Type propagation is incomplete"
        )
    for public_api in ("تحقق_نمط_مع_نوع", "هل_نمط_غير_قابل_للدحض_مع_نوع"):
        found = definitions(files, public_api)
        if len(found) != 1 or found[0] != TYPE_VALIDATION:
            problems.append(f"compiler/frontend/pattern: missing canonical Type-directed Pattern API {public_api}")

    # Do not invent literal Pattern syntax/semantics when the current AST has none.
    ast_code = code_only(read(AST_PATTERNS))
    ast_has_literal = bool(re.search(r"(?<!\w)حرفي(?!\w)", ast_code))
    semantic_has_literal = bool(re.search(r"\|\s*حرفي\b", model_code))
    if ast_has_literal != semantic_has_literal:
        problems.append(
            "compiler/frontend/pattern: literal semantic support must exactly follow current AST support"
        )

    # Exact source association must be retained on every semantic variant.
    for variant in ("بدل", "ربط", "صف", "سجل", "منشئ"):
        if not re.search(rf"\|\s*{variant}\s+من\s+(?:نطاق|\(نطاق,)", model_code):
            problems.append(f"compiler/frontend/pattern/pattern_model.sn: {variant} semantic Pattern lacks exact span")

    # LOC is a final cohesion review, not the primary architecture criterion.
    # type_validation.sn intentionally centralizes the entire Type↔Pattern boundary so
    # Type dependencies do not leak into multiple Pattern owners; give that one owner
    # a bounded higher threshold rather than splitting the boundary across files.
    for path in files:
        count = len(texts[path].splitlines())
        limit = 750 if path == TYPE_VALIDATION else 500
        if count > limit:
            problems.append(f"{path.relative_to(ROOT)}: cohesion review required ({count} LOC)")

    if problems:
        for item in problems:
            print(item)
        print(f"Pattern architecture audit: FAIL ({len(problems)} issue(s))", file=sys.stderr)
        return 1

    print(
        "Pattern architecture audit: PASS "
        "(canonical owner, dependency direction, Name/Type boundaries, recursive traversal, "
        "coherent fields, source spans, Type-directed integration, cohesion)"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
