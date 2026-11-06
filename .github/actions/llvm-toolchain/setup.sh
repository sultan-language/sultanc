#!/usr/bin/env bash
set -euo pipefail

# Resolve, provision, verify, cache and activate SultanC's pinned LLVM SDK.
#
#   setup.sh resolve   CONTRACTS_JSON            cache path/key for this host
#   setup.sh activate  CONTRACTS_JSON CACHE_HIT  verify a restored SDK, or, on
#                                                a miss, provision a "prebuilt"
#                                                host in this job
#   setup.sh provision CONTRACTS_JSON            provision with the host's
#                                                declared method
#
# Methods are declared per host in contracts.json (ci_hosts[].llvm.method):
#   prebuilt  pinned upstream release archive, SHA-256 verified before use,
#             only the declared members extracted; SultanC links a monolithic
#             shared libLLVM, which the upstream Linux/macOS archives do not
#             ship, so it is linked from the archive's component archives.
#   source    fallback: pinned llvm-project source with the host's pinned
#             compiler (clang-cl on Windows, never cl.exe), bounded compile and
#             link jobs, separate checked build and install steps.
#   none      the host declares no LLVM SDK.
# Every method ends in the same SDK verification, prune, manifest and cache.

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

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=../../scripts/host-path.sh
source "$script_dir/../../scripts/host-path.sh"
tool_path() { host_path_to_shell "$1"; }
native_path() { host_path_to_native "$1"; }

sha256_file() {
  if command -v sha256sum >/dev/null 2>&1; then
    sha256sum "$1" | awk '{print $1}'
  else
    require_command shasum
    shasum -a 256 "$1" | awk '{print $1}'
  fi
}

sha256_text() {
  if command -v sha256sum >/dev/null 2>&1; then
    sha256sum | awk '{print $1}'
  else
    require_command shasum
    shasum -a 256 | awk '{print $1}'
  fi
}

