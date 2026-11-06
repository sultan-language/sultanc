#!/usr/bin/env bash
set -euo pipefail

# Execute a delivered SultanC test runner and verify its result-count contract.
# CI never infers execution from files present on disk.
#
#   test-suite.sh CANDIDATE_DIR full      tests.runner (tools/tests/ci_run.sh)
#                                         with SC=build/sultanc and the host
#                                         target; result file from the runner
#   test-suite.sh CANDIDATE_DIR portable  tests.portable.runner
#                                         (tools/tests/ispeak_test.sh) with
#                                         Stage1 from the seed and the host's
#                                         tests.portable_target; this script
#                                         turns its summary into the same
#                                         result schema
# Both profiles are then checked by tests.result_contract.

usage() {
  echo "usage: test-suite.sh CANDIDATE_DIR {full|portable}" >&2
  exit 2
}

fail() {
  echo "$*" >&2
  exit 1
}

verify_test_roots() {
  local test_root

  while IFS= read -r test_root; do
    [[ -d "$candidate_dir/$test_root" ]] || fail \
      "required test root is missing: $test_root"
  done < <(jq -r '.tests.roots[]' "$contracts")
}

run_with_timeout() {
  local timeout_seconds="${SULTANC_TEST_TIMEOUT_SECONDS:-3600}"
  local timeout_marker="$diag/timeout.marker"
  local runner_pid
  local watchdog_pid
  local runner_rc

  rm -f "$timeout_marker"

  set +e
  bash -euo pipefail -c "$runner" \
    >"$diag/runner.stdout" \
    2>"$diag/runner.stderr" &
  runner_pid=$!

  (
    sleep "$timeout_seconds"
    if kill -0 "$runner_pid" 2>/dev/null; then
      : > "$timeout_marker"
      kill -TERM "$runner_pid" 2>/dev/null || true
      sleep 5
      kill -KILL "$runner_pid" 2>/dev/null || true
    fi
  ) &
  watchdog_pid=$!

  wait "$runner_pid"
  runner_rc=$?
  kill "$watchdog_pid" 2>/dev/null || true
  wait "$watchdog_pid" 2>/dev/null || true
  set -e

  if [[ -e "$timeout_marker" ]]; then
    fail "required test runner timed out after ${timeout_seconds}s"
  fi

  return "$runner_rc"
}

read_count() {
  local query="$1"
  local label="$2"
  local value

  value="$(jq -er "$query | numbers" "$result_file")" || fail \
    "invalid $label count in result file"
  [[ "$value" =~ ^[0-9]+$ ]] || fail \
    "test counts must be non-negative integers"
  printf '%s\n' "$value"
}

# ispeak_test.sh writes one row per test after a header row; a row fails when
# it contains FAIL or rc=, the same rule tools/tests/ci_run.sh applies. A row
# that built but ran nothing ("-" in NAT_RUN: the target is not the host) is
# compile-only and is reported apart from the passes, as ci_run.sh does.
write_portable_result() {
  local summary_file="$1"
  local total
  local failed
  local skipped
  local compile_only

  [[ -s "$summary_file" ]] || fail "portable test summary was not produced: $summary_file"
  cp -- "$summary_file" "$diag/summary.txt"
  total=$(($(wc -l < "$summary_file") - 1))
  ((total > 0)) || fail "portable test summary lists no tests: $summary_file"
  failed="$(tail -n +2 "$summary_file" | grep -Ec -- "$failure_regex" || true)"
  skipped="$(tail -n +2 "$summary_file" | grep -Ec -- "$skip_regex" || true)"
  compile_only="$(tail -n +2 "$summary_file" | grep -Ev -- "$failure_regex" |
    grep -Ec -- '^[^ ]+ +run +PASS +PASS +- ' || true)"
  jq -n --argjson total "$total" --argjson failed "$failed" --argjson skipped "$skipped" \
    --argjson compile_only "$compile_only" --arg target "$test_target" \
    '{executed: ($total - $skipped), passed: ($total - $skipped - $failed - $compile_only),
      failed: $failed, skipped: $skipped, compile_only: $compile_only, target: $target,
      suites: {ispeak: {executed: ($total - $skipped), failed: $failed,
                        compile_only: $compile_only}}}' > "$result_file"
}

