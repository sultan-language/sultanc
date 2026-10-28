#!/usr/bin/env bash
set -euo pipefail

# Verify that a native runner and built compiler match the declared target contract.

fail() {
  echo "$*" >&2
  exit 1
}

target="${SULTANC_TARGET:?target is required}"
expected_os="${EXPECTED_HOST_OS:?expected host OS is required}"
expected_arch="${EXPECTED_HOST_ARCH:?expected host architecture is required}"
expected_pattern="${EXPECTED_FILE_PATTERN:?file pattern is required}"
binary="${SULTANC_BINARY:?binary path is required}"
working_directory="${SULTANC_WORKING_DIRECTORY:?working directory is required}"

diag="${RUNNER_TEMP:-/tmp}/sultanc-ci/target/$target"
mkdir -p "$diag"
cd -- "$working_directory"

actual_os="$(uname -s)"
actual_arch="$(uname -m)"
if [[ "${RUNNER_OS:-}" == "Windows" ]]; then
  actual_os="$RUNNER_OS"
  case "${RUNNER_ARCH:-}" in
    X64) actual_arch=x86_64 ;;
    ARM64) actual_arch=arm64 ;;
    *) fail "unsupported Windows runner architecture: ${RUNNER_ARCH:-unset}" ;;
  esac
  [[ "$binary" == *.exe ]] || binary+=".exe"
fi
printf '%s\n' "$actual_os" > "$diag/host-os.txt"
printf '%s\n' "$actual_arch" > "$diag/host-arch.txt"
printf '%s\n' "${RUNNER_OS:-unknown} / ${RUNNER_ARCH:-unknown}" > "$diag/github-runner.txt"

[[ -x "$binary" ]] || fail "compiler binary is not executable: $binary"
[[ "$actual_os" == "$expected_os" ]] || fail \
  "runner OS mismatch: expected=$expected_os actual=$actual_os"
[[ "$actual_arch" == "$expected_arch" ]] || fail \
  "runner architecture mismatch: expected=$expected_arch actual=$actual_arch"

description="$(file "$binary")"
printf '%s\n' "$description" | tee "$diag/compiler-file.txt"
grep -Eiq -- "$expected_pattern" <<< "$description" || fail \
  "compiler artifact does not match expected target pattern"

if [[ "$actual_os" == "Windows" ]]; then
  [[ -n "${LLVM_CONFIG:-}" ]] || fail "pinned LLVM_CONFIG is required to inspect PE headers"
  readobj="$(dirname -- "$LLVM_CONFIG")/llvm-readobj.exe"
  [[ -x "$readobj" ]] || fail "pinned LLVM llvm-readobj.exe is missing: $readobj"
  "$readobj" --file-headers "$binary" > "$diag/compiler-pe-headers.txt"
  grep -Eiq '^Format: COFF-' "$diag/compiler-pe-headers.txt" || fail \
    "compiler output is not COFF/PE"
  grep -Eiq 'Magic: (0x20[bB]|0X20[Bb])' "$diag/compiler-pe-headers.txt" || fail \
    "compiler output is not PE32+"
  case "$target" in
    x86_64-windows-msvc) machine='IMAGE_FILE_MACHINE_AMD64|0x8664' ;;
    aarch64-windows-msvc) machine='IMAGE_FILE_MACHINE_ARM64|0xAA64' ;;
    *) fail "unexpected Windows target: $target" ;;
  esac
  grep -Eiq "Machine: ($machine)" "$diag/compiler-pe-headers.txt" || fail \
    "PE machine does not match target $target"
fi

{
  printf '### SultanC target: `%s`\n' "$target"
  printf -- '- Host: `%s / %s`\n' "$actual_os" "$actual_arch"
  printf -- '- Compiler: `%s`\n' "$description"
} >> "$GITHUB_STEP_SUMMARY"
