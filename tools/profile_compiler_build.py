#!/usr/bin/env python3
"""Profile SultanC's full source build without modifying the compiler.

This tool traces the existing build.sh at command granularity and samples the
build process tree. It is intended to answer two questions before any compiler
code is changed:

1. Which bootstrap/self-host stage owns the wall-clock regression?
2. Is expensive work being repeated unexpectedly?

It does not instrument, patch, or rebuild any source differently from build.sh.
The final compiler is written to a profiling output path so the requested final
artifact does not overwrite build/sultanc. Note that build.sh itself still owns
and refreshes build/bootstrap/sultanc-stage0 and sultanc-stage1 as usual.

Typical use on macOS:

    python3 tools/profile_compiler_build.py --target arm64-darwin

Optional native stack samples for long-running SultanC child processes:

    python3 tools/profile_compiler_build.py --target arm64-darwin --stacks

Reports are written under build/profile/<timestamp>/ by default:

    summary.txt     Human-readable full rundown
    profile.json    Machine-readable timing/process data
    commands.csv    Every traced shell command and measured duration
    processes.csv   Process-tree samples (CPU/RSS/command)
    build.log       Timestamped build + shell-trace output
    stacks/         Optional macOS `sample` output
"""

from __future__ import annotations

import argparse
import csv
import datetime as _dt
import json
import os
from pathlib import Path
import platform
import re
import shlex
import shutil
import subprocess
import sys
import threading
import time
from collections import defaultdict
from dataclasses import asdict, dataclass
from typing import Dict, Iterable, List, Optional, Sequence, Tuple

TRACE_PREFIX = "__SULTANC_PROFILE_TRACE__ "


@dataclass
class TraceEvent:
    index: int
    started_s: float
    duration_s: float
    category: str
    command: str
    normalized_command: str


@dataclass
class ProcessSample:
    at_s: float
    pid: int
    ppid: int
    cpu_percent: float
    rss_kb: int
    family: str
    command: str


@dataclass
class RunningTrace:
    started_s: float
    command: str


def detect_host_target() -> str:
    system = platform.system()
    machine = platform.machine().lower()
    if system == "Darwin" and machine in {"arm64", "aarch64"}:
        return "arm64-darwin"
    if system == "Linux" and machine in {"x86_64", "amd64"}:
        return "x86_64-linux"
    raise RuntimeError(f"unsupported SultanC host for profiler: {system}/{machine}")


def project_root() -> Path:
    root = Path(__file__).resolve().parent.parent
    if not (root / "build.sh").is_file() or not (root / "compiler/main.sn").is_file():
        raise RuntimeError(f"could not identify SultanC repository root from {__file__}")
    return root


def safe_name(text: str) -> str:
    return re.sub(r"[^A-Za-z0-9_.-]+", "_", text).strip("_") or "process"


def classify_command(command: str) -> str:
    """Classify a shell-traced command into the source-build stage it belongs to."""
    c = command

    if "sultanc-stage0" in c and "--finalize-stage1" in c:
        return "stage1-finalize"

    if "sultanc-stage0" in c and "compiler/main.sn" in c:
        return "stage0-compile-compiler"

    if "sultanc-stage1" in c and "compiler/main.sn" in c:
        return "stage1-final-selfhost"

    if (
        (" -std=c11 " in f" {c} " or c.startswith("cc -std=c11") or c.startswith("clang -std=c11"))
        and "bootstrap/include" in c
    ):
        return "stage0-c-build"

    if "llvm-config" in c or "pkg-config" in c or "SULTANC_LLVM_LIBRARY" in c:
        return "llvm-discovery"

    if c.startswith("find ") or c.startswith("sort ") or c.startswith("cat "):
        return "source-discovery"

    if c.startswith("mkdir ") or c.startswith("rm ") or c.startswith("chmod ") or c.startswith("cd "):
        return "filesystem/setup"

    if c.startswith("uname "):
        return "host-detection"

    if re.match(r"^[A-Za-z_][A-Za-z0-9_]*=", c):
        return "shell/setup"

    if c.startswith("printf ") or c.startswith("echo "):
        return "reporting"

    return "other"


