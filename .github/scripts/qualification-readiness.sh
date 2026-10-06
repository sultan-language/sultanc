#!/usr/bin/env bash
set -euo pipefail

# Reduce all external qualification contracts to one promotion-readiness boolean.
# A green workflow with pending contracts is intentionally not promotion-grade.

fail() {
  echo "$*" >&2
  exit 1
}

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
contracts="${1:-$script_dir/../ci/contracts.json}"
targets="${2:-$script_dir/../ci/llvm-targets.json}"
summary="${GITHUB_STEP_SUMMARY:-/dev/null}"

command -v jq >/dev/null 2>&1 || fail "jq is required by the CI harness"

cli_state="$(jq -r '.cli.state' "$contracts")"
build_state="$(jq -r '.build.backend_forwarding.state' "$contracts")"
llvm_state="$(jq -r '.llvm_toolchain.state' "$contracts")"
smoke_state="$(jq -r '.backend_smoke.state' "$contracts")"
examples_state="$(jq -r '.examples.state' "$contracts")"
test_state="$(jq -r '.tests.state' "$contracts")"
required_targets="$(
  jq '[.targets[] | select(.output_contract.state == "required")] | length' \
    "$targets"
)"

ready="true"
[[ "$cli_state" == "delivered" ]] || ready="false"
[[ "$build_state" == "confirmed" ]] || ready="false"
[[ "$llvm_state" == "confirmed" ]] || ready="false"
[[ "$smoke_state" == "required" ]] || ready="false"
[[ "$examples_state" == "required" ]] || ready="false"
[[ "$test_state" == "required" ]] || ready="false"
[[ "$required_targets" -eq 34 ]] || ready="false"

if [[ -n "${GITHUB_OUTPUT:-}" ]]; then
  printf 'ready=%s\n' "$ready" >> "$GITHUB_OUTPUT"
fi

{
  echo "### Qualification contract readiness"
  printf -- '- CLI flags: `%s`\n' "$cli_state"
  printf -- '- build.sh backend forwarding: `%s`\n' "$build_state"
  printf -- '- Pinned LLVM toolchain: `%s`\n' "$llvm_state"
  printf -- '- Explicit backend smoke: `%s`\n' "$smoke_state"
  printf -- '- Examples: `%s`\n' "$examples_state"
  printf -- '- Test suite: `%s`\n' "$test_state"
  printf -- '- LLVM output contracts: `%s / 34 required`\n' "$required_targets"
  printf -- '- Promotion-grade readiness: `%s`\n' "$ready"
} >> "$summary"
