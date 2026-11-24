#!/usr/bin/env python3
"""Which functions of the standard library's core and alloc need the heap, exactly.

A function needs the heap when its compiled code reaches the allocator's
entry points (the canonical allocator interface, library/core/memory/
allocator.sn): it allocates, grows a vector, or drops an owned value (a
drop frees). Core, in Rust's sense, is the code that needs neither the heap
nor the OS; alloc is the code that needs the heap only.

The answer comes from the compiler, not from reading the source: each module
is compiled on its own to LLVM IR at -O0 (no inlining), and the call graph is
walked from every function the module defines, through the debug info that
names each function's source file. A function reaches the OS when it reaches
an external function other than an LLVM intrinsic, or code under std/os
other than the allocator.

Needs a compiler built with the LLVM backend (./build.sh --enable-llvm).

  python3 tools/audit/heap_layers_audit.py --compiler build/sultanc [--json out.json] [module ...]

With no modules, every module of library/core and library/alloc is audited,
and a function of library/core that needs the heap is an error (exit 1):
core is the code that runs without an allocator. Name modules as
LIBRARY:a.b (core:text.compare) or a.b (looked up in core, alloc, std).
"""
from __future__ import annotations

import argparse
import collections
import json
import re
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
LIBRARY = ROOT / "library"
ALLOCATOR_FILES = ("os/allocation.sn", "os/allocation_cache.sn")


def library_modules(library: str) -> list[str]:
    """LIBRARY:a.b for every module file of library/LIBRARY (root files with
    nothing but spellings are skipped)."""
    found = []
    base = LIBRARY / library
    for path in sorted(base.rglob("*.sn")):
        rel = path.relative_to(base).with_suffix("")
        parts = rel.parts
        is_root = (len(parts) == 1 and parts[0] == library) or (len(parts) >= 2 and parts[-1] == parts[-2])
        if is_root:
            code = [line for line in path.read_text(encoding="utf-8").splitlines()
                    if line.strip() and not line.lstrip().startswith(("//", "module ", "وحدة "))]
            if not code:
                continue
        found.append(f"{library}:{'.'.join(parts)}")
    return found


def module_file(module: str) -> tuple[str, str]:
    """The library and the module's file path below it."""
    if ":" in module:
        library, name = module.split(":", 1)
        return library, name.replace(".", "/") + ".sn"
    name = module.replace(".", "/") + ".sn"
    for library in ("core", "alloc", "std"):
        if (LIBRARY / library / name).is_file():
            return library, name
    return "std", name


def decode(name: str) -> str:
    raw = re.sub(rb"\\([0-9A-F]{2})", lambda m: bytes([int(m.group(1), 16)]), name.encode())
    return raw.decode(errors="replace")