def normalize_command(command: str, root: Path, profile_dir: Path) -> str:
    """Normalize volatile paths/PIDs so repeated work groups together."""
    text = command
    text = text.replace(str(root), "<ROOT>")
    text = text.replace(str(profile_dir), "<PROFILE>")
    text = re.sub(r"/tmp/sultanc-[^ /]+\.\d+", "/tmp/<SULTANC_TMP>", text)
    text = re.sub(r"/var/folders/[^ ]+/T/sultanc-[^ /]+\.\d+", "<SULTANC_TMP>", text)
    text = re.sub(r"(sultanc-[A-Za-z0-9_-]+\.)\d+", r"\1<PID>", text)
    return text


def command_label(command: str, limit: int = 145) -> str:
    compact = " ".join(command.split())
    if len(compact) <= limit:
        return compact
    return compact[: limit - 3] + "..."


def process_family(command: str) -> str:
    try:
        words = shlex.split(command)
    except ValueError:
        words = command.split()
    if not words:
        return "unknown"

    exe = os.path.basename(words[0])
    joined = command
    if "sultanc-stage0" in joined:
        if "compiler/main.sn" in joined:
            return "sultanc-stage0:compiler"
        if "--finalize-stage1" in joined:
            return "sultanc-stage0:finalizer"
        return "sultanc-stage0"
    if "sultanc-stage1" in joined:
        if "compiler/main.sn" in joined:
            return "sultanc-stage1:compiler"
        return "sultanc-stage1"
    if exe in {"cc", "clang", "gcc"}:
        return "host-c-compiler"
    if "llvm" in exe.lower():
        return exe
    return exe


def ps_snapshot() -> List[Tuple[int, int, float, int, str]]:
    """Return pid, ppid, %cpu, rss_kb, command for all visible processes."""
    try:
        result = subprocess.run(
            ["ps", "-axo", "pid=,ppid=,pcpu=,rss=,command="],
            check=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.DEVNULL,
            text=True,
        )
    except (OSError, subprocess.CalledProcessError):
        return []

    rows: List[Tuple[int, int, float, int, str]] = []
    for raw in result.stdout.splitlines():
        parts = raw.strip().split(None, 4)
        if len(parts) < 5:
            continue
        try:
            pid = int(parts[0])
            ppid = int(parts[1])
            cpu = float(parts[2])
            rss = int(parts[3])
        except ValueError:
            continue
        rows.append((pid, ppid, cpu, rss, parts[4]))
    return rows


def descendants(root_pid: int, rows: Sequence[Tuple[int, int, float, int, str]]) -> List[Tuple[int, int, float, int, str]]:
    children: Dict[int, List[Tuple[int, int, float, int, str]]] = defaultdict(list)
    for row in rows:
        children[row[1]].append(row)

    out: List[Tuple[int, int, float, int, str]] = []
    stack = [root_pid]
    seen = {root_pid}
    while stack:
        parent = stack.pop()
        for row in children.get(parent, []):
            if row[0] in seen:
                continue
            seen.add(row[0])
            out.append(row)
            stack.append(row[0])
    return out


def write_commands_csv(path: Path, events: Sequence[TraceEvent]) -> None:
    with path.open("w", newline="", encoding="utf-8") as handle:
        writer = csv.writer(handle)
        writer.writerow(["index", "start_s", "duration_s", "category", "command", "normalized_command"])
        for e in events:
            writer.writerow([
                e.index,
                f"{e.started_s:.6f}",
                f"{e.duration_s:.6f}",
                e.category,
                e.command,
                e.normalized_command,
            ])


