#!/usr/bin/env bash
set -euo pipefail

# Resolve the configured trusted credential and invoke the promotion controller.

fail() {
  echo "$*" >&2
  exit 1
}

: "${AUTH_MODE:?authentication mode is required}"

case "$AUTH_MODE" in
  pat)
    [[ -n "${SULTANC_PAT:-}" ]] || fail \
      "configured PAT secret is unavailable"
    token="$SULTANC_PAT"
    ;;
  github-app)
    [[ -n "${SULTANC_APP_ID:-}" ]] || fail \
      "configured GitHub App ID secret is unavailable"
    [[ -n "${SULTANC_APP_PRIVATE_KEY:-}" ]] || fail \
      "configured GitHub App private key secret is unavailable"
    token="$(./harness/.github/scripts/github-app-token.sh)"
    ;;
  *)
    fail "unsupported configured promotion auth mode"
    ;;
esac

echo "::add-mask::$token"
export GH_TOKEN="$token"
./harness/.github/scripts/promotion-controller.sh
