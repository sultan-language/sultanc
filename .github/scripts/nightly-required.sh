#!/usr/bin/env bash
set -euo pipefail

# Nightly is valid only when both tracked development branches complete deep CI.

: "${MAIN_RESULT:?main result is required}"
: "${BETA_RESULT:?beta result is required}"
: "${MAIN_SHA:?main SHA is required}"
: "${BETA_SHA:?beta SHA is required}"

[[ "$MAIN_RESULT" == "success" ]] || {
  echo "main deep qualification did not succeed" >&2
  exit 1
}
[[ "$BETA_RESULT" == "success" ]] || {
  echo "beta deep qualification did not succeed" >&2
  exit 1
}

{
  echo "### Nightly"
  printf -- '- main: `%s` PASS\n' "$MAIN_SHA"
  printf -- '- beta: `%s` PASS\n' "$BETA_SHA"
} >> "$GITHUB_STEP_SUMMARY"
