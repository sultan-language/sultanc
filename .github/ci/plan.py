#!/usr/bin/env python3
"""Plans one CI run: which jobs and hosts run for this trigger.

The tiers (.github/workflows/ci.yml; the policy is written out in CI.md):

  docs     only documentation changed: the docs check, no compiler build
  quick    push to main: lint, the Linux x86_64 seed route (bootstrap,
           Stage3 == Stage4, seed reproducible, test suite) and the Linux
           x86_64 LLVM route (C Stage0 -> LLVM-enabled Stage2, convergence)
  focused  pull request: quick, plus the hosts and routes of the components
           the pull request changes (COMPONENTS below)
  full     beta qualification (beta.yml), master without evidence (master.yml),
           nightly, manual dispatch with scope=full:
           every route on every host, every test step

Every host and route is defined once (HOSTS); a tier only selects from them,
so a tier never changes what a selected job does. The jobs themselves are the
same steps, the same scripts (linux.sh, macos.sh, windows.ps1) and the same
canonical build and test commands in every tier.

Environment:
  EVENT          github.event_name of the run that started CI
  SCOPE          the scope input: quick | full (empty: from EVENT)
  CHANGED_FILES  a file holding the changed paths, one per line, or empty when
                 they are unknown (the run is then planned as if every
                 component changed, never as documentation-only)
  GITHUB_OUTPUT  where the outputs go (else stdout)
  GITHUB_STEP_SUMMARY  where the plan is described (else stdout)

Outputs (strings; matrices are JSON {"include": [...]}):
  tier, docs, seed, seed_matrix, seed_update, llvm, llvm_sdk_matrix,
  llvm_matrix, expected (JSON list of the jobs that must succeed)

Standard library only.
"""

import json
import os
import re
import sys

# Every CI host, once. native: the host has the SultanC Native backend, so
# the seed route builds Stage2..Stage4 and runs the test suite there.
HOSTS = {
    "linux-x86_64": {"runner": "ubuntu-24.04", "script": "linux", "native": True, "exe": "sultanc"},
    "linux-arm64": {"runner": "ubuntu-24.04-arm", "script": "linux", "native": False, "exe": "sultanc"},
    "macos-arm64": {"runner": "macos-15", "script": "macos", "native": True, "exe": "sultanc"},
    "macos-x86_64": {"runner": "macos-15-intel", "script": "macos", "native": False, "exe": "sultanc"},
    "windows-x86_64": {"runner": "windows-2025", "script": "windows", "native": False, "exe": "sultanc.exe"},
    "windows-arm64": {"runner": "windows-11-arm", "script": "windows", "native": False, "exe": "sultanc.exe"},
}

# Paths that are documentation only (as the former paths-ignore).
DOCS = re.compile(r"^(docs/|LICENSE$)|\.md$")

# Components a pull request can touch, and what each adds to the quick tier.
#   seed: seed-route hosts      llvm: LLVM-route hosts (tests on)
#   llvm_tests: run the test step on these LLVM-route hosts
#   seed_update: the seed-update job
COMPONENTS = [
    ("macOS / Mach-O / arm64 Native",
     re.compile(r"(^|/)(macos|darwin|macho|apple)([/_.]|$)|^compiler/backend/architectures/arm64/"
                r"|^\.github/ci/macos\.sh$"),
     {"seed": ["macos-arm64"]}),
    ("Windows / COFF / PE",
     re.compile(r"(^|/)(windows|win32|win64|coff|pe|msvc|mingw)([/_.]|$)|^\.github/ci/windows\.ps1$"),
     {"llvm": ["windows-x86_64"]}),
    ("arm64 Linux",
     re.compile(r"(^|/)(aarch64|arm64)([/_.]|$)"),
     {"llvm": ["linux-arm64"]}),
    ("LLVM provider / C Stage0 / build",
     re.compile(r"^compiler/backend/provider/llvm/|^bootstrap/|^cmake/|^build\.sh$|^\.github/ci/llvm\.lock$"
                r"|^\.github/ci/linux\.sh$"),
     {"llvm_tests": ["linux-x86_64"]}),
    ("wasm seed",
     re.compile(r"^bootstrap-wasm/"),
     {"seed_update": True}),
]