# A checksum that is not a real SHA-256 (for example "TODO-verify") must never
# be used: it marks a value that still has to be verified and filled in.
require_sha256() {
  local value="$1"
  local label="$2"

  [[ "$value" =~ ^[0-9a-f]{64}$ ]] || fail \
    "SHA-256 for $label is not filled in (value: '$value'). Verify the upstream digest and set it in .github/ci/contracts.json before using this host."
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
  link_mode="$(jq -r '.llvm_toolchain.link_contract.mode // empty' "$contracts")"

  [[ "$state" == "confirmed" ]] || fail \
    "LLVM toolchain contract must be confirmed before qualification"
  [[ "$version" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]] || fail \
    "LLVM toolchain version must be an exact pinned release: $version"
  [[ "$provider" == "sultanc-actions-cache" ]] || fail \
    "unsupported LLVM toolchain provider: $provider"
  [[ -n "$namespace" && -n "$cache_schema" ]] || fail \
    "LLVM cache namespace and schema version are required"
  [[ -n "$source_archive" && -n "$source_root" && -n "$source_url" ]] || fail \
    "LLVM source archive contract is incomplete"
  [[ "$build_type" == "Release" ]] || fail \
    "LLVM provisioning build type must be Release"
  [[ "$link_mode" == "shared" ]] || fail \
    "SultanC links the monolithic shared libLLVM / LLVM-C.dll; link_contract.mode must be shared"

  # The shared matrix resolver validates the registry and matches RUNNER_OS /
  # RUNNER_ARCH (and SULTANC_CI_HOST_ID). Unknown runners fail.
  host_contract="$("$script_dir/../../scripts/host-matrix.sh" "$contracts" --current)"
  contract_platform="$(jq -r '.id' <<< "$host_contract")"
  platform="$(jq -r '.runner_os | ascii_downcase' <<< "$host_contract")"
  architecture="$(jq -r '.host_arch' <<< "$host_contract")"
  method="$(jq -r '.llvm.method' <<< "$host_contract")"
  [[ "$method" != "none" ]] || fail \
    "CI host $contract_platform declares no LLVM SDK (ci_hosts[].llvm.method is none)"

  host_paths_init "$host_contract"
  mapfile -t archive_environment < <(
    jq -r '.archive_environment | to_entries[] | "\(.key)=\(.value)"' <<< "$host_contract"
  )
  mapfile -t archive_member_arguments < <(jq -r '.archive_commands.member_arguments[]' <<< "$host_contract")
  llvm_config_relative="$(jq -r '.llvm_config' <<< "$host_contract")"
  compiler_c_name="$(jq -r '.compiler_c' <<< "$host_contract")"
  compiler_cxx_name="$(jq -r '.compiler_cxx' <<< "$host_contract")"
  compiler_id="$(jq -r '.compiler_id' <<< "$host_contract")"
  compiler_binary_pattern="$(jq -r '.compiler_binary_pattern' <<< "$host_contract")"
  executable_suffix="$(jq -r '.executable_suffix' <<< "$host_contract")"
  archive_tar="$(jq -r '.archive_commands.tar' <<< "$host_contract")"
  archive_ln="$(jq -r '.archive_commands.ln' <<< "$host_contract")"
  parallel_jobs="$(jq -r --argjson shared "$(jq '.llvm_toolchain.build.parallel_jobs' "$contracts")" \
    '.llvm.source_build.parallel_jobs // $shared' <<< "$host_contract")"
  link_jobs="$(jq -r --argjson shared "$(jq '.llvm_toolchain.build.link_jobs' "$contracts")" \
    '.llvm.source_build.link_jobs // $shared' <<< "$host_contract")"
  [[ "$parallel_jobs" =~ ^[1-9][0-9]*$ && "$link_jobs" =~ ^[1-9][0-9]*$ ]] || fail \
    "LLVM provisioning parallel_jobs and link_jobs must be positive integers"

  # Cache identity: the shared LLVM contract plus the toolchain-relevant part
  # of this host's record. Editing a host's checks does not evict its SDK;
  # editing its LLVM data, compilers or prerequisites does.
  contract_hash="$(jq -S -c --argjson host "$host_contract" '
    {llvm_toolchain: .llvm_toolchain,
     host: ($host | {id, runner_os, runner_arch, host_arch, llvm_config,
                     compiler_c, compiler_cxx, compiler_id, compiler_binary_pattern,
                     executable_suffix, bootstrap, path_converter,
                     archive_environment, archive_commands, llvm})}
  ' "$contracts" | sha256_text)"

  tool_root="$(tool_path "${RUNNER_TOOL_CACHE:-${RUNNER_TEMP:-/tmp}}")"
  toolchain_path="$tool_root/$namespace/$version/$contract_platform/$contract_hash"
  cache_key="${namespace}-v${cache_schema}-${version}-${contract_platform}-${contract_hash}"
  llvm_config="$toolchain_path/$llvm_config_relative"
  manifest="$toolchain_path/.sultanc-llvm-manifest.json"
}

write_output() {
  local output_file="${GITHUB_OUTPUT:-}"

  [[ -n "$output_file" ]] || fail "GITHUB_OUTPUT is required in Actions"
  printf '%s=%s\n' "$1" "$2" >> "$output_file"
}

resolve_contract() {
  load_contract "$1"

  write_output toolchain_path "$(native_path "$toolchain_path")"
  write_output cache_key "$cache_key"
  write_output version "$version"
  write_output platform "$platform"
  write_output architecture "$architecture"
  write_output method "$method"
}

prepare_runtime_environment() {
  local bin_dir="$toolchain_path/bin"
  if [[ ":$PATH:" != *":$bin_dir:"* ]]; then
    export PATH="$bin_dir:$PATH"
  fi
  export LLVM_CONFIG="$llvm_config"
}

