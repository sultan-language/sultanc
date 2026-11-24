#!/usr/bin/env python3
"""Focused SultanC macOS ARM64 codegen-quality audit.

Measures real file bytes and __text separately from Mach-O virtual reservation.
Uses llvm-objdump when available (including xcrun's copy on macOS).  On macOS,
raw `size` and `otool -tvV` output are also captured when those tools exist.
"""
from __future__ import annotations

import argparse
import json
import pathlib
import re
import shutil
import subprocess
import sys


def tool(name: str) -> str | None:
    direct = shutil.which(name)
    if direct:
        return direct
    if name == "llvm-objdump":
        candidate = pathlib.Path("/usr/local/swift/usr/bin/llvm-objdump")
        if candidate.exists():
            return str(candidate)
        xcrun = shutil.which("xcrun")
        if xcrun:
            result = subprocess.run([xcrun, "-f", name], text=True, capture_output=True)
            if result.returncode == 0 and result.stdout.strip():
                return result.stdout.strip()
    return None


def run(command: list[str]) -> str:
    result = subprocess.run(command, text=True, capture_output=True)
    if result.returncode != 0:
        return ""
    return result.stdout


def metric(binary: pathlib.Path, function_names: list[str]) -> dict[str, object]:
    objdump = tool("llvm-objdump")
    if objdump is None:
        raise SystemExit("llvm-objdump is required for the focused ARM64 audit")

    disassembly = run([objdump, "--macho", "-d", str(binary)])
    sections = run([objdump, "--macho", "--section-headers", str(binary)])
    headers = run([objdump, "--macho", "--private-headers", str(binary)])
    frames = run([objdump, "--macho", "--dwarf=frames", str(binary)])

    instruction_lines = [
        line for line in disassembly.splitlines()
        if re.match(r"^\s*[0-9a-fA-F]+:", line)
    ]

    def count(pattern: str) -> int:
        rx = re.compile(pattern)
        return sum(1 for line in instruction_lines if rx.search(line))

    text_bytes = 0
    match = re.search(r"^\s*\d+\s+__text\s+([0-9a-fA-F]+)\s+", sections, re.MULTILINE)
    if match:
        text_bytes = int(match.group(1), 16)
    if text_bytes == 0:
        text_bytes = len(instruction_lines) * 4

    segment_sizes: dict[str, int] = {}
    pagezero = 0
    current_segment = None
    for line in headers.splitlines():
        stripped = line.strip()
        if stripped.startswith("segname "):
            current_segment = stripped.split(None, 1)[1]
        elif current_segment and stripped.startswith("vmsize "):
            value = int(stripped.split()[1], 16)
            segment_sizes[current_segment] = value
            if current_segment == "__PAGEZERO":
                pagezero = value
            current_segment = None

    fde_sizes: list[int] = []
    for start, end in re.findall(r"FDE .* pc=([0-9a-fA-F]+)\.\.\.([0-9a-fA-F]+)", frames):
        fde_sizes.append(int(end, 16) - int(start, 16))
    functions = {
        name: fde_sizes[index]
        for index, name in enumerate(function_names)
        if index < len(fde_sizes)
    }

    raw_size = ""
    size_tool = tool("size")
    if size_tool:
        raw_size = run([size_tool, str(binary)])
    raw_otool = ""
    otool = tool("otool")
    if otool:
        raw_otool = run([otool, "-tvV", str(binary)])

    return {
        "binary": str(binary),
        "executable_file_bytes": binary.stat().st_size,
        "text_bytes": text_bytes,
        "instruction_count": len(instruction_lines),
        "loads": count(r"\s(?:ldur|ldr|ldrb|ldrh|ldp)\b"),
        "stores": count(r"\s(?:stur|str|strb|strh|stp)\b"),
        "stack_pointer_adjustments": count(r"\s(?:add|sub)\s+sp,\s*sp"),
        "branches": count(r"\s(?:b\.[a-z]+|b\b|cbz\b|cbnz\b|br\b|blr\b|ret\b)"),
        "calls": count(r"\s(?:bl|blr)\b"),
        "syscalls": count(r"\ssvc\b"),
        "cset": count(r"\scset\b"),
        "stp": count(r"\sstp\b"),
        "ldp": count(r"\sldp\b"),
        "pagezero_virtual_bytes": pagezero,
        "segment_virtual_bytes": segment_sizes,
        "function_bytes": functions,
        "size_report": raw_size.strip() or None,
        "otool_available": bool(raw_otool),
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("after", type=pathlib.Path)
    parser.add_argument("--before", type=pathlib.Path)
    parser.add_argument("--functions", default="")
    parser.add_argument("--json-out", type=pathlib.Path)
    args = parser.parse_args()
    names = [item.strip() for item in args.functions.split(",") if item.strip()]

    report: dict[str, object] = {"after": metric(args.after, names)}
    if args.before is not None:
        report["before"] = metric(args.before, names)
    text = json.dumps(report, ensure_ascii=False, indent=2)
    if args.json_out:
        args.json_out.parent.mkdir(parents=True, exist_ok=True)
        args.json_out.write_text(text + "\n")
    print(text)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