QUICK_SEED = ["linux-x86_64"]
QUICK_LLVM = ["linux-x86_64"]


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
    elif scope == "quick":
        tier = "quick"
    elif event == "pull_request":
        tier = "focused"
    else:
        tier = "quick"

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

    seed_hosts, llvm_hosts, llvm_test_hosts = set(), set(), set()
    seed_update = False
    touched = []
    if tier == "full":
        seed_hosts = set(HOSTS)
        llvm_hosts = set(HOSTS)
        llvm_test_hosts = set(HOSTS)
        seed_update = True
    elif tier in ("quick", "focused"):
        seed_hosts = set(QUICK_SEED)
        llvm_hosts = set(QUICK_LLVM)
        if tier == "focused":
            for label, pattern, adds in COMPONENTS:
                if changed is None or any(pattern.search(f) for f in changed):
                    touched.append(label)
                    seed_hosts.update(adds.get("seed", []))
                    llvm_hosts.update(adds.get("llvm", []))
                    llvm_test_hosts.update(adds.get("llvm", []))
                    llvm_hosts.update(adds.get("llvm_tests", []))
                    llvm_test_hosts.update(adds.get("llvm_tests", []))
                    seed_update = seed_update or adds.get("seed_update", False)
        elif changed is None or any(COMPONENTS[-1][1].search(f) for f in changed):
            # A push that changes the seed still proves the seed update.
            seed_update = True

    run_docs = tier in ("docs", "full") or docs_changed
    seed = matrix(seed_hosts)
    llvm_sdk = matrix(llvm_hosts)
    llvm = matrix(llvm_hosts, tests=lambda h: h in llvm_test_hosts)

    expected = ["plan", "lint"]
    if run_docs:
        expected.append("docs")
    if seed["include"]:
        expected.append("seed")
    if seed_update:
        expected.append("seed-update")
    if llvm["include"]:
        expected += ["llvm-sdk", "llvm"]

    outputs = {
        "tier": tier,
        "docs": json.dumps(run_docs),
        "seed": json.dumps(bool(seed["include"])),
        "seed_matrix": json.dumps(seed),
        "seed_update": json.dumps(seed_update),
        "llvm": json.dumps(bool(llvm["include"])),
        "llvm_sdk_matrix": json.dumps(llvm_sdk),
        "llvm_matrix": json.dumps(llvm),
        "expected": json.dumps(expected),
    }
    write("GITHUB_OUTPUT", "".join(f"{k}={v}\n" for k, v in outputs.items()))

    def hosts(m, test_key=None):
        if not m["include"]:
            return "not run"
        parts = []
        for e in m["include"]:
            if test_key is None:
                parts.append(e["host"] + ("" if e["native"] else " (bootstrap only)"))
            else:
                parts.append(e["host"] + ("" if e[test_key] else " (build + convergence, no tests)"))
        return ", ".join(parts)

    lines = [
        f"## CI plan: {tier}",
        "",
        f"- trigger: `{event or 'local'}`" + (f", scope `{scope}`" if scope else ""),
        f"- changed files: {'unknown' if changed is None else len(changed)}",
        f"- docs check: {'run' if run_docs else 'not run'}",
        f"- seed route: {hosts(seed)}",
        f"- seed update: {'run' if seed_update else 'not run'}",
        f"- LLVM route: {hosts(llvm, 'tests')}",
    ]
    if touched:
        lines.append(f"- components changed: {', '.join(touched)}")
    for note in notes:
        lines.append(f"- note: {note}")
    if tier != "full":
        lines.append("- the full matrix runs nightly, on manual dispatch with scope `full`, "
                     "and in beta qualification (beta.yml); a passing quick or focused run "
                     "qualifies nothing")
    write("GITHUB_STEP_SUMMARY", "\n".join(lines) + "\n")
    return 0


if __name__ == "__main__":
    sys.exit(main())
