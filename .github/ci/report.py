#!/usr/bin/env python3
"""Reports a SultanC test run (tools/tests/ci_run.sh) to the CI.

Reads, under the repository root given as the only argument:
  build/tests-result.json                    totals (ci_run.sh)
  build/tests-ci/runner.log                  PASS/FAIL/SKIP<TAB>name<TAB>reason lines
  build/ispeak-tests-<target>/summary.txt    one row per iSpeak test

Writes:
  build/tests-failures.txt: every failed test with its whole reason, the
      suite errors, and the tail of the log of a suite that did not build or
      crashed; the CI step wrappers (common.sh ci_step, windows.ps1 Step)
      print it after the step's collapsed log group, so it is the open end
      of a failed step's log (outside GitHub Actions it is printed here)
  the job summary ($GITHUB_STEP_SUMMARY, else stdout): totals, then every
      failed test with its reason
  one ::error annotation per failed test (at most 50)
  build/tests-junit.xml: the same results as JUnit XML, for any test viewer

Standard library only; runs on every runner (python3, or python on Windows).
Exit status 0 even when tests failed: the test step's own status is the gate.
"""

import json
import os
import sys
from xml.sax.saxutils import escape, quoteattr


def read_lines(path):
    try:
        with open(path, encoding="utf-8", errors="replace") as f:
            return f.read().splitlines()
    except OSError:
        return []


def main():
    root = sys.argv[1] if len(sys.argv) > 1 else "."
    result_path = os.path.join(root, "build", "tests-result.json")
    try:
        with open(result_path, encoding="utf-8") as f:
            result = json.load(f)
    except (OSError, ValueError) as error:
        emit_summary(["### Tests", "", f"No test result ({result_path}: {error})."])
        os.makedirs(os.path.join(root, "build"), exist_ok=True)
        with open(os.path.join(root, "build", "tests-failures.txt"), "w", encoding="utf-8") as f:
            f.write(f"\nNo test result ({result_path}: {error}): tools/tests/ci_run.sh stopped "
                    "before writing it; its own output is in the log group above.\n")
        return 0

    target = result.get("target", "?")
    cases = []  # (suite, name, status, reason)
    # A reason can span lines (compiler diagnostics, a diff): the lines up to
    # the next result line belong to the result before them.
    markers = ("PASS\t", "FAIL\t", "SKIP\t", "TIMEOUT\t")
    runner_log = os.path.join(root, "build", "tests-ci", "runner.log")
    for line in read_lines(runner_log):
        parts = line.split("\t")
        if len(parts) >= 2 and parts[0] in ("PASS", "FAIL", "SKIP"):
            cases.append(["runner", parts[1], parts[0], parts[2] if len(parts) > 2 else ""])
        elif line.startswith("TIMEOUT\t") or "الكلي=" in line:
            continue
        elif cases and cases[-1][2] == "FAIL" and line.strip() and not line.startswith(markers):
            if cases[-1][3].count("\n") < 60:
                cases[-1][3] += "\n" + line
    cases = [tuple(c) for c in cases]
    summary_rows = read_lines(os.path.join(root, "build", f"ispeak-tests-{target}", "summary.txt"))
    for row in summary_rows[1:]:
        cols = row.split()
        if len(cols) < 2:
            continue
        name, rest = cols[0], " ".join(cols[2:])
        if "FAIL" in row or "rc=" in row:
            status = "FAIL"
        elif all(c == "skip" for c in cols[2:]):
            status = "SKIP"
        else:
            status = "PASS"
        cases.append(("ispeak", name, status, rest if status != "PASS" else ""))

    failed = [c for c in cases if c[2] == "FAIL"]
    lines = [
        f"### Tests ({target})",
        "",
        "| executed | passed | failed | compile-only | skipped |",
        "|---:|---:|---:|---:|---:|",
        "| {executed} | {passed} | {failed} | {compile_only} | {skipped} |".format(
            **{k: result.get(k, "?") for k in ("executed", "passed", "failed", "compile_only", "skipped")}
        ),
    ]
    for error in result.get("errors", []):
        lines.append(f"- ⚠️ {error}")
    if failed:
        lines += ["", f"<details open><summary>{len(failed)} failed</summary>", ""]
        for suite, name, _, reason in failed[:200]:
            lines.append(f"- `{suite}` **{name}**: {reason[:300]}")
        if len(failed) > 200:
            lines.append(f"- … {len(failed) - 200} more (see the test logs artifact)")
        lines += ["", "</details>"]
    emit_summary(lines)
    log_failures(root, target, result, failed)

    if os.environ.get("GITHUB_ACTIONS"):
        for suite, name, _, reason in failed[:50]:
            message = reason.replace("%", "%25").replace("\r", "").replace("\n", "%0A")
            print(f"::error title=test {suite}/{name}::{message or 'failed'}")

    junit = os.path.join(root, "build", "tests-junit.xml")
    with open(junit, "w", encoding="utf-8") as f:
        f.write('<?xml version="1.0" encoding="UTF-8"?>\n')
        f.write(
            f"<testsuites name={quoteattr('sultanc-' + target)} tests=\"{len(cases)}\" "
            f"failures=\"{len(failed)}\">\n"
        )
        for suite in ("runner", "ispeak"):
            mine = [c for c in cases if c[0] == suite]
            fails = sum(1 for c in mine if c[2] == "FAIL")
            skips = sum(1 for c in mine if c[2] == "SKIP")
            f.write(
                f"  <testsuite name={quoteattr(suite)} tests=\"{len(mine)}\" "
                f"failures=\"{fails}\" skipped=\"{skips}\">\n"
            )
            for _, name, status, reason in mine:
                f.write(f"    <testcase classname={quoteattr(suite)} name={quoteattr(name)}")
                if status == "PASS":
                    f.write("/>\n")
                elif status == "SKIP":
                    f.write(f"><skipped message={quoteattr(reason)}/></testcase>\n")
                else:
                    f.write(f"><failure message={quoteattr(reason[:500])}>{escape(reason)}</failure></testcase>\n")
            f.write("  </testsuite>\n")
        f.write("</testsuites>\n")
    return 0


