#!/usr/bin/env bash
set -euo pipefail

# Resolve, provision, verify, and activate SultanC's pinned LLVM CI toolchain.
# Normal qualification only restores a trusted cache entry; it never provisions LLVM.

usage() {
  echo "usage: setup.sh resolve CONTRACTS_JSON" >&2
  echo "       setup.sh activate CONTRACTS_JSON CACHE_HIT" >&2
  echo "       setup.sh provision CONTRACTS_JSON" >&2
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
  source_archive="$(jq -r '.llvm_toolchain.source.archive // empty' "$contracts")"
  source_root="$(jq -r '.llvm_toolchain.source.root // empty' "$contracts")"
  source_url="$(jq -r '.llvm_toolchain.source.url // empty' "$contracts")"
  source_sha="$(jq -r '.llvm_toolchain.source.sha256 // empty' "$contracts")"
  build_type="$(jq -r '.llvm_toolchain.build.type // empty' "$contracts")"
  parallel_jobs="$(jq -r '.llvm_toolchain.build.parallel_jobs // empty' "$contracts")"
  build_dylib="$(jq -r '.llvm_toolchain.build.llvm_build_llvm_dylib // false' "$contracts")"
  link_dylib="$(jq -r '.llvm_toolchain.build.llvm_link_llvm_dylib // false' "$contracts")"
  dylib_components="$(
    jq -r '.llvm_toolchain.build.llvm_dylib_components // empty' "$contracts"
  )"
  link_libraries="$(jq -r '.llvm_toolchain.link_contract.libraries // empty' "$contracts")"
  shared_runtime="$(jq -r '.llvm_toolchain.link_contract.shared_runtime // false' "$contracts")"

  [[ "$state" == "confirmed" ]] || fail \
    "LLVM toolchain contract must be confirmed before qualification"
  [[ "$version" == "22.1.8" ]] || fail \
    "LLVM toolchain version must remain pinned to 22.1.8"
  [[ "$provider" == "sultanc-actions-cache" ]] || fail \
    "unsupported LLVM toolchain provider: $provider"
  [[ -n "$namespace" && -n "$cache_schema" ]] || fail \
    "LLVM cache namespace and schema version are required"
  [[ -n "$source_archive" && -n "$source_root" && -n "$source_url" ]] || fail \
    "LLVM source archive contract is incomplete"
  [[ "$source_sha" =~ ^[0-9a-f]{64}$ ]] || fail \
    "LLVM source archive requires a lowercase SHA-256 digest"
  [[ "$build_type" == "Release" ]] || fail \
    "LLVM provisioning build type must be Release"
  [[ "$parallel_jobs" =~ ^[1-9][0-9]*$ ]] || fail \
    "LLVM provisioning parallel_jobs must be a positive integer"
  [[ "$build_dylib" == "true" && "$link_dylib" == "true" ]] || fail \
    "LLVM provisioning must build and link the monolithic shared library"
  [[ "$dylib_components" == "all" ]] || fail \
    "LLVM shared library must contain all required components"
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
  llvm_config_relative="$(
    jq -r --arg platform "$platform" \
      '.llvm_toolchain.platforms[$platform].llvm_config // empty' "$contracts"
  )"
  runtime_relative="$(
    jq -r --arg platform "$platform" \
      '.llvm_toolchain.platforms[$platform].shared_runtime // empty' "$contracts"
  )"

  [[ "$configured_arch" == "$architecture" ]] || fail \
    "LLVM host architecture mismatch: expected=$configured_arch actual=$architecture"
  [[ -n "$llvm_config_relative" && -n "$runtime_relative" ]] || fail \
    "LLVM platform contract is incomplete for $platform"

  contract_hash="$(
    jq -S -c '.llvm_toolchain' "$contracts" | sha256_text
  )"

  tool_root="${RUNNER_TOOL_CACHE:-${RUNNER_TEMP:-/tmp}}"
  toolchain_path="$tool_root/$namespace/$version/$platform-$architecture/$contract_hash"
  cache_key="${namespace}-v${cache_schema}-${version}-${platform}-${architecture}"
  cache_key="${cache_key}-${contract_hash}"
  llvm_config="$toolchain_path/$llvm_config_relative"
  manifest="$toolchain_path/.sultanc-llvm-manifest.json"
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

