#!/usr/bin/env bash
set -euo pipefail

# Build Stage0 under CodeQL extraction and retain the compiler build log.

: "${SECURITY_LANE:?security lane is required}"

diag="${RUNNER_TEMP:-/tmp}/sultanc-ci/security/$SECURITY_LANE"
mkdir -p "$diag"
chmod +x ./build.sh
./build.sh stage0 2>&1 | tee "$diag/stage0.log"
