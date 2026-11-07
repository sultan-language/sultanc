#!/usr/bin/env python3
"""Enforce the compiler backend ownership/topology architecture.

Core law:
    ONE FACT -> ONE OWNER
    ONE MECHANISM -> ONE ENGINE

This audit is intentionally structural.  It does not compile SultanC sources and
must remain independent of backend implementation availability.
"""
from __future__ import annotations

from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[2]
COMPILER = ROOT / "compiler"
BACKEND = COMPILER / "backend"
ARCHITECTURES = BACKEND / "architectures"
TARGET = BACKEND / "target"
ABI = BACKEND / "abi"

ARCHITECTURE_NAMES = {
    "arm64",
    "x86_64",
    "wasm32",
    "riscv64",
    "armv7",
}
SYSTEM_NAMES = {
    "macos",
    "ios",
    "linux",
    "windows",
    "android",
    "freebsd",
    "wasi",
    "browser",
    "baremetal",
}
GENERIC_BUCKETS = {"common", "shared", "generic", "helpers", "utils", "misc"}

# These are mechanisms, not policy/rule filenames.  Architecture/system-local
# strategy/rule files may exist, but a second engine may not.
LINKER_ENGINE_STEMS = {
    "linker",
    "link_engine",
    "linker_engine",
    "linking_engine",
}
LOWERING_ENGINE_STEMS = {
    "lowering",
    "lowerer",
    "lowering_engine",
    "msir_lowering",
    "msir_lowerer",
}
REGALLOC_ENGINE_STEMS = {
    "regalloc",
    "register_allocator",
    "register_allocation_engine",
    "register_allocator_engine",
}
OBJECT_ENGINE_STEMS = {
    "object_engine",
    "object_writer",
    "object_serializer",
    "object_format_engine",
    "macho_writer",
    "elf_writer",
    "coff_writer",
}

# System/architecture policy markers are forbidden in the abstract ABI contract.
ABI_POLICY_MARKERS = (
    "ذراع_أربعة_وستون",
    "إكس_ستة_وثمانون_أربعة_وستون",
    "ويب_أسمبلي_اثنان_وثلاثون",
    "رايسك_في_أربعة_وستون",
    "ذراع_سبعة",
    "ماك_أو_إس",
    "آي_أو_إس",
    "لينكس",
    "ويندوز",
    "أندرويد",
    "فري_بي_إس_دي",
    "واسي",
    "متصفح",
    "عتاد_مباشر",
    "ماك_ذراع",
)

# Target owns identity/composition facts, never fake handles to implementations.
TARGET_IMPLEMENTATION_HANDLE = re.compile(
    r"(?<![\w])(?:"
    r"معرف|مقبض"
    r")_[\w\u0600-\u06ff]*(?:"
    r"خلفية|مزود|موفر|واجهة_ثنائية|كائن|تنسيق_كائن|تشغيل|رابط|ربط|مرابط|تنفيذ"
    r")[\w\u0600-\u06ff]*(?![\w])"
)
TARGET_IMPLEMENTATION_TERM = re.compile(
    r"(?<![\w])([\w\u0600-\u06ff]*(?:"
    r"خلفية|مزود|موفر|واجهة_ثنائية|تنسيق_كائن|تشغيل|رابط|مرابط|تنفيذ"
    r")[\w\u0600-\u06ff]*)(?![\w])"
)

# Source-language keywords are not project-owned identifiers.
LATIN_KEYWORDS = {
    "public", "type", "def", "let", "mut", "if", "else", "while", "match",
    "return", "import", "from", "as", "true", "false", "and", "or", "not",
}
LATIN_IDENTIFIER = re.compile(r"(?<![\w])[A-Za-z][A-Za-z0-9_]*(?![\w])")
IMPORT = re.compile(r'^\s*استورد\s+"([^"]+)"\s*;', re.MULTILINE)
DECLARATION = re.compile(r"(?<![\w])(?:عام\s+)?(?:نوع|عرف)\s+([^\s=(]+)")
PUBLIC_CAPABILITY_OPERATION = re.compile(r"(?m)^\s*عام\s+عرف\s+([^\s(]+)\s*\(")
MACOS_ARM64_POLICY_MARKERS = (
    "طريقة_تمرير_واجهة",
    "فئة_سجل_واجهة",
    "تصنيف_قيمة_واجهة",
    "خطة_دالة_واجهة",
    "خطأ_واجهة",
    "واجهة_ماك",
)


