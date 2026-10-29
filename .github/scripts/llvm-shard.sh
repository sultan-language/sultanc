#!/usr/bin/env bash
set -euo pipefail

# Generate and validate every LLVM target assigned to one shard.
# Independent target failures are collected so one target cannot hide later results.

usage() {
  echo "usage: llvm-shard.sh CANDIDATE_DIR SHARD" >&2
  exit 2
}

fail() {
  echo "$*" >&2
  exit 1
}

record_result() {
  local target="$1"
  local state="$2"
  local detail="$3"

  printf '%s\t%s\t%s\n' "$target" "$state" "$detail" >> "$results_file"
}

mark_failure() {
  local target="$1"
  local detail="$2"

  printf -- '- `%s`: `FAIL` — %s\n' "$target" "$detail" >> "$summary"
  record_result "$target" "fail" "$detail"
  failures=$((failures + 1))
}

generate_output() {
  local kind="$1"
  local target="$2"
  local source_path="$3"
  local output="$4"
  local target_diag="$5"

  case "$kind" in
    obj)
      (
        cd -- "$candidate_dir"
        ./build/sultanc \
          -c \
          -b llvm \
          -t "$target" \
          "$source_path" \
          -o "$output"
      ) >"$target_diag/generate.stdout" 2>"$target_diag/generate.stderr"
      ;;
    asm|llvm-ir)
      (
        cd -- "$candidate_dir"
        ./build/sultanc \
          -b llvm \
          -t "$target" \
          --emit="$kind" \
          "$source_path" \
          -o "$output"
      ) >"$target_diag/generate.stdout" 2>"$target_diag/generate.stderr"
      ;;
    *)
      echo "unsupported output kind: $kind" > "$target_diag/generate.stderr"
      return 97
      ;;
  esac
}

[[ $# -eq 2 ]] || usage
candidate_dir="$1"
shard="$2"
script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
targets_file="$script_dir/../ci/llvm-targets.json"
summary="${GITHUB_STEP_SUMMARY:-/dev/null}"

command -v jq >/dev/null 2>&1 || fail "jq is required by the CI harness"
candidate_dir="$(cd -- "$candidate_dir" && pwd)"
[[ -x "$candidate_dir/build/sultanc" ]] || fail "host compiler is missing"

diag="${RUNNER_TEMP:-/tmp}/sultanc-ci/llvm-shard-$shard"
rm -rf "$diag"
mkdir -p "$diag"
results_file="$diag/results.tsv"
printf 'target\tstate\tdetail\n' > "$results_file"
printf '### LLVM target shard `%s`\n\n' "$shard" >> "$summary"

mapfile -t target_ids < <(
  jq -r \
    --argjson shard "$shard" \
    '.targets[] | select(.shard == $shard) | .id' \
    "$targets_file"
)
((${#target_ids[@]} > 0)) || fail \
  "no LLVM targets assigned to shard $shard"

failures=0
for target in "${target_ids[@]}"; do
  target_diag="$diag/${target//\//_}"
  mkdir -p "$target_diag"

  state="$(
    jq -r \
      --arg target "$target" \
      '.targets[] | select(.id == $target) | .output_contract.state' \
      "$targets_file"
  )"

  if [[ "$state" == "pending" ]]; then
    printf -- '- `%s`: `PENDING` output contract\n' "$target" >> "$summary"
    record_result "$target" "pending" "output contract"
    continue
  fi

  if [[ "$state" != "required" ]]; then
    mark_failure "$target" "unsupported contract state $state"
    continue
  fi

  kind="$(
    jq -r \
      --arg target "$target" \
      '.targets[] | select(.id == $target) | .output_contract.kind // empty' \
      "$targets_file"
  )"
  source_path="$(
    jq -r \
      --arg target "$target" \
      '.targets[] | select(.id == $target) | .output_contract.source // empty' \
      "$targets_file"
  )"
  validator="$(
    jq -c \
      --arg target "$target" \
      '.targets[] | select(.id == $target) | .output_contract.validator // empty' \
      "$targets_file"
  )"

  if [[ -z "$kind" || -z "$source_path" || \
        -z "$validator" || "$validator" == "null" ]]; then
    mark_failure "$target" "incomplete kind/source/validator contract"
    continue
  fi

  if [[ ! -f "$candidate_dir/$source_path" ]]; then
    mark_failure "$target" "source missing: $source_path"
    continue
  fi

  output="$target_diag/output.${kind//[^A-Za-z0-9]/_}"
  set +e
  generate_output "$kind" "$target" "$source_path" "$output" "$target_diag"
  generate_rc=$?
  set -e

  if ((generate_rc != 0)); then
    mark_failure "$target" "generation exit $generate_rc"
    continue
  fi

  set +e
  evidence="$(
    "$script_dir/validate-output.sh" \
      "$output" \
      "$validator" \
      2>"$target_diag/validation.stderr"
  )"
  validation_rc=$?
  set -e
  printf '%s\n' "$evidence" > "$target_diag/validation.stdout"

  if ((validation_rc != 0)); then
    detail="$(
      tr '\n' ' ' < "$target_diag/validation.stderr" | cut -c1-300
    )"
    mark_failure "$target" "validation: $detail"
    continue
  fi

  printf -- '- `%s`: `PASS` — %s\n' "$target" "$evidence" >> "$summary"
  record_result "$target" "pass" "$evidence"
done

if ((failures > 0)); then
  fail "$failures LLVM target(s) failed in shard $shard"
fi
