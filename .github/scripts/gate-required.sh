#!/usr/bin/env bash
set -euo pipefail

# Collapse the reusable qualification result into one stable required gate.

: "${RESULT:?qualification result is required}"
: "${DESTINATION:?destination branch is required}"
: "${CANDIDATE_SHA:?candidate SHA is required}"

if [[ "$RESULT" != "success" ]]; then
  echo "$DESTINATION candidate qualification did not succeed" >&2
  exit 1
fi

{
  echo "### Required gate"
  printf -- '- Destination: `%s`\n' "$DESTINATION"
  printf -- '- Candidate: `%s`\n' "$CANDIDATE_SHA"
  echo '- Result: `PASS`'
} >> "$GITHUB_STEP_SUMMARY"
