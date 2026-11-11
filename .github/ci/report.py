#!/usr/bin/env python3
"""Reports a SultanC test run (tools/tests/ci_run.sh) to the CI.

Reads, under the repository root given as the only argument:
  build/tests-result.json                    totals (ci_run.sh)
  build/tests-ci/runner.log                  PASS/FAIL/SKIP<TAB>name<TAB>reason lines
  build/ispeak-tests-<target>/summary.txt    one row per iSpeak test

Writes:
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
        return 0

    target = result.get("target", "?")
    cases = []  # (suite, name, status, reason)
    for line in read_lines(os.path.join(root, "build", "tests-ci", "runner.log")):
        parts = line.split("\t")
        if len(parts) >= 2 and parts[0] in ("PASS", "FAIL", "SKIP"):
            cases.append(("runner", parts[1], parts[0], parts[2] if len(parts) > 2 else ""))
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
