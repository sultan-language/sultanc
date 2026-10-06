#!/usr/bin/env bash
set -euo pipefail

# Validate trusted promotion authentication metadata without reading secret values.

fail() {
  echo "$*" >&2
  exit 1
}

: "${CONTRACTS_FILE:?contracts file is required}"
: "${SOURCE_BRANCH:?source branch is required}"
: "${DESTINATION_BRANCH:?destination branch is required}"
: "${SOURCE_SHA:?source SHA is required}"
: "${DESTINATION_SHA:?destination SHA is required}"
: "${TRIGGER_RUN_ID:?integration run id is required}"
: "${TRIGGER_RUN_ATTEMPT:?integration run attempt is required}"
: "${READINESS:?qualification readiness is required}"

state="$(jq -r '.promotion.state' "$CONTRACTS_FILE")"
auth_mode="$(jq -r '.promotion.auth_mode // empty' "$CONTRACTS_FILE")"
app_id_secret="$(jq -r '.promotion.app_id_secret // empty' "$CONTRACTS_FILE")"
private_key_secret="$(
  jq -r '.promotion.private_key_secret // empty' "$CONTRACTS_FILE"
)"
token_secret="$(jq -r '.promotion.token_secret // empty' "$CONTRACTS_FILE")"
merge_method="$(jq -r '.promotion.merge_method // empty' "$CONTRACTS_FILE")"

if [[ "$state" == "configured" ]]; then
  case "$auth_mode" in
    pat)
      [[ -n "$token_secret" ]] || fail \
        "configured PAT mode requires token_secret"
      ;;
    github-app)
      [[ -n "$app_id_secret" && -n "$private_key_secret" ]] || fail \
        "configured GitHub App mode requires App secret names"
      ;;
    *)
      fail "configured promotion requires auth_mode=pat or github-app"
      ;;
  esac

  case "$merge_method" in
    merge|squash|rebase) ;;
    *) fail "configured promotion has unsupported merge_method" ;;
  esac
fi

{
  printf 'state=%s\n' "$state"
  printf 'auth_mode=%s\n' "$auth_mode"
  printf 'app_id_secret=%s\n' "$app_id_secret"
  printf 'private_key_secret=%s\n' "$private_key_secret"
  printf 'token_secret=%s\n' "$token_secret"
  printf 'merge_method=%s\n' "$merge_method"
} >> "$GITHUB_OUTPUT"

{
  echo "### Promotion preflight"
  printf -- '- Source: `%s`\n' "$SOURCE_BRANCH"
  printf -- '- Destination: `%s`\n' "$DESTINATION_BRANCH"
  printf -- '- Source SHA: `%s`\n' "$SOURCE_SHA"
  printf -- '- Destination SHA: `%s`\n' "$DESTINATION_SHA"
  printf -- '- Integration run: `%s` attempt `%s`\n' \
    "$TRIGGER_RUN_ID" \
    "$TRIGGER_RUN_ATTEMPT"
  printf -- '- Contract readiness: `%s`\n' "$READINESS"
  printf -- '- Credential state: `%s`\n' "$state"
} >> "$GITHUB_STEP_SUMMARY"
