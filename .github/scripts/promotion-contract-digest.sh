#!/usr/bin/env bash
set -euo pipefail

# Bind promotion evidence to both qualification contract files.

: "${CONTRACTS_FILE:?contracts file is required}"
: "${TARGETS_FILE:?targets file is required}"

digest="$(
  (
    sha256sum "$CONTRACTS_FILE"
    sha256sum "$TARGETS_FILE"
  ) | sha256sum | awk '{print $1}'
)"
printf 'digest=%s\n' "$digest" >> "$GITHUB_OUTPUT"