def code_only(source: str) -> str:
    """Blank comments and literals while preserving newlines/column shape."""
    out: list[str] = []
    i = 0
    state = "code"
    while i < len(source):
        char = source[i]
        nxt = source[i + 1] if i + 1 < len(source) else ""
        if state == "code":
            if char == "/" and nxt == "/":
                out.extend("  ")
                i += 2
                state = "line"
                continue
            if char == "/" and nxt == "*":
                out.extend("  ")
                i += 2
                state = "block"
                continue
            if char == '"':
                out.append(" ")
                i += 1
                state = "string"
                continue
            if char == "'":
                out.append(" ")
                i += 1
                state = "char"
                continue
            if char == "«":
                out.append(" ")
                i += 1
                state = "guillemet"
                continue
            out.append(char)
            i += 1
            continue
        if char == "\n":
            out.append("\n")
            i += 1
            if state == "line":
                state = "code"
            continue
        if state == "block" and char == "*" and nxt == "/":
            out.extend("  ")
            i += 2
            state = "code"
            continue
        if state in {"string", "char"} and char == "\\" and nxt:
            out.extend("  ")
            i += 2
            continue
        if state == "string" and char == '"':
            out.append(" ")
            i += 1
            state = "code"
            continue
        if state == "char" and char == "'":
            out.append(" ")
            i += 1
            state = "code"
            continue
        if state == "guillemet" and char == "»":
            out.append(" ")
            i += 1
            state = "code"
            continue
        out.append(" ")
        i += 1
    return "".join(out)


def relative(path: Path) -> str:
    return path.relative_to(ROOT).as_posix()


def line_of(source: str, offset: int) -> int:
    return source.count("\n", 0, offset) + 1


def add(problems: list[str], path: Path, line: int, message: str) -> None:
    problems.append(f"{relative(path)}:{line}: {message}")


def source_files() -> list[Path]:
    return sorted(BACKEND.rglob("*.sn")) if BACKEND.is_dir() else []


def architecture_context(path: Path) -> tuple[str, str | None] | None:
    try:
        rel = path.relative_to(ARCHITECTURES)
    except ValueError:
        return None
    parts = rel.parts
    if not parts:
        return None
    family = parts[0]
    system: str | None = None
    if len(parts) >= 3 and parts[1] == "systems":
        system = parts[2]
    return family, system


def engine_stem(stem: str, kind: str) -> bool:
    if kind == "linker":
        return (
            stem in LINKER_ENGINE_STEMS
            or stem.endswith("_linker")
            or "linker_engine" in stem
            or "link_engine" in stem
        )
    if kind == "lowering":
        if stem.endswith(("_rules", "_constraints", "_policy")):
            return False
        return (
            stem in LOWERING_ENGINE_STEMS
            or stem.endswith("_lowering")
            or "lowering_engine" in stem
            or stem.endswith("_lowerer")
        )
    if kind == "regalloc":
        if stem.endswith(("_rules", "_constraints", "_registers")):
            return False
        return (
            stem in REGALLOC_ENGINE_STEMS
            or "regalloc" in stem
            or "register_allocator" in stem
            or "register_allocation" in stem
        )
    if kind == "object":
        return (
            stem in OBJECT_ENGINE_STEMS
            or "object_writer" in stem
            or "object_serializer" in stem
            or "object_format_engine" in stem
            or stem.endswith(("macho_writer", "elf_writer", "coff_writer"))
        )
    raise ValueError(f"unknown engine kind: {kind}")


def has_real_source(path: Path, *, exclude_systems: bool = False) -> bool:
    for source in path.rglob("*.sn"):
        if exclude_systems:
            try:
                rel = source.relative_to(path)
            except ValueError:
                continue
            if rel.parts and rel.parts[0] == "systems":
                continue
        if code_only(source.read_text(encoding="utf-8")).strip():
            return True
    return False


def resolved_import(source: Path, imported: str) -> Path:
    return (source.parent / imported).resolve()


