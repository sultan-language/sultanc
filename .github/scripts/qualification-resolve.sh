#!/usr/bin/env bash
set -euo pipefail

# Validate qualification inputs and bind a requested ref to one immutable commit.

: "${PROFILE:?profile is required}"
: "${MODE:?mode is required}"
: "${REQUESTED_REF:?requested ref is required}"
: "${CANDIDATE_DIR:=candidate}"

case "$PROFILE" in
  main|beta|master) ;;
  *)
    echo "unsupported qualification profile: $PROFILE" >&2
    exit 2
    ;;
esac

case "$MODE" in
  pr|full|deep) ;;
  *)
    echo "unsupported qualification mode: $MODE" >&2
    exit 2
    ;;
esac

sha="$(git -C "$CANDIDATE_DIR" rev-parse HEAD)"
[[ -n "$sha" ]] || {
  echo "candidate SHA could not be resolved" >&2
  exit 1
}
printf 'sha=%s\n' "$sha" >> "$GITHUB_OUTPUT"

{
  echo "### Qualification identity"
  printf -- '- Candidate: `%s`\n' "$sha"
  printf -- '- Requested ref: `%s`\n' "$REQUESTED_REF"
  printf -- '- Profile: `%s`\n' "$PROFILE"
  printf -- '- Mode: `%s`\n' "$MODE"
} >> "$GITHUB_STEP_SUMMARY"
