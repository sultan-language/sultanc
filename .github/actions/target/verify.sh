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
printf '%s\n' "$actual_os" > "$diag/host-os.txt"
printf '%s\n' "$actual_arch" > "$diag/host-arch.txt"

[[ -x "$binary" ]] || fail "compiler binary is not executable: $binary"
[[ "$actual_os" == "$expected_os" ]] || fail \
  "runner OS mismatch: expected=$expected_os actual=$actual_os"
[[ "$actual_arch" == "$expected_arch" ]] || fail \
  "runner architecture mismatch: expected=$expected_arch actual=$actual_arch"

description="$(file "$binary")"
printf '%s\n' "$description" | tee "$diag/compiler-file.txt"
grep -Eiq -- "$expected_pattern" <<< "$description" || fail \
  "compiler artifact does not match expected target pattern"

{
  printf '### SultanC target: `%s`\n' "$target"
  printf -- '- Host: `%s / %s`\n' "$actual_os" "$actual_arch"
  printf -- '- Compiler: `%s`\n' "$description"
} >> "$GITHUB_STEP_SUMMARY"