def analyse(ir: str, module_file: str) -> list[dict]:
    files = {m.group(1): m.group(2) for m in re.finditer(r'^!(\d+) = !DIFile\(filename: "([^"]*)"', ir, re.M)}
    subprograms = {}
    for m in re.finditer(r"^!(\d+) = distinct !DISubprogram\(([^\n]*)", ir, re.M):
        name = re.search(r'\bname: "([^"]*)"', m.group(2))
        file = re.search(r"\bfile: !(\d+)", m.group(2))
        subprograms[m.group(1)] = (name.group(1) if name else "?", files.get(file.group(1), "?") if file else "?")
    functions: dict[str, tuple[str, str]] = {}
    calls: dict[str, set[str]] = collections.defaultdict(set)
    current = None
    for line in ir.split("\n"):
        if line.startswith("define "):
            symbol = re.search(r"@([\w.$-]+)\(", line).group(1)
            debug = re.search(r"!dbg !(\d+)", line)
            functions[symbol] = subprograms.get(debug.group(1), ("?", "?")) if debug else ("?", "?")
            current = symbol
        elif line.startswith("}"):
            current = None
        elif current:
            calls[current].update(re.findall(r"call [^@]*@([\w.$-]+)\(", line))
    externals = set(re.findall(r"^declare [^@]*@([\w.$-]+)\(", ir, re.M))

    def source(symbol: str) -> str:
        return functions.get(symbol, ("", ""))[1].replace("\\", "/")

    def is_allocator(symbol: str) -> bool:
        return source(symbol).endswith(ALLOCATOR_FILES)

    def is_os(symbol: str) -> bool:
        if symbol in externals:
            return not symbol.startswith("llvm.")
        return "/os/" in source(symbol) and not is_allocator(symbol)

    def path_to(start: str, goal) -> list[str]:
        queue = collections.deque([(start, [start])])
        seen = {start}
        while queue:
            symbol, path = queue.popleft()
            if symbol != start and goal(symbol):
                return path
            for callee in calls.get(symbol, ()):
                if callee not in seen:
                    seen.add(callee)
                    queue.append((callee, path + [callee]))
        return []

    def describe(path: list[str]) -> str:
        return " -> ".join(decode(functions.get(s, (s, ""))[0]) for s in path)

    def cause(path: list[str]) -> str:
        last = decode(functions.get(path[-1], ("", ""))[0])
        unnamed_glue = len(path) > 2 and functions.get(path[1], ("?", ""))[0] == "?"
        if last.startswith("حرر"):
            return "drops an owned value" if unnamed_glue or len(path) == 2 else "frees through a callee"
        if last.startswith("أعد_تخصيص"):
            return "grows a vector"
        return "allocates"

    result = []
    for symbol, (name, file) in functions.items():
        if not file.replace("\\", "/").endswith("/" + module_file):
            continue
        heap = path_to(symbol, is_allocator)
        system = path_to(symbol, is_os)
        result.append({
            "function": decode(name),
            "heap": bool(heap),
            "heap_cause": cause(heap) if heap else "",
            "heap_path": describe(heap),
            "os_only": bool(system) and not heap,
        })
    return result


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--compiler", required=True, help="an LLVM-enabled sultanc")
    parser.add_argument("--json", help="write the per-function result here")
    parser.add_argument("modules", nargs="*", help="dotted module names (default: the pure layer)")
    args = parser.parse_args()
    modules = args.modules or (library_modules("core") + library_modules("alloc"))
    report = {}
    with tempfile.TemporaryDirectory() as work:
        out = Path(work) / "module.ll"
        for module in modules:
            library, relative = module_file(module)
            run = subprocess.run(
                [args.compiler, "--lang", "en", "-O0", "--emit=llvm-ir", "--target", "x86_64-linux",
                 str(Path("library") / library / relative), "-o", str(out)],
                cwd=ROOT, capture_output=True, text=True, timeout=600)
            key = f"{library}:{relative[:-3].replace('/', '.')}"
            if run.returncode != 0:
                report[key] = {"error": (run.stderr or run.stdout)[:400]}
                continue
            report[key] = {"functions": analyse(out.read_text(encoding="utf-8", errors="replace"), relative)}

    kinds = collections.Counter()
    print(f"{'module':40} {'functions':>9} {'heap-free':>9} {'heap':>5}  layer")
    for module, entry in report.items():
        if "error" in entry:
            print(f"{module:40} compile error: {entry['error'].splitlines()[0] if entry['error'] else ''}")
            continue
        functions = entry["functions"]
        heap = sum(f["heap"] for f in functions)
        system = sum(f["os_only"] for f in functions)
        layer = "std (OS)" if system else ("core" if heap == 0 else ("alloc" if heap == len(functions) else "mixed"))
        kinds[layer] += 1
        print(f"{module:40} {len(functions):9} {len(functions) - heap:9} {heap:5}  {layer}")
    causes = collections.Counter(f["heap_cause"] for e in report.values() for f in e.get("functions", []) if f["heap"])
    print("\nmodules by layer:", dict(kinds))
    print("heap functions by cause:", dict(causes))
    if args.json:
        Path(args.json).write_text(json.dumps(report, ensure_ascii=False, indent=1), encoding="utf-8")
    broken = [m for m, e in report.items() if m.startswith("core:")
              and ("error" in e or any(f["heap"] for f in e.get("functions", [])))]
    for module in broken:
        entry = report[module]
        if "error" in entry:
            print(f"error: {module} does not compile")
            continue
        for function in entry["functions"]:
            if function["heap"]:
                print(f"error: {module} {function['function']} needs the heap ({function['heap_cause']}: {function['heap_path']})")
    return 1 if broken else 0


if __name__ == "__main__":
    sys.exit(main())