# Substitute {prefix} and {file} in a declared argv template (no shell text).
expand_template() {
  local prefix="$1"
  local file="$2"
  local json="$3"

  jq -r --arg prefix "$prefix" --arg file "$file" '
    .[] | gsub("\\{prefix\\}"; $prefix) | gsub("\\{file\\}"; $file)
  ' <<< "$json"
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

  targets=" $("$llvm_config" --targets-built | tr -d '\r') "
  while IFS= read -r target; do
    [[ -n "$target" ]] || continue
    [[ "$targets" == *" $target "* ]] || fail \
      "required LLVM target family missing from llvm-config --targets-built: $target"
  done < <(jq -r '.llvm_toolchain.required_targets_built[]?' "$contracts")
}

# The library SultanC links must export the C API it uses and the
# initialization entry point of every required target family.
verify_exports() {
  local prefix="$1"
  local file
  local symbol_prefix
  local listing
  local symbol
  local -a command

  file="$prefix/$(jq -r '.llvm.exports.file' <<< "$host_contract")"
  symbol_prefix="$(jq -r '.llvm.exports.symbol_prefix' <<< "$host_contract")"
  [[ -f "$file" ]] || fail "LLVM shared library missing: $file"
  mapfile -t command < <(expand_template "$prefix" "$(native_path "$file")" \
    "$(jq -c '.llvm.exports.command' <<< "$host_contract")")
  command[0]="$(tool_path "${command[0]}")"
  listing="$(mktemp "${RUNNER_TEMP:-/tmp}/sultanc-llvm-exports.XXXXXX")"
  "${command[@]}" | tr -d '\r' > "$listing" || {
    rm -f "$listing"
    fail "cannot list the exports of $file"
  }

  while IFS= read -r symbol; do
    grep -Eq -- "(^|[^A-Za-z0-9_])${symbol_prefix}${symbol}([^A-Za-z0-9_]|$)" "$listing" || {
      rm -f "$listing"
      fail "LLVM shared library does not export $symbol_prefix$symbol: $file"
    }
  done < <(
    jq -r '
      .llvm_toolchain as $t |
      ($t.link_contract.required_symbols[]),
      ($t.required_targets_built[] as $family |
        $t.link_contract.per_target_symbol | gsub("\\{family\\}"; $family))
    ' "$contracts"
  )
  rm -f "$listing"
}

verify_sdk_tree() {
  local prefix="$1"
  local relative
  local tool

  while IFS= read -r relative; do
    [[ -e "$prefix/$relative" ]] || fail \
      "required LLVM SDK file missing: $prefix/$relative"
  done < <(jq -r '.llvm.required_files[]' <<< "$host_contract")
  while IFS= read -r tool; do
    [[ -n "$tool" ]] || continue
    [[ -x "$prefix/bin/$tool$executable_suffix" ]] || fail \
      "required pinned LLVM tool is missing: bin/$tool$executable_suffix"
  done < <(jq -r '.llvm_toolchain.required_tools[]?' "$contracts")
}

verify_development_contract() {
  local actual_version
  local include_dir

  [[ -x "$llvm_config" ]] || fail \
    "LLVM toolchain is incomplete: $llvm_config is missing"

  prepare_runtime_environment
  actual_version="$("$llvm_config" --version | tr -d '\r')"
  [[ "$actual_version" == "$version" ]] || fail \
    "LLVM version mismatch: expected=$version actual=$actual_version"

  include_dir="$(tool_path "$("$llvm_config" --includedir | tr -d '\r')")"
  [[ -d "$include_dir" ]] || fail "LLVM include directory is missing: $include_dir"
  [[ "$include_dir" -ef "$toolchain_path/include" ]] || fail \
    "llvm-config reports a foreign include directory: $include_dir"

  verify_headers "$include_dir"
  verify_targets
  verify_sdk_tree "$toolchain_path"
  verify_exports "$toolchain_path"
}

toolchain_payload_checksum() {
  local entry
  local relative

  (
    while IFS= read -r entry; do
      relative="${entry#"$toolchain_path"/}"
      [[ "$entry" != "$manifest" ]] || continue
      if [[ -L "$entry" ]]; then
        printf 'L\t%s\t%s\n' "$relative" "$(readlink "$entry")"
      else
        printf 'F\t%s\t%s\n' "$relative" "$(sha256_file "$entry")"
      fi
    done < <(find "$toolchain_path" \( -type f -o -type l \) -print | LC_ALL=C sort)
  ) | sha256_text
}

