#!/usr/bin/env bash
set -euo pipefail

# Validate native runner identity and the produced compiler using the same
# resolved host contract consumed by LLVM provisioning and backend smoke.
fail() {
  echo "$*" >&2
  exit 1
}

target="${SULTANC_TARGET:?target is required}"
binary="${SULTANC_BINARY:?binary path is required}"
working_directory="${SULTANC_WORKING_DIRECTORY:?working directory is required}"
script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
contracts="$script_dir/../../ci/contracts.json"
host="$("$script_dir/../../scripts/host-matrix.sh" "$contracts" --current)"

expected_target="$(jq -r '.target' <<< "$host")"
expected_os="$(jq -r '.host_os' <<< "$host")"
expected_arch="$(jq -r '.host_arch' <<< "$host")"
expected_pattern="$(jq -r '.file_pattern' <<< "$host")"
identity_command="$(jq -r '.identity.command' <<< "$host")"
validator="$(jq -c '.object_validator' <<< "$host")"
suffix="$(jq -r '.executable_suffix' <<< "$host")"

[[ "$target" == "$expected_target" ]] || fail \
  "target does not match registered runner: requested=$target registered=$expected_target"
# host-matrix --current already matches RUNNER_OS and RUNNER_ARCH, including
# multiple runner labels on the same OS/architecture when host ID is supplied.
if [[ -n "$identity_command" ]]; then
  command -v "$identity_command" >/dev/null || fail \
    "registered native identity tool is missing: $identity_command"
  mapfile -t os_arguments < <(jq -r '.identity.os_arguments[]' <<< "$host")
  mapfile -t arch_arguments < <(jq -r '.identity.arch_arguments[]' <<< "$host")
  actual_os="$("$identity_command" "${os_arguments[@]}")"
  actual_arch="$("$identity_command" "${arch_arguments[@]}")"
  [[ "$actual_os" == "$expected_os" ]] || fail \
    "native OS mismatch: expected=$expected_os actual=$actual_os"
  [[ "$actual_arch" == "$expected_arch" ]] || fail \
    "native architecture mismatch: expected=$expected_arch actual=$actual_arch"
else
  # The Actions runner tuple is attested by the runner and already checked.
  # A host without an optional native probe must still pass binary inspection.
  actual_os="$expected_os"
  actual_arch="$expected_arch"
fi

if [[ -n "$suffix" && "$binary" != *"$suffix" ]]; then
  binary+="$suffix"
fi

diag="${RUNNER_TEMP:-/tmp}/sultanc-ci/target/$target"
mkdir -p "$diag"
cd -- "$working_directory"
[[ -x "$binary" ]] || fail "compiler binary is not executable: $binary"
printf '%s\n' "$actual_os" > "$diag/host-os.txt"
printf '%s\n' "$actual_arch" > "$diag/host-arch.txt"
printf '%s\n' "${RUNNER_OS:-unknown} / ${RUNNER_ARCH:-unknown}" > "$diag/github-runner.txt"

description="$(file -b "$binary")"
printf '%s\n' "$description" | tee "$diag/compiler-file.txt"
grep -Eiq -- "$expected_pattern" <<< "$description" || fail \
  "compiler executable does not match registered file signature: $expected_pattern"

"$script_dir/../../scripts/validate-output.sh" "$binary" "$validator" executable \
  > "$diag/compiler-validation.txt"

{
  printf '### SultanC target: `%s`\n' "$target"
  printf -- '- Host: `%s / %s`\n' "$actual_os" "$actual_arch"
  printf -- '- Compiler: `%s`\n' "$description"
  printf -- '- Format validation: `PASS`\n'
} >> "${GITHUB_STEP_SUMMARY:-/dev/null}"
