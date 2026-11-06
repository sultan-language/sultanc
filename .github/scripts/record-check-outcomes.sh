#!/usr/bin/env bash
set -euo pipefail

# Summarize every check of this host job: its gate (required / experimental)
# and the outcome of its step. An experimental failure is reported as a
# warning; it never changes the job result (the step ran with
# continue-on-error). Required failures already failed their own step.
#
# Inputs (environment): GATES (matrix.gates as JSON), OUTCOMES (check id ->
# steps.<id>.outcome as JSON).

gates="${GATES:?GATES is required}"
outcomes="${OUTCOMES:?OUTCOMES is required}"
summary="${GITHUB_STEP_SUMMARY:-/dev/null}"

jq -e 'type == "object"' <<< "$gates" >/dev/null
jq -e 'type == "object"' <<< "$outcomes" >/dev/null

{
  echo "### Host check outcomes"
  echo '| Check | Gate | Outcome |'
  echo '|---|---|---|'
  jq -r --argjson outcomes "$outcomes" '
    to_entries[] | select(.value != "off") |
    "| `\(.key)` | \(.value) | \($outcomes[.key] // "" | if . == "" then "not run" else . end) |"
  ' <<< "$gates"
} >> "$summary"

jq -r --argjson outcomes "$outcomes" '
  to_entries[] | select(.value == "experimental") |
  select(($outcomes[.key] // "") | test("failure")) | .key
' <<< "$gates" | while IFS= read -r check; do
  echo "::warning title=Experimental check failed::$check failed on ${SULTANC_CI_HOST_ID:-this host}; it is not gating (ci_hosts[].checks.experimental)."
done
