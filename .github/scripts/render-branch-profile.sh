#!/usr/bin/env bash
set -euo pipefail

# Project one protected branch profile into the active workflow directory.
# Only the small allowlist below may vary by branch; shared CI stays shared.

usage() {
  echo "usage: render-branch-profile.sh PROFILE [REPOSITORY_ROOT]" >&2
  exit 2
}

[[ $# -ge 1 && $# -le 2 ]] || usage
profile="$1"
repo="${2:-.}"

case "$profile" in
  main|beta|master) ;;
  *)
    echo "unsupported branch profile: $profile" >&2
    exit 2
    ;;
esac

repo="$(cd -- "$repo" && pwd)"
profile_dir="$repo/.github/ci/branch-profiles/$profile"
[[ -d "$profile_dir" ]] || {
  echo "missing profile directory: $profile_dir" >&2
  exit 1
}

readonly -a workflow_allowlist=(
  gate.yml
  integration.yml
  security.yml
  nightly.yml
  promotion.yml
)

for workflow in "${workflow_allowlist[@]}"; do
  source_file="$profile_dir/$workflow"
  active_file="$repo/.github/workflows/$workflow"

  if [[ -f "$source_file" ]]; then
    cp -- "$source_file" "$active_file"
  else
    rm -f -- "$active_file"
  fi
done
