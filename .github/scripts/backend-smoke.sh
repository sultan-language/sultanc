#!/usr/bin/env bash
set -euo pipefail

# Compile one explicit backend/target pair and validate the emitted artifact.

usage() {
  echo "usage: backend-smoke.sh CANDIDATE_DIR BACKEND TARGET" >&2
  exit 2
}

fail() {
  echo "$*" >&2
  exit 1
}

write_pending_summary() {
  {
    printf '### Explicit backend smoke — `%s` / `%s`\n' "$backend" "$target"
    echo '- Status: `PENDING`'
    printf -- '- Reason: %s\n' "$reason"
  } >> "$summary"
}

[[ $# -eq 3 ]] || usage
candidate_dir="$1"
backend="$2"
target="$3"

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
contracts="$script_dir/../ci/contracts.json"
summary="${GITHUB_STEP_SUMMARY:-/dev/null}"

command -v jq >/dev/null 2>&1 || fail "jq is required by the CI harness"

state="$(jq -r '.backend_smoke.state' "$contracts")"
source_path="$(jq -r '.backend_smoke.source // empty' "$contracts")"
reason="$(jq -r '.backend_smoke.reason // empty' "$contracts")"

if [[ "$state" == "pending" ]]; then
  write_pending_summary
  exit 0
fi

[[ "$state" == "required" ]] || fail \
  "unsupported backend_smoke state: $state"
[[ -n "$source_path" ]] || fail \
  "backend_smoke.source is required"

case "$backend" in
  native|llvm) ;;
  *)
    echo "unsupported backend: $backend" >&2
    exit 2
    ;;
esac

validator="$(
  jq -c \
    --arg target "$target" \
    '.backend_smoke.validators[$target] // empty' \
    "$contracts"
)"
[[ -n "$validator" && "$validator" != "null" ]] || fail \
  "no backend smoke validator for $target"

if [[ "$backend" == "llvm" ]]; then
  llvm_state="$(jq -r '.llvm_toolchain.state' "$contracts")"
  [[ "$llvm_state" == "confirmed" ]] || fail \
    "LLVM backend smoke requires confirmed LLVM toolchain; current=$llvm_state"
fi

candidate_dir="$(cd -- "$candidate_dir" && pwd)"
[[ -x "$candidate_dir/build/sultanc" ]] || fail "compiler missing"
[[ -f "$candidate_dir/$source_path" ]] || fail \
  "smoke source missing: $source_path"

diag="${RUNNER_TEMP:-/tmp}/sultanc-ci/backend-smoke/$backend-$target"
rm -rf "$diag"
mkdir -p "$diag"
out="$diag/$backend-$target.o"

set +e
(
  cd -- "$candidate_dir"
  ./build/sultanc \
    -c \
    -b "$backend" \
    -t "$target" \
    "$source_path" \
    -o "$out"
) >"$diag/compile.stdout" 2>"$diag/compile.stderr"
compile_rc=$?
set -e

if ((compile_rc != 0)); then
  echo "backend smoke compile failed with exit $compile_rc" >&2
  exit "$compile_rc"
fi

set +e
evidence="$(
  "$script_dir/validate-output.sh" \
    "$out" \
    "$validator" \
    2>"$diag/validation.stderr"
)"
validation_rc=$?
set -e
printf '%s\n' "$evidence" > "$diag/validation.stdout"

if ((validation_rc != 0)); then
  cat "$diag/validation.stderr" >&2
  exit "$validation_rc"
fi

printf '%s\n' "$evidence"
{
  printf '### Explicit backend smoke — `%s` / `%s`\n' "$backend" "$target"
  printf -- '- Source: `%s`\n' "$source_path"
  printf -- '- Evidence: `%s`\n' "$evidence"
  echo '- Result: `PASS`'
} >> "$summary"
