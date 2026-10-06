#!/usr/bin/env bash
set -euo pipefail

# Execute the delivered SultanC test runner and verify its result-count contract.
# CI never infers execution from files present on disk.

usage() {
  echo "usage: test-suite.sh CANDIDATE_DIR" >&2
  exit 2
}

fail() {
  echo "$*" >&2
  exit 1
}

require_result_contract() {
  [[ -n "$result_path" ]] || fail "tests.result_contract.path is required"
  [[ -n "$executed_jq" ]] || fail \
    "tests.result_contract.executed_jq is required"
  [[ -n "$passed_jq" ]] || fail \
    "tests.result_contract.passed_jq is required"
  [[ -n "$failed_jq" ]] || fail \
    "tests.result_contract.failed_jq is required"
}

verify_test_roots() {
  local test_root

  while IFS= read -r test_root; do
    [[ -d "$candidate_dir/$test_root" ]] || fail \
      "required test root is missing: $test_root"
  done < <(jq -r '.tests.roots[]' "$contracts")
}

run_with_timeout() {
  local timeout_seconds="${SULTANC_TEST_TIMEOUT_SECONDS:-1800}"
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

[[ $# -eq 1 ]] || usage
candidate_dir="$1"
script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
contracts="$script_dir/../ci/contracts.json"
summary="${GITHUB_STEP_SUMMARY:-/dev/null}"

command -v jq >/dev/null 2>&1 || fail "jq is required by the CI harness"

state="$(jq -r '.tests.state' "$contracts")"
runner="$(jq -r '.tests.runner // empty' "$contracts")"
reason="$(jq -r '.tests.reason // empty' "$contracts")"

echo "### SultanC test suite" >> "$summary"
if [[ "$state" == "pending" ]]; then
  {
    echo '- Status: `PENDING`'
    echo '- Test suite not executed.'
    printf -- '- Reason: %s\n' "$reason"
  } >> "$summary"
  exit 0
fi

[[ "$state" == "required" ]] || fail "unsupported tests state: $state"
[[ -n "$runner" ]] || fail "tests.runner is required when tests are required"

result_path="$(jq -r '.tests.result_contract.path // empty' "$contracts")"
executed_jq="$(jq -r '.tests.result_contract.executed_jq // empty' "$contracts")"
passed_jq="$(jq -r '.tests.result_contract.passed_jq // empty' "$contracts")"
failed_jq="$(jq -r '.tests.result_contract.failed_jq // empty' "$contracts")"
require_result_contract

candidate_dir="$(cd -- "$candidate_dir" && pwd)"
verify_test_roots

result_file="$candidate_dir/$result_path"
diag="${RUNNER_TEMP:-/tmp}/sultanc-ci/tests"
rm -f -- "$result_file"
rm -rf "$diag"
mkdir -p "$diag"

(
  cd -- "$candidate_dir"
  run_with_timeout
)

[[ -s "$result_file" ]] || fail \
  "required test result file was not produced: $result_path"
cp -- "$result_file" "$diag/result.json"
jq -e . "$result_file" >/dev/null || fail \
  "required test result file is malformed JSON"

executed="$(read_count "$executed_jq" "executed-test")"
passed="$(read_count "$passed_jq" "passed-test")"
failed="$(read_count "$failed_jq" "failed-test")"

((executed > 0)) || fail "required runner reported zero executed tests"
((failed == 0)) || fail "required runner reported $failed failed tests"
((passed <= executed)) || fail "passed count cannot exceed executed count"

{
  printf -- '- Executed: `%d`\n' "$executed"
  printf -- '- Passed: `%d`\n' "$passed"
  printf -- '- Failed: `%d`\n' "$failed"
  echo '- Result: `PASS`'
} >> "$summary"