write_manifest() {
  local payload_sha

  payload_sha="$(toolchain_payload_checksum)"
  jq -n \
    --arg version "$version" \
    --arg platform "$platform" \
    --arg architecture "$architecture" \
    --arg host "$contract_platform" \
    --arg method "$method" \
    --arg contract_hash "$contract_hash" \
    --arg cache_key "$cache_key" \
    --arg payload_sha256 "$payload_sha" \
    '{schema: 2, version: $version, platform: $platform,
      architecture: $architecture, host: $host, method: $method,
      contract_hash: $contract_hash, cache_key: $cache_key,
      payload_sha256: $payload_sha256}' > "$manifest.tmp"
  mv "$manifest.tmp" "$manifest"
}

verify_manifest() {
  local field
  local expected_payload

  [[ -f "$manifest" ]] || fail \
    "LLVM SDK has no SultanC toolchain manifest: $manifest"
  [[ "$(jq -r '.schema // empty' "$manifest")" == "2" ]] || fail \
    "unsupported SultanC LLVM toolchain manifest schema"
  for field in version platform architecture method contract_hash cache_key; do
    [[ "$(jq -r --arg f "$field" '.[$f] // empty' "$manifest")" == "${!field}" ]] || fail \
      "LLVM SDK manifest $field does not match the contract"
  done
  [[ "$(jq -r '.host // empty' "$manifest")" == "$contract_platform" ]] || fail \
    "LLVM SDK manifest host does not match this runner"

  expected_payload="$(jq -r '.payload_sha256 // empty' "$manifest")"
  [[ "$expected_payload" =~ ^[0-9a-f]{64}$ ]] || fail \
    "LLVM SDK manifest payload checksum is invalid"
  [[ "$(toolchain_payload_checksum)" == "$expected_payload" ]] || fail \
    "LLVM SDK payload checksum does not match its provisioning manifest"
}

export_toolchain_environment() {
  prepare_runtime_environment
  if [[ -n "${GITHUB_PATH:-}" ]]; then
    printf '%s\n' "$(native_path "$toolchain_path/bin")" >> "$GITHUB_PATH"
  fi
  if [[ -n "${GITHUB_ENV:-}" ]]; then
    printf 'LLVM_CONFIG=%s\n' "$(native_path "$llvm_config")" >> "$GITHUB_ENV"
    printf 'SULTANC_LLVM_SDK=%s\n' "$(native_path "$toolchain_path")" >> "$GITHUB_ENV"
  fi
}

write_summary() {
  local source="$1"
  local summary="${GITHUB_STEP_SUMMARY:-/dev/null}"

  {
    echo "### LLVM SDK"
    printf -- '- Host: `%s` (method `%s`)\n' "$contract_platform" "$method"
    printf -- '- Source: %s\n' "$source"
    printf -- '- Version: `%s`\n' "$version"
    printf -- '- Cache key: `%s`\n' "$cache_key"
    printf -- '- Required LLVM target families: `%s` (llvm-config and exported initializers checked)\n' \
      "$(jq '.llvm_toolchain.required_targets_built | length' "$contracts")"
    echo '- Linkage: monolithic shared libLLVM / LLVM-C.dll, as cmake/modules/LLVMToolchain.cmake requires'
  } >> "$summary"
}

# Download one pinned file and verify it before anything reads it.
download_verified() {
  local url="$1"
  local expected="$2"
  local destination="$3"
  local label="$4"
  local actual

  require_sha256 "$expected" "$label"
  require_command curl
  curl --fail --location --silent --show-error --retry 5 --retry-delay 5 \
    --retry-all-errors --output "$destination" "$url"
  actual="$(sha256_file "$destination")"
  [[ "$actual" == "$expected" ]] || {
    rm -f -- "$destination"
    fail "SHA-256 mismatch for $label: expected=$expected actual=$actual"
  }
  echo "verified $label: sha256=$actual"
}