prepare_runtime_environment() {
  local bin_dir="$toolchain_path/bin"
  local lib_dir="$toolchain_path/lib"

  if [[ ":$PATH:" != *":$bin_dir:"* ]]; then
    export PATH="$bin_dir:$PATH"
  fi
  export LLVM_CONFIG="$llvm_config"

  if [[ "$platform" == "linux" ]]; then
    if [[ ":${LD_LIBRARY_PATH:-}:" != *":$lib_dir:"* ]]; then
      export LD_LIBRARY_PATH="$lib_dir${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
    fi
  elif [[ ":${DYLD_LIBRARY_PATH:-}:" != *":$lib_dir:"* ]]; then
    export DYLD_LIBRARY_PATH="$lib_dir${DYLD_LIBRARY_PATH:+:$DYLD_LIBRARY_PATH}"
  fi
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
  local lib_dir="$1"
  local runtime_path="$toolchain_path/$runtime_relative"
  local shared_mode
  local runtime_files
  local runtime_file
  local monolithic_runtime=false

  [[ -e "$runtime_path" ]] || fail \
    "LLVM shared runtime required by Stage1 is missing: $runtime_path"

  "$llvm_config" --ldflags --libs all --system-libs >/dev/null
  shared_mode="$($llvm_config --shared-mode all)"
  [[ "$shared_mode" == "shared" ]] || fail \
    "llvm-config does not select the shared LLVM link contract"

  runtime_files="$($llvm_config --link-shared --libfiles all)"
  for runtime_file in $runtime_files; do
    case "$(basename -- "$runtime_file")" in
      libLLVM*.dylib|libLLVM*.so|libLLVM*.so.*)
        [[ -e "$runtime_file" ]] || continue
        monolithic_runtime=true
        break
        ;;
    esac
  done

  [[ "$monolithic_runtime" == "true" ]] || fail \
    "llvm-config does not expose an installed monolithic shared LLVM runtime"
  [[ -d "$lib_dir" ]] || fail "LLVM library directory is missing: $lib_dir"
}

ensure_runtime_alias() {
  local runtime_path="$toolchain_path/$runtime_relative"
  local runtime_file
  local runtime_files

  [[ -e "$runtime_path" ]] && return

  prepare_runtime_environment
  runtime_files="$($llvm_config --link-shared --libfiles all)"
  for runtime_file in $runtime_files; do
    case "$(basename -- "$runtime_file")" in
      libLLVM*.dylib|libLLVM*.so|libLLVM*.so.*)
        [[ -e "$runtime_file" ]] || continue
        [[ "$(dirname -- "$runtime_file")" == "$(dirname -- "$runtime_path")" ]] || \
          continue
        ln -s "$(basename -- "$runtime_file")" "$runtime_path"
        return
        ;;
    esac
  done

  fail "provisioned LLVM does not contain a monolithic shared runtime"
}

toolchain_payload_checksum() {
  local entry
  local relative
  local digest

  (
    while IFS= read -r entry; do
      relative="${entry#"$toolchain_path"/}"
      [[ "$entry" != "$manifest" ]] || continue

      if [[ -L "$entry" ]]; then
        printf 'L\t%s\t%s\n' "$relative" "$(readlink "$entry")"
      else
        digest="$(sha256_file "$entry")"
        printf 'F\t%s\t%s\n' "$relative" "$digest"
      fi
    done < <(
      find "$toolchain_path" \( -type f -o -type l \) -print | LC_ALL=C sort
    )
  ) | sha256_text
}

write_manifest() {
  local payload_sha
  local temporary

  payload_sha="$(toolchain_payload_checksum)"
  temporary="$manifest.tmp"

  jq -n \
    --arg version "$version" \
    --arg platform "$platform" \
    --arg architecture "$architecture" \
    --arg contract_hash "$contract_hash" \
    --arg cache_key "$cache_key" \
    --arg payload_sha256 "$payload_sha" \
    '{
      schema: 1,
      version: $version,
      platform: $platform,
      architecture: $architecture,
      contract_hash: $contract_hash,
      cache_key: $cache_key,
      payload_sha256: $payload_sha256
    }' > "$temporary"
  mv "$temporary" "$manifest"
}

verify_manifest() {
  local expected_payload
  local actual_payload

  [[ -f "$manifest" ]] || fail \
    "restored LLVM cache has no SultanC toolchain manifest: $manifest"

  [[ "$(jq -r '.schema // empty' "$manifest")" == "1" ]] || fail \
    "unsupported SultanC LLVM toolchain manifest schema"
  [[ "$(jq -r '.version // empty' "$manifest")" == "$version" ]] || fail \
    "cached LLVM manifest version does not match the contract"
  [[ "$(jq -r '.platform // empty' "$manifest")" == "$platform" ]] || fail \
    "cached LLVM manifest platform does not match the runner"
  [[ "$(jq -r '.architecture // empty' "$manifest")" == "$architecture" ]] || fail \
    "cached LLVM manifest architecture does not match the runner"
  [[ "$(jq -r '.contract_hash // empty' "$manifest")" == "$contract_hash" ]] || fail \
    "cached LLVM manifest contract hash does not match"
  [[ "$(jq -r '.cache_key // empty' "$manifest")" == "$cache_key" ]] || fail \
    "cached LLVM manifest cache key does not match"

  expected_payload="$(jq -r '.payload_sha256 // empty' "$manifest")"
  [[ "$expected_payload" =~ ^[0-9a-f]{64}$ ]] || fail \
    "cached LLVM manifest payload checksum is invalid"

  actual_payload="$(toolchain_payload_checksum)"
  [[ "$actual_payload" == "$expected_payload" ]] || fail \
    "cached LLVM toolchain checksum does not match its provisioning manifest"
}