def tail(path, count):
    lines = read_lines(path)
    return lines[-count:] if lines else [f"({path}: missing or empty)"]


def log_failures(root, target, result, failed):
    """The failures, printed last in the step log (not in a group)."""
    out = []
    errors = result.get("errors", [])
    report = os.path.join(root, "build", "tests-failures.txt")
    if os.path.exists(report):
        os.remove(report)
    if not failed and not errors:
        return
    out.append("")
    out.append("=" * 72)
    out.append(f"SultanC tests on {target}: {result.get('failed', '?')} failed "
               f"of {result.get('executed', '?')} executed")
    out.append("=" * 72)
    for error in errors:
        out.append(f"suite error: {error}")
    suites = result.get("suites", {})
    tests_ci = os.path.join(root, "build", "tests-ci")
    runner = suites.get("runner", {})
    if runner.get("built") == 0:
        out.append("")
        out.append("--- tests/runner/main.sn did not build; last lines of runner-build.log:")
        out += tail(os.path.join(tests_ci, "runner-build.log"), 60)
    elif any("runner:" in e for e in errors):
        out.append("")
        out.append("--- the runner reported no usable summary; last lines of runner.log:")
        out += tail(os.path.join(tests_ci, "runner.log"), 60)
    if any("ispeak:" in e for e in errors) or any(c[0] == "ispeak" for c in failed):
        out.append("")
        out.append("--- last lines of ispeak.log:")
        out += tail(os.path.join(tests_ci, "ispeak.log"), 40)
    for index, (suite, name, _, reason) in enumerate(failed, 1):
        out.append("")
        out.append(f"--- FAIL {index}/{len(failed)}: {suite} {name}")
        out += (reason or "(no reason given)").splitlines()
    out.append("")
    out.append("Full logs: the tests-<route>-<host> artifact of this run "
               "(build/tests-ci/*.log, build/tests-junit.xml).")
    text = "\n".join(out) + "\n"
    with open(report, "w", encoding="utf-8") as f:
        f.write(text)
    if not os.environ.get("GITHUB_ACTIONS"):
        sys.stdout.write(text)


def emit_summary(lines):
    text = "\n".join(lines) + "\n"
    path = os.environ.get("GITHUB_STEP_SUMMARY")
    if path:
        with open(path, "a", encoding="utf-8") as f:
            f.write(text)
    else:
        sys.stdout.write(text)


if __name__ == "__main__":
    sys.exit(main())