def write_processes_csv(path: Path, samples: Sequence[ProcessSample]) -> None:
    with path.open("w", newline="", encoding="utf-8") as handle:
        writer = csv.writer(handle)
        writer.writerow(["at_s", "pid", "ppid", "cpu_percent", "rss_kb", "family", "command"])
        for s in samples:
            writer.writerow([
                f"{s.at_s:.6f}",
                s.pid,
                s.ppid,
                f"{s.cpu_percent:.2f}",
                s.rss_kb,
                s.family,
                s.command,
            ])


def make_summary(
    *,
    total_s: float,
    returncode: int,
    events: Sequence[TraceEvent],
    samples: Sequence[ProcessSample],
    target: str,
    output_path: Path,
    profile_dir: Path,
    sample_interval_s: float,
) -> str:
    lines: List[str] = []
    lines.append("SultanC full source-build profile")
    lines.append("=" * 72)
    lines.append(f"target:                 {target}")
    lines.append(f"build exit code:        {returncode}")
    lines.append(f"total wall time:        {total_s:.3f} s")
    lines.append(f"profiled final output:  {output_path}")
    lines.append(f"report directory:       {profile_dir}")
    lines.append("")

    major_names = [
        "stage0-c-build",
        "stage0-compile-compiler",
        "stage1-finalize",
        "stage1-final-selfhost",
    ]
    by_category: Dict[str, List[TraceEvent]] = defaultdict(list)
    for event in events:
        by_category[event.category].append(event)

    lines.append("Major source-build stages")
    lines.append("-" * 72)
    major_total = 0.0
    major_values: Dict[str, float] = {}
    for category in major_names:
        entries = by_category.get(category, [])
        seconds = sum(e.duration_s for e in entries)
        major_values[category] = seconds
        major_total += seconds
        lines.append(f"{category:30} {seconds:10.3f} s   calls={len(entries)}")
    remainder = max(0.0, total_s - major_total)
    lines.append(f"{'everything else':30} {remainder:10.3f} s")
    lines.append("")

    lines.append("Top traced commands by wall time")
    lines.append("-" * 72)
    top_events = sorted(events, key=lambda e: e.duration_s, reverse=True)[:25]
    for event in top_events:
        pct = (event.duration_s / total_s * 100.0) if total_s > 0 else 0.0
        lines.append(
            f"{event.duration_s:9.3f} s  {pct:6.2f}%  "
            f"[{event.category}] {command_label(event.command)}"
        )
    lines.append("")

    normalized: Dict[str, List[TraceEvent]] = defaultdict(list)
    for event in events:
        normalized[event.normalized_command].append(event)

    repeated = []
    for command, group in normalized.items():
        if len(group) <= 1:
            continue
        total = sum(e.duration_s for e in group)
        maximum = max(e.duration_s for e in group)
        repeated.append((total, maximum, len(group), command))
    repeated.sort(reverse=True)

    lines.append("Repeated traced commands")
    lines.append("-" * 72)
    if not repeated:
        lines.append("No repeated shell commands detected after normalization.")
    else:
        shown = 0
        for total, maximum, count, command in repeated:
            # Skip shell bookkeeping unless it consumes meaningful wall time.
            if total < 0.050 and count < 5:
                continue
            lines.append(
                f"count={count:5d} total={total:9.3f} s max={maximum:8.3f} s  "
                f"{command_label(command)}"
            )
            shown += 1
            if shown >= 30:
                break
        if shown == 0:
            lines.append("Only negligible repeated shell bookkeeping was detected.")
    lines.append("")

    if samples:
        family_count: Dict[str, int] = defaultdict(int)
        family_cpu: Dict[str, float] = defaultdict(float)
        family_peak_rss: Dict[str, int] = defaultdict(int)
        timeline_rss: Dict[float, int] = defaultdict(int)
        timeline_cpu: Dict[float, float] = defaultdict(float)

        for sample in samples:
            family_count[sample.family] += 1
            family_cpu[sample.family] += sample.cpu_percent
            family_peak_rss[sample.family] = max(family_peak_rss[sample.family], sample.rss_kb)
            timeline_rss[sample.at_s] += sample.rss_kb
            timeline_cpu[sample.at_s] += sample.cpu_percent

        lines.append("Process-tree sampling")
        lines.append("-" * 72)
        lines.append(f"sampling interval:      {sample_interval_s:.3f} s")
        lines.append(f"samples recorded:       {len(samples)}")
        if timeline_rss:
            peak_tree_kb = max(timeline_rss.values())
            lines.append(f"peak descendant RSS:    {peak_tree_kb / 1024.0:.1f} MiB")
        if timeline_cpu:
            peak_tree_cpu = max(timeline_cpu.values())
            lines.append(f"peak descendant CPU:    {peak_tree_cpu:.1f}%")
        lines.append("")
        lines.append("Most-observed process families")
        ranked_families = sorted(
            family_count,
            key=lambda name: (family_count[name], family_cpu[name]),
            reverse=True,
        )[:20]
        for family in ranked_families:
            count = family_count[family]
            approx_observed = count * sample_interval_s
            avg_cpu = family_cpu[family] / count if count else 0.0
            peak_mib = family_peak_rss[family] / 1024.0
            lines.append(
                f"{family:31} observed~{approx_observed:8.2f} s "
                f"avgCPU={avg_cpu:7.1f}% peakRSS={peak_mib:8.1f} MiB"
            )
        lines.append("")

    lines.append("Automatic findings")
    lines.append("-" * 72)
    findings: List[str] = []

    final_s = major_values.get("stage1-final-selfhost", 0.0)
    stage0_compile_s = major_values.get("stage0-compile-compiler", 0.0)
    stage0_c_s = major_values.get("stage0-c-build", 0.0)

    if final_s > 0 and stage0_compile_s > 0:
        ratio = stage0_compile_s / final_s
        findings.append(
            f"Stage0 compiling compiler/main.sn took {stage0_compile_s:.3f}s versus "
            f"{final_s:.3f}s for Stage1's final self-host compile ({ratio:.2f}x)."
        )
        if ratio >= 2.0:
            findings.append(
                "The bootstrap Stage0 compiler path is disproportionately slower than the "
                "self-hosted compiler path; investigate Stage0/bootstrap behavior first."
            )

    if stage0_c_s > 0 and total_s > 0 and stage0_c_s / total_s >= 0.25:
        findings.append(
            f"Building the C/LLVM Stage0 seed consumes {stage0_c_s / total_s * 100.0:.1f}% "
            "of the complete source build."
        )

    compiler_main_invocations = [
        e for e in events
        if e.category in {"stage0-compile-compiler", "stage1-final-selfhost"}
    ]
    if len(compiler_main_invocations) > 2:
        findings.append(
            f"compiler/main.sn was compiled {len(compiler_main_invocations)} times by expensive "
            "compiler invocations; a normal full build is expected to need the Stage0 and Stage1 passes."
        )

    expensive_repeats = [r for r in repeated if r[0] >= 1.0 and r[2] > 1]
    if expensive_repeats:
        findings.append(
            f"Detected {len(expensive_repeats)} repeated normalized shell command(s) consuming "
            "at least 1 second in aggregate; inspect the repeated-command section."
        )

    if not findings:
        findings.append(
            "No automatic shell-level anomaly crossed the profiler thresholds. Use the major-stage "
            "and top-command timings to choose the next code-level investigation boundary."
        )

    for item in findings:
        lines.append(f"- {item}")

    lines.append("")
    lines.append("Interpretation guide")
    lines.append("-" * 72)
    lines.append("stage0-c-build            Host C compiler + LLVM build of bootstrap/src/*.c")
    lines.append("stage0-compile-compiler    Stage0 compiles compiler/main.sn into compiler.o")
    lines.append("stage1-finalize            Darwin bootstrap finalizer turns compiler.o into Stage1")
    lines.append("stage1-final-selfhost       Stage1 compiles compiler/main.sn into the final compiler")
    lines.append("")
    lines.append("If stage1-final-selfhost is close to the ~34s standalone compile but the full")
    lines.append("build is near 5 minutes, the missing time is bootstrap work above it, not the")
    lines.append("ordinary final SultanC compiler pipeline.")

    return "\n".join(lines) + "\n"


