#!/usr/bin/env python3
"""The cross-host summary of a CI run (the workflow's summary job).

Arguments: the directory the test-result artifacts were downloaded into
(one tests-<route>-<host>/ directory per job, each holding the
tests-result.json written by tools/tests/ci_run.sh).
Environment: RESULTS, the workflow's `needs` context as JSON; TIER and
EXPECTED, the plan's tier and planned jobs (plan.py), so a job the plan left
out reads "not planned", never as a pass or a failure.

Writes to $GITHUB_STEP_SUMMARY (else stdout): the result of every job, then
one row per host and route with the test totals. Standard library only.
"""

import json
import os
import sys

ICON = {"success": "✅", "failure": "❌", "cancelled": "⏹️", "skipped": "➖"}


def main():
    results_dir = sys.argv[1] if len(sys.argv) > 1 else "results"
    needs = json.loads(os.environ.get("RESULTS", "{}"))
    tier = os.environ.get("TIER") or "unknown"
    try:
        expected = set(json.loads(os.environ.get("EXPECTED") or "[]"))
    except ValueError:
        expected = set()
    lines = [f"## SultanC CI ({tier} tier)", "", "| job | planned | result |", "|---|---|---|"]
    for job, info in needs.items():
        result = info.get("result", "?")
        planned = "yes" if job in expected else "no"
        shown = "not planned" if job not in expected and result == "skipped" else result
        lines.append(f"| {job} | {planned} | {ICON.get(result, '❔')} {shown} |")

    rows = []
    if os.path.isdir(results_dir):
        for name in sorted(os.listdir(results_dir)):
            path = os.path.join(results_dir, name, "tests-result.json")
            if not os.path.isfile(path):
                continue
            try:
                with open(path, encoding="utf-8") as f:
                    r = json.load(f)
            except (OSError, ValueError):
                rows.append(f"| {name[len('tests-'):]} | unreadable result | | | | |")
                continue
            failed = r.get("failed", 0)
            mark = "✅" if failed == 0 else "❌"
            rows.append(
                f"| {mark} {name[len('tests-'):]} | {r.get('target', '?')} | {r.get('executed', '?')} "
                f"| {r.get('passed', '?')} | {failed} | {r.get('compile_only', '?')} | {r.get('skipped', '?')} |"
            )
    if rows:
        lines += [
            "",
            "### Tests",
            "",
            "| route / host | target | executed | passed | failed | compile-only | skipped |",
            "|---|---|---:|---:|---:|---:|---:|",
        ] + rows

    text = "\n".join(lines) + "\n"
    out = os.environ.get("GITHUB_STEP_SUMMARY")
    if out:
        with open(out, "a", encoding="utf-8") as f:
            f.write(text)
    else:
        sys.stdout.write(text)
    return 0


if __name__ == "__main__":
    sys.exit(main())
