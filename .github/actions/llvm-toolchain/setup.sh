#!/usr/bin/env bash
set -euo pipefail

# Install and verify the LLVM development toolchain declared by CI contracts.
# Unpinned or incomplete toolchain contracts remain explicitly unqualified.

usage() {
  echo "usage: setup.sh CONTRACTS_JSON" >&2
  exit 2
}

fail() {
  echo "$*" >&2
  exit 1
}

require_command() {
  command -v "$1" >/dev/null 2>&1 || fail "$1 is required"
}

write_pending_summary() {
  local actual="unavailable"

  if command -v llvm-config >/dev/null 2>&1; then
    actual="$(llvm-config --version)"
  fi

  {
    echo "### LLVM toolchain"
    echo '- Status: `PENDING / UNQUALIFIED`'
    printf -- '- Platform: `%s`\n' "$platform"
    printf -- '- Runner LLVM: `%s`\n' "$actual"
    echo '- No unpinned package installation was performed.'
  } >> "$summary"
}

install_linux_toolchain() {
  local -a packages=()
  local value

  while IFS= read -r value; do
    [[ -n "$value" ]] && packages+=("$value")
  done < <(
    jq -r '.llvm_toolchain.platforms.linux.packages[]?' "$contracts"
  )

  ((${#packages[@]} > 0)) || fail \
    "confirmed Linux LLVM contract requires explicit versioned packages"

  sudo apt-get update
  sudo apt-get install -y "${packages[@]}"
}

install_macos_toolchain() {
  local formula
  local prefix

  formula="$(
    jq -r '.llvm_toolchain.platforms.macos.formula // empty' "$contracts"
  )"
  [[ -n "$formula" ]] || fail \
    "confirmed macOS LLVM contract requires a pinned Homebrew formula"

  if ! brew list "$formula" >/dev/null 2>&1; then
    brew install "$formula"
  fi

  prefix="$(brew --prefix "$formula")"
  echo "$prefix/bin" >> "$GITHUB_PATH"
  export PATH="$prefix/bin:$PATH"
}

resolve_llvm_config() {
  if [[ "$llvm_config_cmd" == /* ]]; then
    printf '%s\n' "$llvm_config_cmd"
  else
    command -v "$llvm_config_cmd" || true
  fi
}

verify_headers() {
  local header

  for header in "${required_headers[@]}"; do
    [[ -f "$include_dir/$header" ]] || fail \
      "required LLVM development header missing: $include_dir/$header"
  done
}

verify_components() {
  local component
  local components

  components=" $($llvm_config --components) "
  for component in "${required_components[@]}"; do
    [[ "$components" == *" $component "* ]] || fail \
      "required LLVM component missing: $component"
  done

  # Listing a component is not enough; prove the development package can link it.
  "$llvm_config" --libs "${required_components[@]}" >/dev/null
  "$llvm_config" --system-libs "${required_components[@]}" >/dev/null
}

verify_targets() {
  local target
  local targets

  targets=" $($llvm_config --targets-built) "
  for target in "${required_targets[@]}"; do
    [[ "$targets" == *" $target "* ]] || fail \
      "required LLVM target family missing: $target"
  done
}

write_confirmed_summary() {
  {
    echo "### LLVM toolchain"
    echo '- Status: `CONFIRMED`'
    printf -- '- Platform: `%s`\n' "$platform"
    printf -- '- Distribution: `%s`\n' "$distribution"
    printf -- '- Version: `%s`\n' "$actual_version"
    printf -- '- llvm-config: `%s`\n' "$llvm_config"
    printf -- '- Include dir: `%s`\n' "$include_dir"
    printf -- '- Library dir: `%s`\n' "$lib_dir"
    printf -- '- Required headers: `%d`\n' "${#required_headers[@]}"
    printf -- '- Required components: `%d`\n' "${#required_components[@]}"
    printf -- '- Required target families: `%d`\n' "${#required_targets[@]}"
  } >> "$summary"
}

[[ $# -eq 1 ]] || usage
contracts="$1"
summary="${GITHUB_STEP_SUMMARY:-/dev/null}"
require_command jq

state="$(jq -r '.llvm_toolchain.state' "$contracts")"
expected_version="$(jq -r '.llvm_toolchain.version // empty' "$contracts")"

case "$(uname -s)" in
  Linux) platform="linux" ;;
  Darwin) platform="macos" ;;
  *) fail "unsupported LLVM CI host: $(uname -s)" ;;
esac

if [[ "$state" == "pending" ]]; then
  write_pending_summary
  exit 0
fi

[[ "$state" == "confirmed" ]] || fail \
  "unsupported llvm_toolchain.state: $state"
[[ -n "$expected_version" ]] || fail \
  "confirmed LLVM toolchain requires version"

distribution="$(
  jq -r \
    --arg platform "$platform" \
    '.llvm_toolchain.platforms[$platform].distribution // empty' \
    "$contracts"
)"
llvm_config_cmd="$(
  jq -r \
    --arg platform "$platform" \
    '.llvm_toolchain.platforms[$platform].llvm_config // empty' \
    "$contracts"
)"

[[ -n "$distribution" && -n "$llvm_config_cmd" ]] || fail \
  "confirmed LLVM toolchain requires platform distribution and llvm-config"

required_headers=()
while IFS= read -r value; do
  [[ -n "$value" ]] && required_headers+=("$value")
done < <(jq -r '.llvm_toolchain.required_headers[]?' "$contracts")

required_components=()
while IFS= read -r value; do
  [[ -n "$value" ]] && required_components+=("$value")
done < <(jq -r '.llvm_toolchain.required_components[]?' "$contracts")

required_targets=()
while IFS= read -r value; do
  [[ -n "$value" ]] && required_targets+=("$value")
done < <(jq -r '.llvm_toolchain.required_targets_built[]?' "$contracts")

((${#required_headers[@]} > 0)) || fail \
  "confirmed LLVM toolchain requires development headers"
((${#required_components[@]} > 0)) || fail \
  "confirmed LLVM toolchain requires linkable components"
((${#required_targets[@]} > 0)) || fail \
  "confirmed LLVM toolchain requires built target families"

case "$platform:$distribution" in
  linux:ubuntu-apt)
    install_linux_toolchain
    ;;
  macos:homebrew)
    install_macos_toolchain
    ;;
  *)
    fail "unsupported LLVM distribution: $platform / $distribution"
    ;;
esac

llvm_config="$(resolve_llvm_config)"
[[ -x "$llvm_config" ]] || fail \
  "configured llvm-config is unavailable: $llvm_config_cmd"

echo "LLVM_CONFIG=$llvm_config" >> "$GITHUB_ENV"

actual_version="$($llvm_config --version)"
[[ "$actual_version" == "$expected_version" ]] || fail \
  "LLVM version mismatch: expected=$expected_version actual=$actual_version"

include_dir="$($llvm_config --includedir)"
lib_dir="$($llvm_config --libdir)"
[[ -d "$include_dir" && -d "$lib_dir" ]] || fail \
  "LLVM development include/lib directories are missing"

verify_headers
verify_components
verify_targets
write_confirmed_summary
