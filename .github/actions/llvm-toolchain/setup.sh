#!/usr/bin/env bash
set -euo pipefail

# Resolve, provision, and verify the pinned LLVM development bundle used by CI.
# Package managers are intentionally not used in normal qualification runs.

usage() {
  echo "usage: setup.sh resolve CONTRACTS_JSON" >&2
  echo "       setup.sh activate CONTRACTS_JSON CACHE_HIT" >&2
  exit 2
}

fail() {
  echo "$*" >&2
  exit 1
}

require_command() {
  command -v "$1" >/dev/null 2>&1 || fail "$1 is required"
}

host_platform() {
  case "$(uname -s)" in
    Linux) printf '%s\n' linux ;;
    Darwin) printf '%s\n' macos ;;
    *) fail "unsupported LLVM CI host: $(uname -s)" ;;
  esac
}

host_architecture() {
  case "$(uname -m)" in
    x86_64|amd64) printf '%s\n' x86_64 ;;
    arm64|aarch64) printf '%s\n' arm64 ;;
    *) fail "unsupported LLVM CI architecture: $(uname -m)" ;;
  esac
}

sha256_file() {
  local file="$1"

  if command -v sha256sum >/dev/null 2>&1; then
    sha256sum "$file" | awk '{print $1}'
    return
  fi

  require_command shasum
  shasum -a 256 "$file" | awk '{print $1}'
}

sha256_text() {
  if command -v sha256sum >/dev/null 2>&1; then
    sha256sum | awk '{print $1}'
    return
  fi

  require_command shasum
  shasum -a 256 | awk '{print $1}'
}

load_contract() {
  contracts="$1"
  [[ -f "$contracts" ]] || fail "LLVM contracts file not found: $contracts"

  require_command jq

  state="$(jq -r '.llvm_toolchain.state // empty' "$contracts")"
  version="$(jq -r '.llvm_toolchain.version // empty' "$contracts")"
  provider="$(jq -r '.llvm_toolchain.provider // empty' "$contracts")"
  namespace="$(jq -r '.llvm_toolchain.cache.namespace // empty' "$contracts")"
  cache_schema="$(jq -r '.llvm_toolchain.cache.schema_version // empty' "$contracts")"
  link_libraries="$(jq -r '.llvm_toolchain.link_contract.libraries // empty' "$contracts")"
  shared_runtime="$(jq -r '.llvm_toolchain.link_contract.shared_runtime // false' "$contracts")"

  [[ "$state" == "confirmed" ]] || fail \
    "LLVM toolchain contract must be confirmed before qualification"
  [[ -n "$version" ]] || fail "LLVM toolchain version is required"
  [[ "$provider" == "llvm-project-release" ]] || fail \
    "unsupported LLVM toolchain provider: $provider"
  [[ -n "$namespace" && -n "$cache_schema" ]] || fail \
    "LLVM cache namespace and schema version are required"
  [[ "$link_libraries" == "all" ]] || fail \
    "SultanC Stage0 requires the complete LLVM link set"
  [[ "$shared_runtime" == "true" ]] || fail \
    "SultanC Stage1 requires an LLVM shared runtime"

  platform="$(host_platform)"
  architecture="$(host_architecture)"
  configured_arch="$(
    jq -r --arg platform "$platform" \
      '.llvm_toolchain.platforms[$platform].host_arch // empty' "$contracts"
  )"
  archive="$(
    jq -r --arg platform "$platform" \
      '.llvm_toolchain.platforms[$platform].archive // empty' "$contracts"
  )"
  archive_root="$(
    jq -r --arg platform "$platform" \
      '.llvm_toolchain.platforms[$platform].root // empty' "$contracts"
  )"
  archive_url="$(
    jq -r --arg platform "$platform" \
      '.llvm_toolchain.platforms[$platform].url // empty' "$contracts"
  )"
  archive_sha="$(
    jq -r --arg platform "$platform" \
      '.llvm_toolchain.platforms[$platform].sha256 // empty' "$contracts"
  )"
  llvm_config_relative="$(
    jq -r --arg platform "$platform" \
      '.llvm_toolchain.platforms[$platform].llvm_config // empty' "$contracts"
  )"

  [[ "$configured_arch" == "$architecture" ]] || fail \
    "LLVM host architecture mismatch: expected=$configured_arch actual=$architecture"
  [[ -n "$archive" && -n "$archive_root" && -n "$archive_url" ]] || fail \
    "LLVM release archive contract is incomplete for $platform"
  [[ "$archive_sha" =~ ^[0-9a-f]{64}$ ]] || fail \
    "LLVM release archive requires a lowercase SHA-256 digest"
  [[ -n "$llvm_config_relative" ]] || fail \
    "LLVM release archive requires an llvm-config path"

  contract_hash="$(
    jq -S -c '.llvm_toolchain' "$contracts" | sha256_text
  )"

  tool_root="${RUNNER_TOOL_CACHE:-${RUNNER_TEMP:-/tmp}}"
  toolchain_path="$tool_root/$namespace/$version/$platform-$architecture/$contract_hash"
  cache_key="${namespace}-v${cache_schema}-${version}-${platform}-${architecture}"
  cache_key="${cache_key}-${contract_hash}"
  llvm_config="$toolchain_path/$llvm_config_relative"
}

