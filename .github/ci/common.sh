#!/bin/sh
# Helpers shared by .github/ci/linux.sh and macos.sh (POSIX sh; sourced).
#
#   ci_load_llvm_lock       LLVM_VERSION and LLVM_RECIPE from llvm.lock
#   ci_step NAME CMD...     runs CMD in a log group, times it, and records
#                           "NAME: ok|FAILED (exit N), M:SS" in the job summary
#   ci_summary TEXT         appends a line to the job summary (or stdout)
#   ci_report_tests         test results (build/tests-result.json) to the summary
#
# Outside GitHub Actions the groups are plain banners and the summary goes to
# stdout, so the scripts run the same way on a developer machine.

ci_root=$(cd "$(dirname "$0")/../.." && pwd)

ci_load_llvm_lock() {
    # shellcheck disable=SC1091 # path is computed
    . "$ci_root/.github/ci/llvm.lock"
    : "${LLVM_VERSION:?llvm.lock sets no LLVM_VERSION}"
}

ci_summary() {
    if [ -n "${GITHUB_STEP_SUMMARY:-}" ]; then
        printf '%s\n' "$*" >> "$GITHUB_STEP_SUMMARY"
    else
        printf 'summary: %s\n' "$*"
    fi
}

ci_group_start() {
    if [ -n "${GITHUB_ACTIONS:-}" ]; then echo "::group::$1"; else echo "==> $1"; fi
}

ci_group_end() {
    if [ -n "${GITHUB_ACTIONS:-}" ]; then echo "::endgroup::"; fi
}

ci_seconds() { date +%s; }

ci_duration() { # seconds -> M:SS
    printf '%d:%02d' $(($1 / 60)) $(($1 % 60))
}

ci_step() { # name command...
    ci_step_name=$1
    shift
    ci_group_start "$ci_step_name"
    ci_step_start=$(ci_seconds)
    # The step runs in a subshell with errexit on, so the first failing
    # command of a multi-command step ends it with that command's status.
    set +e
    (set -e; "$@")
    ci_step_status=$?
    set -e
    ci_step_time=$(ci_duration $(($(ci_seconds) - ci_step_start)))
    ci_group_end
    if [ "$ci_step_status" -eq 0 ]; then
        ci_summary "- ✅ $ci_step_name ($ci_step_time)"
    else
        ci_summary "- ❌ $ci_step_name: exit $ci_step_status ($ci_step_time)"
        if [ -n "${GITHUB_ACTIONS:-}" ]; then
            echo "::error title=$ci_step_name::failed with exit $ci_step_status after $ci_step_time"
        fi
        return "$ci_step_status"
    fi
}

ci_python() {
    if command -v python3 > /dev/null 2>&1; then
        python3 "$@"
    else
        python "$@"
    fi
}

ci_report_tests() {
    ci_python "$ci_root/.github/ci/report.py" "$ci_root"
}

# Runs the test suite (tools/tests/ci_run.sh) and reports it; the exit status
# is the suite's.
ci_run_tests() { # compiler
    set +e
    SC=$1 bash "$ci_root/tools/tests/ci_run.sh"
    ci_tests_status=$?
    set -e
    ci_report_tests || true
    return "$ci_tests_status"
}