# Extract only the declared members of a verified archive (GNU tar needs
# --wildcards for patterns; that is declared per host, not inferred).
extract_members() {
  local archive_path="$1"
  local destination="$2"
  local record="$3"
  local root
  local member
  local -a members=()

  root="$(jq -r '.root' <<< "$record")"
  while IFS= read -r member; do
    members+=("$root/$member")
  done < <(jq -r '.members[]' <<< "$record")
  require_command "$archive_tar"
  mkdir -p "$destination"
  env "${archive_environment[@]}" "$archive_tar" -xf "$archive_path" -C "$destination" \
    "${archive_member_arguments[@]}" "${members[@]}" || fail \
    "cannot extract the declared members of $(basename -- "$archive_path")"
  [[ -d "$destination/$root" ]] || fail "archive root missing after extraction: $root"
}

# Copy exactly the declared SDK files (symbolic links preserved) into the
# cache path. Anything else from an archive or install tree is dropped.
install_sdk_tree() {
  local prefix="$1"
  local relative
  local -a keep=()

  while IFS= read -r relative; do
    [[ -e "$prefix/$relative" || -L "$prefix/$relative" ]] || fail \
      "declared LLVM SDK file is missing before install: $prefix/$relative"
    keep+=("$relative")
  done < <(jq -r '.llvm.keep[]' <<< "$host_contract")
  rm -rf -- "$toolchain_path"
  mkdir -p "$toolchain_path"
  (cd -- "$prefix" && env "${archive_environment[@]}" "$archive_tar" -cf - "${keep[@]}") | \
    (cd -- "$toolchain_path" && env "${archive_environment[@]}" "$archive_tar" -xf -)
}

# Link the monolithic shared libLLVM from the archive's component archives.
assemble_shared_library() {
  local prefix="$1"
  local output
  local entry
  local alias_path
  local alias_target
  local argument
  local staged_config="$prefix/$llvm_config_relative"
  local -a archives=()
  local -a system_libs=()
  local -a command=()

  [[ "$(jq -r '.llvm.shared_library // empty | type' <<< "$host_contract")" == "object" ]] || return 0
  output="$prefix/$(jq -r '.llvm.shared_library.output' <<< "$host_contract")"
  read -r -a archives <<< "$("$staged_config" --link-static --libfiles all | tr -d '\r')"
  read -r -a system_libs <<< "$("$staged_config" --link-static --system-libs | tr -d '\r')"
  ((${#archives[@]} > 0)) || fail "llvm-config listed no component archives"
  for argument in "${archives[@]}"; do
    [[ -f "$argument" ]] || fail "component archive missing from the extracted archive: $argument"
  done

  while IFS= read -r argument; do
    case "$argument" in
      '{archives}') command+=("${archives[@]}") ;;
      '{system_libs}') command+=("${system_libs[@]}") ;;
      *)
        argument="${argument//\{prefix\}/$prefix}"
        command+=("${argument//\{output\}/$output}")
        ;;
    esac
  done < <(jq -r '.llvm.shared_library.command[]' <<< "$host_contract")

  mkdir -p "$(dirname -- "$output")"
  echo "linking $(basename -- "$output") from ${#archives[@]} component archives"
  "${command[@]}" || fail "cannot link the monolithic shared LLVM library $output"
  [[ -s "$output" ]] || fail "shared LLVM library was not produced: $output"

  while IFS=$'\t' read -r entry alias_target; do
    alias_path="$prefix/$entry"
    rm -f -- "$alias_path"
    (cd -- "$(dirname -- "$alias_path")" && "$archive_ln" -s "$alias_target" "$(basename -- "$alias_path")")
  done < <(jq -r '.llvm.shared_library.aliases | to_entries[] | "\(.key)\t\(.value)"' <<< "$host_contract")
}