write_output() {
  local name="$1"
  local value="$2"
  local output_file="${GITHUB_OUTPUT:-}"

  [[ -n "$output_file" ]] || fail "GITHUB_OUTPUT is required in Actions"
  printf '%s=%s\n' "$name" "$value" >> "$output_file"
}

resolve_contract() {
  load_contract "$1"

  write_output toolchain_path "$toolchain_path"
  write_output cache_key "$cache_key"
  write_output version "$version"
  write_output platform "$platform"
  write_output architecture "$architecture"
}

provision_toolchain() {
  local download_dir
  local archive_path
  local extract_dir
  local actual_sha

  require_command curl
  require_command tar

  download_dir="$(mktemp -d "${RUNNER_TEMP:-/tmp}/sultanc-llvm.XXXXXX")"
  archive_path="$download_dir/$archive"
  extract_dir="$download_dir/extract"
  mkdir -p "$extract_dir" "$(dirname -- "$toolchain_path")"

  trap 'rm -rf "$download_dir"' RETURN

  curl --fail --location --silent --show-error --retry 3 --retry-delay 2 \
    --output "$archive_path" "$archive_url"

  actual_sha="$(sha256_file "$archive_path")"
  [[ "$actual_sha" == "$archive_sha" ]] || fail \
    "LLVM archive SHA-256 mismatch: expected=$archive_sha actual=$actual_sha"

  tar -xf "$archive_path" -C "$extract_dir"
  [[ -x "$extract_dir/$archive_root/$llvm_config_relative" ]] || fail \
    "LLVM archive does not contain the configured llvm-config"

  rm -rf "$toolchain_path"
  mv "$extract_dir/$archive_root" "$toolchain_path"
  trap - RETURN
  rm -rf "$download_dir"
}

verify_headers() {
  local include_dir="$1"
  local header

  while IFS= read -r header; do
    [[ -n "$header" ]] || continue
    [[ -f "$include_dir/$header" ]] || fail \
      "required LLVM development header missing: $include_dir/$header"
  done < <(jq -r '.llvm_toolchain.required_headers[]?' "$contracts")
}

verify_targets() {
  local targets
  local target

  targets=" $($llvm_config --targets-built) "
  while IFS= read -r target; do
    [[ -n "$target" ]] || continue
    [[ "$targets" == *" $target "* ]] || fail \
      "required LLVM target family missing: $target"
  done < <(jq -r '.llvm_toolchain.required_targets_built[]?' "$contracts")
}

