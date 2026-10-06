#!/usr/bin/env bash
set -euo pipefail

# Verify that landed qualification evidence belongs to the exact pushed commit.

: "${RESULT:?qualification result is required}"
: "${BRANCH:?branch is required}"
: "${READY:?promotion readiness is required}"
: "${CANDIDATE_SHA:?candidate SHA is required}"
: "${EXPECTED_SHA:?expected pushed SHA is required}"
: "${HARNESS_SHA:?harness SHA is required}"

[[ "$RESULT" == "success" ]] || {
  echo "landed $BRANCH qualification did not succeed" >&2
  exit 1
}
[[ "$CANDIDATE_SHA" == "$EXPECTED_SHA" ]] || {
  echo "qualification SHA mismatch" >&2
  exit 1
}

{
  echo "### Landed commit"
  printf -- '- Branch: `%s`\n' "$BRANCH"
  printf -- '- Candidate SHA: `%s`\n' "$CANDIDATE_SHA"
  printf -- '- Harness SHA: `%s`\n' "$HARNESS_SHA"
  printf -- '- Promotion-grade readiness: `%s`\n' "$READY"
  echo '- Result: `PASS`'
} >> "$GITHUB_STEP_SUMMARY"