provision_prebuilt() {
  local temp_root
  local work
  local record
  local root

  temp_root="$(tool_path "${RUNNER_TEMP:-/tmp}")"
  record="$(jq -c '.llvm.prebuilt' <<< "$host_contract")"
  root="$(jq -r '.root' <<< "$record")"
  [[ "$(jq -r '.archive' <<< "$record")" == *"$version"* ]] || fail \
    "prebuilt archive name does not carry the pinned version $version"

  require_sha256 "$(jq -r '.sha256' <<< "$record")" "$(jq -r '.archive' <<< "$record")"
  work="$(mktemp -d "$temp_root/sultanc-llvm-prebuilt.XXXXXX")"
  download_verified "$(jq -r '.url' <<< "$record")" "$(jq -r '.sha256' <<< "$record")" \
    "$work/$(jq -r '.archive' <<< "$record")" "$(jq -r '.archive' <<< "$record")"
  extract_members "$work/$(jq -r '.archive' <<< "$record")" "$work/x" "$record"
  rm -f -- "$work/$(jq -r '.archive' <<< "$record")"

  verify_headers "$work/x/$root/include"
  assemble_shared_library "$work/x/$root"
  install_sdk_tree "$work/x/$root"
  rm -rf -- "$work"

  verify_development_contract
  write_manifest
  verify_manifest
}

# Extract the verified source archive with the host-declared POSIX tar and
# symlink behavior; a broken source tree fails before compilation.
extract_source_archive() {
  local archive_path="$1"
  local extract_dir="$2"
  local probe_dir="$extract_dir/.sultanc-symlink-probe"
  local expected_links
  local actual_links

  require_command "$archive_tar"
  require_command "$archive_ln"
  mkdir -p "$probe_dir"
  printf '%s\n' 'native-symlink-probe' > "$probe_dir/target"
  env "${archive_environment[@]}" "$archive_ln" -s target "$probe_dir/link" || fail \
    "LLVM provisioning host cannot create source archive symbolic links"
  [[ -L "$probe_dir/link" && "$(cat "$probe_dir/link")" == 'native-symlink-probe' ]] || fail \
    "LLVM provisioning host did not preserve source archive symbolic links"
  rm -rf "$probe_dir"

  expected_links="$(
    env "${archive_environment[@]}" "$archive_tar" -tvf "$archive_path" | \
      LC_ALL=C awk 'substr($0, 1, 1) == "l" { n++ } END { print n+0 }'
  )"
  env "${archive_environment[@]}" "$archive_tar" -xf "$archive_path" -C "$extract_dir"
  [[ -d "$extract_dir/$source_root/llvm" ]] || fail \
    "LLVM source archive root is missing after extraction: $source_root"
  actual_links="$(find "$extract_dir/$source_root" -type l | LC_ALL=C awk 'END { print NR+0 }')"
  [[ "$expected_links" == "$actual_links" ]] || fail \
    "LLVM source symbolic-link count mismatch: archive=$expected_links extracted=$actual_links"
  [[ -f "$extract_dir/$source_root/llvm/include/llvm-c/Analysis.h" ]] || fail \
    "verified LLVM source is missing llvm-c/Analysis.h before compilation"
}

