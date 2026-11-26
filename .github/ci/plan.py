#!/usr/bin/env python3
"""Plans one CI run: which jobs and hosts run for this trigger.

The tiers (.github/workflows/ci.yml).
The compiler is built the way mature toolchains build theirs: each build is
one compile of the sources by the previous compiler (N-1), the self-host
fixpoint is checked when a commit lands, and the wide matrix runs once a day.

  docs     only documentation changed: the docs check, no compiler build
  pr       pull request into main: one build of the compiler with the
           previous compiler (the cached compiler of the base commit, else
           Stage1 from the seed) and the test suite, on linux-x86_64 plus
           the hosts of the components the pull request changes
           (COMPONENTS); no second compile, no fixpoint
  main     push to main (a merge): from the seed, S2 == S3 (two compiles),
           the seed reproduced by the sources, the test suite; the compiler
           is cached for the pull requests based on this commit
  full     nightly, qualification (promote.yml, beta.yml), manual dispatch
           with scope=full: the seed route on every host (Stage1 on the
           hosts without a Native backend), the seed update, the LLVM
           fixpoint and cross-backend equality, the cross-target compilers
           and their runs on the hosts without a Native backend

The frozen C Stage0 runs in none of these: the release check
(.github/workflows/chain.yml, monthly and before a release) runs it.

Every host is defined once (HOSTS); a tier only selects from them, so a tier
never changes what a selected job does. The jobs are the same steps, the same
scripts (linux.sh, macos.sh, windows.ps1) and the same build commands in
every tier.

Environment:
  EVENT          github.event_name of the run that started CI
  SCOPE          the scope input: main | full (empty: from EVENT)
  CHANGED_FILES  a file holding the changed paths, one per line, or empty when
                 they are unknown (the run is then planned as if every
                 component changed, never as documentation-only)
  GITHUB_OUTPUT  where the outputs go (else stdout)
  GITHUB_STEP_SUMMARY  where the plan is described (else stdout)

Outputs (strings; matrices are JSON {"include": [...]}):
  tier, docs, build, build_matrix, seed, seed_matrix, seed_update, llvm,
  llvm_matrix, cross, cross_run_matrix, expected (JSON list of the jobs that
  must succeed)

The cross-target compilers are built by the LLVM job of linux-x86_64
(matrix field cross_build) and run by cross-run.

Standard library only.
"""

import json
import os
import re
import sys

# Every CI host, once. native: the host has the SultanC Native backend, so a
# compiler for it is built there and the test suite runs there. cross: the
# target of the compiler cross-built for a host without one (linux.sh cross).
HOSTS = {
    "linux-x86_64": {"runner": "ubuntu-24.04", "script": "linux", "native": True, "exe": "sultanc"},
    "linux-arm64": {"runner": "ubuntu-24.04-arm", "script": "linux", "native": False, "exe": "sultanc",
                    "cross": "aarch64-linux-gnu"},
    "macos-arm64": {"runner": "macos-15", "script": "macos", "native": True, "exe": "sultanc"},
    "macos-x86_64": {"runner": "macos-15-intel", "script": "macos", "native": False, "exe": "sultanc",
                     "cross": "x86_64-darwin"},
    "windows-x86_64": {"runner": "windows-2025", "script": "windows", "native": False, "exe": "sultanc.exe",
                       "cross": "x86_64-windows-msvc"},
    "windows-arm64": {"runner": "windows-11-arm", "script": "windows", "native": False, "exe": "sultanc.exe",
                      "cross": "aarch64-windows-msvc"},
}

# Paths that are documentation only.
DOCS = re.compile(r"^(docs/|LICENSE$)|\.md$")

# Components a pull request can touch, and the hosts each adds to its build
# job (build) or the LLVM-enabled build it adds (llvm).
COMPONENTS = [
    ("macOS / Darwin / Mach-O",
     re.compile(r"(^|/)(macos|darwin|macho|mach_o|apple)([/_.]|$)|^\.github/ci/macos\.sh$"),
     {"build": ["macos-arm64", "macos-x86_64"]}),
    ("Windows / COFF / PE",
     re.compile(r"(^|/)(windows|win32|coff|pe|msvc)([/_.]|$)|^\.github/ci/windows\.ps1$"),
     {"build": ["windows-x86_64"]}),
    ("arm64 Linux / Windows",
     re.compile(r"(^|/)(aarch64|arm64)([/_.]|$)"),
     {"build": ["linux-arm64", "windows-arm64"]}),
    ("LLVM provider / build",
     re.compile(r"^compiler/backend/provider/llvm/|^build\.sh$|^tools/build/|^bootstrap-wasm/"
                r"|^\.github/ci/llvm\.lock$|^\.github/ci/linux\.sh$"),
     {"llvm": ["linux-x86_64"]}),
]

PR_BUILD = ["linux-x86_64"]
MAIN_SEED = ["linux-x86_64"]
FULL_LLVM = ["linux-x86_64", "macos-arm64"]


def write(path_variable, text):
    path = os.environ.get(path_variable)
    if path:
        with open(path, "a", encoding="utf-8") as f:
            f.write(text)
    else:
        sys.stdout.write(text)


