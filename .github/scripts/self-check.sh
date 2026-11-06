#!/usr/bin/env bash
set -euo pipefail

# Check-only self-compile: Stage1 from the wasm seed (built by
# seed-bootstrap.sh) runs `check compiler/main.sn` from the repository root.
# It needs no native backend for the host, so it runs on every host.
#
#   self-check.sh CANDIDATE_DIR

fail() {
  echo "$*" >&2
  exit 1
}

[[ $# -eq 1 ]] || { echo "usage: self-check.sh CANDIDATE_DIR" >&2; exit 2; }
script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
contracts="$script_dir/../ci/contracts.json"
host="$("$script_dir/host-matrix.sh" "$contracts" --current)"
suffix="$(jq -r '.executable_suffix' <<< "$host")"
stage1="$(jq -r '.seed_bootstrap.output_root' "$contracts")/stage1$suffix"
mapfile -t arguments < <(jq -r '.seed_bootstrap.self_check_arguments[]' "$contracts")
diag="${RUNNER_TEMP:-/tmp}/sultanc-ci/self-check"
mkdir -p "$diag"

cd -- "$1"
[[ -x "$stage1" ]] || fail "Stage1 from the seed is missing: $stage1 (run the seed check first)"

start="$(date +%s)"
set +e
"$stage1" "${arguments[@]}" >"$diag/check.stdout" 2>"$diag/check.stderr"
rc=$?
set -e
elapsed=$(( $(date +%s) - start ))
cat "$diag/check.stdout" "$diag/check.stderr"

{
  echo "### Check-only self-compile"
  printf -- '- Command: `%s %s`\n' "$stage1" "${arguments[*]}"
  printf -- '- Exit: `%s` in %ss\n' "$rc" "$elapsed"
} >> "${GITHUB_STEP_SUMMARY:-/dev/null}"
((rc == 0)) || fail "Stage1 check of compiler/main.sn failed with exit $rc"