# Confirm CMake accepted the registered compiler; otherwise a Windows image can
# fall back to cl.exe and exhaust its heap on generated LLVM sources (C1002).
verify_configured_compilers() {
  local build_dir="$1"
  local expected_c="$2"
  local expected_cxx="$3"
  local cache="$build_dir/CMakeCache.txt"
  local configured
  local expected
  local language
  local recorded_id

  [[ -f "$cache" ]] || fail "LLVM CMake cache was not created: $cache"
  for language in C CXX; do
    if [[ "$language" == C ]]; then expected="$expected_c"; else expected="$expected_cxx"; fi
    configured="$(sed -n "s/^CMAKE_${language}_COMPILER:[^=]*=//p" "$cache" | head -n 1)"
    [[ -n "$configured" ]] || fail "LLVM CMake did not record its $language compiler"
    [[ "$(tool_path "$configured")" -ef "$(tool_path "$expected")" ]] || fail \
      "LLVM $language compiler differs from the registered host compiler: expected=$expected actual=$configured"
    recorded_id="$(sed -n 's/^[[:space:]]*set(CMAKE_.*_COMPILER_ID "\([^"]*\)").*/\1/p' \
      "$build_dir"/CMakeFiles/*/CMake${language}Compiler.cmake | head -n 1)"
    [[ "$recorded_id" == "$compiler_id" ]] || fail \
      "LLVM $language compiler family mismatch: expected=$compiler_id actual=$recorded_id"
  done
}

provision_source() {
  local temp_root
  local work
  local targets
  local compiler_c
  local compiler_cxx
  local compiler_record
  local tool
  local -a compiler_tools=()
  local -a extra_arguments=()

  temp_root="$(tool_path "${RUNNER_TEMP:-/tmp}")"
  require_sha256 "$source_sha" "$source_archive"
  if [[ -n "$(jq -r '.llvm.host_compiler.sha256 // empty' <<< "$host_contract")" ]]; then
    require_sha256 "$(jq -r '.llvm.host_compiler.sha256' <<< "$host_contract")" \
      "$(jq -r '.llvm.host_compiler.archive' <<< "$host_contract")"
  fi
  require_command cmake
  require_command ninja
  work="$(mktemp -d "$temp_root/sultanc-llvm-source.XXXXXX")"

  # Host compiler: a pinned, checksum-verified compiler archive when the host
  # declares one (Windows: upstream clang-cl/lld-link), else the named tools.
  compiler_record="$(jq -c '.llvm.host_compiler // empty' <<< "$host_contract")"
  if [[ -n "$compiler_record" ]]; then
    local compiler_root
    compiler_root="$work/compiler/$(jq -r '.root' <<< "$compiler_record")"
    download_verified "$(jq -r '.url' <<< "$compiler_record")" "$(jq -r '.sha256' <<< "$compiler_record")" \
      "$work/$(jq -r '.archive' <<< "$compiler_record")" "$(jq -r '.archive' <<< "$compiler_record")"
    extract_members "$work/$(jq -r '.archive' <<< "$compiler_record")" "$work/compiler" "$compiler_record"
    rm -f -- "$work/$(jq -r '.archive' <<< "$compiler_record")"
    compiler_c="$compiler_root/$(jq -r '.c' <<< "$compiler_record")"
    compiler_cxx="$compiler_root/$(jq -r '.cxx' <<< "$compiler_record")"
    [[ -x "$compiler_c" && -x "$compiler_cxx" ]] || fail "pinned host compiler missing from its archive"
    for tool in linker ar; do
      if [[ -n "$(jq -r --arg t "$tool" '.[$t] // empty' <<< "$compiler_record")" ]]; then
        compiler_tools+=("$tool=$compiler_root/$(jq -r --arg t "$tool" '.[$t]' <<< "$compiler_record")")
      fi
    done
  else
    require_command "$compiler_c_name"
    require_command "$compiler_cxx_name"
    compiler_c="$(command -v "$compiler_c_name")"
    compiler_cxx="$(command -v "$compiler_cxx_name")"
  fi

  if [[ -n "$compiler_binary_pattern" ]]; then
    local description
    require_command file
    description="$(file -b "$compiler_cxx")"
    grep -Eiq -- "$compiler_binary_pattern" <<< "$description" || fail \
      "LLVM host C++ compiler has an unexpected binary format: $compiler_cxx ($description)"
  fi
  for tool in "${compiler_tools[@]}"; do
    case "${tool%%=*}" in
      linker) extra_arguments+=("-DCMAKE_LINKER=$(native_path "${tool#*=}")") ;;
      ar) extra_arguments+=("-DCMAKE_AR=$(native_path "${tool#*=}")") ;;
    esac
  done
  mapfile -t -O "${#extra_arguments[@]}" extra_arguments < <(
    jq -r '.llvm.source_build.cmake_arguments[]?' <<< "$host_contract"
  )

  download_verified "$source_url" "$source_sha" "$work/$source_archive" "$source_archive"
  mkdir -p "$work/source" "$work/build" "$work/install"
  extract_source_archive "$work/$source_archive" "$work/source"
  rm -f -- "$work/$source_archive"

  targets="$(jq -r '.llvm_toolchain.required_targets_built | join(";")' "$contracts")"
  cmake \
    -S "$(native_path "$work/source/$source_root/llvm")" \
    -B "$(native_path "$work/build")" \
    -G Ninja \
    -DCMAKE_BUILD_TYPE="$build_type" \
    -DCMAKE_INSTALL_PREFIX="$(native_path "$work/install")" \
    -DCMAKE_C_COMPILER="$(native_path "$compiler_c")" \
    -DCMAKE_CXX_COMPILER="$(native_path "$compiler_cxx")" \
    -DLLVM_TARGETS_TO_BUILD="$targets" \
    -DLLVM_PARALLEL_COMPILE_JOBS="$parallel_jobs" \
    -DLLVM_PARALLEL_LINK_JOBS="$link_jobs" \
    -DLLVM_INSTALL_TOOLCHAIN_ONLY=OFF \
    -DLLVM_BUILD_TOOLS=ON \
    -DLLVM_BUILD_UTILS=OFF \
    -DLLVM_BUILD_TESTS=OFF \
    -DLLVM_INCLUDE_TESTS=OFF \
    -DLLVM_BUILD_EXAMPLES=OFF \
    -DLLVM_INCLUDE_EXAMPLES=OFF \
    -DLLVM_INCLUDE_BENCHMARKS=OFF \
    -DLLVM_INCLUDE_DOCS=OFF \
    -DLLVM_ENABLE_BINDINGS=OFF \
    "${extra_arguments[@]}"

  verify_configured_compilers "$work/build" "$compiler_c" "$compiler_cxx"
  # Compilation and SDK installation are separate checked stages: a failed or
  # partial build can never be installed and mistaken for a complete SDK.
  cmake --build "$(native_path "$work/build")" --parallel "$parallel_jobs" || fail \
    "LLVM source build failed (jobs=$parallel_jobs, link jobs=$link_jobs)"
  cmake --install "$(native_path "$work/build")" --config "$build_type" || fail \
    "LLVM install step failed"
  [[ -f "$work/install/include/llvm-c/Analysis.h" ]] || fail \
    "LLVM install did not provide C API headers under $work/install/include/llvm-c"
  verify_headers "$work/install/include"
  install_sdk_tree "$work/install"
  rm -rf -- "$work"

  verify_development_contract
  write_manifest
  verify_manifest
}

provision_toolchain() {
  case "$method" in
    prebuilt)
      provision_prebuilt
      write_summary "pinned upstream archive \`$(jq -r '.llvm.prebuilt.archive' <<< "$host_contract")\` (SHA-256 verified), monolithic shared library linked from its component archives"
      ;;
    source)
      provision_source
      write_summary "pinned \`$source_archive\` built with the registered host compiler (jobs=$parallel_jobs, link jobs=$link_jobs)"
      ;;
    *) fail "unsupported LLVM provisioning method: $method" ;;
  esac
}

activate_toolchain() {
  local cache_hit="$2"

  load_contract "$1"
  if [[ "$cache_hit" == "true" ]]; then
    verify_development_contract
    verify_manifest
    export_toolchain_environment
    write_summary "SultanC Actions cache (manifest and payload checksum verified)"
    return
  fi

  # A cache miss is recoverable in this job only when provisioning takes
  # minutes. A source build is a trusted, separate workflow.
  if [[ "$method" != "prebuilt" ]]; then
    fail "Pinned SultanC LLVM SDK cache is missing for $contract_platform (key=$cache_key). Its method is '$method': run the 'Provision SultanC LLVM Toolchain' workflow on main with host=$contract_platform (or host=all)."
  fi
  provision_toolchain
  export_toolchain_environment
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