verify_development_contract() {
  local actual_version
  local include_dir
  local lib_dir
  local bin_dir

  [[ -x "$llvm_config" ]] || fail \
    "LLVM toolchain is incomplete: $llvm_config is missing"

  prepare_runtime_environment
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
}

export_toolchain_environment() {
  local bin_dir="$toolchain_path/bin"

  prepare_runtime_environment
  printf '%s\n' "$bin_dir" >> "$GITHUB_PATH"
  printf 'LLVM_CONFIG=%s\n' "$llvm_config" >> "$GITHUB_ENV"

  if [[ "$platform" == "linux" ]]; then
    printf 'LD_LIBRARY_PATH=%s\n' "$LD_LIBRARY_PATH" >> "$GITHUB_ENV"
  else
    printf 'DYLD_LIBRARY_PATH=%s\n' "$DYLD_LIBRARY_PATH" >> "$GITHUB_ENV"
  fi
}

write_summary() {
  local source="$1"
  local target_count
  local summary="${GITHUB_STEP_SUMMARY:-/dev/null}"

  target_count="$(jq '.llvm_toolchain.required_targets_built | length' "$contracts")"

  {
    echo "### LLVM toolchain"
    echo '- Status: `CONFIRMED`'
    printf -- '- Source: `%s`\n' "$source"
    printf -- '- Platform: `%s / %s`\n' "$platform" "$architecture"
    printf -- '- Version: `%s`\n' "$version"
    printf -- '- Cache key: `%s`\n' "$cache_key"
    printf -- '- llvm-config: `%s`\n' "$llvm_config"
    printf -- '- Shared runtime: `%s`\n' "$runtime_relative"
    printf -- '- Required LLVM target families: `%s`\n' "$target_count"
  } >> "$summary"
}

provision_toolchain() {
  local download_dir
  local archive_path
  local extract_dir
  local build_dir
  local actual_sha
  local targets

  require_command curl
  require_command tar
  require_command cmake
  require_command ninja

  download_dir="$(mktemp -d "${RUNNER_TEMP:-/tmp}/sultanc-llvm-source.XXXXXX")"
  archive_path="$download_dir/$source_archive"
  extract_dir="$download_dir/source"
  build_dir="$download_dir/build"
  mkdir -p "$extract_dir" "$build_dir" "$(dirname -- "$toolchain_path")"

  curl --fail --location --silent --show-error --retry 3 --retry-delay 2 \
    --output "$archive_path" "$source_url"

  actual_sha="$(sha256_file "$archive_path")"
  [[ "$actual_sha" == "$source_sha" ]] || {
    rm -rf "$download_dir"
    fail "LLVM source SHA-256 mismatch: expected=$source_sha actual=$actual_sha"
  }

  tar -xf "$archive_path" -C "$extract_dir"
  [[ -d "$extract_dir/$source_root/llvm" ]] || {
    rm -rf "$download_dir"
    fail "LLVM source archive root is missing: $source_root"
  }

  targets="$(
    jq -r '.llvm_toolchain.required_targets_built[]' "$contracts" | paste -sd ';' -
  )"

  rm -rf "$toolchain_path"
  cmake \
    -S "$extract_dir/$source_root/llvm" \
    -B "$build_dir" \
    -G Ninja \
    -DCMAKE_BUILD_TYPE="$build_type" \
    -DCMAKE_INSTALL_PREFIX="$toolchain_path" \
    -DLLVM_TARGETS_TO_BUILD="$targets" \
    -DLLVM_BUILD_LLVM_DYLIB=ON \
    -DLLVM_LINK_LLVM_DYLIB=ON \
    -DLLVM_DYLIB_COMPONENTS=all \
    -DLLVM_BUILD_TOOLS=ON \
    -DLLVM_BUILD_TESTS=OFF \
    -DLLVM_INCLUDE_TESTS=OFF \
    -DLLVM_BUILD_EXAMPLES=OFF \
    -DLLVM_INCLUDE_EXAMPLES=OFF \
    -DLLVM_INCLUDE_BENCHMARKS=OFF \
    -DLLVM_ENABLE_BINDINGS=OFF

  cmake --build "$build_dir" --target install --parallel "$parallel_jobs"
  rm -rf "$download_dir"

  ensure_runtime_alias
  verify_development_contract
  write_manifest
  verify_manifest
  write_summary "SultanC-provisioned llvm-project source build"
}

activate_toolchain() {
  local cache_hit="$2"

  load_contract "$1"

  if [[ "$cache_hit" != "true" ]]; then
    fail \
      "Pinned SultanC LLVM cache is missing for $platform/$architecture. "\
      "Run the 'Provision SultanC LLVM Toolchain' workflow on main."
  fi

  verify_development_contract
  verify_manifest
  export_toolchain_environment
  write_summary "SultanC Actions cache"
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
  provision)
    [[ $# -eq 1 ]] || usage
    load_contract "$1"
    provision_toolchain
    ;;
  *)
    usage
    ;;
esac
