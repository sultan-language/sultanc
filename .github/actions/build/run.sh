#!/usr/bin/env bash
set -euo pipefail

# Build SultanC through the repository-owned build entry point and retain logs.

fail() {
  echo "$*" >&2
  exit 1
}

working_directory="${SULTANC_WORKING_DIRECTORY:?working directory is required}"
target="${SULTANC_TARGET:-}"
label="${target:-host}"
diag="${RUNNER_TEMP:-/tmp}/sultanc-ci/build/$label"

mkdir -p "$diag"
cd -- "$working_directory"
[[ -f ./build.sh ]] || fail "build.sh is missing"
chmod +x ./build.sh

if [[ -n "$target" ]]; then
  ./build.sh --target="$target" 2>&1 | tee "$diag/build.log"
else
  ./build.sh 2>&1 | tee "$diag/build.log"
fi

[[ -x build/sultanc ]] || fail "build/sultanc was not produced"
file build/sultanc | tee "$diag/compiler-file.txt"
