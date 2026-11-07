#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[2]
COMPILER = ROOT / "compiler"
FRONTEND = COMPILER / "frontend"
SOURCE = FRONTEND / "source"
LEXER = FRONTEND / "lexer"
AST = FRONTEND / "ast"
PARSER = FRONTEND / "parser"
NAMES = FRONTEND / "names"
TYPES = FRONTEND / "types"
ENTRYPOINT = COMPILER / "main.sn"
IDENTIFIER_POLICY_REGISTRY = (
    ROOT / "tools/generate/registry/identifier_policy_registry.def"
)
IDENTIFIER_POLICY_CONSUMERS = {
    ROOT / "bootstrap/src/frontend/identifier_identity.c",
    ROOT / "bootstrap/src/frontend/lexer/lexer_character_is_id_start.c",
    ROOT / "bootstrap/src/frontend/lexer/lexer_character_is_id_rest.c",
    COMPILER / "frontend/identifier_identity.sn",
    COMPILER / "frontend/lexer/identifiers.sn",
}


def problem(message: str) -> None:
    print(message)


def text(path: Path) -> str:
    return path.read_text(encoding="utf-8")


def code_only(source: str) -> str:
    """Mask comments and literals while preserving code/newline positions."""
    result: list[str] = []
    index = 0
    state = "code"
    while index < len(source):
        current = source[index]
        following = source[index + 1] if index + 1 < len(source) else ""
        if state == "code":
            if current == "/" and following == "/":
                result.extend("  "); index += 2; state = "line_comment"; continue
            if current == "/" and following == "*":
                result.extend("  "); index += 2; state = "block_comment"; continue
            if current == '"':
                result.append(" "); index += 1; state = "double_string"; continue
            if current == "'":
                result.append(" "); index += 1; state = "character"; continue
            if current == "«":
                result.append(" "); index += 1; state = "guillemet_string"; continue
            result.append(current); index += 1; continue
        if current == "\n":
            result.append("\n"); index += 1
            if state == "line_comment": state = "code"
            continue
        if state == "block_comment" and current == "*" and following == "/":
            result.extend("  "); index += 2; state = "code"; continue
        if state in {"double_string", "character"} and current == "\\" and following:
            result.extend("  "); index += 2; continue
        if state == "double_string" and current == '"':
            result.append(" "); index += 1; state = "code"; continue
        if state == "character" and current == "'":
            result.append(" "); index += 1; state = "code"; continue
        if state == "guillemet_string" and current == "»":
            result.append(" "); index += 1; state = "code"; continue
        result.append(" "); index += 1
    return "".join(result)


def semantic_definitions(paths: list[Path], name: str) -> list[Path]:
    """Return one path entry per actual type/function definition of name."""
    pattern = re.compile(
        rf"(?<!\w)(?:عام\s+)?(?:نوع|عرف)\s+{re.escape(name)}(?=\s*(?:=|\())"
    )
    definitions: list[Path] = []
    for path in paths:
        definitions.extend([path] * len(pattern.findall(code_only(text(path)))))
    return definitions


def require_unique_definitions(
    problems: list[str], domain: str, paths: list[Path], names: tuple[str, ...]
) -> None:
    for name in names:
        definitions = semantic_definitions(paths, name)
        if len(definitions) != 1:
            shown = [str(path.relative_to(ROOT)) for path in definitions]
            problems.append(
                f"{domain}: semantic fact {name} must have exactly one current definition; "
                f"found {len(definitions)} in {shown!r}"
            )