def parse_args(argv: Optional[Sequence[str]] = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Profile SultanC's complete build.sh source build without modifying compiler code."
    )
    parser.add_argument(
        "--target",
        choices=["arm64-darwin", "x86_64-linux"],
        help="final compiler target (default: detected host target)",
    )
    parser.add_argument(
        "--profile-dir",
        type=Path,
        help="report directory (default: build/profile/<timestamp>)",
    )
    parser.add_argument(
        "--output",
        type=Path,
        help="final profiled compiler output (default: <profile-dir>/sultanc-profiled)",
    )
    parser.add_argument(
        "--sample-interval",
        type=float,
        default=0.20,
        help="process-tree sampling interval in seconds (default: 0.20)",
    )
    parser.add_argument(
        "--stacks",
        action="store_true",
        help="on macOS, capture one `sample` stack profile for long-running SultanC child processes",
    )
    parser.add_argument(
        "--stack-after",
        type=float,
        default=3.0,
        help="seconds a SultanC child must run before optional stack capture (default: 3.0)",
    )
    parser.add_argument(
        "--stack-seconds",
        type=int,
        default=1,
        help="duration of each optional macOS stack capture (default: 1)",
    )
    parser.add_argument(
        "--quiet-build-output",
        action="store_true",
        help="write build output only to build.log while profiling",
    )
    return parser.parse_args(argv)