[[ $# -eq 2 ]] || usage
candidate_dir="$1"
profile="$2"
script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
contracts="$script_dir/../ci/contracts.json"
summary="${GITHUB_STEP_SUMMARY:-/dev/null}"

command -v jq >/dev/null 2>&1 || fail "jq is required by the CI harness"
host="$("$script_dir/host-matrix.sh" "$contracts" --current)"
suffix="$(jq -r '.executable_suffix' <<< "$host")"

state="$(jq -r '.tests.state' "$contracts")"
reason="$(jq -r '.tests.reason // empty' "$contracts")"

echo "### SultanC test suite (\`$profile\`)" >> "$summary"
if [[ "$state" == "pending" ]]; then
  {
    echo '- Status: `PENDING`'
    echo '- Test suite not executed.'
    printf -- '- Reason: %s\n' "$reason"
  } >> "$summary"
  exit 0
fi
[[ "$state" == "required" ]] || fail "unsupported tests state: $state"

case "$profile" in
  full)
    runner="$(jq -r '.tests.runner // empty' "$contracts")"
    result_path="$(jq -r '.tests.result_contract.path // empty' "$contracts")"
    compiler="./build/sultanc$suffix"
    test_target="$(jq -r '.target' <<< "$host")"
    ;;
  portable)
    runner="$(jq -r '.tests.portable.runner // empty' "$contracts")"
    result_path="$(jq -r '.tests.portable.result_path // empty' "$contracts")"
    failure_regex="$(jq -r '.tests.portable.failure_regex // empty' "$contracts")"
    skip_regex="$(jq -r '.tests.portable.skip_regex // empty' "$contracts")"
    compiler="./$(jq -r '.seed_bootstrap.output_root' "$contracts")/stage1$suffix"
    test_target="$(jq -r '.tests.portable_target' <<< "$host")"
    [[ -n "$failure_regex" && -n "$skip_regex" ]] || fail \
      "tests.portable.failure_regex and skip_regex are required"
    ;;
  *) usage ;;
esac
[[ -n "$runner" ]] || fail "the $profile test runner is not configured"
[[ -n "$result_path" ]] || fail "the $profile test result path is not configured"

executed_jq="$(jq -r '.tests.result_contract.executed_jq // empty' "$contracts")"
passed_jq="$(jq -r '.tests.result_contract.passed_jq // empty' "$contracts")"
failed_jq="$(jq -r '.tests.result_contract.failed_jq // empty' "$contracts")"
[[ -n "$executed_jq" && -n "$passed_jq" && -n "$failed_jq" ]] || fail \
  "tests.result_contract executed/passed/failed queries are required"

candidate_dir="$(cd -- "$candidate_dir" && pwd)"
verify_test_roots
[[ -x "$candidate_dir/$compiler" ]] || fail "test compiler is missing: $compiler"

result_file="$candidate_dir/$result_path"
diag="${RUNNER_TEMP:-/tmp}/sultanc-ci/tests-$profile"
rm -f -- "$result_file"
rm -rf "$diag"
mkdir -p "$diag"

# The runners read SC (compiler) and TARGET; they run from the repository root.
export SC="$compiler"
export TARGET="$test_target"
set +e
(
  cd -- "$candidate_dir"
  run_with_timeout
)
runner_rc=$?
set -e
tail -n 40 "$diag/runner.stdout" || true

if [[ "$profile" == "portable" ]]; then
  summary_path="$(jq -r '.tests.portable.summary' "$contracts")"
  write_portable_result "$candidate_dir/${summary_path//\{target\}/$test_target}"
fi

[[ -s "$result_file" ]] || fail \
  "required test result file was not produced: $result_path (runner exit $runner_rc)"
cp -- "$result_file" "$diag/result.json"
jq -e . "$result_file" >/dev/null || fail "required test result file is malformed JSON"

executed="$(read_count "$executed_jq" "executed-test")"
passed="$(read_count "$passed_jq" "passed-test")"
failed="$(read_count "$failed_jq" "failed-test")"

{
  printf -- '- Compiler: `%s`, target: `%s`\n' "$compiler" "$test_target"
  printf -- '- Executed: `%d`\n' "$executed"
  printf -- '- Passed: `%d`\n' "$passed"
  printf -- '- Failed: `%d`\n' "$failed"
  printf -- '- Compile-only (not run): `%s`\n' "$(jq -r '.compile_only // 0' "$result_file")"
  printf -- '- Skipped: `%s`\n' "$(jq -r '.skipped // 0' "$result_file")"
  printf -- '- Runner exit: `%d`\n' "$runner_rc"
} >> "$summary"

if [[ "$failed" -ne 0 ]]; then
  grep -E -- "${failure_regex:-FAIL|rc=}" "$diag/runner.stdout" | head -n 50 >&2 || true
fi
((executed > 0)) || fail "required runner reported zero executed tests"
((failed == 0)) || fail "required runner reported $failed failed tests"
((passed <= executed)) || fail "passed count cannot exceed executed count"
((runner_rc == 0)) || fail "test runner exited with $runner_rc although it reported no failures"
echo '- Result: `PASS`' >> "$summary"
