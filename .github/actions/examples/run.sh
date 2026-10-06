#!/usr/bin/env bash
set -euo pipefail

# Run only approved deterministic example pairs with explicit expectations.

fail() {
  echo "$*" >&2
  exit 1
}

run_program() {
  local command_path="$1"
  local stdout_path="$2"
  local stderr_path="$3"
  shift 3

  set +e
  "$command_path" "$@" >"$stdout_path" 2>"$stderr_path"
  local rc=$?
  set -e
  printf '%s\n' "$rc"
}

compiler="${SULTANC_COMPILER:?compiler is required}"
working_directory="${SULTANC_WORKING_DIRECTORY:?working directory is required}"
contracts="$GITHUB_ACTION_PATH/../../ci/contracts.json"
summary="${GITHUB_STEP_SUMMARY:-/dev/null}"

state="$(jq -r '.examples.state' "$contracts")"
reason="$(jq -r '.examples.reason // empty' "$contracts")"
echo "### SultanC examples" >> "$summary"

if [[ "$state" == "pending" ]]; then
  {
    echo '- Status: `PENDING`'
    echo '- Examples not executed as qualification evidence.'
    printf -- '- Reason: %s\n' "$reason"
  } >> "$summary"
  exit 0
fi

[[ "$state" == "required" ]] || fail "unsupported examples state: $state"
manifest="$(jq -r '.examples.manifest // empty' "$contracts")"
[[ -n "$manifest" ]] || fail "examples.manifest is required"

cd -- "$working_directory"
[[ -x "$compiler" ]] || fail "compiler is not executable: $compiler"
[[ -s "$manifest" ]] || fail "example manifest is missing or empty: $manifest"

count="$(jq 'length' "$manifest")"
((count > 0)) || fail "approved example manifest is empty"

diag="${RUNNER_TEMP:-/tmp}/sultanc-ci/examples"
rm -rf "$diag"
mkdir -p "$diag"

for ((index = 0; index < count; index++)); do
  english="$(jq -r ".[$index].en" "$manifest")"
  arabic="$(jq -r ".[$index].ar" "$manifest")"
  expected_exit="$(jq -r ".[$index].exit" "$manifest")"
  stdout_file="$(jq -r ".[$index].stdout // empty" "$manifest")"
  stderr_file="$(jq -r ".[$index].stderr // empty" "$manifest")"

  [[ -f "$english" && -f "$arabic" ]] || fail \
    "example manifest entry $index references missing source"
  [[ "$expected_exit" =~ ^[0-9]+$ ]] || fail \
    "example manifest entry $index has invalid exit code"

  pair="$diag/$index"
  mkdir -p "$pair"

  "$compiler" "$english" -o "$pair/en-native" \
    >"$pair/en-compile.out" 2>"$pair/en-compile.err"
  "$compiler" "$arabic" -o "$pair/ar-native" \
    >"$pair/ar-compile.out" 2>"$pair/ar-compile.err"

  en_native="$(
    run_program "$pair/en-native" "$pair/en.out" "$pair/en.err"
  )"
  en_run="$(
    run_program "$compiler" "$pair/en-run.out" "$pair/en-run.err" \
      run "$english"
  )"
  ar_native="$(
    run_program "$pair/ar-native" "$pair/ar.out" "$pair/ar.err"
  )"
  ar_run="$(
    run_program "$compiler" "$pair/ar-run.out" "$pair/ar-run.err" \
      run "$arabic"
  )"

  [[ "$en_native" -eq "$expected_exit" ]] || fail \
    "example $index English native exit mismatch"
  [[ "$en_run" -eq "$expected_exit" ]] || fail \
    "example $index English interpreter exit mismatch"
  [[ "$ar_native" -eq "$expected_exit" ]] || fail \
    "example $index Arabic native exit mismatch"
  [[ "$ar_run" -eq "$expected_exit" ]] || fail \
    "example $index Arabic interpreter exit mismatch"

  cmp "$pair/en.out" "$pair/en-run.out"
  cmp "$pair/ar.out" "$pair/ar-run.out"
  cmp "$pair/en.out" "$pair/ar.out"
  cmp "$pair/en.err" "$pair/en-run.err"
  cmp "$pair/ar.err" "$pair/ar-run.err"
  cmp "$pair/en.err" "$pair/ar.err"

  [[ -z "$stdout_file" ]] || cmp "$pair/en.out" "$stdout_file"
  [[ -z "$stderr_file" ]] || cmp "$pair/en.err" "$stderr_file"
done

{
  printf -- '- Approved deterministic pairs: `%d`\n' "$count"
  echo '- Expected results + native/run + bilingual parity: `PASS`'
} >> "$summary"
