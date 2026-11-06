#!/usr/bin/env bash
set -euo pipefail

# Validate generated target output without executing foreign binaries.

usage() {
  echo "usage: validate-output.sh OUTPUT VALIDATOR_JSON [executable]" >&2
  exit 2
}

fail() {
  echo "$*" >&2
  exit 1
}

require_command() {
  command -v "$1" >/dev/null 2>&1 || fail "$1 is required"
}

llvm_readobj() {
  local tool=""
  local candidate

  if [[ -n "${LLVM_CONFIG:-}" ]]; then
    candidate="$(dirname -- "$LLVM_CONFIG")/llvm-readobj${SULTANC_EXECUTABLE_SUFFIX:-}"
    [[ -x "$candidate" ]] && tool="$candidate"
  else
    tool="$(command -v "llvm-readobj${SULTANC_EXECUTABLE_SUFFIX:-}" || true)"
  fi

  [[ -n "$tool" ]] || fail \
    "llvm-readobj is required for $validator_type validation"
  "$tool" --file-headers "$output"
}

require_regex() {
  local label="$1"
  local regex="$2"
  local text="$3"

  [[ -n "$regex" ]] || fail "$label regex is missing from validator contract"
  if ! grep -Eiq -- "$regex" <<< "$text"; then
    fail "$label did not match required regex: $regex"
  fi
}

validate_elf() {
  local description
  local header
  local file_regex
  local expected_class
  local machine_regex

  require_command file
  require_command readelf
  file_regex="$(jq -r '.file_regex // empty' <<< "$validator_json")"
  expected_class="$(jq -r '.class // empty' <<< "$validator_json")"
  machine_regex="$(jq -r '.machine_regex // empty' <<< "$validator_json")"

  [[ -n "$file_regex" && -n "$expected_class" && -n "$machine_regex" ]] || \
    fail "incomplete ELF validator"

  description="$(file -b "$output")"
  require_regex "ELF file identity" "$file_regex" "$description"
  header="$(readelf -h "$output")"
  require_regex "ELF class" "Class:[[:space:]]+$expected_class" "$header"
  require_regex "ELF machine" "Machine:[[:space:]]+($machine_regex)" "$header"
  printf '%s\n' "$description"
}

validate_macho() {
  local description
  local header
  local file_regex
  local header_regex

  require_command file
  file_regex="$(jq -r '.file_regex // empty' <<< "$validator_json")"
  header_regex="$(
    jq -r '.header_regex // .cpu_regex // empty' <<< "$validator_json"
  )"
  [[ -n "$file_regex" && -n "$header_regex" ]] || fail \
    "incomplete Mach-O validator"

  description="$(file -b "$output")"
  require_regex "Mach-O file identity" "$file_regex" "$description"
  header="$(llvm_readobj)"
  require_regex "Mach-O header/architecture" "$header_regex" "$header"
  printf '%s\n' "$description"
}

validate_coff() {
  local description
  local file_regex
  local header
  local header_regex

  header_regex="$(jq -r '.header_regex // empty' <<< "$validator_json")"
  file_regex="$(jq -r '.file_regex // empty' <<< "$validator_json")"
  [[ -n "$header_regex" ]] || fail "COFF validator requires header_regex"

  header="$(llvm_readobj)"
  require_regex "COFF format" '^Format: COFF-' "$header"
  require_regex "COFF header/machine" "$header_regex" "$header"
  if [[ "$artifact_kind" == "executable" ]]; then
    local executable_header_regex
    executable_header_regex="$(jq -r '.executable_header_regex // empty' <<< "$validator_json")"
    if [[ -n "$executable_header_regex" ]]; then
      require_regex "native executable header" "$executable_header_regex" "$header"
    fi
  fi

  if [[ -n "$file_regex" ]]; then
    require_command file
    description="$(file -b "$output")"
    require_regex "COFF file identity" "$file_regex" "$description"
  else
    description="COFF object validated by llvm-readobj"
  fi

  printf '%s\n' "$description"
}

# A new host with an LLVM-recognized binary format does not need a new
# dispatch branch. Its registry entry supplies file and header regexes.
validate_file_header() {
  local description
  local header
  local file_regex
  local header_regex
  local executable_header_regex

  require_command file
  file_regex="$(jq -r '.file_regex // empty' <<< "$validator_json")"
  header_regex="$(jq -r '.header_regex // empty' <<< "$validator_json")"
  [[ -n "$file_regex" && -n "$header_regex" ]] || fail \
    "file-header validator requires file_regex and header_regex"
  description="$(file -b "$output")"
  require_regex "file identity" "$file_regex" "$description"
  header="$(llvm_readobj)"
  require_regex "LLVM file headers" "$header_regex" "$header"
  if [[ "$artifact_kind" == "executable" ]]; then
    executable_header_regex="$(jq -r '.executable_header_regex // empty' <<< "$validator_json")"
    if [[ -n "$executable_header_regex" ]]; then
      require_regex "native executable header" "$executable_header_regex" "$header"
    fi
  fi
  printf '%s\n' "$description"
}

validate_wasm() {
  local header
  local header_regex
  local magic
  local size

  header_regex="$(jq -r '.header_regex // empty' <<< "$validator_json")"
  [[ -n "$header_regex" ]] || fail \
    "WebAssembly validator requires header_regex"
  require_command od

  magic="$(od -An -tx1 -N4 "$output" | tr -d '[:space:]')"
  [[ "$magic" == "0061736d" ]] || fail \
    "invalid WebAssembly magic: $magic"

  header="$(llvm_readobj)"
  require_regex "WebAssembly header/target identity" "$header_regex" "$header"
  size="$(wc -c < "$output" | tr -d '[:space:]')"
  printf 'WebAssembly object (%s bytes)\n' "$size"
}

validate_text() {
  local expected_regex
  local size

  expected_regex="$(jq -r '.expected_regex // empty' <<< "$validator_json")"
  [[ -n "$expected_regex" ]] || fail "text validator requires expected_regex"
  grep -Eq -- "$expected_regex" "$output" || fail \
    "text output did not match required target/format evidence"

  size="$(wc -c < "$output" | tr -d '[:space:]')"
  printf 'validated %s text (%s bytes)\n' "$validator_type" "$size"
}

[[ $# -ge 2 && $# -le 3 ]] || usage
output="$1"
validator_json="$2"
artifact_kind="${3:-object}"
[[ "$artifact_kind" == "object" || "$artifact_kind" == "executable" ]] || usage
require_command jq
[[ -s "$output" ]] || fail "output is missing or empty: $output"

validator_type="$(jq -r '.type // empty' <<< "$validator_json")"
case "$validator_type" in
  elf) validate_elf ;;
  macho) validate_macho ;;
  coff) validate_coff ;;
  file-header) validate_file_header ;;
  wasm) validate_wasm ;;
  text-regex|llvm-ir|assembly) validate_text ;;
  *) fail "unsupported output validator type: $validator_type" ;;
esac