def matrix(names, **extra):
    include = []
    for name in HOSTS:  # host order is the table's
        if name in names:
            entry = {"host": name, **HOSTS[name]}
            entry.setdefault("cross", "")
            for key, value in extra.items():
                entry[key] = value(name) if callable(value) else value
            include.append(entry)
    return {"include": include}


def read_changed():
    path = os.environ.get("CHANGED_FILES", "")
    if not path or not os.path.isfile(path):
        return None
    with open(path, encoding="utf-8") as f:
        files = [line.strip() for line in f if line.strip()]
    return files or None


def main():
    event = os.environ.get("EVENT", "")
    scope = os.environ.get("SCOPE", "").strip()
    changed = read_changed()
    notes = []

    if scope == "full" or event == "schedule":
        tier = "full"
    elif scope in ("main", "quick"):
        tier = "main"
    elif event == "pull_request":
        tier = "pr"
    else:
        tier = "main"

    docs_changed = True
    code_changed = True
    if tier != "full":
        if changed is None:
            notes.append("changed files unknown: planned as if every component changed")
        else:
            docs_changed = any(DOCS.search(f) for f in changed)
            code_changed = any(not DOCS.search(f) for f in changed)
            if not code_changed:
                tier = "docs"

    build_hosts, seed_hosts, llvm_hosts = set(), set(), set()
    llvm_mode = "build"
    seed_update = False
    cross = False
    touched = []
    if tier == "full":
        seed_hosts = set(HOSTS)
        llvm_hosts = set(FULL_LLVM)
        llvm_mode = "verify"
        seed_update = True
        cross = True
    elif tier == "main":
        seed_hosts = set(MAIN_SEED)
    elif tier == "pr":
        build_hosts = set(PR_BUILD)
        for label, pattern, adds in COMPONENTS:
            if changed is None or any(pattern.search(f) for f in changed):
                touched.append(label)
                build_hosts.update(adds.get("build", []))
                llvm_hosts.update(adds.get("llvm", []))

    run_docs = tier in ("docs", "full") or docs_changed
    build = matrix(build_hosts)
    seed = matrix(seed_hosts)
    # The LLVM-enabled compiler of linux-x86_64 also builds the cross-target
    # compilers (linux.sh cross), which cross-run runs on their hosts.
    llvm = matrix(llvm_hosts, mode=llvm_mode, cross_build=lambda h: cross and h == "linux-x86_64")
    cross_run = matrix([h for h in HOSTS if HOSTS[h].get("cross")] if cross else [])

    expected = ["plan", "lint"]
    if run_docs:
        expected.append("docs")
    if build["include"]:
        expected.append("build")
    if seed["include"]:
        expected.append("seed")
    if seed_update:
        expected.append("seed-update")
    if llvm["include"]:
        expected += ["llvm-sdk", "llvm"]
    if cross:
        expected.append("cross-run")

    outputs = {
        "tier": tier,
        "docs": json.dumps(run_docs),
        "build": json.dumps(bool(build["include"])),
        "build_matrix": json.dumps(build),
        "seed": json.dumps(bool(seed["include"])),
        "seed_matrix": json.dumps(seed),
        "seed_update": json.dumps(seed_update),
        "llvm": json.dumps(bool(llvm["include"])),
        "llvm_matrix": json.dumps(llvm),
        "cross": json.dumps(cross),
        "cross_run_matrix": json.dumps(cross_run),
        "expected": json.dumps(expected),
    }
    write("GITHUB_OUTPUT", "".join(f"{k}={v}\n" for k, v in outputs.items()))

    def hosts(m, native_note):
        if not m["include"]:
            return "not run"
        return ", ".join(e["host"] + ("" if e["native"] else f" ({native_note})") for e in m["include"])

    lines = [
        f"## CI plan: {tier}",
        "",
        f"- trigger: `{event or 'local'}`" + (f", scope `{scope}`" if scope else ""),
        f"- changed files: {'unknown' if changed is None else len(changed)}",
        f"- docs check: {'run' if run_docs else 'not run'}",
        f"- build from the previous compiler: {hosts(build, 'Stage1 only')}",
        f"- seed route, S2 == S3, seed check: {hosts(seed, 'Stage1 only')}",
        f"- seed update: {'run' if seed_update else 'not run'}",
        f"- LLVM ({llvm_mode}): {hosts(llvm, '')}",
        f"- cross-target compilers: {hosts(cross_run, 'runs there')}",
    ]
    if touched:
        lines.append(f"- components changed: {', '.join(touched)}")
    for note in notes:
        lines.append(f"- note: {note}")
    if tier != "full":
        lines.append("- the full matrix runs nightly, on manual dispatch with scope `full`, "
                     "and when a commit is qualified for beta (promote.yml); the C Stage0 chain "
                     "runs monthly and before a release (chain.yml)")
    write("GITHUB_STEP_SUMMARY", "\n".join(lines) + "\n")
    return 0


if __name__ == "__main__":
    sys.exit(main())