def main(argv: Optional[Sequence[str]] = None) -> int:
    args = parse_args(argv)
    if args.sample_interval <= 0:
        print("profile_compiler_build.py: --sample-interval must be > 0", file=sys.stderr)
        return 2
    if args.stack_after < 0 or args.stack_seconds <= 0:
        print("profile_compiler_build.py: invalid stack sampling duration", file=sys.stderr)
        return 2

    root = project_root()
    target = args.target or detect_host_target()
    stamp = _dt.datetime.now().strftime("%Y%m%d-%H%M%S")
    profile_dir = (args.profile_dir or (root / "build/profile" / stamp)).resolve()
    profile_dir.mkdir(parents=True, exist_ok=True)
    output_path = (args.output or (profile_dir / "sultanc-profiled")).resolve()
    output_path.parent.mkdir(parents=True, exist_ok=True)

    log_path = profile_dir / "build.log"
    commands_csv = profile_dir / "commands.csv"
    processes_csv = profile_dir / "processes.csv"
    json_path = profile_dir / "profile.json"
    summary_path = profile_dir / "summary.txt"
    stacks_dir = profile_dir / "stacks"
    if args.stacks:
        stacks_dir.mkdir(parents=True, exist_ok=True)
        if platform.system() != "Darwin" or shutil.which("sample") is None:
            print("profile_compiler_build.py: --stacks requires macOS `sample`", file=sys.stderr)
            return 2

    env = os.environ.copy()
    env["PS4"] = TRACE_PREFIX

    command = [
        "/bin/sh",
        "-x",
        str(root / "build.sh"),
        f"--target={target}",
        str(output_path),
    ]

    print(f"Profiling: {' '.join(shlex.quote(x) for x in command)}")
    print(f"Reports:   {profile_dir}")
    print("This runs the normal full source build. No compiler source is modified.")
    print("")

    start = time.monotonic()
    process = subprocess.Popen(
        command,
        cwd=str(root),
        env=env,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        bufsize=1,
    )

    samples: List[ProcessSample] = []
    stop_sampling = threading.Event()
    pid_first_seen: Dict[int, float] = {}
    sampled_pids: set[int] = set()
    stack_processes: List[subprocess.Popen] = []

    def sampler() -> None:
        while not stop_sampling.is_set():
            now = time.monotonic()
            rows = ps_snapshot()
            current = descendants(process.pid, rows)
            for pid, ppid, cpu, rss, proc_command in current:
                rel = now - start
                family = process_family(proc_command)
                samples.append(ProcessSample(rel, pid, ppid, cpu, rss, family, proc_command))
                pid_first_seen.setdefault(pid, now)

                if not args.stacks or pid in sampled_pids:
                    continue
                if "sultanc-stage0" not in proc_command and "sultanc-stage1" not in proc_command:
                    continue
                if now - pid_first_seen[pid] < args.stack_after:
                    continue

                sampled_pids.add(pid)
                stack_file = stacks_dir / f"{pid}-{safe_name(family)}.txt"
                try:
                    sp = subprocess.Popen(
                        ["sample", str(pid), str(args.stack_seconds), "10", "-file", str(stack_file)],
                        stdout=subprocess.DEVNULL,
                        stderr=subprocess.DEVNULL,
                    )
                    stack_processes.append(sp)
                except OSError:
                    pass

            stop_sampling.wait(args.sample_interval)

    sampling_thread = threading.Thread(target=sampler, name="sultanc-build-sampler", daemon=True)
    sampling_thread.start()

    events: List[TraceEvent] = []
    active_trace: Optional[RunningTrace] = None

    with log_path.open("w", encoding="utf-8") as log:
        assert process.stdout is not None
        for line in process.stdout:
            now = time.monotonic()
            rel = now - start
            log.write(f"[{rel:12.6f}] {line}")
            log.flush()

            if not args.quiet_build_output:
                sys.stdout.write(line)
                sys.stdout.flush()

            stripped = line.rstrip("\n")
            if not stripped.startswith(TRACE_PREFIX):
                continue

            traced_command = stripped[len(TRACE_PREFIX):]
            if active_trace is not None:
                index = len(events)
                category = classify_command(active_trace.command)
                events.append(
                    TraceEvent(
                        index=index,
                        started_s=active_trace.started_s,
                        duration_s=max(0.0, rel - active_trace.started_s),
                        category=category,
                        command=active_trace.command,
                        normalized_command=normalize_command(active_trace.command, root, profile_dir),
                    )
                )
            active_trace = RunningTrace(rel, traced_command)

    returncode = process.wait()
    end = time.monotonic()
    total_s = end - start

    if active_trace is not None:
        events.append(
            TraceEvent(
                index=len(events),
                started_s=active_trace.started_s,
                duration_s=max(0.0, total_s - active_trace.started_s),
                category=classify_command(active_trace.command),
                command=active_trace.command,
                normalized_command=normalize_command(active_trace.command, root, profile_dir),
            )
        )

    stop_sampling.set()
    sampling_thread.join(timeout=max(1.0, args.sample_interval * 3.0))
    for sp in stack_processes:
        try:
            sp.wait(timeout=args.stack_seconds + 2)
        except subprocess.TimeoutExpired:
            sp.terminate()

    write_commands_csv(commands_csv, events)
    write_processes_csv(processes_csv, samples)

    summary = make_summary(
        total_s=total_s,
        returncode=returncode,
        events=events,
        samples=samples,
        target=target,
        output_path=output_path,
        profile_dir=profile_dir,
        sample_interval_s=args.sample_interval,
    )
    summary_path.write_text(summary, encoding="utf-8")

    payload = {
        "schema_version": 1,
        "target": target,
        "returncode": returncode,
        "total_wall_seconds": total_s,
        "profiled_output": str(output_path),
        "profile_directory": str(profile_dir),
        "command": command,
        "sample_interval_seconds": args.sample_interval,
        "events": [asdict(e) for e in events],
        "process_samples": [asdict(s) for s in samples],
    }
    json_path.write_text(json.dumps(payload, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")

    print("\n" + summary, end="")
    print("Report files:")
    print(f"  {summary_path}")
    print(f"  {json_path}")
    print(f"  {commands_csv}")
    print(f"  {processes_csv}")
    print(f"  {log_path}")
    if args.stacks:
        print(f"  {stacks_dir}/")

    return returncode


if __name__ == "__main__":
    raise SystemExit(main())
