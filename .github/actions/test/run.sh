#!/usr/bin/env bash
set -euo pipefail

# Execute one trusted repository-owned qualification command.

name="${SULTANC_TEST_NAME:?test name is required}"
command_text="${SULTANC_TEST_COMMAND:?test command is required}"

printf '### %s\n' "$name" >> "$GITHUB_STEP_SUMMARY"
bash -euo pipefail -c "$command_text"
echo '- Result: PASS' >> "$GITHUB_STEP_SUMMARY"
