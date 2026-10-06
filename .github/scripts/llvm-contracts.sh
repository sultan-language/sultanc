#!/usr/bin/env bash
set -euo pipefail

# Validate the exact 34-target LLVM inventory and every activated output contract.

fail() {
  echo "$*" >&2
  exit 1
}

validate_inventory() {
  local count
  local unique_count
  local expected_count
  local actual_ids
  local expected_ids

  count="$(jq '.targets | length' "$targets_file")"
  unique_count="$(jq '[.targets[].id] | unique | length' "$targets_file")"
  expected_count="$(jq '.expected_count' "$targets_file")"

  if [[ "$count" -ne "$expected_count" || \
        "$unique_count" -ne "$expected_count" ]]; then
    fail \
      "LLVM inventory mismatch: count=$count unique=$unique_count "\
      "expected=$expected_count"
  fi

  actual_ids="$(jq -r '.targets[].id' "$targets_file" | LC_ALL=C sort)"
  expected_ids="$(cat <<'EOF_TARGETS' | LC_ALL=C sort
arm64-darwin
x86_64-linux
aarch64-linux-gnu
aarch64-windows-msvc
aarch64-android
aarch64-freebsd
x86_64-darwin
x86_64-windows-msvc
x86_64-android
x86_64-freebsd
i686-linux-gnu
armv7-linux-gnueabihf
armv7-android
riscv64-linux-gnu
riscv64-freebsd
riscv32-unknown-elf
wasm32-wasi
wasm32-browser
wasm64-unknown-unknown
amdgcn-amd-amdhsa
avr-unknown-unknown
bpfel-unknown-none
hexagon-unknown-elf
lanai-unknown-unknown
loongarch64-linux-gnu
mips64el-linux-gnuabi64
msp430-unknown-elf
nvptx64-nvidia-cuda
powerpc64le-linux-gnu
sparcv9-linux-gnu
spirv64-unknown-unknown
s390x-linux-gnu
ve-linux-gnu
xcore-unknown-unknown
EOF_TARGETS
)"

  [[ "$actual_ids" == "$expected_ids" ]] || fail \
    "LLVM target identifiers differ from the agreed 34-target inventory"
}

validate_validator() {
  local target="$1"
  local kind="$2"
  local validator_type="$3"
  local validator

  case "$kind:$validator_type" in
    obj:elf|obj:macho|obj:coff|obj:wasm) ;;
    asm:assembly|asm:text-regex) ;;
    llvm-ir:llvm-ir|llvm-ir:text-regex) ;;
    *)
      fail \
        "unsupported LLVM output contract for $target: "\
        "$kind / $validator_type"
      ;;
  esac

  validator="$(
    jq -c \
      --arg target "$target" \
      '.targets[] | select(.id == $target) | .output_contract.validator' \
      "$targets_file"
  )"

  case "$validator_type" in
    elf)
      jq -e '
        (.file_regex | type == "string" and length > 0) and
        (.class | type == "string" and length > 0) and
        (.machine_regex | type == "string" and length > 0)
      ' <<< "$validator" >/dev/null || fail \
        "incomplete ELF validator for $target"
      ;;
    macho|coff|wasm)
      jq -e '
        .header_regex | type == "string" and length > 0
      ' <<< "$validator" >/dev/null || fail \
        "incomplete $validator_type validator for $target"
      ;;
    assembly|llvm-ir|text-regex)
      jq -e '
        .expected_regex | type == "string" and length > 0
      ' <<< "$validator" >/dev/null || fail \
        "incomplete text validator for $target"
      ;;
  esac
}

write_summary() {
  local expected_count="$1"

  {
    echo "### LLVM target coverage contracts"
    echo
    echo '| Target | Family | Shard | Output contract |'
    echo '|---|---|---:|---|'
    jq -r '
      .targets[] |
      "| `\(.id)` | \(.family) | \(.shard) | " +
      "`\(.output_contract.state)` |"
    ' "$targets_file"
    echo
    printf -- '- Required: `%s / %s`\n' "$required" "$expected_count"
    printf -- '- Pending: `%s / %s`\n' "$pending" "$expected_count"
    if [[ "$complete" == "true" ]]; then
      echo '- Coverage readiness: `READY`'
    else
      echo '- Coverage readiness: `PENDING`'
    fi
  } >> "$summary"
}

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
targets_file="$script_dir/../ci/llvm-targets.json"
contracts_file="$script_dir/../ci/contracts.json"
summary="${GITHUB_STEP_SUMMARY:-/dev/null}"
command -v jq >/dev/null 2>&1 || fail "jq is required by the CI harness"

validate_inventory
expected_count="$(jq '.expected_count' "$targets_file")"
pending="$(
  jq '[.targets[] | select(.output_contract.state == "pending")] | length' \
    "$targets_file"
)"
required="$(
  jq '[.targets[] | select(.output_contract.state == "required")] | length' \
    "$targets_file"
)"
invalid="$(
  jq '[
    .targets[] |
    select(
      .output_contract.state != "pending" and
      .output_contract.state != "required"
    )
  ] | length' "$targets_file"
)"
[[ "$invalid" -eq 0 ]] || fail "unsupported output_contract.state"

# Activated targets must specify source, output kind, and non-executable validation.
incomplete="$(
  jq '[
    .targets[] |
    select(.output_contract.state == "required") |
    select(
      (.output_contract.kind // "") == "" or
      (.output_contract.source // "") == "" or
      (.output_contract.validator // null) == null
    )
  ] | length' "$targets_file"
)"
[[ "$incomplete" -eq 0 ]] || fail \
  "required LLVM entries contain incomplete source/kind/validator contracts"

required_rows_filter='
  .targets[] |
  select(.output_contract.state == "required") |
  [
    .id,
    .output_contract.kind,
    .output_contract.validator.type
  ] |
  @tsv
'
while IFS=$'\t' read -r target kind validator_type; do
  [[ -n "$target" ]] || continue
  validate_validator "$target" "$kind" "$validator_type"
done < <(jq -r "$required_rows_filter" "$targets_file")

shards="$(
  jq -c '[
    .targets[] |
    select(.output_contract.state == "required") |
    .shard
  ] | unique' "$targets_file"
)"
active="false"
complete="false"

if ((required > 0)); then
  llvm_state="$(jq -r '.llvm_toolchain.state' "$contracts_file")"
  [[ "$llvm_state" == "confirmed" ]] || fail \
    "LLVM coverage requires confirmed toolchain; current=$llvm_state"
  active="true"
else
  shards='[0,1,2,3]'
fi

((required == expected_count)) && complete="true"

if [[ -n "${GITHUB_OUTPUT:-}" ]]; then
  {
    printf 'active=%s\n' "$active"
    printf 'complete=%s\n' "$complete"
    printf 'shards=%s\n' "$shards"
  } >> "$GITHUB_OUTPUT"
fi

write_summary "$expected_count"
