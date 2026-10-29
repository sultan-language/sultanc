#!/usr/bin/env bash
set -euo pipefail

# Record the compiler-advertised target inventory for one broad LLVM shard.

: "${CANDIDATE_DIR:=candidate}"
: "${SHARD:?shard is required}"

diag="${RUNNER_TEMP:-/tmp}/sultanc-ci/llvm-shard-$SHARD"
mkdir -p "$diag"
output="$diag/target-inventory.txt"

(
  cd -- "$CANDIDATE_DIR"
  ./build/sultanc targets
) | tee "$output"

{
  echo "### SultanC target inventory"
  echo '```text'
  cat "$output"
  echo '```'
} >> "$GITHUB_STEP_SUMMARY"
