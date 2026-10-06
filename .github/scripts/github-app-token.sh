#!/usr/bin/env bash
set -euo pipefail

# Exchange configured GitHub App credentials for a short-lived installation token.
# The caller owns secret exposure boundaries; this script prints only the token.

fail() {
  echo "$*" >&2
  exit 1
}

require_command() {
  command -v "$1" >/dev/null 2>&1 || fail "$1 is required"
}

base64_url() {
  openssl base64 -A | tr '+/' '-_' | tr -d '='
}

: "${SULTANC_APP_ID:?SULTANC_APP_ID is required}"
: "${SULTANC_APP_PRIVATE_KEY:?SULTANC_APP_PRIVATE_KEY is required}"
: "${SULTANC_REPOSITORY:?SULTANC_REPOSITORY is required}"

require_command curl
require_command jq
require_command openssl

work="$(mktemp -d "${RUNNER_TEMP:-/tmp}/sultanc-app-token.XXXXXX")"
trap 'rm -rf "$work"' EXIT

key="$work/app.pem"
printf '%s' "$SULTANC_APP_PRIVATE_KEY" > "$key"
chmod 600 "$key"

now="$(date +%s)"
iat=$((now - 60))
exp=$((now + 540))
header="$(printf '%s' '{"alg":"RS256","typ":"JWT"}' | base64_url)"
payload="$(
  printf '{"iat":%d,"exp":%d,"iss":"%s"}' \
    "$iat" \
    "$exp" \
    "$SULTANC_APP_ID" \
    | base64_url
)"
signing_input="$header.$payload"
signature="$(
  printf '%s' "$signing_input" \
    | openssl dgst -sha256 -sign "$key" \
    | base64_url
)"
jwt="$signing_input.$signature"

installation_id="$(
  curl -fsSL \
    -H "Authorization: Bearer $jwt" \
    -H 'Accept: application/vnd.github+json' \
    -H 'X-GitHub-Api-Version: 2022-11-28' \
    "https://api.github.com/repos/$SULTANC_REPOSITORY/installation" \
    | jq -er '.id'
)"

token="$(
  curl -fsSL \
    -X POST \
    -H "Authorization: Bearer $jwt" \
    -H 'Accept: application/vnd.github+json' \
    -H 'X-GitHub-Api-Version: 2022-11-28' \
    "https://api.github.com/app/installations/$installation_id/access_tokens" \
    | jq -er '.token'
)"

printf '%s\n' "$token"