def main() -> int:
    problems: list[str] = []
    reviews: list[str] = []

    for stale in [COMPILER / "source", COMPILER / "stages"]:
        if stale.exists():
            problems.append(f"{stale.relative_to(ROOT)}: obsolete catch-all/future tree exists")
    if (COMPILER / "tests").exists():
        problems.append("compiler/tests: test-only source must remain outside production compiler")

    # Kernel-like design is structural, not a generic filename convention.
    # Selfhost modules must expose their actual responsibility in the path/name.
    generic_kernel_files = sorted(COMPILER.rglob("kernel.sn"))
    for path in generic_kernel_files:
        problems.append(
            f"{path.relative_to(ROOT)}: generic kernel.sn name hides subsystem responsibility"
        )
    compiler_text_for_naming = "\n".join(
        text(path) for path in COMPILER.rglob("*.sn") if path.is_file()
    )
    for stale_name in ("نواة_أسماء", "نواة_أنواع"):
        if stale_name in compiler_text_for_naming:
            problems.append(f"compiler: stale generic kernel state name returned: {stale_name}")

    required_dirs = [SOURCE, LEXER, AST, PARSER, NAMES, TYPES]
    for path in required_dirs:
        if not path.is_dir():
            problems.append(f"{path.relative_to(ROOT)}: required ownership directory missing")

    arabic_entry_definitions: list[Path] = []
    english_entry_definitions: list[Path] = []
    for path in sorted(COMPILER.rglob("*.sn")):
        source_code = code_only(text(path))
        if re.search(r"(?<!\w)عرف\s+الرئيسية\s*\(", source_code):
            arabic_entry_definitions.append(path)
        if re.search(r"(?<!\w)(?:عرف|def)\s+main\s*\(", source_code):
            english_entry_definitions.append(path)
    if arabic_entry_definitions != [ENTRYPOINT]:
        shown = [str(path.relative_to(ROOT)) for path in arabic_entry_definitions]
        problems.append(
            "compiler entrypoint: expected exactly one عرف الرئيسية in "
            f"{ENTRYPOINT.relative_to(ROOT)}; found {shown!r}"
        )
    if english_entry_definitions:
        shown = [str(path.relative_to(ROOT)) for path in english_entry_definitions]
        problems.append(f"compiler entrypoint: production main definition forbidden: {shown!r}")

    # Name System Blocks 1-5 own identity, scopes, declarations, name resolution, the direct-import graph, and visibility.
    if NAMES.is_dir():
        name_files = sorted(NAMES.rglob("*.sn"))
        names_text = "\n".join(text(p) for p in name_files)
        # Structural gate: semantic owners must exist exactly once, but future Name-System
        # blocks may add/split files without editing an exact filename allow-list.
        required_owners = (
            "معرف_وحدة", "معرف_نطاق", "معرف_اسم", "معرف_رمز", "نطاق_أسماء",
            "اسم_مقنن", "رمز_دلالي", "نتيجة_إضافة", "وحدة_دلالية",
            "جدول_تصريحات", "اجمع_تصريحات_الوحدة",
            "رسم_وحدات", "نتيجة_إضافة_استيراد", "نتيجة_حل_اسم",
            "حل_اسم_محلي", "حل_اسم_مرئي", "حل_اسم_في_وحدة",
        )
        require_unique_definitions(problems, "compiler/frontend/names", name_files, required_owners)
        for path in name_files:
            count = len(text(path).splitlines())
            if count > 3000:
                reviews.append(f"LOC REVIEW: critical cohesion review for {path.relative_to(ROOT)} ({count} LOC)")
            elif count > 2000 and "SULTANC_COHESION_JUSTIFICATION:" not in text(path)[:4000]:
                reviews.append(f"LOC REVIEW: cohesion justification review for {path.relative_to(ROOT)} ({count} LOC)")
            elif count > 1500:
                reviews.append(f"LOC REVIEW: architectural warning for {path.relative_to(ROOT)} ({count} LOC)")
            elif count > 800:
                reviews.append(f"LOC REVIEW: inspect cohesion for {path.relative_to(ROOT)} ({count} LOC)")
        for forbidden in ["lexer", "parser", "Quad", "ARM64", "Mach-O", "ABI"]:
            if forbidden in names_text:
                problems.append(f"compiler/frontend/names: later/frontend implementation coupling {forbidden!r}")
        if re.search(r"\bنوع\s+(?:خريطة|جدول_تجزئة|جدول)\b", names_text):
            problems.append("compiler/frontend/names: generic container implementation belongs in stdlib")
        collector = NAMES / "declaration_collection.sn"
        if collector.is_file():
            collector_text = code_only(text(collector))
            # Reading the body Option to classify foreign-vs-defined is metadata, not body traversal.
            for forbidden in [".المعاملات", ".الاستيرادات", ".العبارات["]:
                if forbidden in collector_text:
                    problems.append(f"compiler/frontend/names/declaration_collection.sn: Block-2 collector traverses forbidden data {forbidden!r}")

    # The Type System owns canonical type identities, interning, AST resolution,
    # compatibility, and conversion policy. Occurrences and call sites are consumers;
    # only actual type/function definitions count as owners.
    if TYPES.is_dir():
        type_files = sorted(TYPES.rglob("*.sn"))
        required_type_owners = (
            "معرف_نوع", "نوع_بدائي_مقنن", "نوع_مقنن", "نتيجة_حل_نوع",
            "سجل_أنواع", "قنن_نوعا", "قنن_نوعا_بدائيا", "حل_نوع_مسمى",
            "قنن_نوع_تركيبي", "نوعان_متوافقان", "صنف_تحويل_بين",
            "هل_عدد_صحيح_يلائم",
        )
        require_unique_definitions(problems, "compiler/frontend/types", type_files, required_type_owners)
        for path in type_files:
            count = len(text(path).splitlines())
            if count > 3000:
                reviews.append(f"LOC REVIEW: critical cohesion review for {path.relative_to(ROOT)} ({count} LOC)")
            elif count > 2000 and "SULTANC_COHESION_JUSTIFICATION:" not in text(path)[:4000]:
                reviews.append(f"LOC REVIEW: cohesion justification review for {path.relative_to(ROOT)} ({count} LOC)")
            elif count > 1500:
                reviews.append(f"LOC REVIEW: architectural warning for {path.relative_to(ROOT)} ({count} LOC)")
            elif count > 800:
                reviews.append(f"LOC REVIEW: inspect cohesion for {path.relative_to(ROOT)} ({count} LOC)")

    # Every frontend owner, including root-level identity policy and Source,
    # must stay independent of semantic Name and Type Systems.
    syntax_frontend_imports: list[tuple[Path, Path]] = []
    syntax_owners = [SOURCE, LEXER, AST, PARSER]
    syntax_owners.extend(
        path
        for path in FRONTEND.glob("*.sn")
        if path.is_file() and path.name != "semantic_pipeline.sn"
    )
    for owner_root in syntax_owners:
        paths = [owner_root] if owner_root.is_file() else list(owner_root.rglob("*.sn"))
        for path in paths:
            for match in re.finditer(r'^استورد\s+"([^"]+)"\s*;', text(path), re.MULTILINE):
                syntax_frontend_imports.append((path, (path.parent / match.group(1)).resolve()))
    for owner, imported in syntax_frontend_imports:
        if imported.is_relative_to(NAMES.resolve()):
            problems.append(
                f"{owner.relative_to(ROOT)}: syntax frontend imports semantic Name System"
            )
        if imported.is_relative_to(TYPES.resolve()):
            problems.append(
                f"{owner.relative_to(ROOT)}: syntax frontend imports semantic Type System"
            )

    # Source representation is intentionally tiny and must not grow compiler concerns.
    source_files = {p.name for p in SOURCE.glob("*.sn")} if SOURCE.is_dir() else set()
    if source_files != {"source.sn", "position.sn", "scope.sn", "source_identity.sn"}:
        problems.append(f"compiler/frontend/source: unexpected owners {sorted(source_files)!r}")
    source_text = "\n".join(text(p) for p in SOURCE.glob("*.sn")) if SOURCE.is_dir() else ""
    for forbidden in ["رمز_مصدر", "مؤشر_مسح", "الرمز_التالي", "نوع تعبير", "نوع عبارة", "خطأ_نحوي"]:
        if forbidden in source_text:
            problems.append(f"source ownership contains compiler concern {forbidden!r}")

    # General-purpose implementations have one standard-library owner and no compiler duplicate.
    compiler_code = "\n".join(text(p) for p in COMPILER.rglob("*.sn"))
    generic_owners = {
        "متاح_في_النص": ROOT / "library/std/text/range.sn",
        "فك_محرف_يوتيف8": ROOT / "library/std/encoding/utf8.sn",
        "ألحق_محرف_يوتيف8": ROOT / "library/std/encoding/utf8.sn",
        "يطابق_عند": ROOT / "library/std/text/compare.sn",
        "يطابق_شريحة": ROOT / "library/std/text/compare.sn",
        "رقم_عشري": ROOT / "library/std/chars/ascii.sn",
        "رقم_سداسي": ROOT / "library/std/chars/ascii.sn",
        "قيمة_سداسية": ROOT / "library/std/chars/ascii.sn",
    }
    std_sources = sorted((ROOT / "library" / "std").rglob("*.sn"))
    for name, owner in generic_owners.items():
        definitions = [
            path
            for path in std_sources
            if re.search(rf"\bعرف\s+{re.escape(name)}\b", text(path))
        ]
        if definitions != [owner]:
            shown = [str(path.relative_to(ROOT)) for path in definitions]
            problems.append(
                f"library/std: operation {name} must have exactly one owner "
                f"{owner.relative_to(ROOT)}; found {shown!r}"
            )
        if re.search(rf"\bعرف\s+{re.escape(name)}\b", compiler_code):
            problems.append(f"compiler: duplicates stdlib operation {name}")

    std_code = "\n".join(text(path) for path in std_sources)
    if "متاح_في_text_المشفر" in std_code:
        problems.append("CANONICAL OWNERSHIP FAIL: obsolete duplicate UTF-8 text-range helper returned")

    # Frontend/stdlib foundation ownership rules. These checks target semantic domains,
    # not exact function layout or current line numbers.
    if not IDENTIFIER_POLICY_REGISTRY.is_file():
        problems.append("CANONICAL OWNERSHIP FAIL: SultanC identifier policy registry is missing")
    else:
        registry_text = code_only(text(IDENTIFIER_POLICY_REGISTRY))
        tatweel_rows = re.findall(
            r"SULTANC__IDENTIFIER_IGNORED_SCALAR__\(0x0640\)", registry_text
        )
        if len(tatweel_rows) != 1:
            problems.append(
                "CANONICAL OWNERSHIP FAIL: identifier policy registry must own exactly one "
                "Tatweel U+0640 ignored-scalar row"
            )
        normalized_rows = set(re.findall(
            r"SULTANC__IDENTIFIER_NORMALIZED_SCALAR__\((0x[0-9A-F]+),\s*(0x[0-9A-F]+)\)",
            registry_text,
        ))
        expected_arabic_alef_normalization = {
            ("0x0622", "0x0627"),
            ("0x0623", "0x0627"),
            ("0x0625", "0x0627"),
        }
        if normalized_rows != expected_arabic_alef_normalization:
            problems.append(
                "CANONICAL OWNERSHIP FAIL: identifier policy registry must exclusively own "
                "Arabic Alef normalization (آ/أ/إ -> ا)"
            )
        for consumer in sorted(IDENTIFIER_POLICY_CONSUMERS):
            if not consumer.is_file():
                problems.append(
                    f"CANONICAL OWNERSHIP FAIL: generated identifier-policy consumer is missing: "
                    f"{consumer.relative_to(ROOT)}"
                )
                continue
            if "GENERATED by tools/generate/generate_identifier_policy.py" not in text(consumer)[:300]:
                problems.append(
                    f"CANONICAL OWNERSHIP FAIL: identifier-policy consumer is not generated: "
                    f"{consumer.relative_to(ROOT)}"
                )
        policy_sources = sorted(
            list((ROOT / "bootstrap/src/frontend").rglob("*.c"))
            + list(FRONTEND.rglob("*.sn"))
        )
        for path in policy_sources:
            if path in IDENTIFIER_POLICY_CONSUMERS:
                continue
            if "0x0640" in code_only(text(path)):
                problems.append(
                    "CANONICAL OWNERSHIP FAIL: Tatweel policy duplicated outside registry/generated "
                    f"consumers: {path.relative_to(ROOT)}"
                )

    utf8_owner = ROOT / "library/std/encoding/utf8.sn"
    utf8_text = text(utf8_owner) if utf8_owner.is_file() else ""
    for required in ("بايت_استمرار_يوتيف8", "صالح_كسلم_يونيكود"):
        definitions = [
            path for path in std_sources
            if re.search(rf"\bعرف\s+{re.escape(required)}\b", text(path))
        ]
        if definitions != [utf8_owner]:
            shown = [str(path.relative_to(ROOT)) for path in definitions]
            problems.append(
                f"CANONICAL OWNERSHIP FAIL: UTF-8/Unicode operation {required} must have one standard-library owner; found {shown!r}"
            )

    continuation_pattern = re.compile(r"&\s*0xC0\s*\)\s*(?:==|!=)\s*0x80")
    for path in sorted(list(COMPILER.rglob("*.sn")) + list((ROOT / "library" / "std").rglob("*.sn"))):
        if path == utf8_owner:
            continue
        if continuation_pattern.search(code_only(text(path))):
            problems.append(
                f"CANONICAL OWNERSHIP FAIL: UTF-8 continuation classification duplicated outside standard-library owner: {path.relative_to(ROOT)}"
            )

    utf16_owner = ROOT / "library/std/encoding/utf16.sn"
    for path in sorted(list(COMPILER.rglob("*.sn")) + list((ROOT / "library" / "std").rglob("*.sn"))):
        if path == utf8_owner:
            continue
        source_code = code_only(text(path))
        # UTF-16 must classify surrogate code units as part of its wire encoding.
        # That is distinct from owning the Unicode scalar-validity policy itself.
        forbidden_scalar_markers = ("0x10FFFF",) if path == utf16_owner else ("0x10FFFF", "0xD800", "0xDFFF")
        if any(value in source_code for value in forbidden_scalar_markers):
            problems.append(
                f"CANONICAL OWNERSHIP FAIL: Unicode scalar-validity policy duplicated outside standard-library owner: {path.relative_to(ROOT)}"
            )
        if re.search(r"(?:^|_)(?:part|chunk|section)_?\d+$", path.stem, re.IGNORECASE):
            problems.append(f"COHESION FAIL: obvious LOC-only split name lacks a semantic owner: {path.relative_to(ROOT)}")

    lexer_main = LEXER / "lexer.sn"
    if lexer_main.is_file() and re.search(r"\bعرف\s+محرف_فارغ\b", code_only(text(lexer_main))):
        problems.append("CANONICAL OWNERSHIP FAIL: lexer wrapper محرف_فارغ duplicates standard-library ASCII whitespace classification")

    source_owner = SOURCE / "source.sn"
    if source_owner.is_file():
        source_code = code_only(text(source_owner))
        if "متاح_في_source" in source_code and "متاح_في_النص" not in source_code:
            problems.append("CANONICAL OWNERSHIP FAIL: Source bounds reconstruct generic text-range arithmetic instead of consuming stdlib")

    name_context = NAMES / "name_context.sn"
    if name_context.is_file():
        name_code = code_only(text(name_context))
        if re.search(r"\bعرف\s+بايتات_تطابق_", name_code):
            problems.append("CANONICAL OWNERSHIP FAIL: Name System reimplements generic/identifier byte equality instead of consuming canonical identity")

    compare_owner = ROOT / "library/std/text/compare.sn"
    if compare_owner.is_file() and not re.search(r"\bعرف\s+نصان_متساويان\b", text(compare_owner)):
        problems.append("CANONICAL OWNERSHIP FAIL: generic whole-text equality is missing from standard-library text owner")

    # Module identity is one domain-specific leaf, not split across Name identifiers/Module graph.
    module_identity_owner = NAMES / "module_identity.sn"
    name_sources = sorted(NAMES.glob("*.sn"))
    for name in ("معرف_وحدة", "معرف_وحدة_من", "معرفا_وحدة_متساويان"):
        definitions = semantic_definitions(name_sources, name)
        if definitions != [module_identity_owner]:
            shown = [str(path.relative_to(ROOT)) for path in definitions]
            problems.append(
                f"CANONICAL OWNERSHIP FAIL: Module identity fact {name} must be owned only by "
                f"{module_identity_owner.relative_to(ROOT)}; found {shown!r}"
            )

    declaration_owner = NAMES / "declaration_collection.sn"
    for name in ("تكرار_تصريح", "جدول_تصريحات"):
        definitions = semantic_definitions(name_sources, name)
        if definitions != [declaration_owner]:
            shown = [str(path.relative_to(ROOT)) for path in definitions]
            problems.append(
                f"CANONICAL OWNERSHIP FAIL: declaration-collection result {name} must be owned only by "
                f"{declaration_owner.relative_to(ROOT)}; found {shown!r}"
            )

    target_spelling = COMPILER / "backend/target/target_spelling.sn"
    if target_spelling.is_file():
        target_code = code_only(text(target_spelling))
        if re.search(r"\bعرف\s+نصان_متساويان_للتهجئة\b", target_code):
            problems.append(
                "CANONICAL OWNERSHIP FAIL: Target spelling duplicates standard-library whole-text equality"
            )
        if '../../../library/std/text/compare.sn' not in text(target_spelling) or 'نصان_متساويان(' not in target_code:
            problems.append(
                "CANONICAL OWNERSHIP FAIL: Target spelling must consume stdlib نصان_متساويان"
            )

    # Semantic-to-MSIR lowering remains one MSIR-owned translation mechanism even
    # though its physical implementation is split for cohesion under msir/lowering/.
    # The public boundary and private whole-unit assembly stay together in
    # semantic_lowering.sn. lowering/functions.sn owns function-level lowering only.
    semantic_lowering = COMPILER / "msir/semantic_lowering.sn"
    lowering_root = COMPILER / "msir/lowering"
    lowering_functions = lowering_root / "functions.sn"
    expected_lowering_parts = {
        "model.sn",
        "msir_context.sn",
        "expressions.sn",
        "places.sn",
        "control_flow.sn",
        "aggregates.sn",
        "patterns.sn",
        "statements.sn",
        "functions.sn",
        "monomorphization.sn",
        "diagnostics.sn",
    }
    if lowering_root.is_dir():
        actual_parts = {path.name for path in lowering_root.glob("*.sn")}
        if actual_parts != expected_lowering_parts:
            problems.append(
                "CANONICAL OWNERSHIP FAIL: msir/lowering cohesion set drifted; "
                f"expected {sorted(expected_lowering_parts)!r}, found {sorted(actual_parts)!r}"
            )

    public_definitions = semantic_definitions(
        sorted(COMPILER.rglob("*.sn")), "اخفض_واجهة_وحدة_إلى_مسير"
    )
    if public_definitions != [semantic_lowering]:
        shown = [str(path.relative_to(ROOT)) for path in public_definitions]
        problems.append(
            "CANONICAL OWNERSHIP FAIL: public Frontend-to-MSIR lowering boundary must exist "
            f"only at {semantic_lowering.relative_to(ROOT)}; found {shown!r}"
        )

    unit_definitions = semantic_definitions(
        sorted(COMPILER.rglob("*.sn")), "اخفض_وحدة_دلالية_إلى_مسير"
    )
    if unit_definitions != [semantic_lowering]:
        shown = [str(path.relative_to(ROOT)) for path in unit_definitions]
        problems.append(
            "CANONICAL OWNERSHIP FAIL: private whole-unit MSIR construction must remain "
            f"behind the public boundary at {semantic_lowering.relative_to(ROOT)}; found {shown!r}"
        )

    semantic_code = code_only(text(semantic_lowering)) if semantic_lowering.is_file() else ""
    for forbidden_public in (
        "اخفض_وحدة_دلالية_إلى_مسير",
        "اخفض_واجهة_دلالية_إلى_مسير_داخليا",
    ):
        if re.search(rf"(?<!\w)عام\s+عرف\s+{re.escape(forbidden_public)}\s*\(", semantic_code):
            problems.append(
                "CANONICAL OWNERSHIP FAIL: internal whole-unit lowering helper is public: "
                f"{forbidden_public}"
            )

    lowering_files = [semantic_lowering] + sorted(lowering_root.glob("*.sn"))
    for lowering_file in lowering_files:
        if not lowering_file.is_file():
            continue
        imports = re.findall(
            r'^استورد\s+"([^"]+)"\s*;', text(lowering_file), re.MULTILINE
        )
        if any("backend/" in imported or imported.startswith("../backend") for imported in imports):
            problems.append(
                "CANONICAL OWNERSHIP FAIL: MSIR lowering imports backend/Target policy: "
                f"{lowering_file.relative_to(ROOT)}"
            )

    if semantic_lowering.is_file():
        semantic_text = text(semantic_lowering)
        semantic_code = code_only(semantic_text)
        for required in ('"msir_builder.sn"', '"msir_verifier.sn"'):
            if required not in semantic_text:
                problems.append(
                    "CANONICAL OWNERSHIP FAIL: MSIR whole-unit lowering must consume canonical "
                    f"{required.strip(chr(34)).split('/')[-1]}"
                )
        if "تحقق_برنامج_مسير(" not in semantic_code:
            problems.append(
                "CANONICAL OWNERSHIP FAIL: MSIR whole-unit lowering must verify MSIR before success"
            )

    if (COMPILER / "lowering").exists():
        problems.append(
            "CANONICAL OWNERSHIP FAIL: parallel top-level lowering owner exists outside compiler/msir"
        )

    # Parser consumes token semantics, never spelling banks or scanner implementation.
    parser_imports = "\n".join(
        line for p in PARSER.glob("*.sn") for line in text(p).splitlines() if line.startswith("استورد ")
    ) if PARSER.is_dir() else ""
    for forbidden in ["words/", "english.sn", "arabic.sn", "parser.sn\";"]:
        # The last check is scoped below so parser's own parser.sn file name is harmless.
        if forbidden != "parser.sn\";" and forbidden in parser_imports:
            problems.append(f"parser imports lexical spelling implementation: {forbidden}")
    for line in parser_imports.splitlines():
        if "lexer/parser.sn" in line or "lexer/cursor.sn" in line:
            problems.append(f"parser imports lexical scanning implementation: {line.strip()}")

    # AST depends only on source model (and standard-library primitives), never lexer/parser.
    ast_text = "\n".join(text(p) for p in AST.glob("*.sn")) if AST.is_dir() else ""
    for forbidden in ["lexer", "parser", "خطأ_نحوي", "فئة_خطأ_نحوي", "نتيجة_تحليل"]:
        if forbidden in ast_text:
            problems.append(f"AST contains forbidden lexer/parser dependency {forbidden!r}")
    machine_file = AST / "types.sn"
    if machine_file.is_file():
        machine = text(machine_file)
        if "نوع نوع_آلة" not in machine:
            problems.append("AST machine scalar type is missing its AST-owned نوع_آلة")
        if re.search(r"آلة\s+(من\s+)?\([^\n]*\bرمز\b", machine):
            problems.append("AST machine type stores lexical رمز instead of نوع_آلة")

    # Token display strings are qualification-only.
    production_token_text = "\n".join(text(p) for p in LEXER.rglob("*.sn")) if LEXER.is_dir() else ""
    if re.search(r"\bعرف\s+اسم_الرمز\b", production_token_text):
        problems.append("production token model contains test-only اسم_الرمز table")
    # File and directory names under compiler are Arabic-oriented; external protocol suffixes are allowed in contents only.
    latin_name = re.compile(r"[A-Za-z]")
    for path in COMPILER.rglob("*"):
        if False:
            problems.append(f"{path.relative_to(ROOT)}: non-English compiler path name")

    obsolete_ops = [
        "و_منطقي", "أو_منطقي", "و_بت", "أو_بت", "إزاحة_يسار", "إزاحة_يمين",
        "إسناد_بنقطتين",
    ]
    # Match complete identifiers only. Short obsolete names such as و_بت must not
    # reject their canonical replacements such as و_بتي or إسناد_و_بتي.
    for stale in obsolete_ops:
        pattern = rf"(?<!\w){re.escape(stale)}(?!\w)"
        if re.search(pattern, compiler_code):
            problems.append(f"compiler: obsolete operator constructor returned: {stale}")

    for item in problems:
        problem(item)
    for item in reviews:
        print(item)
    if problems:
        print(f"compiler ownership audit: FAIL ({len(problems)} issue(s))", file=sys.stderr)
        return 1
    print("compiler ownership audit: PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
