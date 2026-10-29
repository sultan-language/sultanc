#!/usr/bin/env bash
set -euo pipefail

# Verify that active branch wrappers match the selected protected branch profile.

usage() {
  echo "usage: validate-branch-profile.sh PROFILE REPOSITORY_ROOT" >&2
  exit 2
}

fail() {
  echo "$*" >&2
  exit 1
}

check_branch_filter() {
  local workflow="$1"
  local path="$repo/.github/workflows/$workflow"

  grep -Eq "branches: \\[$profile\\]" "$path" || fail \
    "$workflow branch filter does not match $profile"
}

[[ $# -eq 2 ]] || usage
profile="$1"
repo="$(cd -- "$2" && pwd)"

case "$profile" in
  main|beta|master) ;;
  *)
    echo "unsupported branch profile: $profile" >&2
    exit 2
    ;;
esac

for workflow in gate.yml integration.yml security.yml; do
  [[ -f "$repo/.github/workflows/$workflow" ]] || fail \
    "missing active workflow: $workflow"
  check_branch_filter "$workflow"
done

if [[ "$profile" == "master" ]]; then
  [[ -f "$repo/.github/workflows/nightly.yml" ]] || fail \
    "master nightly workflow is missing"
  [[ -f "$repo/.github/workflows/promotion.yml" ]] || fail \
    "master promotion workflow is missing"
else
  [[ ! -e "$repo/.github/workflows/nightly.yml" ]] || fail \
    "non-master profile contains active nightly workflow"
  [[ ! -e "$repo/.github/workflows/promotion.yml" ]] || fail \
    "non-master profile contains active promotion workflow"
fi

release_pattern='(^|[^A-Za-z])(release|gh release|create-release|upload-release)'
if grep -RIEq "$release_pattern" "$repo/.github/workflows"; then
  fail "release automation is forbidden"
fi