def main() -> int:
    problems: list[str] = []
    if not BACKEND.is_dir():
        print("compiler/backend:1: backend root is missing", file=sys.stderr)
        return 1

    files = source_files()
    texts = {path: path.read_text(encoding="utf-8") for path in files}
    codes = {path: code_only(text) for path, text in texts.items()}

    # ONE MECHANISM -> ONE ENGINE: the rejected central backend catalog is not an owner.
    catalog = BACKEND / "backend_catalog.sn"
    if catalog.exists():
        add(problems, catalog, 1, "rejected central backend catalog; composition must not be a peer implementation switch")

    # Architecture families belong only under backend/architectures/.
    direct_architectures = [
        path for path in BACKEND.iterdir()
        if path.is_dir()
        and (
            path.name in ARCHITECTURE_NAMES
            or (path / "systems").is_dir()
            or any((path / system).is_dir() for system in SYSTEM_NAMES)
        )
    ]
    if direct_architectures and not ARCHITECTURES.is_dir():
        add(problems, BACKEND, 1, "architecture implementation root must be compiler/backend/architectures/")
    for path in direct_architectures:
        add(problems, path, 1, "architecture family is a backend peer; move under backend/architectures/<architecture>/")

    # Any directory named as a system must have the canonical family/systems/system shape.
    for path in sorted(p for p in BACKEND.rglob("*") if p.is_dir() and p.name in SYSTEM_NAMES):
        context = architecture_context(path)
        canonical = context is not None and context[1] == path.name and path.parent.name == "systems"
        if not canonical:
            add(problems, path, 1, "system implementation must live under architectures/<architecture>/systems/<system>/")

    # Canonical architecture/system directories may not exist as placeholders.
    if ARCHITECTURES.is_dir():
        for family in sorted(p for p in ARCHITECTURES.iterdir() if p.is_dir()):
            if family.name in GENERIC_BUCKETS:
                continue
            if not has_real_source(family):
                add(problems, family, 1, "placeholder architecture family: no production source in family or implemented systems")
            systems = family / "systems"
            if systems.is_dir():
                for system in sorted(p for p in systems.iterdir() if p.is_dir()):
                    if not has_real_source(system):
                        add(problems, system, 1, "placeholder system implementation: no production source")

    # No generic ownership buckets anywhere in the backend production tree.
    for path in sorted(BACKEND.rglob("*")):
        if path == BACKEND:
            continue
        components = path.relative_to(BACKEND).parts
        offending = next((part for part in components if Path(part).stem in GENERIC_BUCKETS), None)
        if offending is not None:
            add(problems, path, 1, f"generic ownership bucket {offending!r} is forbidden")

    # Generic ABI is contract vocabulary only: no architecture/system policy.
    if ABI.is_dir():
        for path in sorted(ABI.rglob("*.sn")):
            code = codes.get(path, "")
            for marker in ABI_POLICY_MARKERS:
                match = re.search(rf"(?<![\w]){re.escape(marker)}(?![\w])", code)
                if match is not None:
                    add(problems, path, line_of(code, match.start()), f"backend/abi contains architecture/system policy marker {marker!r}")

    # macOS ARM64 ABI policy has one exact owner subtree.  Target identity files may
    # mention ARM64 and macOS together; that is not ABI policy unless ABI vocabulary
    # is also present.
    expected_macos_arm64 = ARCHITECTURES / "arm64/systems/macos"
    for path, code in codes.items():
        try:
            path.relative_to(TARGET)
            continue
        except ValueError:
            pass
        arm64 = code.find("ذراع_أربعة_وستون")
        macos = code.find("ماك_أو_إس")
        abi_positions = [code.find(marker) for marker in MACOS_ARM64_POLICY_MARKERS]
        abi_positions = [position for position in abi_positions if position >= 0]
        if arm64 < 0 or macos < 0 or not abi_positions:
            continue
        try:
            path.relative_to(expected_macos_arm64)
        except ValueError:
            add(
                problems,
                path,
                line_of(code, min([arm64, macos, *abi_positions])),
                "macOS ARM64 ABI policy must live only under architectures/arm64/systems/macos/",
            )

    # Target identity/composition must not carry fake implementation/provider handles.
    if TARGET.is_dir():
        for path in sorted(TARGET.rglob("*.sn")):
            code = codes.get(path, "")
            seen_offsets: set[int] = set()
            for match in TARGET_IMPLEMENTATION_HANDLE.finditer(code):
                seen_offsets.add(match.start())
                add(
                    problems,
                    path,
                    line_of(code, match.start()),
                    f"Target carries implementation/provider handle {match.group(0)!r}",
                )

    # System-local linker engines are duplicate mechanisms; only policies/strategy may be local.
    # Architecture-local lowering/regalloc engines are also forbidden duplicate mechanisms.
    object_engines: list[Path] = []
    for path in files:
        context = architecture_context(path)
        stem = path.stem.lower()
        if engine_stem(stem, "object"):
            object_engines.append(path)
        if context is None:
            continue
        family, system = context
        if system is not None and engine_stem(stem, "linker"):
            add(problems, path, 1, f"per-system linker engine is forbidden ({family}/{system})")
        if engine_stem(stem, "lowering"):
            add(problems, path, 1, f"per-architecture lowering engine is forbidden ({family})")
        if engine_stem(stem, "regalloc"):
            add(problems, path, 1, f"per-architecture register allocator is forbidden ({family})")
        if engine_stem(stem, "object"):
            add(problems, path, 1, f"architecture/system-local object-format engine is forbidden ({family})")
    if len(object_engines) > 1:
        locations = ", ".join(relative(path) for path in object_engines)
        problems.append(f"compiler/backend:1: duplicated object-format engine owners: {locations}")

    # Deletion isolation: architecture families may not import peer families and systems may
    # not import sibling systems. Concrete system implementations are visible only to the
    # Target composition layer and to files within that same system subtree. This keeps
    # generic ABI/Layout/frontend/MSIR and architecture-family code unchanged when a system
    # implementation is removed.
    for path, text in texts.items():
        source_context = architecture_context(path)
        for match in IMPORT.finditer(text):
            target = resolved_import(path, match.group(1))
            target_context = architecture_context(target)
            if source_context is not None and target_context is not None:
                source_family, source_system = source_context
                target_family, target_system = target_context
                if source_family != target_family:
                    add(problems, path, line_of(text, match.start()), "architecture family imports an unrelated peer family")
                elif source_system is not None and target_system is not None and source_system != target_system:
                    add(problems, path, line_of(text, match.start()), "system implementation imports a sibling system implementation")

    compiler_files = sorted(COMPILER.rglob("*.sn")) if COMPILER.is_dir() else []
    compiler_texts = {path: path.read_text(encoding="utf-8") for path in compiler_files}
    registered_systems: set[Path] = set()
    for path, text in compiler_texts.items():
        source_context = architecture_context(path)
        source_is_target = False
        try:
            path.relative_to(TARGET)
            source_is_target = True
        except ValueError:
            pass

        for match in IMPORT.finditer(text):
            imported = match.group(1)
            target = resolved_import(path, imported)
            target_context = architecture_context(target)
            if target_context is None or target_context[1] is None:
                continue

            target_family, target_system = target_context
            target_system_root = (ARCHITECTURES / target_family / "systems" / target_system).resolve()
            same_system = (
                source_context is not None
                and source_context[0] == target_family
                and source_context[1] == target_system
            )

            if source_is_target:
                if not target.is_file():
                    add(
                        problems,
                        path,
                        line_of(text, match.start()),
                        f"Target composition references missing system implementation {imported!r}",
                    )
                else:
                    target_entry_code = code_only(target.read_text(encoding="utf-8"))
                    exported_operations = {
                        declaration.group(1)
                        for declaration in PUBLIC_CAPABILITY_OPERATION.finditer(target_entry_code)
                    }
                    source_code = code_only(text)
                    consumed_operations = {
                        operation
                        for operation in exported_operations
                        if re.search(rf"(?<![\w]){re.escape(operation)}\s*\(", source_code) is not None
                    }
                    if not exported_operations:
                        add(
                            problems,
                            target,
                            1,
                            "Target-imported system entry exports no public capability operation",
                        )
                    elif not consumed_operations:
                        add(
                            problems,
                            path,
                            line_of(text, match.start()),
                            "Target imports concrete system entry without consuming any exported public capability operation",
                        )
                    else:
                        registered_systems.add(target_system_root)
                continue

            if not same_system:
                add(
                    problems,
                    path,
                    line_of(text, match.start()),
                    "concrete system implementation may be consumed only by Target composition or its own system subtree",
                )

    # A concrete system subtree with production source must be connected through Target
    # composition. Otherwise it is an unbound implementation/placeholder and cannot satisfy
    # add/remove isolation. One Target import may register an entry file whose sibling files
    # remain private to the same system subtree.
    if ARCHITECTURES.is_dir():
        for family in sorted(p for p in ARCHITECTURES.iterdir() if p.is_dir()):
            systems = family / "systems"
            if not systems.is_dir():
                continue
            for system in sorted(p for p in systems.iterdir() if p.is_dir()):
                if has_real_source(system) and system.resolve() not in registered_systems:
                    add(
                        problems,
                        system,
                        1,
                        "system implementation has production source but no Target composition registration",
                    )

    # MSIR is the sole target-neutral IR boundary.  Backend must not consume frontend
    # parser/AST/Pattern/Name/ownership semantics or an alternate target-neutral IR.
    forbidden_frontend = ("/frontend/ast/", "/frontend/parser/", "/frontend/pattern/", "/frontend/names/", "/frontend/ssf/", "/frontend/memory/")
    for path, text in texts.items():
        for match in IMPORT.finditer(text):
            imported = match.group(1).replace("\\", "/")
            normalized = "/" + imported.lstrip("./")
            if any(fragment in normalized for fragment in forbidden_frontend):
                add(problems, path, line_of(text, match.start()), f"backend bypasses MSIR boundary via {imported!r}")
            lower = normalized.lower()
            if "/quad/" in lower or "/hir/" in lower or "/backend_ir/" in lower or "/interpreter_ir/" in lower:
                add(problems, path, line_of(text, match.start()), f"alternate target-neutral IR boundary is forbidden: {imported!r}")

    # MSIR is the only compiler-wide target-neutral IR. Post-selection instructions
    # are architecture-owned Selected Code; the rejected generic Machine-IR layer must
    # never return.
    machine_ir = BACKEND / "machine_ir"
    if machine_ir.exists():
        add(problems, machine_ir, 1, "generic Machine IR ownership is forbidden; selected target code is architecture-owned")

    required_selected = (
        BACKEND / "codegen" / "virtual_registers.sn",
        BACKEND / "codegen" / "codegen_boundary.sn",
        ARCHITECTURES / "arm64" / "instruction_model.sn",
        ARCHITECTURES / "arm64" / "selected_code.sn",
        ARCHITECTURES / "x86_64" / "instruction_model.sn",
        ARCHITECTURES / "x86_64" / "selected_code.sn",
    )
    for required in required_selected:
        if not required.is_file():
            add(problems, required, 1, "selected-code architecture foundation file is missing")

    rejected_machine_names = ("رمز_تعليمة_معمارية", "عملية_آلة", "برنامج_آلة", "تعليمة_آلة")
    for path, code in codes.items():
        for name in rejected_machine_names:
            match = re.search(rf"(?<![\w]){re.escape(name)}(?![\w])", code)
            if match:
                add(problems, path, line_of(code, match.start()), f"rejected generic machine-instruction ownership {name!r}")

    codegen_root = BACKEND / "codegen"
    if codegen_root.is_dir():
        for path in sorted(codegen_root.rglob("*.sn")):
            code = codes.get(path, code_only(path.read_text(encoding="utf-8")))
            for marker in ("تعليمة_ذراع", "تعليمة_إكس", "عملية_خلفية", "رمز_عملية"):
                match = re.search(re.escape(marker), code)
                if match:
                    add(problems, path, line_of(code, match.start()), "shared codegen substrate must not own target instruction operations")

    forbidden_ir_buckets = {"quad", "hir", "backend_ir", "interpreter_ir", "semantic_ir", "machine_ir"}
    for path in sorted(BACKEND.rglob("*")):
        if any(Path(part).stem.lower() in forbidden_ir_buckets for part in path.relative_to(BACKEND).parts):
            add(problems, path, 1, "alternate target-neutral IR ownership bucket; verified MSIR is the sole boundary")
    for path, code in codes.items():
        for match in re.finditer(r"(?<![\w])(?:عام\s+)?نوع\s+([^\s=]+)", code):
            name = match.group(1)
            if "مسير" in name:
                add(problems, path, line_of(code, match.start()), f"backend duplicates MSIR model type {name!r}")

    # B1.6/B2 permanent Selected-Code ownership gates.
    shared_core = codegen_root / "selected_code_core.sn"
    selection_contract = codegen_root / "selection_contract.sn"
    for required in (shared_core, selection_contract):
        if not required.is_file():
            add(problems, required, 1, "canonical shared Selected-Code owner/selection contract is missing")

    selected_models = (
        ARCHITECTURES / "arm64" / "selected_code.sn",
        ARCHITECTURES / "x86_64" / "selected_code.sn",
    )
    duplicated_metadata_fields = ("المعرف:", "الخلفاء:", "عدد_السجلات_الافتراضية:", "الهدف:")
    for path in selected_models:
        if not path.is_file():
            continue
        code = codes.get(path, code_only(path.read_text(encoding="utf-8")))
        for marker in duplicated_metadata_fields:
            match = re.search(re.escape(marker), code)
            if match:
                add(problems, path, line_of(code, match.start()),
                    f"architecture Selected Code duplicates shared metadata field {marker!r}")

    common_predicates = (
        "يساوي", "لا_يساوي", "أصغر_موقع", "أصغر_أو_يساوي_موقع",
        "أكبر_موقع", "أكبر_أو_يساوي_موقع", "أصغر_غير_موقع",
        "أصغر_أو_يساوي_غير_موقع", "أكبر_غير_موقع", "أكبر_أو_يساوي_غير_موقع",
    )
    for path in (
        ARCHITECTURES / "arm64" / "instruction_model.sn",
        ARCHITECTURES / "x86_64" / "instruction_model.sn",
    ):
        if not path.is_file():
            continue
        code = codes.get(path, code_only(path.read_text(encoding="utf-8")))
        for predicate in common_predicates:
            match = re.search(rf"(?m)^\s*\|\s*{re.escape(predicate)}(?:\s|$)", code)
            if match:
                add(problems, path, line_of(code, match.start()),
                    f"architecture duplicates common selection predicate {predicate!r}")

    # Shared codegen may own references/contracts, never a universal target operation language.
    generic_instruction_markers = (
        "تعليمة_منتقاة", "تعليمة_هدف", "عملية_منتقاة", "عملية_هدف",
        "جمع_مشترك", "تحميل_مشترك", "تخزين_مشترك", "نداء_مشترك",
    )
    for path in sorted(codegen_root.rglob("*.sn")) if codegen_root.is_dir() else []:
        code = codes.get(path, code_only(path.read_text(encoding="utf-8")))
        for marker in generic_instruction_markers:
            match = re.search(re.escape(marker), code)
            if match:
                add(problems, path, line_of(code, match.start()),
                    f"shared codegen owns forbidden universal target operation {marker!r}")

    # Selection/Selected-Code layers must use the explicit Backend query boundary, not Frontend semantics.
    selection_layer_files = [
        *(sorted(codegen_root.rglob("*.sn")) if codegen_root.is_dir() else []),
        ARCHITECTURES / "arm64" / "selector.sn",
        ARCHITECTURES / "arm64" / "instruction_model.sn",
        ARCHITECTURES / "arm64" / "selected_code.sn",
        ARCHITECTURES / "x86_64" / "instruction_model.sn",
        ARCHITECTURES / "x86_64" / "selected_code.sn",
    ]
    for path in selection_layer_files:
        if not path.is_file():
            continue
        text = path.read_text(encoding="utf-8")
        for match in IMPORT.finditer(text):
            imported = match.group(1).replace("\\", "/")
            if "/frontend/types/" in ("/" + imported.lstrip("./")):
                add(problems, path, line_of(text, match.start()),
                    f"selection/Selected-Code layer bypasses Backend type-query boundary via {imported!r}")

    # Canonical documentation must describe Selected Code, never the rejected generic intermediate layer.
    canonical_docs = (
        ROOT / "docs" / "MSIR_CODEGEN_ARCHITECTURE.md",
        COMPILER / "manifest.txt",
    )
    stale_doc = re.compile(r"Machine[ -]?IR|machine_ir|\bMIR\b", re.IGNORECASE)
    for path in canonical_docs:
        if not path.is_file():
            add(problems, path, 1, "canonical backend architecture document is missing")
            continue
        text = path.read_text(encoding="utf-8")
        match = stale_doc.search(text)
        if match:
            add(problems, path, line_of(text, match.start()),
                "canonical documentation still claims rejected generic machine intermediate architecture")

    # Internal project-owned production identifiers are Arabic-only.  English filenames and
    # import paths are literals and were blanked by code_only().
    for path, code in codes.items():
        for match in LATIN_IDENTIFIER.finditer(code):
            token = match.group(0)
            if token in LATIN_KEYWORDS:
                continue
            add(problems, path, line_of(code, match.start()), f"project-owned Latin identifier {token!r}; compiler production identifiers must be Arabic")

    if problems:
        for problem in problems:
            print(problem)
        print(f"Backend architecture audit: FAIL ({len(problems)} issue(s))", file=sys.stderr)
        return 1

    print(
        "Backend architecture audit: PASS "
        "(one-owner topology, family/system isolation, ABI policy separation, target handle purity, "
        "single engines, registered system composition, MSIR boundary, Arabic identifiers)"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