verify_link_contract() {
  local runtime_names
  local runtime_name
  local runtime_found=false
  local lib_dir="$1"

  "$llvm_config" --ldflags --libs all --system-libs >/dev/null

  runtime_names="$($llvm_config --link-shared --libnames all 2>/dev/null || true)"
  if [[ -z "$runtime_names" ]]; then
    runtime_names="$($llvm_config --libnames all 2>/dev/null || true)"
  fi

  for runtime_name in $runtime_names; do
    runtime_name="$(basename -- "$runtime_name")"
    case "$runtime_name" in
      libLLVM*.dylib|libLLVM*.so|libLLVM*.so.*)
        if [[ -f "$lib_dir/$runtime_name" ]]; then
          runtime_found=true
          break
        fi
        ;;
    esac
  done

  [[ "$runtime_found" == "true" ]] || fail \
    "LLVM shared runtime required by Stage1 was not found in $lib_dir"
}

export_toolchain_environment() {
  local bin_dir="$1"
  local lib_dir="$2"
  local loader_value

  printf '%s\n' "$bin_dir" >> "$GITHUB_PATH"
  printf 'LLVM_CONFIG=%s\n' "$llvm_config" >> "$GITHUB_ENV"

  export PATH="$bin_dir:$PATH"
  export LLVM_CONFIG="$llvm_config"

  if [[ "$platform" == "linux" ]]; then
    loader_value="$lib_dir${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
    printf 'LD_LIBRARY_PATH=%s\n' "$loader_value" >> "$GITHUB_ENV"
    export LD_LIBRARY_PATH="$loader_value"
  else
    loader_value="$lib_dir${DYLD_LIBRARY_PATH:+:$DYLD_LIBRARY_PATH}"
    printf 'DYLD_LIBRARY_PATH=%s\n' "$loader_value" >> "$GITHUB_ENV"
    export DYLD_LIBRARY_PATH="$loader_value"
  fi
}

write_summary() {
  local source="$1"
  local actual_version="$2"
  local include_dir="$3"
  local lib_dir="$4"
  local target_count
  local summary="${GITHUB_STEP_SUMMARY:-/dev/null}"

  target_count="$(jq '.llvm_toolchain.required_targets_built | length' "$contracts")"

  {
    echo "### LLVM toolchain"
    echo '- Status: `CONFIRMED`'
    printf -- '- Source: `%s`\n' "$source"
    printf -- '- Platform: `%s / %s`\n' "$platform" "$architecture"
    printf -- '- Version: `%s`\n' "$actual_version"
    printf -- '- Cache key: `%s`\n' "$cache_key"
    printf -- '- llvm-config: `%s`\n' "$llvm_config"
    printf -- '- Include dir: `%s`\n' "$include_dir"
    printf -- '- Library dir: `%s`\n' "$lib_dir"
    printf -- '- Required LLVM target families: `%s`\n' "$target_count"
  } >> "$summary"
}

activate_toolchain() {
  local cache_hit="$2"
  local source
  local actual_version
  local include_dir
  local lib_dir
  local bin_dir

  load_contract "$1"

  if [[ "$cache_hit" == "true" ]]; then
    [[ -x "$llvm_config" ]] || fail \
      "restored LLVM cache is incomplete: $llvm_config is missing"
    source="Actions cache"
  else
    provision_toolchain
    source="verified llvm-project release archive"
  fi

  actual_version="$($llvm_config --version)"
  [[ "$actual_version" == "$version" ]] || fail \
    "LLVM version mismatch: expected=$version actual=$actual_version"

  include_dir="$($llvm_config --includedir)"
  lib_dir="$($llvm_config --libdir)"
  bin_dir="$($llvm_config --bindir)"
  [[ -d "$include_dir" && -d "$lib_dir" && -d "$bin_dir" ]] || fail \
    "LLVM development include/lib/bin directories are incomplete"

  verify_headers "$include_dir"
  verify_targets
  verify_link_contract "$lib_dir"
  export_toolchain_environment "$bin_dir" "$lib_dir"
  write_summary "$source" "$actual_version" "$include_dir" "$lib_dir"
}

[[ $# -ge 2 ]] || usage
command_name="$1"
shift

case "$command_name" in
  resolve)
    [[ $# -eq 1 ]] || usage
    resolve_contract "$1"
    ;;
  activate)
    [[ $# -eq 2 ]] || usage
    activate_toolchain "$1" "$2"
    ;;
  *)
    usage
    ;;
esac
